#include "msl/mslBank.h"
#include "dolphin/arq.h"
#include "dolphin/cache.h"
#include "mw/mwMemHeap.h"
#include "msl/mslsupport.h"
#include "msl/mslStreamFile.h"
#include "msl/mslARam.h"
#include "msl/mslSound_internal.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "msl/mslgcn_break.h"


void mslBankLoadResidentWaveChunkDone(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data);
extern unsigned long g_MSL_GCN_ARAM_ZeroBase;
extern unsigned char g_listPoolSound[];
extern _mslSystem* gMsi;

static void mslBankLoadResidentARamUploadComplete(void* callback_data);
void mslBankOpenSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
static void mslBankReadSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
static void mslBankReadAssetHeaderComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
static void mslBankLoadAsyncFailed(mslAsyncBank* bank, _mslError_e error);
mslAsyncBank g_BP_Load_Async;
int g_BP_Load_Async_InUse;

/* Soft ceiling: ~98.44% -- one pooled-string address instruction remains. */
extern "C" mslAssetWave* mslBankFileEntryFind(
    mslLoadedBank* bank, const char* name) {
    mslAssetWave* wave;
    unsigned long name_length;
    int i;

    if (((unsigned long)name & 0xf0000000) == 0x20000000) {
        name_length = 0;
    } else {
        name_length = strlen(name);
    }

    wave = bank->asset_info->waves + 1;
    for (i = 1; i <= bank->wave_count; wave++, i++) {
        if ((wave->name.offset & 0xf0000000) == 0x20000000) {
            if ((long)name == wave->name.token) {
                return wave;
            }
        } else if (strnicmp(wave->name.pointer, name, name_length) == 0) {
            if ((wave->has_secondary != 0 &&
                 stricmp(
                     wave->name.pointer + name_length, "_left.spt") == 0) ||
                (wave->has_secondary == 0 &&
                 stricmp(
                     wave->name.pointer + name_length, "_mono.spt") == 0)) {
                return wave;
            }
        }
    }
    return 0;
}

/* 100%: signed serialized token vs relocated pointer overlay preserves cmpw. */
extern "C" mslBankWaveEntry* mslBankWavesFind(
    mslLoadedBank* bank, const char* name) {
    int i;
    mslBankWaveEntry* wave;

    if (bank->waves.pointer == 0) {
        return 0;
    }

    wave = bank->waves.pointer;
    for (i = 0; i < bank->wave_count; i++, wave++) {
        if ((bank->flags & 0x10) == 0) {
            if (stricmp(wave->name.pointer, name) == 0) {
                return wave;
            }
        } else if (wave->name.token == (long)name) {
            return wave;
        }
    }
    return 0;
}

/*
 * Tear down a live bank in retail ownership order: unlink it from the MSL
 * system, stop its active sounds, unload constructed bank sounds, release
 * asset/file/ARAM storage, then free the bank itself.
 * Soft ceiling: ~98.47% -- exact retail size and control flow; remaining
 * differences are one scheduled instruction and pure GPR coloring.
 */
extern "C" void* mslBankUnLoad(mslLoadedBank* bank) {
    if (bank == 0) {
        return 0;
    }

    bank->system->pending_bank_loads--;
    if (bank != 0 && bank->system != 0) {
        int saved_guard;
        _ListNode* node;
        mslBankSoundEntry* sound_entry;
        int i;
        unsigned long sound_id;
        _mslSound* sound;

        saved_guard = bank->system->sound_list_guard;
        bank->system->sound_list_guard = 0;
        node = bank->system->active_sounds;
        while (node != 0) {
            sound = (_mslSound*)ListNodeData(0, node);
            sound_id = ListNodeID((ListPool*)g_listPoolSound, node);

            ListNext(&node);
            if (sound->owner_bank == bank) {
                mslDebugPrintf(
                    "Stopping sound in mslBankUnUseSound, %x (ID %08x)\n",
                    sound, sound_id);
                _mslSoundStop(sound);
            }
        }
        bank->system->sound_list_guard = saved_guard;

        sound_entry = bank->sounds.pointer;
        for (i = 0; i < bank->sound_count; i++, sound_entry++) {
            if (sound_entry->sound != 0) {
                mslSoundUncommit(sound_entry->sound);
                mslSoundUnLoad(sound_entry->sound);
            }
            sound_entry->sound = 0;
        }
        bank->system = 0;
    }

    if (bank->asset_info != 0) {
        if (bank->asset_info->temporary_names != 0) {
            _mwMemFree(bank->asset_info->temporary_names, 0, 0);
            bank->asset_info->temporary_names = 0;
        }
        _mwMemFree(bank->asset_info, 0, 0);
        bank->asset_info = 0;
    }

    if (bank->waves_file != 0) {
        mwFileCommand* close_command =
            mwFileCloseAsync(bank->waves_file, 0, 0);
        bank->waves_file = 0;
        mwFileFreeCommand(close_command);
    }

    if (bank->resident_aram_block != 0) {
        bank->resident_aram_block->Release();
        bank->resident_aram_block = 0;
    }

    _mwMemFree(bank, 0, 0);
    return 0;
}

/*
 * Convert the v11 bank body's ILP32 offsets into live pointers, publish each
 * sound definition's command list, then relocate command string references.
 * Near miss: ~86.93%. The operations, layouts, and relocation loops are
 * recovered. The sole caller rejects banks whose flags do not mark sound names
 * as omitted, corroborating that the retail count-driven sound-name loop had
 * its MSL_SKIP_SOUND_NAMES body compiled out while MWCC retained its unrolled
 * trip-count shell. Preserve clean C rather than adding that empty loop; the
 * other residue is GPR coloring and scheduling.
 */
extern "C" void* mslBankUpdatePtrs(mslLoadedBank* bank) {
    mslBankSoundDefinition* definition;
    mslBankSoundEntry* sound_entry;
    mslBankWaveEntry* wave;
    mslCmdItem* command_item;
    int i;
    int j;

    bank->waves.pointer =
        (mslBankWaveEntry*)bank->At(bank->waves.offset);
    bank->sounds.pointer =
        (mslBankSoundEntry*)bank->At(bank->sounds.offset);
    bank->definitions.pointer =
        (mslBankSoundDefinition*)bank->At(bank->definitions.offset);
    bank->command_items.pointer =
        (mslCmdItem*)bank->At(bank->command_items.offset);
    bank->sound_ids.pointer = (short*)bank->At(bank->sound_ids.offset);
    bank->unknown24.pointer = bank->At(bank->unknown24.offset);
    bank->string_table.pointer = bank->At(bank->string_table.offset);

    definition = bank->definitions.pointer;
    command_item = bank->command_items.pointer;
    sound_entry = bank->sounds.pointer;
    for (i = 0; i < bank->sound_count; i++, sound_entry++, definition++) {
        sound_entry->definition = definition;
        sound_entry->owner_bank = bank;
    }

    sound_entry = bank->sounds.pointer;
    for (i = 0; i < bank->sound_count; i++, sound_entry++) {
        mslCmdItem** command_slot;

        definition = sound_entry->definition;
        command_slot = &definition->commands;
        for (j = 0; j < definition->command_count; j++) {
            *command_slot = command_item;
            command_item++;
            command_slot = &command_item;
        }
    }

    if ((bank->flags & 0x10) == 0) {
        wave = bank->waves.pointer;
        for (i = 0; i < bank->wave_count; i++, wave++) {
            wave->name.offset += bank->string_table.offset;
        }
    }

    sound_entry = bank->sounds.pointer;
    for (i = 0; i < bank->sound_count; i++, sound_entry++) {
        definition = sound_entry->definition;
        command_item = definition->commands;
        for (j = 0; j < definition->command_count; j++, command_item++) {
            if (command_item->value == 32767000.0f) {
                command_item->value = -1.0f;
            }

            if (command_item->source.offset != 0xffffffff) {
                if (((bank->flags & 0x10) == 0 ||
                     command_item->type == 5 ||
                     command_item->type == 6) &&
                    (command_item->source.offset & 0xf0000000) !=
                        0x20000000) {
                    command_item->source.offset +=
                        bank->string_table.offset;
                }
            } else {
                command_item->source.pointer = 0;
            }

            if (command_item->target.offset != 0xffffffff) {
                command_item->target.offset += bank->string_table.offset;
            } else {
                command_item->target.pointer = 0;
            }
        }
    }

    return 0;
}

/*
 * Complete a resident-wave upload. The callback ABI supplies an opaque
 * payload, but it is the same typed async-bank state used by the surrounding
 * file callbacks.
 * Soft ceiling: ~97.43% -- retail shared-string addressing adds one
 * instruction; the remaining differences are nested-loop GPR coloring.
 */
static void mslBankLoadResidentARamUploadComplete(void* callback_data) {
    _mslAsyncResponse* response;
    mslAsyncBank* async_bank = (mslAsyncBank*)callback_data;
    mslLoadedBank* bank = async_bank->bank_data;
    int use_failed = 0;

    response = async_bank->response;
    if (mslBankUse(async_bank->system, bank) != 0) {
        if (bank != 0 && bank->system != 0) {
            int saved_guard = bank->system->sound_list_guard;
            _ListNode* node;
            mslBankSoundEntry* sound_entry;
            int i;

            bank->system->sound_list_guard = 0;
            node = bank->system->active_sounds;
            while (node != 0) {
                _mslSound* sound = (_mslSound*)ListNodeData(0, node);
                unsigned long sound_id =
                    ListNodeID((ListPool*)g_listPoolSound, node);

                ListNext(&node);
                if (sound->owner_bank == bank) {
                    mslDebugPrintf(
                        "Stopping sound in mslBankUnUseSound, %x (ID %08x)\n",
                        sound, sound_id);
                    _mslSoundStop(sound);
                }
            }
            bank->system->sound_list_guard = saved_guard;

            sound_entry = bank->sounds.pointer;
            for (i = 0; i < bank->sound_count; i++, sound_entry++) {
                if (sound_entry->sound != 0) {
                    mslSoundUncommit(sound_entry->sound);
                    mslSoundUnLoad(sound_entry->sound);
                }
                sound_entry->sound = 0;
            }
            bank->system = 0;
        }
        use_failed = 1;
    }

    if (use_failed) {
        _MSL_GCN_BREAK();
        mslDebugPrintf("mslBankLoadComplete: mslBankUse failed.\n");
        mslBankLoadAsyncFailed(async_bank, MSL_ERROR_SYSTEM);
    } else {
        g_BP_Load_Async_InUse = 0;
        if (bank->asset_info->temporary_names != 0) {
            _mwMemFree(bank->asset_info->temporary_names, 0, 0);
            bank->asset_info->temporary_names = 0;
        }
        mslAsyncComplete(response, true, bank, 0);
    }
}

static void i_ARQCALLBACK_BankLoadResidentARamUpload_Complete(
    unsigned long request_address);

/*
 * Move a resident wave chunk from the stream buffer into the bank's ARAM
 * block, returning ordinary buffers immediately and deferring final bank
 * publication to the game-thread callback queue.
 * Soft ceiling: ~93.55% -- retail buffer/callback-owner publication order is
 * recovered; the remaining delta is partial-TU string pooling and one
 * callback/final-chunk nonvolatile-register lifetime.
 */
void mslBankLoadResidentWaveChunkDone(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data) {
    mslAsyncBank* async_bank = (mslAsyncBank*)callback_data;

    if (error == 0) {
        mslARQRequest* request = mslGetArqRequest();

        if (request == 0) {
            mslStreamFile_ReturnBuffer(buffer);
            mslDebugPrintf(
                "mslBankLoadResidentWaveChunkDone: ARQ DMA structure alloc "
                "failed.\n");
            mslBankLoadAsyncFailed(async_bank, MSL_ERROR_SYSTEM);
        } else {
            void (*callback)(unsigned long);
            unsigned long destination;

            request->stream_buffer = buffer;
            callback = i_ARQCALLBACK_ReturnArqAndUserStreamBuffer;
            request->callback_data = async_bank;
            destination =
                async_bank->bank_data->resident_aram_block->base + offset;
            if (final_chunk != 0) {
                callback =
                    i_ARQCALLBACK_BankLoadResidentARamUpload_Complete;
            }

            DCFlushRange(buffer, size);
            ARQPostRequest(
                request, 0, 0, 0, (unsigned long)buffer, destination, size,
                callback);
        }
    } else {
        mslStreamFile_ReturnBuffer(buffer);
        mslDebugPrintf(
            "mslBankLoadResidentWaveChunkDone: Loading Resident Wave Data "
            "failed.\n");
        mslBankLoadAsyncFailed(async_bank, MSL_ERROR_SYSTEM);
    }
}

static void i_ARQCALLBACK_BankLoadResidentARamUpload_Complete(
    unsigned long request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    void* callback_data = request->callback_data;

    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
    mslTickCallBack_Queue(
        mslBankLoadResidentARamUploadComplete, callback_data);
}

/*
 * Asset-header publication: fix the serialized name and sound-table
 * references, assign resident ARAM addresses, then publish the bank when no
 * resident upload is pending.
 *
 * The successful path and the mslBankUse failure rollback are recovered.
 * Near miss: 92.60%, retail/current size 0x3d8/0x3cc. The entry count is
 * signed (retail uses cmpw in all three serialized-wave loops), the resident
 * ARAM offsets retain their assignment results, and rollback reloads the bank
 * system. The remaining two aligned opcode replacements are the reversed
 * initialization order of a zero index and wave pointer; the rest is GPR
 * coloring plus seven deletes/four inserts of reload scheduling.
 */
static void mslBankReadAssetHeaderComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    mslAsyncBank* async_bank = (mslAsyncBank*)callback_data;
    mslLoadedBank* bank = (mslLoadedBank*)async_bank->bank_data;
    mslAssetInfo* asset_info;
    mslAssetWave* wave;
    char* next_name;
    unsigned long resident_base = 0;
    int i;
    int use_failed = 0;

    mwFileFreeCommand(command);
    if (result.error != 0) {
        mslDebugPrintf(
            "mslBankLoadComplete: Unable to read wave headers, err %d\n",
            result.error);
        mslBankLoadAsyncFailed(async_bank, MSL_ERROR_ASYNC_READ);
        return;
    }

    if (async_bank->wave_size != 0) {
        MSLGCN_ARamBlock* resident_block =
            MSLGCN_ARamBlock::CreateBankBlock(async_bank->wave_size);
        if (resident_block == 0) {
            mslDebugPrintf("mslBankLoadComplete: Unable to alloc ARAM\n");
            mslBankLoadAsyncFailed(async_bank, MSL_ERROR_MEMORY);
            return;
        }
        bank->resident_aram_block = resident_block;

        mslStreamFile_QueueRequest(
            bank->waves_file, async_bank->wave_offset, async_bank->wave_size,
            0, mslBankLoadResidentWaveChunkDone, async_bank);
    }

    asset_info = bank->asset_info;
    next_name = asset_info->At(async_bank->string_offset);

    wave = asset_info->waves;
    for (i = 0; i < async_bank->entry_count; i++, wave++) {
        if ((wave->name.offset & 0xf0000000) != 0x20000000) {
            wave->name.pointer = next_name;
            next_name = strchr(next_name, 0) + 1;
        }
    }

    wave = asset_info->waves;
    for (i = 0; i < async_bank->entry_count; i++, wave++) {
        if (wave->has_secondary != 0 &&
            (wave->secondary_name.offset & 0xf0000000) !=
                0x20000000) {
            wave->secondary_name.pointer = next_name;
            next_name = strchr(next_name, 0) + 1;
        }
    }

    if (bank->resident_aram_block != 0) {
        resident_base = bank->resident_aram_block->base;
    }

    wave = asset_info->waves + 1;
    for (i = 1; i < async_bank->entry_count; i++, wave++) {
        if (wave->resident != 0) {
            unsigned long primary_aram_offset;

            wave->sound_table =
                (SPSoundTable*)asset_info->At(
                    (unsigned long)wave->sound_table);
            primary_aram_offset =
                (wave->primary_aram_offset += resident_base);
            SPInitSoundTable(
                wave->sound_table, primary_aram_offset,
                g_MSL_GCN_ARAM_ZeroBase);

            if (wave->has_secondary != 0) {
                unsigned long secondary_aram_offset;

                wave->secondary_sound_table =
                    (SPSoundTable*)asset_info->At(
                        (unsigned long)wave->secondary_sound_table);
                secondary_aram_offset =
                    (wave->secondary_aram_offset += resident_base);
                SPInitSoundTable(
                    wave->secondary_sound_table,
                    secondary_aram_offset,
                    g_MSL_GCN_ARAM_ZeroBase);
            }
        } else {
            wave->sound_table =
                (SPSoundTable*)asset_info->At(
                    (unsigned long)wave->sound_table);
            SPInitSoundTable(
                wave->sound_table, 0, g_MSL_GCN_ARAM_ZeroBase);

            if (wave->has_secondary != 0) {
                wave->secondary_sound_table =
                    (SPSoundTable*)asset_info->At(
                        (unsigned long)wave->secondary_sound_table);
                SPInitSoundTable(
                    wave->secondary_sound_table, 0,
                    g_MSL_GCN_ARAM_ZeroBase);
            }
        }
    }

    if (async_bank->wave_size == 0) {
        _mslAsyncResponse* response = async_bank->response;

        if (mslBankUse(async_bank->system, bank) != 0) {
            if (bank != 0 && bank->system != 0) {
                int saved_guard = bank->system->sound_list_guard;
                _ListNode* node;
                mslBankSoundEntry* sound_entry;

                bank->system->sound_list_guard = 0;
                node = bank->system->active_sounds;
                while (node != 0) {
                    _mslSound* sound = (_mslSound*)ListNodeData(0, node);
                    unsigned long sound_id =
                        ListNodeID((ListPool*)g_listPoolSound, node);

                    ListNext(&node);
                    if (sound->owner_bank == bank) {
                        mslDebugPrintf(
                            "Stopping sound in mslBankUnUseSound, %x (ID %08x)\n",
                            sound, sound_id);
                        _mslSoundStop(sound);
                    }
                }
                bank->system->sound_list_guard = saved_guard;

                sound_entry = bank->sounds.pointer;
                for (i = 0; i < bank->sound_count; i++, sound_entry++) {
                    if (sound_entry->sound != 0) {
                        mslSoundUncommit(sound_entry->sound);
                        mslSoundUnLoad(sound_entry->sound);
                    }
                    sound_entry->sound = 0;
                }
                bank->system = 0;
            }
            use_failed = 1;
        }

        if (use_failed) {
            _MSL_GCN_BREAK();
            mslDebugPrintf("mslBankLoadComplete: mslBankUse failed.\n");
            mslBankLoadAsyncFailed(async_bank, MSL_ERROR_SYSTEM);
            return;
        }

        g_BP_Load_Async_InUse = 0;
        if (bank->asset_info->temporary_names != 0) {
            _mwMemFree(bank->asset_info->temporary_names, 0, 0);
            bank->asset_info->temporary_names = 0;
        }
        mslAsyncComplete(response, true, bank, 0);
    }
}

/* Soft ceiling: mslBankReadWavesComplete ~99.71% -- four relocation-label
 * argument differences remain; operations and control flow are exact.
 */
static void mslBankReadWavesComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    mslAsyncBank* bank = (mslAsyncBank*)callback_data;
    _mwFileAsyncResult* result_ptr = &result;

    mwFileFreeCommand(command);
    bank->read_state = 0;
    if (result_ptr->error != 0) {
        mslDebugPrintf(
            "mslBankReadWavesComplete: Unable to read asset header, err %d\n",
            result_ptr->error);
        mslBankLoadAsyncFailed(bank, MSL_ERROR_ASYNC_READ);
    } else {
        mslLoadedBank* loaded_bank = (mslLoadedBank*)bank->bank_data;

        if (bank->asset_version != 2) {
            mslDebugPrintf(
                "mslBankReadWavesComplete: Asset header is of wrong version - expected %d, got version %d\n",
                2, bank->asset_version);
            mslBankLoadAsyncFailed(bank, MSL_ERROR_BANK_FORMAT);
        } else {
            loaded_bank->asset_info = (mslAssetInfo*)_mwMemCalloc(
                MWSOUND_HEAP, 1, bank->asset_info_size, 3, 0, 0, 0);
            if (loaded_bank->asset_info == 0) {
                mslDebugPrintf(
                    "mslBankReadWavesComplete() couldn't allocate memory\n");
                mslBankLoadAsyncFailed(bank, MSL_ERROR_MEMORY);
            } else if (mwFileReadAsync(
                           loaded_bank->waves_file, 0,
                           loaded_bank->asset_info, bank->asset_info_size, 0,
                           mslBankReadAssetHeaderComplete, bank) == 0) {
                mslDebugPrintf(
                    "mslBankReadWavesComplete: Unable to start asset information read\n");
                mslBankLoadAsyncFailed(bank, MSL_ERROR_ASYNC_READ);
            }
        }
    }
}

/*
 * Sound-bank read completion: close the .msg handle, launch the 0x1c-byte
 * asset-header read from .mbg, publish the waves handle into the loaded bank,
 * then validate and relocate the v11 bank body.
 * Soft ceiling: ~99.62% -- the complete callback contract is recovered; only
 * eight pooled-string relocation arguments remain.
 */
static void mslBankReadSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    mslAsyncBank* bank = (mslAsyncBank*)callback_data;
    mwFileCommand* close_command;

    mwFileFreeCommand(command);
    close_command = mwFileCloseAsync(bank->sounds_file, 0, 0);
    bank->sounds_file = 0;
    if (close_command == 0) {
        mslDebugPrintf(
            "mslBankReadSoundsComplete: Unable to close bank file handle\n");
        mslBankLoadAsyncFailed(bank, MSL_ERROR_ASYNC_READ);
    } else {
        mwFileFreeCommand(close_command);
        if (result.error != 0) {
            mslDebugPrintf(
                "mslBankReadSoundsComplete: Unable to read input bank, err %d\n",
                result.error);
            mslBankLoadAsyncFailed(bank, MSL_ERROR_ASYNC_READ);
        } else if (mwFileReadAsync(
                       bank->waves_file, 0, &bank->asset_version, 0x1c, 0,
                       mslBankReadWavesComplete, bank) == 0) {
            mslDebugPrintf(
                "mslBankReadSoundsComplete: Could not kick of asset header read\n");
            mslBankLoadAsyncFailed(bank, MSL_ERROR_ASYNC_READ);
        } else {
            mslLoadedBank* loaded_bank = (mslLoadedBank*)bank->bank_data;

            loaded_bank->waves_file = bank->waves_file;
            bank->waves_file = 0;
            mslDebugPrintf(
                "        Sound Bank file version=%d\n", loaded_bank->version);
            if (loaded_bank->version != 11) {
                mslDebugPrintf(
                    "mslBankReadSoundsComplete: Can't parse bank version %d.  Must use version %d\n",
                    loaded_bank->version, 11);
                mslBankLoadAsyncFailed(bank, MSL_ERROR_BANK_FORMAT);
            } else if ((loaded_bank->flags & 1) == 0) {
                mslDebugPrintf(
                    "mslBankReadSoundsComplete: MSL compiled with MSL_SKIP_SOUND_NAMES,\n but bank is formatted with them.\n");
                mslBankLoadAsyncFailed(bank, MSL_ERROR_BANK_FORMAT);
            } else if ((loaded_bank->flags & 2) != 0) {
                mslDebugPrintf(
                    "mslBankReadSoundsComplete: MSL compiled to use callback data,\n (i.e. MSL_SKIP_CALLBACK_DATA not defined),\n but bank is formatted without it.\n");
                mslBankLoadAsyncFailed(bank, MSL_ERROR_BANK_FORMAT);
            } else if ((loaded_bank->flags & 8) != 0) {
                mslDebugPrintf(
                    "mslBankReadSoundsComplete: MSL compiled to use LOOP markers,\n (i.e. MSL_NO_LOOP_MARKERS not defined),\n but bank is formatted without them.\n");
                mslBankLoadAsyncFailed(bank, MSL_ERROR_BANK_FORMAT);
            } else {
                mslBankUpdatePtrs(loaded_bank);
            }
        }
    }
}

/* Soft ceiling: mslBankOpenWavesComplete ~99.75% -- two diagnostic-string
 * relocation arguments remain.
 */
void mslBankOpenWavesComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    mslAsyncBank* bank = (mslAsyncBank*)callback_data;
    _mwFile* waves_file = result.value.file;

    mwFileFreeCommand(command);
    if (waves_file != 0) {
        bank->waves_file = waves_file;
        if (mwFileReadAsync(
                bank->sounds_file, 0, bank->bank_data, bank->sounds_size, 0,
                mslBankReadSoundsComplete, bank) == 0) {
            mslDebugPrintf(
                "mslBankOpenWavesComplete: couldn't kick off bank read: %s\n",
                bank->filename);
            mslBankLoadAsyncFailed(bank, MSL_ERROR_MEMORY);
        }
    } else {
        mslDebugPrintf(
            "mslBankOpenWavesComplete: Couldn't open WAVES file: %s\n",
            bank->filename);
        mslBankLoadAsyncFailed(bank, MSL_ERROR_WAVES_OPEN);
    }
}

/* Soft ceiling: mslBankOpenSoundsComplete ~99.67% -- six pooled-string
 * relocation arguments remain; operations and control flow are exact.
 */
void mslBankOpenSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    char filename[0x100];
    mslAsyncBank* bank = (mslAsyncBank*)callback_data;
    _mwFile* sounds_file = result.value.file;

    mwFileFreeCommand(command);
    if (sounds_file != 0) {
        int file_size = (int)mwFileGetSize(sounds_file);

        if (file_size > 0) {
            mslLoadedBank* bank_data;

            bank->sounds_file = sounds_file;
            bank->sounds_size = (unsigned long)file_size;
            bank_data = (mslLoadedBank*)_mwMemMalloc(
                MWSOUND_HEAP, bank->sounds_size, 3, 0, 0, 0);
            if (bank_data != 0) {
                mslDebugPrintf(
                    "mslBank %s is loaded at 0x%p\n",
                    bank->filename, bank_data);
                memset(bank_data, 0, 0x48);
                bank->bank_data = bank_data;
            }

            if (bank_data == 0) {
                mslDebugPrintf(
                    "mslBankOpenSoundsComplete: Out of memory for "
                    "structure:%s\n",
                    bank->filename);
                mslBankLoadAsyncFailed(bank, MSL_ERROR_MEMORY);
            } else {
                mslFileNameNoExt(bank->filename, filename);
                strcpy(bank->filename, filename);
                strcat(bank->filename, ".msg");
                if (mwFileOpenAsync(
                        bank->filename, 1, mslBankOpenWavesComplete, bank) ==
                    0) {
                    mslDebugPrintf(
                        "mslBankOpenSoundsComplete: Couldn't start file "
                        "read: %s\n",
                        bank->filename);
                    mslBankLoadAsyncFailed(bank, MSL_ERROR_MEMORY);
                }
            }
        } else {
            mwFileFreeCommand(mwFileCloseAsync(sounds_file, 0, 0));
            mslDebugPrintf(
                "mslBankOpenSoundsComplete: invalid input bank, file size "
                "<= 0  %s.\n",
                bank->filename);
            mslBankLoadAsyncFailed(bank, MSL_ERROR_FILE_OPEN);
        }
    } else {
        mslDebugPrintf(
            "mslBankOpenSoundsComplete: Unable to open input bank: %s.\n",
            bank->filename);
        mslBankLoadAsyncFailed(bank, MSL_ERROR_FILE_OPEN);
    }
}

/* Soft ceiling: mslBankLoadAsyncInternal ~99.83% -- two pooled-string
 * relocation arguments remain.
 */
void mslBankLoadAsyncInternal(
    _mslSystem* system, unsigned long flags, char* filename,
    _mslAsyncResponse* response) {
    char path[0x100];
    mslAsyncBank* bank = &g_BP_Load_Async;

    memset(bank, 0, sizeof(mslAsyncBank));
    g_BP_Load_Async_InUse = 1;
    system->pending_bank_loads++;
    bank->response = response;
    bank->system = system;

    if (system->bank_path[0] != 0) {
        strcpy(path, system->bank_path);
        strcat(path, filename);
    } else {
        strcpy(path, filename);
    }

    mslFileNameNoExt(path, bank->filename);
    strcat(bank->filename, ".mbg");
    if (mwFileOpenAsync(
            bank->filename, 1, mslBankOpenSoundsComplete, bank) == 0) {
        mslDebugPrintf(
            "mslBankLoadAsync Error: Couldn't start async open: %s\n",
            bank->filename);
        mslBankLoadAsyncFailed(bank, MSL_ERROR_FILE_OPEN);
    }
}

/* Soft ceiling: ~99.94% -- one diagnostic relocation argument remains. */
static void mslBankLoadAsyncFailed(
    mslAsyncBank* async_bank, _mslError_e error) {
    _mslAsyncResponse* response;
    mslLoadedBank* bank;

    if (async_bank == 0) {
        mslDebugPrintf(
            "CRITICAL ERROR: mslBankLoadAsyncFailed cannot report "
            "mslError #%d to NULL responder\n",
            error);
        return;
    }

    async_bank->system->pending_bank_loads--;
    response = async_bank->response;

    if (async_bank->waves_file != 0) {
        mwFileCommand* command =
            mwFileCloseAsync(async_bank->waves_file, 0, 0);
        async_bank->waves_file = 0;
        mwFileFreeCommand(command);
    }
    if (async_bank->sounds_file != 0) {
        mwFileCommand* command =
            mwFileCloseAsync(async_bank->sounds_file, 0, 0);
        async_bank->sounds_file = 0;
        mwFileFreeCommand(command);
    }
    if (async_bank->read_state != 0) {
        _MSL_GCN_BREAK();
        _mwMemFree((void*)async_bank->read_state, 0, 0);
        async_bank->read_state = 0;
    }

    bank = async_bank->bank_data;
    if (bank != 0) {
        async_bank->bank_data = 0;

        if (bank->waves_file != 0) {
            mwFileCommand* command =
                mwFileCloseAsync(bank->waves_file, 0, 0);
            bank->waves_file = 0;
            mwFileFreeCommand(command);
        }
        if (bank->asset_info != 0) {
            if (bank->asset_info->temporary_names != 0) {
                _MSL_GCN_BREAK();
                _mwMemFree(
                    bank->asset_info->temporary_names, 0, 0);
                bank->asset_info->temporary_names = 0;
            }
            _mwMemFree(bank->asset_info, 0, 0);
            bank->asset_info = 0;
        }
        if (bank->resident_aram_block != 0) {
            bank->resident_aram_block->Release();
            bank->resident_aram_block = 0;
        }
        _mwMemFree(bank, 0, 0);
    }

    g_BP_Load_Async_InUse = 0;
    mslAsyncComplete(response, false, 0, (void*)error);
}

static inline mslBankSoundEntry* mslBankFindID(
    mslLoadedBank* bank, int sound_id) {
    if (bank == 0) {
        mslDebugPrintf("mslBankFindID NULL bank\n");
        return 0;
    }

    if (sound_id < 0 || sound_id >= bank->sound_id_count) {
        mslDebugPrintf(
            "mslBankFindID ID %d out of range (%d).\n", sound_id,
            bank->sound_id_count);
        return 0;
    }

    int sound_index = bank->sound_ids.pointer[sound_id] - 1;
    if (sound_index < 0 || sound_index >= bank->sound_count) {
        mslDebugPrintf(
            "mslBankFindID ID %d doesn't exist (index==%d)\n",
            sound_id, sound_index);
        return 0;
    }
    return &bank->sounds.pointer[sound_index];
}

static inline char* mslBankSoundGetNameInline(
    mslBankSoundEntry* bank_sound) {
    static char name[0x100] = "               ";
    char* result;

    sprintf(name, "%08x", bank_sound);
    if (bank_sound == 0) {
        mslDebugPrintf("mslBankSoundGetName NULL sound\n");
        result = name;
    } else if (bank_sound->definition == 0) {
        mslDebugPrintf("mslBankSoundGetName never loaded??\n");
        result = name;
    } else {
        result = name;
    }
    return result;
}

static inline _ListNode* mslBankSoundUseInline(
    mslBankSoundEntry* bank_sound, _mslSystem* system) {
    _ListNode* node = 0;
    char* name = mslBankSoundGetNameInline(bank_sound);

    if (bank_sound->sound != 0) {
        node = mslSoundNew(system, 0);
        if (node == 0) {
            mslDebugPrintf(
                "mslBankSoundUse sound array exhausted: \"%s\"\n",
                name);
        } else {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);
            mslRuntimeSound* source;

            copy->flags = bank_sound->flags;
            source = (mslRuntimeSound*)bank_sound->sound;
            source->bank_ref_count++;
        }
    } else if ((bank_sound->flags & 2) != 0) {
        node = mslSoundNew(system, 0);
        if (node == 0) {
            mslDebugPrintf(
                "mslBankSoundUse sound array exhausted: \"%s\"\n",
                name);
        } else {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);
            copy->flags = bank_sound->flags;
        }
    } else {
        mslDebugPrintf(
            "mslBankSoundUse nonLOD sound wasn't loaded: \"%s\"\n",
            name);
    }
    return node;
}

static inline int mslBankSoundUnUseInline(
    mslBankSoundEntry* bank_sound) {
    int unloaded = 0;
    mslRuntimeSound* sound;
    char* name = mslBankSoundGetNameInline(bank_sound);

    sound = (mslRuntimeSound*)bank_sound->sound;
    if (sound == 0) {
        mslDebugPrintf(
            "mslBankSoundUnUse ERROR:  bank sound [%s] already UnLoaded!\n",
            name);
        return unloaded;
    }

    sound->bank_ref_count--;
    sound = (mslRuntimeSound*)bank_sound->sound;
    if (sound->bank_ref_count > 0) {
        return unloaded;
    }
    if (sound->bank_ref_count < 0) {
        mslDebugPrintf(
            "mslBankSoundUnUse ERROR:  bank sound [%s] count %d < 0\n",
            name, sound->bank_ref_count);
    }
    if ((bank_sound->flags & 2) == 0) {
        return unloaded;
    }
    mslSoundUnLoad(bank_sound->sound);
    bank_sound->sound = 0;
    return 1;
}

static inline void mslBankFinishPlayInline(
    bool loaded, _ListNode* node, mslBankSoundEntry* bank_sound) {
    mslRuntimeSound* copy =
        (mslRuntimeSound*)ListNodeData(0, node);
    unsigned long error_id =
        ListNodeID((ListPool*)g_listPoolSound, node);

    if (loaded) {
        if ((copy->flags & 0x10) != 0) {
            mslRuntimeSound* source =
                (mslRuntimeSound*)bank_sound->sound;
            mslCmdItem* command = source->definition->commands;

            while (command->type != 7) {
                if (command->attached_wave != 0) {
                    command->attached_wave->flags |= 0x80;
                }
                command++;
            }
        }

        if (mslSoundAttach(copy, bank_sound) == 0) {
            copy->owner_bank =
                ((mslRuntimeSound*)bank_sound->sound)->owner_bank;
            loaded = mslSoundPlayNow(node) != 0;
            if (loaded) {
                return;
            }
        }

        mslDebugPrintf(
            "Error: async sound did not play.  ID = %d\n", error_id);
        mslSoundUnCopy(node);
        mslBankSoundUnUseInline(bank_sound);
    } else {
        mslSoundUnCopy(node);
    }
}

/*
 * Resolve the facade's bank-local ID through the serialized +1 index table,
 * acquire a live sound node, lazily construct LOD sounds, attach the command
 * graph, and start playback. This is the vertical shell-FX contract; the
 * retail function inlines the ID lookup and use/unuse helpers below. Near
 * miss: ~97.84%, retail/current size 0x6a4/0x6a0, with exact operations and
 * control flow.
 * Remaining differences are pooled-string address instructions, GPR coloring,
 * and two scheduled instructions.
 */
extern "C" unsigned long mslBankPlayVol(
    mslLoadedBank* bank, int sound_id, unsigned long play_arg0,
    unsigned long play_arg1, float volume, unsigned long play_flags) {
    mslBankSoundEntry* bank_sound;
    _ListNode* node;
    unsigned long handle;

    if (bank == 0) {
        mslDebugPrintf("mslBankPlayVol: NULL bank pointer.\n");
        return 0;
    }

    bank_sound = mslBankFindID(bank, sound_id);

    if (bank_sound != 0) {
        node = mslBankSoundUseInline(bank_sound, gMsi);
        if (node != 0) {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);

            handle = ListNodeID((ListPool*)g_listPoolSound, node);
            copy->flags |= play_flags;
            copy->priority = play_arg1;
            copy->track = play_arg0;
            copy->volume = volume;

            if (bank_sound->sound != 0) {
                mslBankFinishPlayInline(true, node, bank_sound);
                return handle;
            } else {
                _mslSound* loaded_sound = mslSoundLoad(
                    gMsi, bank, bank_sound->definition, bank_sound->flags);
                if (loaded_sound != 0) {
                    mslRuntimeSound* runtime =
                        (mslRuntimeSound*)loaded_sound;

                    bank_sound->sound = loaded_sound;
                    runtime->bank_ref_count = 1;
                    runtime->owner_bank = bank;
                } else {
                    mslDebugPrintf("Unable to load async sound.\n");
                }
                mslBankFinishPlayInline(
                    bank_sound->sound != 0, node, bank_sound);
                return handle;
            }
        }
    }

    mslDebugPrintf("mslBankPlayVol::MSL Bank Play error.\n");
    return 0;
}

/*
 * Resolve and play a bank sound with the full runtime volume, pan, and pitch
 * overlay. Retail inlines the ID lookup and bank-sound use/unuse helpers into
 * this path. Near miss: ~97.88%, retail/current size 0x6c4/0x6c0. Retail
 * loaded-first order, sound publication, inlined helper CFG, and all
 * operations are exact; pooled-string addressing, GPR coloring, and two
 * scheduled instructions remain.
 */
extern "C" unsigned long mslBankPlayVolPanPitch(
    mslLoadedBank* bank, int sound_id, unsigned long play_arg0,
    unsigned long play_arg1, float volume, float pan, float pitch,
    unsigned long play_flags) {
    mslBankSoundEntry* bank_sound;
    _ListNode* node;
    unsigned long handle;

    if (bank == 0) {
        mslDebugPrintf("mslBankPlayVol: NULL bank pointer.\n");
        return 0;
    }

    bank_sound = mslBankFindID(bank, sound_id);

    if (bank_sound != 0) {
        node = mslBankSoundUseInline(bank_sound, gMsi);
        if (node != 0) {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);

            handle = ListNodeID((ListPool*)g_listPoolSound, node);
            copy->flags |= play_flags;
            copy->priority = play_arg1;
            copy->track = play_arg0;
            copy->volume = volume;
            copy->pan = pan;
            copy->pitch = pitch;

            if (bank_sound->sound != 0) {
                mslBankFinishPlayInline(true, node, bank_sound);
                return handle;
            } else {
                _mslSound* loaded_sound = mslSoundLoad(
                    gMsi, bank, bank_sound->definition, bank_sound->flags);
                if (loaded_sound != 0) {
                    mslRuntimeSound* runtime =
                        (mslRuntimeSound*)loaded_sound;

                    bank_sound->sound = loaded_sound;
                    runtime->bank_ref_count = 1;
                    runtime->owner_bank = bank;
                } else {
                    mslDebugPrintf("Unable to load async sound.\n");
                }
                mslBankFinishPlayInline(
                    bank_sound->sound != 0, node, bank_sound);
                return handle;
            }
        }
    }

    mslDebugPrintf(
        "mslBankPlayVolPanitch::MSL Bank Play error.\n");
    return 0;
}

/*
 * Release one bank-owned reference. LOD sounds (flag 0x2) are unloaded when
 * their last live copy goes away; ordinary resident sounds remain cached.
 * Soft ceiling: ~98.14% -- one pooled-string address instruction and six
 * relocation/register arguments remain.
 */
int mslBankSoundUnUse(mslBankSoundEntry* bank_sound) {
    int unloaded = 0;
    mslRuntimeSound* sound;
    char* name = mslBankSoundGetNameInline(bank_sound);

    sound = (mslRuntimeSound*)bank_sound->sound;
    if (sound == 0) {
        mslDebugPrintf(
            "mslBankSoundUnUse ERROR:  bank sound [%s] already UnLoaded!\n",
            name);
        return 0;
    }

    sound->bank_ref_count--;
    if (sound->bank_ref_count <= 0) {
        if (sound->bank_ref_count < 0) {
            mslDebugPrintf(
                "mslBankSoundUnUse ERROR:  bank sound [%s] count %d < "
                "0\n",
                name, sound->bank_ref_count);
        }
        if ((bank_sound->flags & 2) != 0) {
            mslSoundUnLoad(bank_sound->sound);
            bank_sound->sound = 0;
            unloaded = 1;
        }
    }
    return unloaded;
}

/*
 * Allocate a live sound-list node and carry the bank definition flags into
 * its runtime overlay. An already-loaded bank sound gains one reference;
 * an unloaded non-LOD sound is an error.
 * Soft ceiling: ~97.63% -- shared-pool offsets plus one source-equivalent
 * flags/base-sound load-order island remain.
 */
_ListNode* mslBankSoundUse(
    mslBankSoundEntry* bank_sound, _mslSystem* system) {
    _ListNode* node = 0;
    char* name = mslBankSoundGetNameInline(bank_sound);

    if (bank_sound->sound != 0) {
        node = mslSoundNew(system, 0);
        if (node == 0) {
            mslDebugPrintf(
                "mslBankSoundUse sound array exhausted: \"%s\"\n",
                name);
        } else {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);
            mslRuntimeSound* source =
                (mslRuntimeSound*)bank_sound->sound;

            copy->flags = bank_sound->flags;
            source->bank_ref_count++;
        }
    } else if ((bank_sound->flags & 2) != 0) {
        node = mslSoundNew(system, 0);
        if (node == 0) {
            mslDebugPrintf(
                "mslBankSoundUse sound array exhausted: \"%s\"\n",
                name);
        } else {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);
            copy->flags = bank_sound->flags;
        }
    } else {
        mslDebugPrintf(
            "mslBankSoundUse nonLOD sound wasn't loaded: \"%s\"\n",
            name);
    }
    return node;
}

/*
 * Soft ceiling: mslBankUse ~99.94% -- typed inlined wave lookup and saved
 * register allocation are exact; one pooled diagnostic relocation remains.
 */
extern "C" int mslBankUse(
    _mslSystem* system, mslLoadedBank* bank) {
    int i;
    int command_index;
    mslBankSoundEntry* sound;

    bank->next = 0;
    bank->previous = 0;
    bank->system = system;

    sound = bank->sounds.pointer;
    for (i = 0; i < bank->sound_count; i++, sound++) {
        if ((sound->flags & 2) != 0) {
            mslCmdItem* command = sound->definition->commands;

            for (command_index = 0;
                 command_index < sound->definition->command_count;
                command_index++, command++) {
                if (command->type == 1) {
                    mslBankWaveEntry* wave = mslBankWavesFind(
                        bank, (const char*)command->source.pointer);
                    if (wave != 0) {
                        wave->flags |= 1;
                    }
                }
            }
        }
    }

    {
        int load_index;
        mslBankSoundEntry* load_sound = bank->sounds.pointer;

        for (load_index = 0; load_index < bank->sound_count;
             load_index++, load_sound++) {
            if ((load_sound->flags & 2) != 0) {
                load_sound->sound = 0;
            }
            if (load_sound->sound == 0) {
                load_sound->sound = mslSoundLoad(
                    system, bank, load_sound->definition, load_sound->flags);
            }
            if (load_sound->sound != 0) {
                mslRuntimeSound* runtime =
                    (mslRuntimeSound*)load_sound->sound;
                runtime->owner_bank = bank;
            } else {
                mslDebugPrintf(
                    "Unable to load sound: [0x%08x]\n", load_index);
            }
        }
    }
    return 0;
}

/*
 * Recovered callback ownership path.
 * The runtime command owner and inlined bank reference release are recovered;
 * retain field reloads at their retail ownership sites.
 */
void callbackPlay(
    bool loaded, mslBankSoundEntry* bank_sound, _ListNode* node) {
    mslRuntimeSound* copy =
        (mslRuntimeSound*)ListNodeData(0, node);
    unsigned long error_id =
        ListNodeID((ListPool*)g_listPoolSound, node);

    if (loaded) {
        if ((copy->flags & 0x10) != 0) {
            mslRuntimeSound* source =
                (mslRuntimeSound*)bank_sound->sound;
            mslCmdItem* command = source->definition->commands;

            while (command->type != 7) {
                if (command->attached_wave != 0) {
                    command->attached_wave->flags |= 0x80;
                }
                command++;
            }
        }

        if (mslSoundAttach(copy, bank_sound) == 0) {
            copy->owner_bank =
                ((mslRuntimeSound*)bank_sound->sound)->owner_bank;
            loaded = mslSoundPlayNow(node) != 0;
            if (loaded) {
                return;
            }
        }

        mslDebugPrintf(
            "Error: async sound did not play.  ID = %d\n", error_id);
        mslSoundUnCopy(node);
        mslBankSoundUnUseInline(bank_sound);
    } else {
        mslSoundUnCopy(node);
    }
}

/* Soft ceiling: asyncLoadSound ~96.25% -- two pooled-string address
 * instructions and one relocation argument remain.
 */
void asyncLoadSound(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundEntry* bank_sound, mslAsyncSoundCallback callback,
    _ListNode* node) {
    if (bank_sound->sound != 0) {
        callback(true, bank_sound, node);
    } else {
        _mslSound* loaded = mslSoundLoad(
            system, bank, bank_sound->definition, bank_sound->flags);
        if (loaded != 0) {
            mslRuntimeSound* runtime = (mslRuntimeSound*)loaded;

            bank_sound->sound = loaded;
            runtime->bank_ref_count = 1;
            runtime->owner_bank = bank;
        } else {
            mslDebugPrintf("Unable to load async sound.\n");
        }
        callback(bank_sound->sound != 0, bank_sound, node);
    }
}

typedef char msl_async_bank_size_must_be_0x134[
    sizeof(mslAsyncBank) == 0x134 ? 1 : -1];

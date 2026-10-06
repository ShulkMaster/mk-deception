#include "msl/mslBank.h"
#include "dolphin/arq.h"
#include "dolphin/cache.h"
#include "mw/mwMemHeap.h"
#include "msl/mslsupport.h"
#include "msl/mslStreamFile.h"
#include "msl/mslARam.h"
#include "msl/mslSound_internal.h"
#include "msl/mslgcn_globals.h"
#include "runtime/cstring.h"
#include "runtime/cstdio.h"
#include "msl/mslgcn_break.h"

void mslBankLoadResidentWaveChunkDone(
    void* buffer, unsigned long offset, int size, int error,
    int final_chunk, void* callback_data);
extern _mslSystem* gMsi;

static void mslBankLoadResidentARamUploadComplete(void* callback_data);
static void mslBankReadSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
static void mslBankReadAssetHeaderComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data);
static void mslBankLoadAsyncFailed(mslAsyncBank* bank, _mslError_e error);
mslAsyncBank g_BP_Load_Async;
int g_BP_Load_Async_InUse;

extern "C" int mslTrackIsPlaying(int track)
{
    mslDebugPrintf("Error: mslTrackIsPlaying index %d out of range.\n", track);
    return 0;
}

extern "C" int mslBankFindSoundID(mslLoadedBank* bank, const char* name)
{
    mslDebugPrintf("mslBankFindSoundID not active, MSL_SKIP_SOUND_NAMES defined.\n");
    return -1;
}

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

static inline void mslBankStopActiveSounds(mslLoadedBank* bank)
{
    _mslSound* sound;
    _ListNode* node;
    msl_u32 sound_id;
    int saved_guard;

    saved_guard = bank->system->sound_list_guard;
    bank->system->sound_list_guard = 0;
    node = bank->system->active_sounds;
    while (node != 0) {
        sound = (_mslSound*)ListNodeData(0, node);
        sound_id = ListNodeID(&g_listPoolSound, node);

        ListNext(&node);
        if (sound->owner_bank == bank) {
            mslDebugPrintf(
                "Stopping sound in mslBankUnUseSound, %x (ID %08x)\n",
                sound, sound_id);
            _mslSoundStop(sound);
        }
    }
    bank->system->sound_list_guard = saved_guard;
}

static inline void mslBankUnloadSounds(mslLoadedBank* bank)
{
    int i;
    mslBankSoundEntry* sound_entry;

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

/* TODO: [near miss] 99.73%; five unload-entry register rows remain; stop at coloring. */
extern "C" void* mslBankUnLoad(mslLoadedBank* bank) {
    if (bank == 0) {
        return 0;
    }

    bank->system->pending_bank_loads--;
    if (bank != 0 && bank->system != 0) {

        mslBankStopActiveSounds(bank);

        mslBankUnloadSounds(bank);
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

/* TODO: [breakthrough needed] 87.34%; retained source offset restored; command-slot abstraction and stripped name-loop shell remain unresolved. */
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
            unsigned long source_offset;
            if (command_item->value == 32767000.0f) {
                command_item->value = -1.0f;
            }

            source_offset = command_item->source.offset;
            if (source_offset != 0xffffffff) {
                if (((bank->flags & 0x10) == 0 ||
                     command_item->type == 5 ||
                     command_item->type == 6) &&
                    (source_offset & 0xf0000000) !=
                        0x20000000) {
                    command_item->source.offset =
                        source_offset + bank->string_table.offset;
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

/* TODO: [near miss] 99.72%; five unload-entry register rows remain; stop at coloring. */
static void mslBankLoadResidentARamUploadComplete(void* callback_data) {
    _mslAsyncResponse* response;
    mslAsyncBank* async_bank = (mslAsyncBank*)callback_data;
    mslLoadedBank* bank = async_bank->bank_data;
    int use_failed = 0;

    response = async_bank->response;
    if (mslBankUse(async_bank->system, bank) != 0) {
        if (bank != 0 && bank->system != 0) {
            mslBankStopActiveSounds(bank);
            mslBankUnloadSounds(bank);
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

static inline void mslBankUploadResidentChunk(
    void* buffer, unsigned long offset, int size, int final_chunk,
    mslAsyncBank* async_bank, mslARQRequest* request) {
    unsigned long destination;
    void (*callback)(unsigned long);

    request->stream_buffer = buffer;
    callback = i_ARQCALLBACK_ReturnArqAndUserStreamBuffer;
    request->callback_data = async_bank;
    destination = async_bank->bank_data->resident_aram_block->base;
    destination += offset;
    if (final_chunk != 0) {
        callback =
            i_ARQCALLBACK_BankLoadResidentARamUpload_Complete;
    }

    DCFlushRange(buffer, size);
    ARQPostRequest(
        &request->request, 0, 0, 0, (unsigned long)buffer, destination, size,
        callback);
}

/* TODO: [near miss] 98.98%; request/context GPR pair remains; stop at coloring. */
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
            mslBankUploadResidentChunk(
                buffer, offset, size, final_chunk, async_bank, request);
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

/* TODO: [near miss] 98.21%; saved-owner registers and failure-path staging remain; inspect lifetimes. */
static void mslBankReadAssetHeaderComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    _mslAsyncResponse* response;
    int saved_guard;
    mslAsyncBank* async_bank = (mslAsyncBank*)callback_data;
    mslLoadedBank* bank;
    mslAssetInfo* asset_info;
    msl_u32 sound_id;
    _mslSound* sound;
    mslAssetWave* wave;
    char* next_name;
    unsigned long resident_base;
    int i;

    mwFileFreeCommand(command);
    bank = async_bank->bank_data;
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
    for (i = 0; i < async_bank->entry_count; wave++, i++) {
        if ((wave->name.offset & 0xf0000000) != 0x20000000) {
            wave->name.pointer = next_name;
            next_name = strchr(next_name, 0) + 1;
        }
    }

    wave = bank->asset_info->waves;
    for (i = 0; i < async_bank->entry_count; wave++, i++) {
        if (wave->has_secondary != 0 &&
            (wave->secondary_name.offset & 0xf0000000) !=
                0x20000000) {
            wave->secondary_name.pointer = next_name;
            next_name = strchr(next_name, 0) + 1;
        }
    }

    asset_info = bank->asset_info;
    resident_base = 0;
    if (bank->resident_aram_block != 0) {
        resident_base = bank->resident_aram_block->base;
    }

    wave = asset_info->waves + 1;
    for (i = 1; i < async_bank->entry_count; wave++, i++) {
        if (wave->resident != 0) {
            unsigned long primary_aram_offset;

            wave->sound_table =
                (SPSoundTable*)asset_info->At(
                    (unsigned long)wave->sound_table);
            primary_aram_offset = resident_base + wave->primary_aram_offset;
            wave->primary_aram_offset = primary_aram_offset;
            SPInitSoundTable(
                wave->sound_table, primary_aram_offset,
                g_MSL_GCN_ARAM_ZeroBase);

            if (wave->has_secondary != 0) {
                unsigned long secondary_aram_offset;

                wave->secondary_sound_table =
                    (SPSoundTable*)asset_info->At(
                        (unsigned long)wave->secondary_sound_table);
                secondary_aram_offset =
                    resident_base + wave->secondary_aram_offset;
                wave->secondary_aram_offset = secondary_aram_offset;
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
        bank = async_bank->bank_data;
        response = async_bank->response;
        int use_failed = 0;

        if (mslBankUse(async_bank->system, bank) != 0) {
            if (bank != 0 && bank->system != 0) {
                _ListNode* node;
                mslBankSoundEntry* sound_entry;

                saved_guard = bank->system->sound_list_guard;

                bank->system->sound_list_guard = 0;
                node = bank->system->active_sounds;
                while (node != 0) {
                    sound = (_mslSound*)ListNodeData(0, node);
                    sound_id = ListNodeID(&g_listPoolSound, node);

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
        mslLoadedBank* loaded_bank = bank->bank_data;

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
            mslLoadedBank* loaded_bank = bank->bank_data;

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

void mslBankOpenSoundsComplete(
    mwFileCommand* command, _mwFileAsyncResult result, void* callback_data) {
    char filename[0x100];
    mslAsyncBank* bank = (mslAsyncBank*)callback_data;
    _mwFile* sounds_file = result.value.file;

    mwFileFreeCommand(command);
    if (sounds_file != 0) {
        int file_size = mwFileGetSize(sounds_file);

        if (file_size > 0) {
            mslLoadedBank* bank_data;

            bank->sounds_file = sounds_file;
            bank->sounds_size = file_size;
            bank_data = (mslLoadedBank*)_mwMemMalloc(
                MWSOUND_HEAP, bank->sounds_size, 3, 0, 0, 0);
            if (bank_data != 0) {
                mslDebugPrintf(
                    "mslBank %s is loaded at 0x%p\n",
                    bank->filename, bank_data);
                memset(bank_data, 0, sizeof(*bank_data));
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
        ListNodeID(&g_listPoolSound, node);

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

extern "C" unsigned long mslBankPlayQ(mslLoadedBank* bank, int sound_id, int track)
{
    if (bank == 0) {
        mslDebugPrintf("mslBankPlayQ: NULL bank pointer.\n");
        return 0;
    }
    if (track < 0) {
        mslDebugPrintf("MSL Queue Play error...track out of range.\n");
        return 0;
    }
    if (mslBankFindID(bank, sound_id) == 0) {
        mslDebugPrintf("MSL Queue Play error.\n");
    }
    return 0;
}

extern "C" unsigned long mslBankPlayPrep(mslLoadedBank* bank, int sound_id)
{
    mslBankSoundEntry* bank_sound;
    _ListNode* node;

    if (bank == 0) {
        mslDebugPrintf("mslBankPlayPrep: NULL bank pointer.\n");
        return 0;
    }

    bank_sound = mslBankFindID(bank, sound_id);
    if (bank_sound != 0) {
        node = mslBankSoundUseInline(bank_sound, gMsi);
        if (node != 0) {
            mslDebugPrintf("Error: async sound could not prep.  ID = %d\n",
                           ListNodeID(&g_listPoolSound, node));
            mslBankSoundUnUseInline(bank_sound);
            mslDebugPrintf("Unable to load async sound.\n");
            return 0;
        }
    }

    mslDebugPrintf("mslBankPlayPrep error.\n");
    return 0;
}

/* TODO: [near miss] 99.59%; operations, calls, literals, and return joins agree; stop at saved-register coloring. */
extern "C" unsigned long mslBankPlayVol(
    mslLoadedBank* bank, int sound_id, unsigned long play_arg0,
    unsigned long play_arg1, float volume, unsigned long play_flags) {
    mslBankSoundEntry* bank_sound;
    _ListNode* node;

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

            unsigned long handle = ListNodeID(&g_listPoolSound, node);
            copy->flags |= play_flags;
            copy->priority = play_arg1;
            copy->track = play_arg0;
            copy->volume = volume;

            _mslSystem* system = gMsi;

            if (bank_sound->sound != 0) {
                mslBankFinishPlayInline(true, node, bank_sound);
            } else {
                _mslSound* loaded_sound = mslSoundLoad(
                    system, bank, bank_sound->definition, bank_sound->flags);
                if (loaded_sound != 0) {
                    bank_sound->sound = loaded_sound;
                    ((mslRuntimeSound*)bank_sound->sound)->bank_ref_count = 1;
                    ((mslRuntimeSound*)bank_sound->sound)->owner_bank = bank;
                } else {
                    mslDebugPrintf("Unable to load async sound.\n");
                }
                mslBankFinishPlayInline(
                    bank_sound->sound != 0, node, bank_sound);
            }
            return handle;
        }
    }

    mslDebugPrintf("mslBankPlayVol::MSL Bank Play error.\n");
    return 0;
}

static inline void mslBankPlaySoundInline(
    _mslSystem* system, mslLoadedBank* bank,
    mslBankSoundEntry* bank_sound, _ListNode* node) {
    if (bank_sound->sound != 0) {
        mslBankFinishPlayInline(true, node, bank_sound);
    } else {
        _mslSound* loaded_sound = mslSoundLoad(
            system, bank, bank_sound->definition, bank_sound->flags);
        if (loaded_sound != 0) {
            bank_sound->sound = loaded_sound;
            ((mslRuntimeSound*)bank_sound->sound)->bank_ref_count = 1;
            ((mslRuntimeSound*)bank_sound->sound)->owner_bank = bank;
        } else {
            mslDebugPrintf("Unable to load async sound.\n");
        }
        mslBankFinishPlayInline(
            bank_sound->sound != 0, node, bank_sound);
    }
}

#pragma push
#pragma inline_max_size(1024)
/* TODO: [near miss] 99.60%; play-phase system load matches; saved-register coloring remains. */
extern "C" unsigned long mslBankPlayVolPanPitch(
    mslLoadedBank* bank, int sound_id, unsigned long play_arg0,
    unsigned long play_arg1, float volume, float pan, float pitch,
    unsigned long play_flags) {
    mslBankSoundEntry* bank_sound;
    unsigned long handle;

    if (bank == 0) {
        mslDebugPrintf("mslBankPlayVol: NULL bank pointer.\n");
        return 0;
    }

    bank_sound = mslBankFindID(bank, sound_id);

    if (bank_sound != 0) {
        _ListNode* node = mslBankSoundUseInline(bank_sound, gMsi);
        if (node != 0) {
            mslRuntimeSound* copy =
                (mslRuntimeSound*)ListNodeData(0, node);

            handle = ListNodeID(&g_listPoolSound, node);
            copy->flags |= play_flags;
            copy->priority = play_arg1;
            copy->track = play_arg0;
            copy->volume = volume;
            copy->pan = pan;
            copy->pitch = pitch;

            mslBankPlaySoundInline(gMsi, bank, bank_sound, node);
            return handle;
        }
    }

    mslDebugPrintf(
        "mslBankPlayVolPanitch::MSL Bank Play error.\n");
    return 0;
}
#pragma pop

extern "C" unsigned long mslBankPlay(mslLoadedBank* bank, int sound_id)
{
    if (bank == 0) {
        mslDebugPrintf("mslBankPlay: NULL bank pointer.\n");
        return 0;
    }
    mslDebugPrintf("MSL Bank Play error.\n");
    return 0;
}

extern "C" int mslBankGetNumSounds(mslLoadedBank* bank)
{
    if (bank == 0) {
        mslDebugPrintf("mslBankGetNumSounds NULL bank\n");
        return 0;
    }
    return bank->sound_count;
}

extern "C" int mslBankGetIDs(mslLoadedBank* bank, int max_ids)
{
    if (bank == 0) {
        mslDebugPrintf("mslBankGetIDs error:  NULL bank pointer.\n");
        return 0;
    }
    if (bank->sound_id_count > max_ids) {
        mslDebugPrintf("mslBankGetIDs found more than %d, punting rest\n", max_ids);
    }
    return 0;
}

extern "C" int mslBankSoundHasStream(mslBankSoundEntry* bank_sound)
{
    if (bank_sound == 0) {
        mslDebugPrintf("mslBankSoundHasStream NULL sound\n");
        return 0;
    }
    if (bank_sound->definition == 0) {
        mslDebugPrintf("mslBankSoundHasStream never loaded??\n");
    }
    return 0;
}

extern "C" int mslBankSoundGetID(mslBankSoundEntry* bank_sound, const char* name)
{
    if (bank_sound == 0) {
        mslDebugPrintf("mslBankSoundGetID NULL sound\n");
        return -1;
    }
    if (bank_sound->definition == 0) {
        mslDebugPrintf("mslBankSoundGetID never loaded??\n");
        return -1;
    }
    mslDebugPrintf("mslBankSoundGetID \"%s\" Not Found\n", name);
    return -1;
}

extern "C" void mslBankSoundUnPrep(mslBankSoundEntry* bank_sound)
{
    if (bank_sound == 0) {
        mslDebugPrintf("mslBankSoundUnPrep given NULL msb\n");
        return;
    }
    if (bank_sound->sound == 0) {
        mslDebugPrintf("mslBankSoundUnPrep %x already unprepped!\n", bank_sound);
    }
}

extern "C" int mslBankSoundPrep(mslBankSoundEntry* bank_sound, const char* name)
{
    mslDebugPrintf("mslBankSoundPrep nonLOD sound wasn't loaded: \"%s\"\n", name);
    return 0;
}

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
    sound = (mslRuntimeSound*)bank_sound->sound;
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

    return 0;
}

/* TODO: [near miss] 98.41%; pool now exact; only ListNodeData(0, node) argument setup
 * order differs (retail moves node before the zero); typed null and decl order are neutral. */
void callbackPlay(
    bool loaded, mslBankSoundEntry* bank_sound, _ListNode* node) {
    mslRuntimeSound* copy =
        (mslRuntimeSound*)ListNodeData(0, node);
    unsigned long error_id =
        ListNodeID(&g_listPoolSound, node);

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
            bank_sound->sound = loaded;
            ((mslRuntimeSound*)bank_sound->sound)->bank_ref_count = 1;
            ((mslRuntimeSound*)bank_sound->sound)->owner_bank = bank;
        } else {
            mslDebugPrintf("Unable to load async sound.\n");
        }
        callback(bank_sound->sound != 0, bank_sound, node);
    }
}

typedef char msl_async_bank_size_must_be_0x134[
    sizeof(mslAsyncBank) == 0x134 ? 1 : -1];

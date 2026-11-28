inline void SBPlayable_Stream::iAX_GetVoiceBlock(
    _AXVPB* voice,
    int channel,
    int& block,
    u32* raw_address) {
    SBPlayable_Stream* stream =
        this;
    u32 address = i_AXVPB_GetVoice_CurrentPosition(voice);

    if (address >= g_MSL_GCN_ARAM_ZeroBase_ADPCM_Start &&
        address <= g_MSL_GCN_ARAM_ZeroBase_ADPCM_End) {
        block = -1;
    } else {
        address = ((s32)address >> 1) -
            stream->cache_buffers[channel];
        block = (s32)address >> stream->segment_shift;
        if (block < 0 || block >= stream->ring_block_count) {
            _MSL_GCN_BREAK();
        }
    }
    if (raw_address != 0) {
        *raw_address = address;
    }
}

SBPlayable_Stream::SBPlayable_Stream() {
    ResetValues();
}

SBPlayable_Stream::~SBPlayable_Stream() {
    FreeResources();
}

SBPlayable_Stream* SBPlayable_Stream::GetObject(void) {
    SBPlayable_Stream* stream =
        new (MWSOUND_HEAP, (mwMemFlags)0x10, "SBPlayable_Stream", 0, 0)
            SBPlayable_Stream;

    stream->UpdateList_Insert();
    return stream;
}

void SBPlayable_Stream::FreeObject(void) {
    FreeResources();
    UpdateList_Remove();
    delete this;
}

void SBPlayable_Stream::ResetValues(void) {
    SBPlayable_Stream* self =
        this;

    self->ready_to_play = 0;
    self->last_read_pending = 0;
    self->voices_started = 0;
    self->play_when_ready = 0;
    self->play_flags = 0;
    self->cache_buffers[0] = 0;
    self->cache_buffers[1] = 0;
    self->cache_buffer_size = 0;
    self->cache_buffer1_size = 0;
    self->stream_context = 0;
    self->aligned_size = 0;
    self->primary_offset = 0;
    self->segment_size = 0;
    self->segment_shift = 0;
    self->source_read_offset = 0;
    self->source_bytes_remaining = 0;
    self->pending_file_request = 0;
    self->queued_read_offset = 0;
    self->queued_read_size = 0;
    self->queued_block_count = 0;
    self->partial_read = 0;
    self->at_zero_buffer = 0;
    self->crossed_stream_end = 0;
    self->end_of_stream = 0;
    self->pending_arq_count = 0;
    self->ring_block_count = 0;
    self->source_blocks_remaining = 0;
    self->ring_write_block = 0;
    self->ring_play_block = 0;
    self->pending_ax_block = -1;
    self->ax_end_block = -1;
    self->end_pass_count = 0;
    self->stream_end_block = -1;
    self->initial_position = 0;
    self->ring_scan_stop_block = -1;
}

void SBPlayable_Stream::FreeResources(void) {
    Stop();
    SoundBuffer_Playable::FreeResources();
}

void SBPlayable_Stream::iPrepForPlay(void) {
    if (ready_to_play == 0 && last_read_pending == 0) {
        cache_buffers[0] = mslStreamCache_GetStreamBuffer();
        if (cache_buffers[0] == 0) {
            UnPrep();
            return;
        }
        cache_buffer_size = mslStreamCache_GetSizeBuffer();
        if (file_entry->has_secondary != 0) {
            ring_block_count = cache_buffer_size / 0x2000;
            cache_buffers[1] = mslStreamCache_GetStreamBuffer();
            if (cache_buffers[1] == 0) {
                UnPrep();
                return;
            }
            cache_buffer1_size = cache_buffer_size;
        } else {
            ring_block_count = cache_buffer_size / 0x4000;
        }

        if (cache_buffer_size >= file_entry->aram_size) {
            source_blocks_remaining = -1;
        } else {
            source_blocks_remaining = ring_block_count;
        }
        ring_write_block = 0;
        ring_play_block = 0;
        source_read_offset = 0;
        source_bytes_remaining = aligned_size;
        last_read_pending = 1;
    }
}

void SBPlayable_Stream::PrepForPlay(void) {
    while (pending_arq_count != 0) {
    }

    BOOL enabled = OSDisableInterrupts();
    iPrepForPlay();
    OSRestoreInterrupts(enabled);
}

int SBPlayable_Stream::IsReadyToPlay(void) {
    SBPlayable_Stream* self =
        this;

    return self->ready_to_play;
}

int SBPlayable_Stream::Play(u32 flags) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;
    int result = -1;

    if ((self->state & 2) == 0) {
        BOOL enabled;

        stream->play_flags = flags;
        this->PrepForPlay();
        enabled = OSDisableInterrupts();
        if ((flags & 1) != 0) {
            self->state = 3;
        } else {
            self->state = 2;
        }
        stream->play_flags = flags;
        if (stream->ready_to_play != 0) {
            result = iPlayPrepped();
        } else if (stream->last_read_pending != 0) {
            stream->play_when_ready = 1;
            result = 0;
        }
        OSRestoreInterrupts(enabled);
    }
    return result;
}

int SBPlayable_Stream::Stop(void) {
    BOOL enabled = OSDisableInterrupts();
    int result = SoundBuffer_Playable::Stop();

    voices_started = 0;
    UnPrep();
    OSRestoreInterrupts(enabled);
    return result;
}

int SBPlayable_Stream::Pause(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;
    BOOL voice_enabled;
    int result;
    BOOL enabled = OSDisableInterrupts();
    result = -1;

    this->StopIfDonePlaying();
    if ((self->state & 2) != 0) {
        voice_enabled = OSDisableInterrupts();

        if (self->voices[0] != 0) {
            AXSetVoiceState(self->voices[0], 0);
            result = 0;
        }
        if (self->voices[1] != 0) {
            AXSetVoiceState(self->voices[1], 0);
            result = 0;
        }
        OSRestoreInterrupts(voice_enabled);
        self->state = (self->state & 1) | 4;
    }
    stream->voices_started = 0;
    stream->play_when_ready = 0;
    OSRestoreInterrupts(enabled);
    return result;
}

void SBPlayable_Stream::iUpdate_AXUser(void) {
    SBPlayable_Stream* stream = this;
    int primary_block;
    int secondary_block;
    int channels;
    int crossed_scan_stop;
    s32 next_block;
    u32 primary_raw_address;

    if (stream->voices_started == 0) {
        return;
    }
    if (stream->source_blocks_remaining == -1) {
        return;
    }

    iAX_GetVoiceBlock(
        stream->voices[0], 0, primary_block,
        &primary_raw_address);

    if (stream->file_entry->has_secondary != 0) {
        u32 secondary_raw_address;

        channels = 2;
        iAX_GetVoiceBlock(
            stream->voices[1], 1, secondary_block,
            &secondary_raw_address);
    } else {
        channels = 1;
    }
    if (channels != 1 && secondary_block != primary_block) {
        return;
    }

    if (primary_block == -1) {
        if (stream->end_of_stream != 0 &&
            stream->stream_end_block == stream->ax_end_block) {
            if (stream->end_pass_count == 0) {
                stream->end_pass_count = 1;
            }
        } else {
            stream->crossed_stream_end = 1;
            if (stream->ring_play_block ==
                stream->stream_end_block - 1) {
                stream->ring_play_block =
                    stream->stream_end_block;
            }

            next_block = stream->ax_end_block + 1;
            if (next_block >= stream->ring_block_count) {
                next_block = 0;
            }
            crossed_scan_stop = 0;
            if (stream->ring_scan_stop_block == next_block) {
                next_block++;
                crossed_scan_stop = 1;
                if (next_block >= stream->ring_block_count) {
                    next_block = 0;
                }
            }
            if (next_block != stream->ring_write_block) {
                if (crossed_scan_stop != 0) {
                    stream->ring_scan_stop_block = -1;
                }
                while (stream->ring_play_block != next_block) {
                    stream->source_blocks_remaining++;
                    stream->ring_play_block++;
                    if (stream->ring_play_block >=
                        stream->ring_block_count) {
                        stream->ring_play_block = 0;
                    }
                }

                u32 block_address =
                    (stream->ring_play_block <<
                     (stream->segment_shift + 1)) + 2;

                i_AXVPB_SetVoice_CurrentPosition(stream->voices[0],
                    block_address + (stream->cache_buffers[0] << 1));
                i_AXVPB_SetVoice_ADPCM_StartPredScale(stream->voices[0],
                    stream->block_headers[0][stream->ring_play_block]);

                if (stream->voices[1] != 0) {
                    i_AXVPB_SetVoice_CurrentPosition(stream->voices[1],
                        block_address + (stream->cache_buffers[1] << 1));
                    i_AXVPB_SetVoice_ADPCM_StartPredScale(stream->voices[1],
                        stream->block_headers[1][stream->ring_play_block]);
                }

                iAX_FindNewEndBlock(next_block);
                AXSetVoiceState(
                    stream->voices[0], 1);
                if (stream->voices[1] != 0) {
                    AXSetVoiceState(
                        stream->voices[1], 1);
                }
            }
        }
    }

    if (primary_block == -1) {
        stream->at_zero_buffer = 1;
        return;
    }
    stream->at_zero_buffer = 0;

    if (stream->pending_ax_block == -1) {
        next_block = stream->ax_end_block + 1;
        if (next_block >= stream->ring_block_count) {
            next_block = 0;
        }
        crossed_scan_stop = 0;
        if (stream->ring_scan_stop_block == next_block) {
            next_block++;
            crossed_scan_stop = 1;
            if (next_block >= stream->ring_block_count) {
                next_block = 0;
            }
        }
        if (next_block != stream->ring_write_block) {
            if (crossed_scan_stop != 0) {
                stream->ring_scan_stop_block = -1;
            }
            stream->pending_ax_block = next_block;

            u32 block_address =
                (next_block <<
                 (stream->segment_shift + 1)) + 2;
            i_AXVPB_SetVoice_LoopPosition_PredScale(stream->voices[0],
                block_address + (stream->cache_buffers[0] << 1),
                stream->block_headers[0][next_block],
                stream->ax_end_block == stream->stream_end_block);
            if (stream->voices[1] != 0) {
                i_AXVPB_SetVoice_LoopPosition_PredScale(stream->voices[1],
                    block_address + (stream->cache_buffers[1] << 1),
                    stream->block_headers[1][next_block],
                    stream->ax_end_block == stream->stream_end_block);
            }
        }
    }

    if (primary_block < stream->ring_play_block ||
        primary_block > stream->ax_end_block) {
        if (stream->ax_end_block ==
            stream->stream_end_block) {
            stream->end_pass_count++;
            stream->stream_end_block = -1;
        }
        iAX_FindNewEndBlock(primary_block);
    }

    while (stream->ring_play_block != primary_block) {
        stream->source_blocks_remaining++;
        stream->ring_play_block++;
        if (stream->ring_play_block >=
            stream->ring_block_count) {
            stream->ring_play_block = 0;
        }
    }
}

void SBPlayable_Stream::iAX_FindNewEndBlock(int block) {
    SBPlayable_Stream* stream =
        this;
    int scan_stop = stream->ring_scan_stop_block;
    int end_block;

    do {
        end_block = block;
        block++;
    } while (block != scan_stop &&
             block < stream->ring_block_count &&
             block != stream->ring_write_block &&
             end_block != stream->stream_end_block);

    if (stream->ax_end_block == end_block) {
        return;
    }
    stream->ax_end_block = end_block;

    u32 end_address;
    if (end_block == stream->stream_end_block) {
        end_address =
            stream->initial_position +
            (end_block << (stream->segment_shift + 1));
    } else {
        end_address =
            ((end_block + 1) <<
             (stream->segment_shift + 1)) - 1;
    }
    stream->pending_ax_block = -1;

    _AXVPB* voice = stream->voices[0];

    i_AXVPB_SetVoice_EndPosition(
        voice, end_address + (stream->cache_buffers[0] << 1));
    i_AXVPB_SetVoice_Loop_To_Zero(voice);

    voice = stream->voices[1];
    if (voice == 0) {
        return;
    }
    i_AXVPB_SetVoice_EndPosition(
        voice, end_address + (stream->cache_buffers[1] << 1));
    i_AXVPB_SetVoice_Loop_To_Zero(voice);
}

void SBPlayable_Stream::iUpdate_MslTick(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;

    if (stream->ready_to_play != 0 ||
        stream->last_read_pending != 0) {
        if (stream->source_bytes_remaining == 0 &&
            stream->source_blocks_remaining != -1) {
            if ((self->state & 1) != 0) {
                stream->source_read_offset = 0;
                stream->source_bytes_remaining =
                    stream->aligned_size;
            } else {
                stream->end_of_stream = 1;
            }
        }

        this->StopIfDonePlaying();

        if ((stream->source_blocks_remaining > 0 ||
             stream->source_blocks_remaining == -1) &&
            stream->pending_arq_count == 0 &&
            stream->source_bytes_remaining != 0 &&
            stream->pending_file_request == 0) {
            s32 block_count;
            s32 read_size;
            s32 bytes_remaining;
            int cannot_read = 0;

            stream->partial_read = 0;
            if (stream->source_blocks_remaining == -1) {
                read_size = stream->source_bytes_remaining;
            } else {
                bytes_remaining =
                    stream->source_bytes_remaining;
                read_size =
                    ((bytes_remaining + 0x3FFF) >> 14) - 2;
                block_count = stream->source_blocks_remaining;
                if (block_count > read_size) {
                    block_count = read_size;
                }
                if (block_count > 0) {
                    if ((stream->last_read_pending != 0 ||
                         stream->at_zero_buffer != 0) &&
                        block_count > 3) {
                        block_count = 3;
                    }
                    read_size = block_count << 14;
                } else if (block_count == 0) {
                    if (stream->ring_write_block ==
                        stream->ring_block_count - 1) {
                        stream->ring_write_block = 0;
                        stream->ring_scan_stop_block =
                            stream->ring_block_count - 1;
                        stream->source_blocks_remaining--;
                    }
                    if (stream->source_blocks_remaining < 2) {
                        cannot_read = 1;
                    } else {
                        read_size = 0x8000;
                        if (bytes_remaining <= read_size) {
                            read_size = bytes_remaining;
                        }
                        stream->stream_end_block =
                            stream->ring_write_block + 1;
                    }
                } else {
                    cannot_read = 1;
                }
            }

            if (cannot_read == 0) {
                block_count =
                    (read_size + 0x3FFF) >> 14;
                if (read_size != block_count << 14) {
                    stream->partial_read = 1;
                }
                stream->queued_read_offset =
                    stream->source_read_offset;
                stream->queued_read_size = read_size;
                stream->queued_block_count = block_count;
                stream->pending_file_request =
                    mslStreamFile_QueueRequest(
                        stream->stream_context,
                        stream->primary_offset +
                            stream->queued_read_offset,
                        stream->queued_read_size,
                        stream->last_read_pending != 0
                            ? 0x10
                            : 0x10,
                        StreamFileRead_CallBack, stream);
            }
        }
    }

    if (stream->ready_to_play != 0 &&
        stream->play_when_ready != 0) {
        stream->play_when_ready = 0;
        iPlayPrepped();
    }
}

void SBPlayable_Stream::StopIfDonePlaying(void) {
    SBPlayable_Stream* stream =
        this;
    SoundBuffer_Playable* self = stream;

    if ((self->state & 2) != 0) {
        if (stream->source_blocks_remaining == -1) {
            if (stream->voices_started != 0 &&
                (self->state & 1) == 0) {
                int done = 0;

                if (self->voices[0] != 0 &&
                    (self->voices[0]->pb.state & 1) == 0) {
                    done = 1;
                } else if (self->voices[1] != 0 &&
                           (self->voices[1]->pb.state & 1) == 0) {
                    done = 1;
                }
                if (done != 0) {
                    this->Stop();
                }
            }
        } else if (stream->end_of_stream != 0 &&
                   stream->at_zero_buffer != 0 &&
                   stream->stream_end_block ==
                       stream->ax_end_block &&
                   (stream->ring_play_block ==
                        stream->stream_end_block ||
                    stream->ring_play_block ==
                        stream->stream_end_block - 1)) {
            this->Stop();
        }
    }
}

void SBPlayable_Stream::UnPrep(void) {
    BOOL enabled = OSDisableInterrupts();

    ready_to_play = 0;
    last_read_pending = 0;
    play_when_ready = 0;
    if (cache_buffers[0] != 0) {
        mslStreamCache_ReleaseBuffer(cache_buffers[0]);
    }
    if (cache_buffers[1] != 0) {
        mslStreamCache_ReleaseBuffer(cache_buffers[1]);
    }
    cache_buffers[0] = 0;
    cache_buffers[1] = 0;
    cache_buffer_size = 0;
    cache_buffer1_size = 0;
    if (pending_file_request != 0) {
        mslStreamFile_CancelRequest(pending_file_request);
        pending_file_request = 0;
    }
    OSRestoreInterrupts(enabled);
}

int SBPlayable_Stream::iPlayPrepped(void) {
    int result = -1;

    if (ready_to_play == 0 ||
        voices_started != 0) {
        return result;
    }

    if (voices[0] == 0) {
        int use_stream_loop;
        int use_loop_address;

        ring_play_block = 0;
        at_zero_buffer = 0;
        voices[0] = AXAcquireVoice(
            0x14, AcquireVoiceCallback, (uptr)this);
        if (voices[0] == 0) {
            return result;
        }
        AXSetVoicePriority(voices[0], 0x19);

        use_stream_loop = 1;
        use_loop_address = 1;
        if (source_blocks_remaining == -1) {
            use_stream_loop = 0;
            if ((state & 1) == 0) {
                use_loop_address = 0;
            }
        }

        if (file_entry->has_secondary != 0) {
            voices[1] = AXAcquireVoice(
                0x14, AcquireVoiceCallback, (uptr)this);
            if (voices[1] == 0) {
                AXFreeVoice(voices[0]);
                voices[0] = 0;
                return result;
            }
            AXSetVoicePriority(voices[1], 0x19);
            SPSoundEntry* sound = GetSoundEntryLeft();
            InitializeVoice(
                voices[0], sound, use_stream_loop,
                use_loop_address);
            sound = GetSoundEntryRight();
            InitializeVoice(
                voices[1], sound, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                voices[0], 0, 0, -960, -960,
                0, 0x7F, mix_fader);
            MIXInitChannel(
                voices[1], 0, 0, -960, -960,
                0x7F, 0x7F, mix_fader);
        } else {
            SPSoundEntry* sound = GetSoundEntryLeft();
            InitializeVoice(
                voices[0], sound, use_stream_loop,
                use_loop_address);
            MIXInitChannel(
                voices[0], 0, 0, -960, -960,
                pan, volume, mix_fader);
        }

        if (source_blocks_remaining == -1) {
            int channel;

            ring_play_block = 0;
            if (ring_write_block != 0) {
                ax_end_block =
                    ring_write_block - 1;
            } else {
                ax_end_block =
                    ring_block_count - 1;
            }
            if (use_loop_address != 0) {
                pending_ax_block = 0;
            } else {
                pending_ax_block = -1;
            }

            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    voices[channel];
                u32 base =
                    cache_buffers[channel] << 1;

                i_AXVPB_SetVoice_CurrentPosition(
                    voice, base + i_AXVPB_GetVoice_CurrentPosition(voice));
                i_AXVPB_SetVoice_EndPosition(
                    voice, base + i_AXVPB_GetVoice_EndPosition(voice));
                if (use_loop_address) {
                    i_AXVPB_SetVoice_LoopPosition(
                        voice, base + i_AXVPB_GetVoice_LoopPosition(voice));
                }
            }
        } else {
            int channel;

            ax_end_block =
                ring_play_block;
            pending_ax_block = -1;
            for (channel = 0;
                 channel < GetNumChannels(); channel++) {
                _AXVPB* voice =
                    voices[channel];
                u32 base =
                    cache_buffers[channel] << 1;

                i_AXVPB_SetVoice_CurrentPosition(voice, base + 2);
                i_AXVPB_SetVoice_EndPosition(
                    voice, base + (segment_size << 1) - 1);
                i_AXVPB_SetVoice_Loop_To_Zero(voice);
            }
        }
    }

    if (voices[0] != 0) {
        voices_started = 1;
        crossed_stream_end = 0;
        end_of_stream = 0;
        if (at_zero_buffer == 0) {
            AXSetVoiceState(voices[0], 1);
            if (voices[1] != 0) {
                AXSetVoiceState(voices[1], 1);
            }
        }
        result = 0;
    }
    return result;
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArq(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_ReturnBuffer(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
}

void SBPlayable_Stream::i_ARQCALLBACK_ArqComplete_LastRead(
    u32 request_address) {
    mslARQRequest* request = (mslARQRequest*)request_address;
    SBPlayable_Stream* stream =
        (SBPlayable_Stream*)request->callback_data;

    stream->pending_arq_count--;
    i_ARQCALLBACK_ReturnArqAndUserStreamBuffer(request_address);
    if (stream->last_read_pending != 0) {
        stream->last_read_pending = 0;
        stream->ready_to_play = 1;
    }
    if (stream->ready_to_play != 0) {
        stream->source_read_offset += stream->queued_read_size;
        stream->source_bytes_remaining -= stream->queued_read_size;
        if (stream->source_blocks_remaining > 0) {
            stream->source_blocks_remaining -=
                stream->queued_block_count;
        }
        stream->ring_write_block += stream->queued_block_count;
        if (stream->ring_write_block >= stream->ring_block_count) {
            stream->ring_write_block -= stream->ring_block_count;
        }
    }
}

void SBPlayable_Stream::StreamFileRead_CallBack(
    void* buffer, u32 offset, int size, int error,
    int final_chunk, void* callback_data) {
    SBPlayable_Stream* stream;
    ARQCallback callback;
    ARQCallback last_callback;
    BOOL enabled;

    enabled = OSDisableInterrupts();
    stream = (SBPlayable_Stream*)callback_data;

    if (error == 0) {
        if (final_chunk != 0) {
            stream->pending_file_request = 0;
            last_callback = i_ARQCALLBACK_ArqComplete_LastRead;
        } else {
            last_callback = i_ARQCALLBACK_ArqComplete_ReturnBuffer;
        }
        callback = i_ARQCALLBACK_ArqComplete;

        if (size == 0) {
            mslStreamFile_ReturnBuffer(buffer);
        } else {
            unsigned char* cursor;
            mslARQRequest* request;
            int remaining;
            s32 segment = (s32)offset >> stream->segment_shift;
            s32 channel;
            s32 block;
            s32 segment_size = stream->segment_size;
            s32 in_segment;
            s32 segment_remaining;

            remaining = size;
            cursor = (unsigned char*)buffer;
            if (stream->file_entry->has_secondary != 0) {
                channel = segment & 1;
                block = stream->ring_write_block + (segment >> 1);
            } else {
                channel = 0;
                block = stream->ring_write_block + segment;
            }
            in_segment = offset - segment * segment_size;
            segment_remaining = segment_size - in_segment;

            while (remaining != 0) {
                request = mslGetArqRequest();

                if (request == 0) {
                    _MSL_GCN_BREAK();
                    mslStreamFile_ReturnBuffer(buffer);
                    remaining = 0;
                    ((SoundBuffer_Playable*)stream)->Stop();
                } else {
                    u32 destination;

                    stream->pending_arq_count++;
                    request->callback_data = stream;
                    request->stream_buffer = 0;
                    if (block >= stream->ring_block_count) {
                        block -= stream->ring_block_count;
                    }
                    if (segment_remaining > remaining) {
                        segment_remaining = remaining;
                    }
                    remaining -= segment_remaining;
                    if (remaining == 0) {
                        request->stream_buffer = buffer;
                        callback = last_callback;
                        if (stream->partial_read != 0) {
                            segment_remaining =
                                (segment_remaining + 0x1F) & ~0x1FU;
                        }
                    }
                    if (in_segment == 0) {
                        stream->block_headers[channel][block] = *cursor;
                    }
                    destination =
                        stream->cache_buffers[channel] +
                        block * segment_size + in_segment;
                    DCFlushRange(cursor, segment_remaining);
                    ARQPostRequest(
                        &request->request, 0, 0, 1, (uptr)cursor,
                        destination, segment_remaining, callback);
                    cursor += segment_remaining;
                    if (remaining != 0) {
                        if (stream->file_entry->has_secondary != 0) {
                            channel ^= 1;
                            if (channel == 0) {
                                block++;
                            }
                        } else {
                            block++;
                        }
                        in_segment = 0;
                        segment_remaining = segment_size;
                    }
                }
            }
        }
    } else {
        if (error == 0x44DEAD0F) {
            stream->pending_file_request = 0;
        } else {
            _MSL_GCN_BREAK();
            stream->pending_file_request = 0;
        }
    }
    OSRestoreInterrupts(enabled);
}

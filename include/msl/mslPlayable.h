#ifndef MSL_MSLPLAYABLE_H
#define MSL_MSLPLAYABLE_H

#include "dolphin/types.h"

struct _mslBank;
struct _GameCubeFileEntry;
struct _AXVPB;
struct SPSoundEntry;
class MSLGCN_ARamBlock;
class SoundBuffer_Playable;
struct SoundBufferUpdateList;

class IRefCntRes {
public:
    IRefCntRes() : reference_count(1) {}
    virtual ~IRefCntRes();
    virtual void FreeObject(void) = 0;
    virtual void FreeResources(void);
    int reference_count;
};

class SoundBuffer : public IRefCntRes {
public:
    static void SB_MslTickCallback(void);
    static void SB_AXUserCallback(void);
    static SoundBuffer_Playable* CreatePlayableStreamBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    static SoundBuffer_Playable* CreatePlayableStaticBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    SoundBuffer();
    virtual ~SoundBuffer();
    virtual void PrepForPlay(void);
    virtual int IsReadyToPlay(void);
    virtual int GetNumChannels(void);
    virtual int GetStatus(u32* status);
    virtual int SetFrequency(u32 frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(s32 volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(u32 flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(u32 position);
};

class SoundBuffer_Data : public SoundBuffer {
public:
    MSLGCN_ARamBlock* aram_block;
    _mslBank* bank;
    _GameCubeFileEntry* file_entry;
    static SoundBuffer_Data* iCreateAramOrDestroyBuffer(
        SoundBuffer_Data* buffer, _mslBank* bank,
        _GameCubeFileEntry* entry);
    static SoundBuffer_Data* GetObject(void);
    SoundBuffer_Data();
    virtual ~SoundBuffer_Data();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int GetNumChannels(void);
    SPSoundEntry* GetSoundEntryLeft(void);
    SPSoundEntry* GetSoundEntryRight(void);
    void ResetValues(void);
};

class SoundBuffer_Playable : public SoundBuffer_Data {
public:
    static SoundBufferUpdateList ms_UpdateList;
    static void ListUpdate_AXUser(void);
    static void ListUpdate_MslTick(void);
    static SoundBuffer_Playable* GetObject(void);
    SoundBuffer_Playable();
    virtual ~SoundBuffer_Playable();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int IsReadyToPlay(void);
    virtual int GetStatus(u32* status);
    virtual int SetFrequency(u32 frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(s32 volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(u32 flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(u32 position);
    virtual void LostVoice(_AXVPB* voice);
    virtual void iUpdate_AXUser(void);
    virtual void iUpdate_MslTick(void);
    virtual void StopIfDonePlaying(void);
    static void AcquireVoiceCallback(void* voice);
    int iPlay(u32 flags, u32 acquire_priority,
              u32 active_priority);
    void InitializeVoice(_AXVPB* voice, SPSoundEntry* sound,
                         int stream_loop, int use_loop_address);
    void UpdateList_Insert(void);
    void UpdateList_Remove(void);
    void ResetValues(void);

    SoundBuffer_Playable* next;
    SoundBuffer_Playable* previous;
    u32 current_position;
    u32 mix_fader;
    unsigned char pan;
    unsigned char volume;
    u32 frequency;
    s32 state;
    _AXVPB* voices[2];
};

typedef SoundBuffer_Playable mslPlayable;

#endif

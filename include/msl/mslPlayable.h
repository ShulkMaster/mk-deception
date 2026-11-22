#ifndef MSL_MSLPLAYABLE_H
#define MSL_MSLPLAYABLE_H

struct _mslBank;
struct _GameCubeFileEntry;
struct _AXVPB;
struct SPSoundEntry;
class MSLGCN_ARamBlock;
class SoundBuffer_Playable;

class IRefCntRes {
public:
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
    virtual ~SoundBuffer();
    virtual void PrepForPlay(void);
    virtual int IsReadyToPlay(void);
    virtual int GetNumChannels(void);
    virtual int GetStatus(unsigned long* status);
    virtual int SetFrequency(unsigned long frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(long volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(unsigned long flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(unsigned long position);
};

class SoundBuffer_Data : public SoundBuffer {
public:
    MSLGCN_ARamBlock* aram_block;
    _mslBank* bank;
    _GameCubeFileEntry* file_entry;
    virtual ~SoundBuffer_Data();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int GetNumChannels(void);
    SPSoundEntry* GetSoundEntryLeft(void);
    SPSoundEntry* GetSoundEntryRight(void);
};

class SoundBuffer_Playable : public SoundBuffer_Data {
public:
    virtual ~SoundBuffer_Playable();
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int IsReadyToPlay(void);
    virtual int GetStatus(unsigned long* status);
    virtual int SetFrequency(unsigned long frequency);
    virtual int SetRelativeFrequency(float frequency);
    virtual int SetVolume(long volume);
    virtual int SetRelativeVolume(float volume);
    virtual int SetPan(unsigned char pan);
    virtual int SetSurroundPan(unsigned char pan);
    virtual int SetRelativePan(float pan);
    virtual int Play(unsigned long flags);
    virtual int Stop(void);
    virtual int Pause(void);
    virtual int UnPause(void);
    virtual int SetCurrentPosition(unsigned long position);
    virtual void LostVoice(_AXVPB* voice);
    virtual void iUpdate_AXUser(void);
    virtual void iUpdate_MslTick(void);
    virtual void StopIfDonePlaying(void);
    static void AcquireVoiceCallback(void* voice);
    int iPlay(unsigned long flags, unsigned long acquire_priority,
              unsigned long active_priority);
    void InitializeVoice(_AXVPB* voice, SPSoundEntry* sound,
                         int stream_loop, int use_loop_address);

    SoundBuffer_Playable* next;
    SoundBuffer_Playable* previous;
    unsigned long current_position;
    unsigned long mix_fader;
    unsigned char pan;
    unsigned char volume;
    unsigned long frequency;
    long state;
    _AXVPB* voices[2];
};

typedef SoundBuffer_Playable mslPlayable;

#endif

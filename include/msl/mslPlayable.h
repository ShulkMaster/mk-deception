#ifndef MSL_MSLPLAYABLE_H
#define MSL_MSLPLAYABLE_H

struct _mslBank;
struct _GameCubeFileEntry;
struct _AXVPB;
class MSLGCN_ARamBlock;
class SoundBuffer_Playable;

/* Canonical retail buffer hierarchy shared by producers and consumers. */
class IRefCntRes {
public:
    virtual ~IRefCntRes() {}
    virtual void FreeObject(void) = 0;
    virtual void FreeResources(void) {}
    int reference_count; /* +0x04, after the virtual-table pointer */
};

class SoundBuffer : public IRefCntRes {
public:
    static void SB_MslTickCallback(void);
    static void SB_AXUserCallback(void);
    static SoundBuffer_Playable* CreatePlayableStreamBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    static SoundBuffer_Playable* CreatePlayableStaticBuffer(
        _mslBank* bank, _GameCubeFileEntry* entry);
    /* Report-exact body; retail emission placement remains unresolved.
     * Moving this definition late breaks inlining in derived destructors. */
    virtual ~SoundBuffer() {}
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
    MSLGCN_ARamBlock* aram_block; /* +0x08 */
    _mslBank* bank;               /* +0x0C */
    _GameCubeFileEntry* file_entry; /* +0x10 */
    virtual ~SoundBuffer_Data() { FreeResources(); }
    virtual void FreeObject(void);
    virtual void FreeResources(void);
    virtual int GetNumChannels(void);
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

    SoundBuffer_Playable* next;     /* +0x14 */
    SoundBuffer_Playable* previous; /* +0x18 */
    unsigned long current_position; /* +0x1C */
    unsigned long mix_fader;        /* +0x20 */
    unsigned char pan;              /* +0x24 */
    unsigned char volume;           /* +0x25 */
    unsigned long frequency;        /* +0x28 */
    long state;                    /* +0x2C */
    _AXVPB* voices[2];              /* +0x30 */
};

typedef SoundBuffer_Playable mslPlayable;

#endif

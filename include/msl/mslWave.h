#ifndef MSL_MSLWAVE_H
#define MSL_MSLWAVE_H

#include "msl/mslBank.h"

#ifdef __cplusplus
extern "C" {
#endif

void mslWaveUpdateVolPanPitch(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int play_state);

int PlayStatic(
    _mslSystem* system, mslRuntimeWave* wave, int allow_voice);
int PlayStream(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int allow_voice);
int ContinueStatic(_mslSystem* system, mslRuntimeWave* wave);
int ContinueStream(_mslSystem* system, mslRuntimeWave* wave);
int PauseStatic(_mslSystem* system, mslRuntimeWave* wave);
int PauseStream(_mslSystem* system, mslRuntimeWave* wave);
int StopStatic(_mslSystem* system, mslRuntimeWave* wave);
void StopStream(_mslSystem* system, mslRuntimeWave* wave);
void UnCopyStaticWave(_mslSystem* system, mslRuntimeWave* wave);
void UnCopyStreamWave(_mslSystem* system, mslRuntimeWave* wave);

mslRuntimeWave* CopyStaticWave(
    _mslSystem* system, mslLoadedBank* bank,
    const mslRuntimeWave* source, int create_playable);
mslRuntimeWave* CopyStreamWave(
    _mslSystem* system, mslLoadedBank* bank,
    const char* name, const mslRuntimeWave* source, int create_playable);
mslRuntimeWave* LoadStaticWaveFile(
    _mslSystem* system, mslLoadedBank* bank,
    const char* name, unsigned long create_playable);
mslRuntimeWave* LoadStreamWaveFile(
    _mslSystem* system, mslLoadedBank* bank,
    const char* name, unsigned long flags);

int mslWavePlay(
    _mslSystem* system, mslRuntimeSound* sound,
    mslRuntimeWave* wave, int play_state);
int mslWaveUnLoad(_mslSystem* system, mslRuntimeWave* wave);
int mslWaveUnPause(_mslSystem* system, mslRuntimeWave* wave);
int mslWavePause(_mslSystem* system, mslRuntimeWave* wave);
void mslWaveStop(_mslSystem* system, mslRuntimeWave* wave);
void mslWaveUnCopy(_mslSystem* system, mslRuntimeWave* wave);
mslRuntimeWave* mslWaveCopy(
    _mslSystem* system, mslRuntimeWave* source,
    mslLoadedBank* bank, const char* name, int copy_flags);
mslRuntimeWave* mslWaveLoad(
    _mslSystem* system, mslLoadedBank* bank,
    const char* name, unsigned long flags);
int mslWaveSetPitch(mslRuntimeWave* wave, float pitch);
int mslWaveSetPan(mslRuntimeWave* wave, float pan);
int mslWaveSetVol(
    mslRuntimeWave* wave, _mslSystem* system, float volume);
int mslWaveUpdateStatus(mslRuntimeWave* wave);
void mslCreateLogTable(void);

#ifdef __cplusplus
}
#endif

#endif

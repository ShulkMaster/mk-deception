#ifndef MSL_TYPES_H
#define MSL_TYPES_H

/*
 * Public ILP32 MSL facade types used by game code. Sound playback returns a
 * list-pool ID, not an object pointer; zero is the invalid handle.
 */
typedef unsigned long MslSoundHandle;

typedef struct _mslSystem _mslSystem;
typedef struct mslLoadedBank mslLoadedBank;

#endif

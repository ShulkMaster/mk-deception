#ifndef RUNTIME_SOUND_DATA_H
#define RUNTIME_SOUND_DATA_H

/* Shared sound metadata layouts recovered from sound.o and its data units. */
typedef struct SoundEntry {
    int bank_index;
    int bank;
    int sound;
    int field_0c;
    float base_volume;
    unsigned char subgroup;
    unsigned char pad15[3];
    int field_18;
} SoundEntry;

typedef struct SoundSubgroupVolume {
    float volume;
    unsigned char setting_index;
    unsigned char pad05[3];
} SoundSubgroupVolume;

typedef struct RandomSoundRequest {
    int* sounds;
    int count;
    unsigned char previous;
    unsigned char pad09[3];
} RandomSoundRequest;

extern SoundEntry mk_sound_table[7180];
extern SoundSubgroupVolume subgroup_volume[12];
extern RandomSoundRequest random_sound_request[181];
extern float game_volume;

#endif

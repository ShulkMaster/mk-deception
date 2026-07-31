typedef struct SoundSubgroupVolume {
    float volume;
    unsigned char setting_index;
    unsigned char pad05[3];
} SoundSubgroupVolume;

SoundSubgroupVolume subgroup_volume[12] = {
    {0.75f, 0, {0, 0, 0}}, {0.75f, 0, {0, 0, 0}}, {0.85f, 0, {0, 0, 0}},
    {0.6f, 1, {0, 0, 0}},  {0.9f, 2, {0, 0, 0}},  {1.0f, 3, {0, 0, 0}},
    {0.9f, 4, {0, 0, 0}},  {0.7f, 1, {0, 0, 0}},  {0.8f, 0, {0, 0, 0}},
    {0.9f, 5, {0, 0, 0}},  {0.6f, 4, {0, 0, 0}},  {0.6f, 4, {0, 0, 0}},
};

float game_volume = 1.0f;

typedef struct MwsSubtitlePlayer {
    unsigned char reserved_000[0x2AC];
    int subtitle_channel;
} MwsSubtitlePlayer;

void mwPlySetSubtitleCh(MwsSubtitlePlayer* player, int channel) {
    player->subtitle_channel = channel;
}

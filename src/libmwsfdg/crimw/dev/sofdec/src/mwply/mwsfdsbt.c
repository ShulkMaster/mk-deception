void mwPlySetSubtitleCh(void* player, int channel) {
    *(int*)((char*)player + 0x2AC) = channel;
}

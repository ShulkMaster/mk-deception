typedef struct MemberHashtable {
    int unk0;
    char pad[0x28];
} MemberHashtable;

static MemberHashtable member_hashtable;

int init_pakfile_system(void) {
    member_hashtable.unk0 = 0;
    return 1;
}

#ifndef RUNTIME_SECTION_TYPES_H
#define RUNTIME_SECTION_TYPES_H

#include "runtime/mk_fileinfo.h"

typedef struct SecSlot SecSlot;
typedef struct SecSlotFileEntry SecSlotFileEntry;
typedef struct SecSlotGroup SecSlotGroup;
typedef struct SecSysState SecSysState;
typedef struct SectionSlotDef SectionSlotDef;
typedef struct SectionPerSlotDef SectionPerSlotDef;
typedef struct SecFileHeader SecFileHeader;
typedef struct SecArtMember SecArtMember;
typedef struct SsfReq SsfReq;

#define SEC_FILE_TYPE_ART 1
#define SEC_FILE_TYPE_ANIM 2

#define SEC_MAGIC 0x53454320u /* 'SEC ' */

/* Member types in SecArtMember.type (low 30 bits). */
#define SEC_MEMBER_TEXTURE 2
#define SEC_MEMBER_TEXTURE_ALT 3
#define SEC_MEMBER_RELOC 9

/*
 * On-disk / in-memory SEC art pack header (big-endian on disc; already BE in RAM).
 * Member table follows at +0x1C.
 */
struct SecFileHeader {
    unsigned int magic;        /* +0x00 SEC_MAGIC */
    unsigned int field_0x04;   /* +0x04 */
    unsigned int flags;        /* +0x08; 0 = run texture decode pass */
    unsigned int section_id;   /* +0x0C */
    unsigned int member_count; /* +0x10 */
    unsigned int field_0x14;   /* +0x14 */
    unsigned int field_0x18;   /* +0x18 */
};

/*
 * 0x10-byte SEC member. After process_art_section_data:
 *   +0x0C relocated to absolute (name string or payload)
 *   +0x04 for type 2/3 becomes RwTexture* (was file offset into SEC)
 */
struct SecArtMember {
    unsigned int type;     /* +0x00; use (type & 0x3FFFFFFF) */
    union {
        void* data_or_texture;
        unsigned int data_offset;
        struct RwTexture* texture;
    };                     /* +0x04; file offset before decode, pointer after */
    unsigned int size;     /* +0x08; binary member byte size (STRINGS/SCREEN) */
    union {
        char* name_or_data;
        unsigned int name_offset;
    };                     /* +0x0C; file offset before relocation, name after */
};

static inline SecArtMember* sec_file_members(SecFileHeader* header) {
    return (SecArtMember*)((unsigned char*)header + sizeof(*header));
}

struct SecSlot {
    int slot_id;
    unsigned char* base;
    unsigned int buffer_size;
    int file_count;
    SecSlotFileEntry* files;
};

/*
 * Per-file slot entry (0x2C bytes). Section bookkeeping and async file I/O share
 * this layout: +0x00..+0x10 are repurposed while a read is in flight (see
 * section_slot_file.c helpers).
 *
 * After process_art_section_data (asset.o) on a type-1 SEC:
 *   +0x0C  SEC buffer (SecFileHeader*)
 *   +0x10  SEC byte size
 *   +0x14  art section id (SEC+0x0C; e.g. legal_screen = 0x017E)
 *   +0x18  member count
 *   +0x1C  SecArtMember* table (SEC+0x1C)
 */
struct SecSlotFileEntry {
    MkFileInfo* section_info; /* +0x00; open_info during I/O */
    SsfReq* async_req;        /* +0x04 */
    int load_state;
    unsigned char* buffer; /* +0x0C; read buffer / SEC base */
    int size_or_flag;      /* +0x10; read_size / SEC size */
    int section_id;        /* +0x14; art section id after process_art */
    int member_count;      /* +0x18; SEC member count after process_art */
    SecArtMember* members; /* +0x1C; SEC+0x1C after process_art */
    int* palette_table; /* +0x20 - anim palette clear table */
    SecSlotFileEntry* next;
    unsigned char flags;
    char pad29[3];
};

typedef struct SsfReqLink {
    SsfReq* next;
} SsfReqLink;

typedef void (*SsfReqCompletion)(SsfReq* request);

struct SsfReq {
    SsfReqLink link;             /* +0x00 intrusive queue/free-list link */
    MkHwFileRequest* hwfile;     /* +0x04 */
    MkFileEntry* file_entry;     /* +0x08 */
    unsigned char loading;       /* +0x0C */
    unsigned char queued;        /* +0x0D */
    unsigned char cancelled;     /* +0x0E */
    unsigned char field_0x0F;
    MkFileEntry* ssf_file;       /* +0x10 */
    SecSlotFileEntry* owner;     /* +0x14 */
    SecSlot* slot;               /* +0x18 */
    int field_0x1C;
    MkFileInfo* info;            /* +0x20 */
    void* userdata;              /* +0x24 */
    SsfReqCompletion completion; /* +0x28 */
};                              /* 0x2C */

typedef struct SecSlotGroup {
    int group_id;
    int map_index;
    int slot_count;
    void* buffer;
    int buffer_size;
    SecSlot* slots;
    struct SecSlotGroup* next;
} SecSlotGroup;

typedef struct SecSysState {
    int total_memory;
    SectionSlotDef** current_map; /* points into section_memory_maps[] */
    int group_count;
    SecSlotGroup* group_list;
} SecSysState;

typedef struct SectionPerSlotDef {
    int slot_index;
    unsigned int buffer_size;
} SectionPerSlotDef;

typedef struct SectionSlotDef {
    int group_id;
    SectionPerSlotDef* per_slot_defs;
    int group_buffer_size;
} SectionSlotDef;

/*
 * Minimal player blob for get_shared_art_section_for_player.
 * type 0x1001 -> art 0x3000B; 0x1002 -> 0x4000B.
 */
typedef struct SharedArtPlayer {
    char pad00[0x10];
    int type; /* +0x10 */
} SharedArtPlayer;

#endif

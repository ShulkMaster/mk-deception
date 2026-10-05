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

#define SEC_MAGIC 0x53454320u

#define SEC_MEMBER_TEXTURE 2
#define SEC_MEMBER_TEXTURE_ALT 3
#define SEC_MEMBER_RELOC 9

struct SecFileHeader {
    unsigned int magic;
    unsigned int field_0x04;
    unsigned int flags;
    unsigned int section_id;
    unsigned int member_count;
    unsigned int field_0x14;
    unsigned int field_0x18;
};

struct SecArtMember {
    unsigned int type;
    union {
        void* data_or_texture;
        unsigned int data_offset;
        struct RwTexture* texture;
    };
    unsigned int size;
    union {
        char* name_or_data;
        unsigned int name_offset;
    };
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

struct SecSlotFileFlags {
    unsigned char clear_palette : 1;
    unsigned char : 7;
};

struct SecSlotFileEntry {
    MkFileInfo* section_info;
    SsfReq* async_req;
    int load_state;
    unsigned char* buffer;
    int size_or_flag;
    int section_id;
    int member_count;
    SecArtMember* members;
    int* palette_table;
    SecSlotFileEntry* next;
    union {
        unsigned char flags;
        struct SecSlotFileFlags flag_bits;
    };
    char pad29[3];
};

typedef struct SsfReqLink {
    SsfReq* next;
} SsfReqLink;

typedef void (*SsfReqCompletion)(SsfReq* request);

struct SsfReq {
    SsfReqLink link;
    void* hwfile;
    MkFileEntry* file_entry;
    unsigned char loading;
    unsigned char queued;
    unsigned char cancelled;
    unsigned char field_0x0F;
    MkFileEntry* ssf_file;
    SecSlotFileEntry* owner;
    SecSlot* slot;
    int field_0x1C;
    MkFileInfo* info;
    void* userdata;
    SsfReqCompletion completion;
};

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
    SectionSlotDef** current_map;
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
    unsigned int group_buffer_size;
} SectionSlotDef;

typedef struct MkObj MkObj;

#endif

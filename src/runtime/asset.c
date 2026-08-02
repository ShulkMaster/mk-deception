#include "runtime/asset.h"

#include "platform/gcinstance.h"
#include "runtime/section.h"
#include "runtime/section_slot_file.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "platform/display.h"
#include "platform/gcpipemanager.h"
#include "rw/rwcore_types.h"
#include "rw/rwobject.h"
#include "rw/rpworld_types.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef struct RwMemory {
    void* start;
    unsigned int length;
} RwMemory;

/* Native SEC texture payload view; distinct from the stock raster header. */
typedef struct AssetNativeRasterView {
    char pad00[0x28];
    unsigned int source_width;  /* +0x28; first u32 after name in SEC tex blob */
    unsigned int source_height; /* +0x2C; second u32 */
} AssetNativeRasterView;

typedef struct WiffTextureSequence {
    unsigned int frame_count;
    unsigned int first_texture;
    unsigned int has_alpha;
} WiffTextureSequence;

static AniTextureControl* _get_wiff(SecSlotFileEntry* entry,
                                    unsigned int offset);
static RwTexture* pull_texture_from_texdict(RwTexture* texture, void* data);
static RpClump* LoadDffFromSecInMemory(SecSlotFileEntry* entry,
                                       unsigned int offset);

RwStream* RwStreamOpen(int type, int accessType, void* data);
void RwStreamClose(RwStream* stream, void* data);
unsigned long RwStreamRead(RwStream* stream, void* buffer, unsigned long size);
RwStream* RwStreamSkip(RwStream* stream, unsigned long size);
int RwStreamFindChunk(RwStream* stream, unsigned int type,
                      unsigned int* length, unsigned int* version);
RwTexDictionary* RwTexDictionaryGetCurrent(void);
RwTexDictionary* RwTexDictionarySetCurrent(RwTexDictionary* dictionary);
RwTexDictionary* RwTexDictionaryCreate(void);
int RwTexDictionaryDestroy(RwTexDictionary* dictionary);
RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dictionary,
                                     RwTexture* texture);
RwTexDictionary* RwTexDictionaryForAllTextures(
    RwTexDictionary* dictionary,
    RwTexture* (*callback)(RwTexture*, void*), void* data);
RpClump* inplaceClumpStreamRead(RwStream* stream);
void destroy_clump(RpClump* clump);
void specular_condition_clump(RpClump* clump);
void* set_transl_callback(void* atomic, void* data);
RpClump* RpClumpForAllAtomics(
    RpClump* clump, void* (*callback)(void*, void*), void* data);
int strcmp(const char* a, const char* b);
char* strncpy(char* dst, const char* src, unsigned long n);

unsigned int plyr1_ss_tbl[2] = {0x0003000A, 0x0003000B};
unsigned int plyr2_ss_tbl[2] = {0x0004000A, 0x0004000B};

static inline MkObj* load_model_member(SecSlotFileEntry* entry,
                                       int member_index, int object_type,
                                       int transl) {
    RpClump* clump = LoadDffFromSecInMemory(
        entry, (unsigned int)entry->members[member_index].data_or_texture);
    MkObj* object = NULL;
    if (clump != NULL) {
        object = get_mkobj(object_type, clump);
        if (object == NULL) {
            destroy_clump(clump);
        } else {
            MK_CLUMP_PLUGIN(clump)->owner = object;
            specular_condition_clump(clump);
            GCNSetupNonRenderwarePipeline(clump, object);
            add_clump_to_world(World, clump);
        }
    }
    if (object != NULL && transl != 0)
        RpClumpForAllAtomics(object->clump, set_transl_callback, NULL);
    return object;
}

void annihilate_art_section_data(SecSlotFileEntry* entry) {
    unsigned int i;
    SecArtMember* member;
    int mtype;
    RwTexture* tex;
    unsigned int pin;

    i = 0;
    pin = 1;
    while (i < (unsigned int)entry->member_count) {
        member = &entry->members[i];
        mtype = (int)(member->type & 0x3FFFFFFFu);
        if (mtype == SEC_MEMBER_TEXTURE || mtype == SEC_MEMBER_TEXTURE_ALT) {
            tex = member->texture;
            if (tex != NULL) {
                tex->ref_count = pin;
                RwTextureDestroy(tex);
            }
        }
        i += 1;
    }
}

void process_anim_section_data(SecSlotFileEntry* entry) {
    SecFileHeader* sec = (SecFileHeader*)entry->buffer;
    int i;
    if (sec->magic == SEC_MAGIC) {
        entry->members = sec_file_members(sec);
        entry->member_count = sec->member_count;
        for (i = 0; i < entry->member_count; i++) {
            SecArtMember* member = &entry->members[i];
            if (member->data_or_texture != NULL)
                member->data_or_texture =
                    (unsigned char*)sec + member->data_offset;
            else
                member->data_or_texture = NULL;
        }
        if (entry->palette_table != NULL) {
            for (i = 0; i < entry->member_count; i++) {
                void* data = entry->members[i].data_or_texture;
                if (data != NULL) ((short*)data)[0x17] = i;
                entry->palette_table[i] = (int)data;
            }
        }
        sec_slot_file_free_async(entry);
        entry->load_state = 1;
    }
}

void* load_named_model_for_player(char* name, int player, int object_type,
                                  int flags) {
    unsigned int* slots = player == 0 ? plyr1_ss_tbl : plyr2_ss_tbl;
    unsigned int slot_index;
    for (slot_index = 0; slot_index < 2; slot_index++) {
        SecSlotFileEntry* entry =
            get_nth_sec_slot_file_from_handle(slots[slot_index], 1);
        int member_index;
        for (member_index = 0; member_index < entry->member_count;
             member_index++) {
            if (strcmp(entry->members[member_index].name_or_data, name) == 0)
                return load_model_member(entry, member_index, object_type,
                                         flags);
        }
    }
    return NULL;
}

void* load_named_model_for_bgnd(char* name, int object_type, int transl) {
    int file_index = get_slot_file_count(0x2001E);
    while (file_index > 0) {
        SecSlotFileEntry* entry =
            get_nth_sec_slot_file_from_handle(0x2001E, file_index);
        if (entry->section_info->type == SEC_FILE_TYPE_ART) {
            int member_index;
            for (member_index = 0; member_index < entry->member_count;
                 member_index++) {
                if (strcmp(entry->members[member_index].name_or_data, name) == 0)
                    return load_model_member(entry, member_index, object_type,
                                             transl);
            }
        }
        file_index--;
    }
    return NULL;
}

unsigned int get_artid_of_named_item_in_slot(
    int handle, char* name, int unused) {
    int file_count;
    int file_index;
    SecSlotFileEntry* entry;
    unsigned int member_index;

    (void)unused;
    /*
     * Soft ceiling: unreachable beq/li after type==ART (same CR).
     * Returns packed oid (section_id << 16) | member_index for load_tga.
     */
    file_count = get_slot_file_count(handle);
    file_index = 1;
    while (file_index <= file_count) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if (entry->section_info->type == SEC_FILE_TYPE_ART) {
            member_index = 0;
            while ((int)member_index < entry->member_count) {
                if (strcmp(entry->members[member_index].name_or_data, name) ==
                    0) {
                    break;
                }
                member_index += 1;
            }
            if ((int)member_index >= entry->member_count) {
                member_index = 0xffffffffu;
            }
            if (member_index != 0xffffffffu) {
                return ((unsigned int)entry->section_id << 16) |
                       (member_index & 0xFFFFu);
            }
        }
        file_index += 1;
    }
    return 0;
}

void* load_named_bloodpath_data_from_slot(int handle, char* name) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /*
     * Same named ART member scan as load_named_cdf_data_from_slot.
     * Soft ceiling: unreachable beq/li after type==ART (same CR).
     */
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        if (handle == -1) {
            data = NULL;
        } else {
            entry = get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry == NULL) {
                data = NULL;
            } else if (entry->section_info->type != SEC_FILE_TYPE_ART) {
                data = NULL;
            } else {
                found = -1;
                for (i = 0; i < entry->member_count; i++) {
                    member = &entry->members[i];
                    if (strcmp(member->name_or_data, name) == 0) {
                        found = i;
                        break;
                    }
                }
                if (found == -1) {
                    data = NULL;
                } else {
                    member = &entry->members[found];
                    data = entry->buffer + member->data_offset;
                }
            }
        }
        if (data != NULL) {
            return data;
        }
        file_index -= 1;
    }
    return NULL;
}

void* load_named_binary_block_from_file(int handle, int file_index, char* name,
                                        int* out_size) {
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;

    /*
     * Soft ceiling: unreachable beq/li after type==ART (same CR).
     * LoadScreenSet: (slot, add_art file_index, "STRINGS"|"SCREEN", &size).
     * Retail always stores *out_size on hit (no NULL guard).
     */
    if (handle == -1) {
        return NULL;
    }

    entry = get_nth_sec_slot_file_from_handle(handle, file_index);
    if (entry == NULL) {
        return NULL;
    }
    if (entry->section_info->type != SEC_FILE_TYPE_ART) {
        return NULL;
    }

    found = -1;
    for (i = 0; i < entry->member_count; i++) {
        member = &entry->members[i];
        if (strcmp(member->name_or_data, name) == 0) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        return NULL;
    }

    member = &entry->members[found];
    *out_size = (int)member->size;
    return entry->buffer + member->data_offset;
}

void* load_named_binary_block(int handle, char* name, int* out_size) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /*
     * Retail inlines the from_file body (handle==-1 inside the countdown).
     * Soft ceiling: unreachable beq/li after type==ART (same CR).
     */
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        if (handle == -1) {
            data = NULL;
        } else {
            entry = get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry == NULL) {
                data = NULL;
            } else if (entry->section_info->type != SEC_FILE_TYPE_ART) {
                data = NULL;
            } else {
                found = -1;
                for (i = 0; i < entry->member_count; i++) {
                    member = &entry->members[i];
                    if (strcmp(member->name_or_data, name) == 0) {
                        found = i;
                        break;
                    }
                }
                if (found == -1) {
                    data = NULL;
                } else {
                    member = &entry->members[found];
                    data = entry->buffer + member->data_offset;
                    *out_size = (int)member->size;
                }
            }
        }
        if (data != NULL) {
            return data;
        }
        file_index -= 1;
    }
    return NULL;
}

void* load_binary_block(int handle, unsigned int art_oid, int* out_size) {
    SecSlotFileEntry* entry;
    void* data;
    int file_index;
    unsigned int section_id;
    unsigned int member_index;
    SecArtMember* member;

    /* Same oid walk shape as load_tga (keep get_nth r3 through join). */
    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) {
            break;
        }
        file_index -= 1;
    }
    if (file_index <= 0) {
        data = NULL;
    } else {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
        *out_size = (int)member->size;
    }
    return data;
}

void* load_named_cdf_data_from_slot(int handle, char* name) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /*
     * Same named ART member scan as load_named_binary_block (no out_size).
     * Soft ceiling: unreachable beq/li after type==ART (same CR).
     */
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        if (handle == -1) {
            data = NULL;
        } else {
            entry = get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry == NULL) {
                data = NULL;
            } else if (entry->section_info->type != SEC_FILE_TYPE_ART) {
                data = NULL;
            } else {
                found = -1;
                for (i = 0; i < entry->member_count; i++) {
                    member = &entry->members[i];
                    if (strcmp(member->name_or_data, name) == 0) {
                        found = i;
                        break;
                    }
                }
                if (found == -1) {
                    data = NULL;
                } else {
                    member = &entry->members[found];
                    data = entry->buffer + member->data_offset;
                }
            }
        }
        if (data != NULL) {
            return data;
        }
        file_index -= 1;
    }
    return NULL;
}

void* get_nav_data(int handle, unsigned int art_oid) {
    SecSlotFileEntry* entry;
    void* data;
    int file_index;
    unsigned int section_id;
    unsigned int member_index;
    SecArtMember* member;

    /*
     * Soft ceiling: ~69% -- get_nth r3 join / NV spill vs load_tga-shaped walk.
     * Prefer prior shape (no extra file_index re-check) for menu oid loads.
     */
    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    file_index = get_slot_file_count(handle);
    entry = NULL;
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) {
            break;
        }
        file_index -= 1;
    }
    if (file_index <= 0) {
        entry = NULL;
    }
    if (entry == NULL) {
        data = NULL;
    } else {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
    }
    return data;
}

void* get_cdf_data(int handle, unsigned int art_oid) {
    SecSlotFileEntry* entry;
    void* data;
    int file_index;
    unsigned int section_id;
    unsigned int member_index;
    SecArtMember* member;

    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    file_index = get_slot_file_count(handle);
    entry = NULL;
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) {
            break;
        }
        file_index -= 1;
    }
    if (file_index <= 0) {
        entry = NULL;
    }
    if (entry == NULL) {
        data = NULL;
    } else {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
    }
    return data;
}

/*
 * Alpha pair for a named color TGA: find the first member named `name`, then
 * return the texture on the *next* member if it shares the same name.
 * Retail get_nth uses file_count (r25), not the loop index.
 * Soft ceiling: unreachable beq/li after type==ART (same CR).
 */
RwTexture* load_named_alpha_texture_from_slot(int handle, char* name) {
    int file_count;
    int file_index;
    SecSlotFileEntry* entry;
    RwTexture* tex;
    int member_index;

    if (handle == -1) {
        return NULL;
    }

    file_count = get_slot_file_count(handle);
    file_index = 1;
    while (file_index <= file_count) {
        /* Retail: mr r4, r25 (file_count), not file_index. */
        entry = get_nth_sec_slot_file_from_handle(handle, file_count);
        if (entry == NULL) {
            return NULL;
        }
        if (entry->section_info->type == SEC_FILE_TYPE_ART) {
            member_index = 0;
            while (member_index < entry->member_count) {
                if (strcmp(entry->members[member_index].name_or_data, name) ==
                    0) {
                    break;
                }
                member_index += 1;
            }
            if (member_index >= entry->member_count) {
                member_index = -1;
            }
            if (member_index != -1) {
                if ((unsigned int)(member_index + 1) <
                    (unsigned int)entry->member_count) {
                    if (strcmp(entry->members[member_index + 1].name_or_data,
                               name) == 0) {
                        if (entry == NULL) {
                            tex = NULL;
                        } else {
                            tex = entry->members[member_index + 1].texture;
                            if (tex != NULL) {
                                tex->ref_count = 2;
                            }
                        }
                        if (tex != NULL) {
                            strncpy(tex->name, name, 0x20);
                        }
                        return tex;
                    }
                }
            }
        }
        file_index += 1;
    }
    return NULL;
}

/*
 * Soft ceiling: load_named_tga_from_slot (~84.5%).
 * Retail keeps an unreachable beq/li/-1 after type==ART (same CR as the
 * taken beq into that block). Structured C cannot emit that dead block.
 * CreatePoly chrome uses this path; algo OK.
 */
RwTexture* load_named_tga_from_slot(int handle, const char* name) {
    int file_count;
    int file_index;
    SecSlotFileEntry* entry;
    RwTexture* tex;
    int member_index;

    file_count = get_slot_file_count(handle);
    file_index = 1;
    while (file_index <= file_count) {
        if (handle == -1) {
            tex = NULL;
        } else {
            entry = get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry == NULL) {
                tex = NULL;
            } else if (entry->section_info->type != SEC_FILE_TYPE_ART) {
                tex = NULL;
            } else {
                member_index = 0;
                while (member_index < entry->member_count) {
                    if (strcmp(entry->members[member_index].name_or_data,
                               name) == 0) {
                        break;
                    }
                    member_index += 1;
                }
                if (member_index >= entry->member_count) {
                    member_index = -1;
                }
                if (member_index == -1) {
                    tex = NULL;
                } else if (entry == NULL) {
                    /* Retail keeps this null check after a proven non-null entry. */
                    tex = NULL;
                } else {
                    tex = entry->members[member_index].texture;
                    if (tex != NULL) {
                        tex->ref_count = 2;
                    }
                    if (tex != NULL) {
                        strncpy(tex->name, name, 0x20);
                    }
                }
            }
        }
        if (tex != NULL) {
            strncpy(tex->name, name, 0x20);
            return tex;
        }
        file_index += 1;
    }
    return NULL;
}

/*
 * Soft ceiling: load_tga (~78%).
 * Retail keeps get_nth's r3 through the join (stmw r28, no entry NV);
 * structured C still spills entry. Logic matches; CreatePoly chrome OK.
 */
RwTexture* load_tga(int handle, unsigned int art_oid) {
    unsigned int section_id;
    unsigned int member_index;
    int file_index;
    SecSlotFileEntry* entry;
    RwTexture* tex;

    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) {
            break;
        }
        file_index -= 1;
    }
    if (file_index <= 0) {
        tex = NULL;
    } else {
        tex = entry->members[member_index].texture;
        if (tex != NULL) {
            tex->ref_count = 2;
        }
    }
    return tex;
}

void* load_model_from_slot(int handle, unsigned int art_oid, int player) {
    int file_index;
    unsigned int section_id = art_oid >> 16;
    unsigned int member_index = art_oid & 0xFFFF;
    SecSlotFileEntry* entry = NULL;
    RpClump* clump;
    MkObj* object = NULL;
    if (handle == -1) return NULL;
    file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) break;
        file_index--;
    }
    clump = LoadDffFromSecInMemory(
        entry, (unsigned int)entry->members[member_index].data_or_texture);
    if (clump != NULL) {
        object = get_mkobj(player, clump);
        if (object == NULL) {
            destroy_clump(clump);
        } else {
            MK_CLUMP_PLUGIN(clump)->owner = object;
            specular_condition_clump(clump);
            GCNSetupNonRenderwarePipeline(clump, object);
            add_clump_to_world(World, clump);
        }
    }
    return object;
}

void* load_named_wiff_from_slot(int handle, char* name) {
    int file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        AniTextureControl* result = NULL;
        if (handle != -1) {
            SecSlotFileEntry* entry =
                get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry != NULL && entry->section_info->type == SEC_FILE_TYPE_ART) {
                int member_index = 0;
                while (member_index < entry->member_count &&
                       strcmp(entry->members[member_index].name_or_data, name))
                    member_index++;
                if (member_index < entry->member_count)
                    result = _get_wiff(
                        entry, (unsigned int)entry->members[member_index]
                                   .data_or_texture);
            }
        }
        if (result != NULL) return result;
        file_index--;
    }
    return NULL;
}

void* get_wiff_atc_block(int handle, unsigned int art_oid) {
    int file_index = get_slot_file_count(handle);
    unsigned int section_id = art_oid >> 16;
    unsigned int member_index = art_oid & 0xFFFF;
    SecSlotFileEntry* entry = NULL;
    while (file_index > 0) {
        entry = get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id) break;
        file_index--;
    }
    return _get_wiff(
        entry, (unsigned int)entry->members[member_index].data_or_texture);
}

static AniTextureControl* _get_wiff(SecSlotFileEntry* entry,
                                    unsigned int offset) {
    AniTextureControl* control = get_ani_texture_control();
    WiffTextureSequence* sequence;
    unsigned int texture_base = 0;
    unsigned int frame;
    unsigned int member_index;
    if (control == NULL) return NULL;
    sequence = (WiffTextureSequence*)(entry->buffer + offset);
    set_ani_texture_framerate(control, 1.0f);
    while (texture_base < (unsigned int)entry->member_count &&
           (entry->members[texture_base].type & 0x3FFFFFFF) !=
               SEC_MEMBER_TEXTURE_ALT) {
        texture_base++;
    }
    member_index = texture_base + sequence->first_texture;
    for (frame = 0; frame < sequence->frame_count; frame++) {
        RwTexture* texture = entry->members[member_index++].data_or_texture;
        if (texture == NULL) {
            destroy_ani_texture_control(control);
            return NULL;
        }
        set_ani_texture_numframes(control, frame + 1);
        texture->filter_flags = (texture->filter_flags & 0xFFFFFF00) | 2;
        if (is_raster_power_of_two(texture->raster))
            texture->filter_flags =
                (texture->filter_flags & 0xFFFF00FF) | 0x1100;
        else
            texture->filter_flags =
                (texture->filter_flags & 0xFFFF00FF) | 0x3300;
        set_ani_texture_rwtexture(control, frame, texture);
        if (sequence->has_alpha != 0) {
            texture = entry->members[member_index++].data_or_texture;
            if (texture == NULL) {
                destroy_ani_texture_control(control);
                return NULL;
            }
            texture->filter_flags = (texture->filter_flags & 0xFFFFFF00) | 2;
            if (is_raster_power_of_two(texture->raster))
                texture->filter_flags =
                    (texture->filter_flags & 0xFFFF00FF) | 0x1100;
            else
                texture->filter_flags =
                    (texture->filter_flags & 0xFFFF00FF) | 0x3300;
            set_ani_texture_rwtexture_a(control, frame, texture);
            ani_texture_has_alpha_frames(control);
        }
    }
    return control;
}

/*
 * process_art_section_data (~96%). Readable SEC decode; Matching deferred.
 * Soft ceiling: reloc bdnz vs bne / filter ble vs bgt / NV coloring.
 */
void process_art_section_data(SecSlotFileEntry* entry) {
    RwMemory mem;
    SecFileHeader* sec;
    SecArtMember* member;
    unsigned int i;
    int mtype;
    int last_non_tex;
    unsigned int table_end;
    unsigned int rel;
    int* reloc;
    int reloc_count;
    int* reloc_ptr;
    RwStream* stream;
    int skip;
    unsigned char namelen;
    char name_buf[0x100];
    RwTexture* tex;
    unsigned int source_width;
    unsigned int source_height;
    int levels;
    AssetNativeRasterView* raster;

    mem.length = (unsigned int)entry->size_or_flag;
    mem.start = entry->buffer;
    sec = (SecFileHeader*)mem.start;
    if (sec->magic == SEC_MAGIC) {
        entry->member_count = (int)sec->member_count;
        entry->section_id = (int)sec->section_id;
        entry->members = sec_file_members(sec);

        if (sec->flags == 0) {
            last_non_tex = -1;
            table_end = (unsigned int)entry->members +
                        (unsigned int)entry->member_count * sizeof(SecArtMember);
            i = 0;
            while (i < (unsigned int)entry->member_count) {
                member = &entry->members[i];
                mtype = (int)(member->type & 0x3FFFFFFFu);
                if (mtype != SEC_MEMBER_TEXTURE && mtype != SEC_MEMBER_TEXTURE_ALT) {
                    last_non_tex = (int)i;
                }
                rel = member->name_offset;
                member->name_or_data = (char*)(table_end + rel);
                if (mtype == SEC_MEMBER_RELOC) {
                    reloc = (int*)((char*)entry->buffer +
                                   member->data_offset);
                    reloc_count = *reloc;
                    reloc_ptr = reloc + 1;
                    if (reloc_count > 0) {
                        do {
                            *reloc_ptr = *reloc_ptr + (int)(unsigned long)reloc;
                            reloc_ptr += 1;
                            reloc_count -= 1;
                        } while (reloc_count != 0);
                    }
                }
                i += 1;
            }

            if (last_non_tex != entry->member_count - 1) {
                stream = RwStreamOpen(3, 1, &mem);
                if (stream != NULL) {
                    /*
                     * Retail: lwz members[last_non_tex]+0x14 (= next member's
                     * data_or_texture; members[0] when last_non_tex == -1).
                     */
                    skip = entry->members[last_non_tex + 1].data_offset;
                    if (skip != 0) {
                        RwStreamSkip(stream, (unsigned long)skip);
                    }

                    last_non_tex += 1;
                    while ((unsigned int)last_non_tex < (unsigned int)entry->member_count) {
                        member = &entry->members[last_non_tex];
                        skip = member->data_offset -
                               (int)stream->bufferPosition;
                        if (skip != 0) {
                            RwStreamSkip(stream, (unsigned long)skip);
                        }

                        tex = NULL;
                        RwStreamRead(stream, &namelen, 1);
                        if (namelen != 0) {
                            RwStreamRead(stream, name_buf, namelen);
                        }
                        name_buf[namelen] = 0;
                        RwStreamRead(stream, &source_width, 4);
                        RwStreamRead(stream, &source_height, 4);
                        _inplaceNativeTextureRead(stream, &tex);

                        if (tex != NULL) {
                            if (tex->raster == NULL ||
                                (levels = RwRasterGetNumLevels(tex->raster), levels <= 1)) {
                                tex->filter_flags = (tex->filter_flags & 0xFFFFFF00u) | 2u;
                            } else {
                                tex->filter_flags = (tex->filter_flags & 0xFFFFFF00u) | 4u;
                            }
                            RwTextureSetName((RwTexture*)tex, name_buf);
                            raster = (AssetNativeRasterView*)tex->raster;
                            raster->source_height = source_height;
                            raster->source_width = source_width;
                        }

                        member->data_or_texture = tex;
                        last_non_tex += 1;
                    }

                    RwStreamClose(stream, NULL);
                }
            }
        }

        sec_slot_file_free_async(entry);
        entry->load_state = 1;
    }
}

void* load_model_from_slot_transl(int handle, unsigned int art_oid, int player) {
    MkObj* object = load_model_from_slot(handle, art_oid, player);
    if (object != NULL) {
        extern void* set_transl_callback(void* atomic, void* data);
        extern RpClump* RpClumpForAllAtomics(
            RpClump* clump, void* (*callback)(void*, void*), void* data);
        RpClumpForAllAtomics(object->clump, set_transl_callback, NULL);
    }
    return object;
}

void* load_named_model_from_slot(int slot, char* name, int flags,
                                 int user_data) {
    int file_index;
    if (slot == -1) return NULL;
    file_index = get_slot_file_count(slot);
    while (file_index > 0) {
        SecSlotFileEntry* entry =
            get_nth_sec_slot_file_from_handle(slot, file_index);
        if (entry->section_info->type == SEC_FILE_TYPE_ART) {
            int member_index;
            for (member_index = 0; member_index < entry->member_count;
                 member_index++) {
                if (strcmp(entry->members[member_index].name_or_data, name) == 0)
                    return load_model_member(entry, member_index, flags,
                                             user_data);
            }
        }
        file_index--;
    }
    return NULL;
}

static RpClump* LoadDffFromSecInMemory(SecSlotFileEntry* entry,
                                       unsigned int offset) {
    RwMemory memory;
    unsigned int texture_first;
    unsigned int texture_last;
    RwTexDictionary* saved_dictionary = RwTexDictionaryGetCurrent();
    RwTexDictionary* dictionary = NULL;
    RpClump* clump = NULL;
    RwStream* stream;
    memory.start = entry->buffer;
    memory.length = entry->size_or_flag;
    stream = RwStreamOpen(3, 1, &memory);
    if (stream == NULL) return NULL;
    RwStreamSkip(stream, offset);
    RwStreamRead(stream, &texture_first, sizeof(texture_first));
    RwStreamRead(stream, &texture_last, sizeof(texture_last));
    if (texture_last - texture_first + 1 != 0) {
        unsigned int member_index = 0;
        unsigned int texture_count = texture_last - texture_first + 1;
        dictionary = RwTexDictionaryCreate();
        RwTexDictionarySetCurrent(dictionary);
        while (member_index < (unsigned int)entry->member_count) {
            if ((entry->members[member_index].type & 0x3FFFFFFF) ==
                    SEC_MEMBER_TEXTURE_ALT && texture_first-- == 0) {
                unsigned int i;
                for (i = 0; i < texture_count; i++) {
                    RwTexture* texture =
                        entry->members[member_index + i].data_or_texture;
                    if (texture != NULL)
                        RwTexDictionaryAddTexture(dictionary, texture);
                }
                break;
            }
            member_index++;
        }
    }
    if (RwStreamFindChunk(stream, 0x10, NULL, NULL))
        clump = inplaceClumpStreamRead(stream);
    RwStreamClose(stream, NULL);
    if (dictionary != NULL) {
        RwTexDictionarySetCurrent(saved_dictionary);
        RwTexDictionaryForAllTextures(dictionary,
                                      pull_texture_from_texdict, NULL);
        RwTexDictionaryDestroy(dictionary);
    }
    return clump;
}

static RwTexture* pull_texture_from_texdict(RwTexture* texture, void* data) {
    (void)data;
    RwTexDictionaryRemoveTexture(texture);
    return texture;
}

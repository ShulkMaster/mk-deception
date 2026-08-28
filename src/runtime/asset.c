#include "runtime/asset.h"

#include "platform/gcinstance.h"
#include "runtime/cstring.h"
#include "runtime/section.h"
#include "runtime/section_slot_file.h"
#include "runtime/image.h"
#include "runtime/mk_obj.h"
#include "runtime/mk_plugins.h"
#include "runtime/mk_render.h"
#include "platform/display.h"
#include "platform/gcpipemanager.h"
#include "rw/rwcore_types.h"
#include "rw/rwobject.h"
#include "rw/rpworld_types.h"
#include "rw/rwstream.h"

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

typedef struct AssetTextureRange {
    unsigned int first;
    unsigned int last;
} AssetTextureRange;

static AniTextureControl* _get_wiff(SecSlotFileEntry* entry,
                                    unsigned int offset);
static RwTexture* pull_texture_from_texdict(RwTexture* texture, void* data);
static RpClump* LoadDffFromSecInMemory(SecSlotFileEntry* entry,
                                       unsigned int offset);

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
unsigned int plyr1_ss_tbl[2] = {0x0003000A, 0x0003000B};
unsigned int plyr2_ss_tbl[2] = {0x0004000A, 0x0004000B};

static inline MkObj* load_model_member(SecSlotFileEntry* entry,
                                       int member_index, int object_type,
                                       int transl) {
    MkObj* object = NULL;
    RpClump* clump = LoadDffFromSecInMemory(
        entry, (unsigned int)entry->members[member_index].data_or_texture);
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

static inline SecSlotFileEntry* find_slot_section(int handle,
                                                  unsigned int section_id) {
    int file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        SecSlotFileEntry* entry =
            get_nth_sec_slot_file_from_handle(handle, file_index);
        if ((unsigned int)entry->section_id == section_id)
            return entry;
        file_index--;
    }
    return NULL;
}

static inline int find_named_art_member(SecSlotFileEntry* entry,
                                        const char* name) {
    int member_index;

    if (entry->section_info->type != SEC_FILE_TYPE_ART)
        return -1;
    for (member_index = 0; member_index < entry->member_count; member_index++) {
        if (strcmp(entry->members[member_index].name_or_data, name) == 0)
            return member_index;
    }
    return -1;
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
    int* palette_table = entry->palette_table;
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
        if (palette_table != NULL) {
            SecArtMember* palette_members = sec_file_members(sec);
            for (i = 0; i < entry->member_count; i++, palette_table++) {
                void* data = palette_members[i].data_or_texture;
                if (data != NULL) ((short*)data)[0x17] = i;
                *palette_table = (int)data;
            }
        }
        sec_slot_file_free_async(entry);
        entry->load_state = 1;
    }
}

MkObj* load_named_model_for_player(const char* name, int player,
                                   int object_type, int flags) {
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

MkObj* load_named_model_for_bgnd(const char* name, int object_type, int transl) {
    MkObj* object;
    int file_index = get_slot_file_count(0x2001E);
    if (file_index == 0) {
        object = NULL;
    } else {
        while (file_index > 0) {
            SecSlotFileEntry* entry =
                get_nth_sec_slot_file_from_handle(0x2001E, file_index);
            int member_index = find_named_art_member(entry, name);

            if (member_index >= 0) {
                object =
                    load_model_member(entry, member_index, object_type, transl);
                break;
            }
            file_index--;
        }
        if (file_index <= 0)
            object = NULL;
    }
    if (object != NULL && transl != 0)
        RpClumpForAllAtomics(object->clump, set_transl_callback, NULL);
    return object;
}

unsigned int get_artid_of_named_item_in_slot(
    int handle, const char* name, int unused) {
    int file_count;
    int file_index;
    SecSlotFileEntry* entry;
    unsigned int member_index;

    (void)unused;
    /* Returns packed oid (section_id << 16) | member_index for load_tga. */
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

void* load_named_bloodpath_data_from_slot(int handle, const char* name) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /* Same named ART member scan as load_named_cdf_data_from_slot. */
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

void* load_named_binary_block_from_file(int handle, int file_index,
                                        const char* name, int* out_size) {
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;

    /* LoadScreenSet passes the 1-based file index and requires out_size. */
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

void* load_named_binary_block(int handle, const char* name, int* out_size) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /* Retail inlines the from-file body inside this countdown. */
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
    unsigned int section_id;
    unsigned int member_index;
    SecArtMember* member;

    /* Same oid walk shape as load_tga (keep get_nth r3 through join). */
    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    entry = find_slot_section(handle, section_id);
    if (entry != NULL) {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
        *out_size = (int)member->size;
    } else {
        data = NULL;
    }
    return data;
}

void* load_named_cdf_data_from_slot(int handle, const char* name) {
    int file_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    int i;
    int found;
    void* data;

    /* Same named ART member scan as load_named_binary_block (no out_size). */
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
    unsigned int section_id;
    unsigned int member_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    void* data;

    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    entry = find_slot_section(handle, section_id);
    if (entry != NULL) {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
    } else {
        data = NULL;
    }
    return data;
}

void* get_cdf_data(int handle, unsigned int art_oid) {
    unsigned int section_id;
    unsigned int member_index;
    SecSlotFileEntry* entry;
    SecArtMember* member;
    void* data;

    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    entry = find_slot_section(handle, section_id);
    if (entry != NULL) {
        member = &entry->members[member_index];
        data = entry->buffer + member->data_offset;
    } else {
        data = NULL;
    }
    return data;
}

/*
 * Alpha pair for a named color TGA: find the first member named `name`, then
 * return the texture on the *next* member if it shares the same name.
 * Retail get_nth uses file_count (r25), not the loop index.
 */
RwTexture* load_named_alpha_texture_from_slot(int handle, const char* name) {
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

/* Retail retains a duplicated ART-test block after the named-member scan. */
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

RwTexture* load_tga(int handle, unsigned int art_oid) {
    unsigned int section_id;
    unsigned int member_index;
    SecSlotFileEntry* entry;
    RwTexture* tex;

    section_id = art_oid >> 16;
    member_index = art_oid & 0xFFFFu;
    entry = find_slot_section(handle, section_id);
    if (entry != NULL) {
        tex = entry->members[member_index].texture;
        if (tex != NULL) {
            tex->ref_count = 2;
        }
    } else {
        tex = NULL;
    }
    return tex;
}

MkObj* load_model_from_slot(int handle, unsigned int art_oid,
                            int object_type) {
    unsigned int section_id;
    unsigned int member_index;
    SecSlotFileEntry* entry;
    RpClump* clump;
    MkObj* object;

    if (handle == -1)
        return NULL;
    member_index = art_oid & 0xFFFF;
    section_id = art_oid >> 16;
    entry = find_slot_section(handle, section_id);
    object = NULL;
    clump = LoadDffFromSecInMemory(
        entry, (unsigned int)entry->members[member_index].data_or_texture);
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
    return object;
}

AniTextureControl* load_named_wiff_from_slot(int handle, const char* name) {
    int file_index = get_slot_file_count(handle);
    while (file_index > 0) {
        AniTextureControl* result;
        SecSlotFileEntry* entry;
        int member_index;

        if (handle == -1) {
            result = NULL;
        } else {
            entry = get_nth_sec_slot_file_from_handle(handle, file_index);
            if (entry == NULL) {
                result = NULL;
            } else if (entry->section_info->type != SEC_FILE_TYPE_ART) {
                result = NULL;
            } else {
                member_index = find_named_art_member(entry, name);
                if (member_index == -1)
                    result = NULL;
                else
                    result = _get_wiff(
                        entry, (unsigned int)entry->members[member_index]
                                   .data_or_texture);
            }
        }
        if (result != NULL) {
            return result;
        }
        file_index--;
    }
    return NULL;
}

AniTextureControl* get_wiff_atc_block(int handle, unsigned int art_oid) {
    unsigned int section_id = art_oid >> 16;
    unsigned int member_index = art_oid & 0xFFFF;
    SecSlotFileEntry* entry = find_slot_section(handle, section_id);
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

/* Relocate SEC members, then instantiate each native texture payload. */
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
                        RwStreamSkip(stream, skip);
                    }

                    last_non_tex += 1;
                    while ((unsigned int)last_non_tex < (unsigned int)entry->member_count) {
                        member = &entry->members[last_non_tex];
                        skip = member->data_offset -
                               (int)stream->data.memory.position;
                        if (skip != 0) {
                            RwStreamSkip(stream, skip);
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

MkObj* load_model_from_slot_transl(int handle, unsigned int art_oid,
                                   int object_type) {
    MkObj* object = load_model_from_slot(handle, art_oid, object_type);
    if (object != NULL) {
        RpClumpForAllAtomics(object->clump, set_transl_callback, NULL);
    }
    return object;
}

MkObj* load_named_model_from_slot(int slot, const char* name, int object_type,
                                  int transl) {
    int file_index;
    if (slot == -1)
        return NULL;
    file_index = get_slot_file_count(slot);
    if (file_index == 0)
        return NULL;
    while (file_index > 0) {
        SecSlotFileEntry* entry =
            get_nth_sec_slot_file_from_handle(slot, file_index);
        int member_index = find_named_art_member(entry, name);

        if (member_index >= 0)
            return load_model_member(entry, member_index, object_type, transl);
        file_index--;
    }
    return NULL;
}

static RpClump* LoadDffFromSecInMemory(SecSlotFileEntry* entry,
                                       unsigned int offset) {
    RwMemory memory;
    AssetTextureRange texture_range;
    unsigned int texture_first;
    unsigned int texture_count;
    RwTexDictionary* saved_dictionary = RwTexDictionaryGetCurrent();
    RwTexDictionary* dictionary = NULL;
    RpClump* clump = NULL;
    RwStream* stream;

    memory.start = entry->buffer;
    memory.length = entry->size_or_flag;
    stream = RwStreamOpen(3, 1, &memory);
    if (stream == NULL) {
        return NULL;
    }

    RwStreamSkip(stream, offset);
    RwStreamRead(stream, &texture_range, sizeof(texture_range));
    texture_first = texture_range.first;
    texture_count = texture_range.last - texture_range.first + 1;
    if (texture_count != 0) {
        unsigned int member_index;

        dictionary = RwTexDictionaryCreate();
        RwTexDictionarySetCurrent(dictionary);
        for (member_index = 0;
             member_index < (unsigned int)entry->member_count;
             member_index++) {
            if ((int)(entry->members[member_index].type & 0x3FFFFFFF) ==
                    SEC_MEMBER_TEXTURE_ALT && texture_first-- == 0) {
                unsigned int i;
                for (i = 0; i < texture_count; i++) {
                    RwTexture* texture =
                        entry->members[member_index++].data_or_texture;
                    if (texture != NULL)
                        RwTexDictionaryAddTexture(dictionary, texture);
                }
                break;
            }
        }
    }
    if (RwStreamFindChunk(stream, 0x10, NULL, NULL)) {
        clump = inplaceClumpStreamRead(stream);
    }
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

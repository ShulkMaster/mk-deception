#ifndef RUNTIME_MK_FILEINFO_H
#define RUNTIME_MK_FILEINFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MkFileInfo MkFileInfo;
typedef struct MkFileEntry MkFileEntry;
typedef struct MkHwFileRequest MkHwFileRequest;
typedef void (*MkFileOpenCallback)(void* user, MkFileEntry* entry, int success);

struct MkFileInfo {
    char* name;             /* +0x00 */
    MkFileEntry* table;     /* +0x04: owning *_file_table */
    int type;               /* +0x08: MK_FILE_TYPE_* */
};                          /* 0x0C */

#define MK_FILE_TYPE_SECTION   1
#define MK_FILE_TYPE_ANIMATION 2
#define MK_FILE_TYPE_OBJECT    3
#define MK_FILE_TYPE_SSF       4

struct MkFileEntry {
    MkFileInfo* info;       /* +0x00 */
    unsigned int offset;    /* +0x04: byte offset in SSF */
    unsigned int size;      /* +0x08: byte length */
};                          /* 0x0C */

typedef struct SsfContext {
    MkFileEntry* entries;       /* +0x00 */
    MkHwFileRequest* hwfile;    /* +0x04 */
} SsfContext;                   /* 0x08 */

void disable_default_filesystem(void);
MkFileInfo* find_section_by_name(const char* name);
void* mk_file_read_async(void* buffer, int size, int count, MkFileEntry* entry);
int mk_file_length(MkFileEntry* entry);
unsigned int mk_file_read(void* buffer, unsigned int size, unsigned int count,
                          MkFileEntry* entry);
int mk_file_close(MkFileEntry* entry);
MkFileEntry* mk_file_open_language(MkFileInfo* info, const char* mode, void* userdata);
MkFileEntry* mk_file_open(MkFileInfo* info, const char* mode, void* userdata);
MkFileEntry* mk_file_open_async_withcallback(MkFileInfo* info, const char* mode, void* userdata,
                                             MkFileOpenCallback callback, void* user);
void init_file_loading_table(void);
MkFileInfo* offset_mk_file_info(MkFileInfo* info, int language);
int get_ssf_dir_index(MkFileInfo* info);
MkFileInfo* get_mk_file_info_from_current_ssf(int index);
int num_files_in_ssf(MkFileEntry* table);
MkFileEntry* get_current_ssf_file(void);
void load_ssf(MkFileEntry* ssf_entry);
void restore_previous_ssf(void);
void save_current_ssf(void);
void init_ssf_system(void);

extern const MkFileEntry attract_file_table[];
extern const MkFileEntry msel_art_file_table[];
extern const MkFileEntry pselect_file_table[];
extern MkFileInfo ssf_attract;
extern MkFileInfo ssf_msel_art;
extern MkFileInfo sec_scr_main_menu;
extern MkFileInfo sec_attract;
extern MkFileInfo sec_mkd_end_game;
extern MkFileInfo sec_legal_screen;
extern MkFileInfo sec_legal_screen_spa;
extern MkFileInfo sec_legal_screen_ger;
extern MkFileInfo sec_legal_screen_fre;
extern MkFileInfo sec_legal_screen_ita;
extern MkFileInfo sec_demo_logo;
extern MkFileInfo sec_sysart;

#ifdef __cplusplus
}
#endif

#endif

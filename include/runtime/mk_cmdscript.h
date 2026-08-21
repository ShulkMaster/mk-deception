#ifndef MK_CMDSCRIPT_H
#define MK_CMDSCRIPT_H

#include "runtime/mk_fileinfo.h"
#include "runtime/mk_proc.h"

/*
 * MKO command-script interpreter (mk_cmdscript.o).
 * CmdScript is a 0x1B0 MkHdr subclass attached to MkProc::pdata_list.
 *
 * ScriptSlotEntry (0xA8) is one element of script_slot_list.
 * ScriptSlot is the loaded-MKO body at entry+0x10 (0x98); APIs take ScriptSlot*.
 */

typedef struct ScriptTableDef {
    char* name;             /* +0x00 */
    unsigned int is_internal; /* +0x04 nonzero -> index into table_data (retail cmplwi) */
    int data_index;         /* +0x08 word index or absolute ptr if !is_internal */
    unsigned int row_count; /* +0x0c */
} ScriptTableDef; /* 0x10 */

typedef struct ScriptFuncDef {
    int name_offset;         /* +0x00 */
    int code_offset;         /* +0x04 word index into bytecode */
    unsigned int attrs_id;   /* +0x08 */
} ScriptFuncDef; /* 0x0c */

typedef struct LoadBgndCtx LoadBgndCtx;

typedef struct ScriptSlot {
    char name[0x40];              /* +0x00 strcpy target from file_info */
    int slot_index;               /* +0x40 */
    unsigned int func_count;      /* +0x44 */
    unsigned int pad48;           /* +0x48 */
    unsigned int string_limit;    /* +0x4c */
    unsigned int pad50;           /* +0x50 */
    unsigned int hdr_word0;       /* +0x54 from mko hdr[0] */
    unsigned int table_count;     /* +0x58 */
    unsigned int load_size;       /* +0x5c temp size during load */
    unsigned int max_table;       /* +0x60 */
    int data_words;               /* +0x64 */
    ScriptTableDef* table_defs;   /* +0x68 */
    unsigned int* table_data;     /* +0x6c */
    unsigned int tables_fixed_up; /* +0x70 */
    ScriptFuncDef* func_defs;     /* +0x74 */
    int string_reloc;             /* +0x78 */
    unsigned int string_base;     /* +0x7c */
    unsigned int table_schema_base; /* +0x80 */
    void* load_buf;               /* +0x84 */
    unsigned int* bytecode;       /* +0x88 */
    unsigned int pad8c;           /* +0x8c */
    LoadBgndCtx* load_ctx;        /* +0x90 - background load context */
    MkPtr* pdata_list;            /* +0x94 */
} ScriptSlot; /* 0x98 */

typedef struct ScriptSlotEntry {
    int state;          /* +0x00 0=empty 1=loading 2=ready */
    MkFileInfo* file_info; /* +0x04 */
    MkFileEntry* file;     /* +0x08 */
    void* async_req;    /* +0x0c */
    ScriptSlot body;    /* +0x10 .. +0xA7 */
} ScriptSlotEntry; /* 0xA8 */

typedef struct CmdScriptStackFrame {
    unsigned int* return_pc; /* +0x00 */
    int keep_alive;      /* +0x04 */
    unsigned int* saved_prev_pc; /* +0x08 */
    int args[17];        /* +0x0c .. frame stride 0x50 */
} CmdScriptStackFrame;

typedef struct CmdScript {
    MkVtable5* vtbl;                /* +0x00 */
    unsigned int instance;          /* +0x04 */
    ScriptSlot* mko;                /* +0x08 loaded MKO body / script slot */
    char* func_name;                /* +0x0c */
    int arg_word_count;             /* +0x10 hi half of stream word */
    unsigned int* pc;               /* +0x14 bytecode instruction pointer */
    unsigned int* prev_pc;          /* +0x18 previous bytecode instruction */
    unsigned int* arg_header;       /* +0x1c */
    int state;                      /* +0x20 0=idle 1=run 2=overflow */
    void* attrs_table;              /* +0x24 */
    int unk28;                      /* +0x28 */
    unsigned int regs[14];          /* +0x2c .. +0x63 */
    CmdScriptStackFrame stack_mem[4]; /* +0x64 .. +0x1A3 */
    CmdScriptStackFrame* stack_sp;  /* +0x1A4 */
    CmdScriptStackFrame* stack_end; /* +0x1A8 */
    int call_flags;                 /* +0x1AC */
} CmdScript; /* 0x1B0 */

/* One-shot tinystack pdata (0x10). */
typedef struct OneShotScriptPdata {
    MkHdr hdr;               /* +0x00 */
    ScriptSlot* script;      /* +0x08 */
    unsigned int func_index; /* +0x0c */
} OneShotScriptPdata;

extern CmdScript* active_cmdscript;
extern unsigned int* current_args;
extern CmdScript global_script_interpreter;

void one_shot_script_func(void* script, unsigned int function, int wait);
float p_run_one_shot_script(void);
void load_string_bank_async(unsigned int bank, char* name);
void load_string_bank(unsigned int bank, char* name);
char* get_string_by_id(unsigned int id);
void cmdscript_reset_stack(void);
void cmdscript_step_backward(void);
unsigned int get_script_stack_depth(void);
void push_script_stack_frame(int keep_alive);
void register_c_table(const char* name, void* table);
void parse_args(const char* fmt, ...);
char* get_script_string_arg(int index);
void* get_function_attributes_table(ScriptSlot* slot, int func_index);
void* get_data_table_by_name(const char* name);
int get_script_function_by_name(ScriptSlot* slot, const char* name);
int check_script_function_exists(ScriptSlot* slot, const char* name);
char* get_name_of_table_by_pointer(ScriptSlot* slot, void* table);
char* get_name_of_table(ScriptSlot* slot, unsigned int index);
unsigned int get_table_index_by_pointer(ScriptSlot* slot, void* table);
unsigned int get_row_count_for_table_by_pointer(ScriptSlot* slot, void* table);
unsigned int get_row_count_for_table(ScriptSlot* slot, unsigned int index);
void* get_data_table(ScriptSlot* slot, unsigned int index);
void cmdscript_execute(ScriptSlot* slot);
void cmdscript_setup_execution(ScriptSlot* slot, unsigned int func_index);
void cmdscript_set_parameters(CmdScript* script, unsigned int count, ...);
float call_player_script_function(ScriptSlot* slot);
void cmdscript_unload(ScriptSlot* slot);
int vdestroy_cmdscript(CmdScript* script);
void unload_script(int slot_index);
ScriptSlot* cmdscript_loadfile_language_by_name_async(int language, char* name);
ScriptSlot* cmdscript_loadfile_language_by_name(int language, char* name);
ScriptSlot* cmdscript_loadfile_by_name(int language, const char* name);
ScriptSlot* cmdscript_loadfile_language_async(int language, MkFileInfo* file_info);
ScriptSlot* cmdscript_loadfile_language(int language, MkFileInfo* file_info);
ScriptSlot* cmdscript_loadfile(int slot_index, MkFileInfo* file_info);
ScriptSlot* cmdscript_finish_load(int slot_index);
void deactivate_cmdscript(void);
void activate_cmdscript(void);
void set_process_as_scriptable(MkProc* proc);
CmdScript* get_cmdscript_for_proc(MkProc* proc);
CmdScript* alloc_cmdscript(void);
void fixup_data_tables(ScriptSlot* slot);
void script_system_reset(void);
void init_cmdscript_system(void);

#endif

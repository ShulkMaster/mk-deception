#include "runtime/mk_cmdscript.h"

#include "runtime/cstring.h"
#include "runtime/hashtable.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_struct.h"

typedef struct {
    unsigned char gpr;
    unsigned char fpr;
    unsigned char reserved[2];
    char* input_arg_area;
    char* reg_save_area;
} __va_list[1];

#define va_start(list, last_arg) __va_start(list, last_arg)
#define va_end(list) ((void)0)

void* __va_arg(__va_list ap, int type);
int get_language(void);
void mk_hwfile_cancel(void* req);
void mk_hwfile_free_request(void* req);
void mk_hwfile_wait_for_completion(void* req);
void* _mwMemMalloc(void* heap, unsigned long size, int flags, void* a, void* b, void* c);
void _mwMemFree(void* ptr, int a, int b);
void trial_register_script_function(unsigned int func_index);
float j_exit(void);

extern _mwMemHeap* SystemSwappableHeap;
extern void** script_callable_function_table;
extern int number_of_script_functions;
extern MkVtable5 vtbl_cmdscript;

typedef void (*ScriptBuiltinFn)(void);

typedef struct CmdScriptProcVtable {
    void* prefix[6];
    void (*sleep)(void);
    void* stack_ops[2];
    MkProcJumpSleepFn jump_sleep;
} CmdScriptProcVtable;

#define CMDSCRIPT_PROC_VTBL(proc_) ((CmdScriptProcVtable*)(proc_)->vtbl)

void _set_bit_field(void);
void _get_bit_field(void);
void _copy_stream_to_address(void);
void _call_script_function(void);
void _load_table_address(void);
void _unconditional_branch(void);
void _conditional_branch(void);
void _compare_float_float(void);
void _compare_uint_uint(void);
void _compare_int_int(void);
void _combine_float_float(void);
void _combine_uint_uint(void);
void _combine_int_int(void);
void _copy_register_to_address(void);
void _copy_column_address_to_register(void);
void _copy_column_to_register(void);
void _copy_constant_to_variable(void);
void _copy_register_to_variable(void);
void _copy_variable_to_register(void);
void _copy_register_to_register(void);
void _copy_constant_to_register(void);
void _copy_register_to_instruction(void);

static ScriptBuiltinFn builtin_script_function_table[22] = {
    _set_bit_field,
    _get_bit_field,
    _copy_stream_to_address,
    _call_script_function,
    _load_table_address,
    _unconditional_branch,
    _conditional_branch,
    _compare_float_float,
    _compare_uint_uint,
    _compare_int_int,
    _combine_float_float,
    _combine_uint_uint,
    _combine_int_int,
    _copy_register_to_address,
    _copy_column_address_to_register,
    _copy_column_to_register,
    _copy_constant_to_variable,
    _copy_register_to_variable,
    _copy_variable_to_register,
    _copy_register_to_register,
    _copy_constant_to_register,
    _copy_register_to_instruction,
};

/* Retail .bss -- 20 x 0xA8 entries */
static ScriptSlotEntry script_slot_list[20];
static Hashtable c_table_list;
CmdScript global_script_interpreter;
static int gap_06_803AB1FC_bss;

/* Retail .sbss: MWCC reverse decl -> current_args @0, active_cmdscript @4 */
CmdScript* active_cmdscript;
unsigned int* current_args;

static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kZero = 0.0f;
static const int gap_09_805118D4_sdata2 = 0;

enum { SCRIPT_SLOT_COUNT = 20 };

static inline MkProc* cmdscript_create_tinystack(int proc_id, int priority, MkProcEntryFn proc_fn,
                                                 int pdata_size, MkHdr** pdata_out) {
    return _create_mkproc_generic_tinystack(proc_id, priority, proc_fn, pdata_size, pdata_out);
}

static inline ScriptSlotEntry* slot_entry_at(int index) {
    return &script_slot_list[index];
}

static inline ScriptSlot* slot_at(int index) {
    return &script_slot_list[index].body;
}

static inline void* resolve_table_row(ScriptSlot* slot, unsigned int table_id) {
    ScriptTableDef* def;

    if (table_id > slot->max_table || table_id == 0) {
        return 0;
    }
    def = &slot->table_defs[table_id - 1];
    if (def->is_internal > 0) {
        return slot->table_data + def->data_index;
    }
    return (void*)def->data_index;
}

static inline CmdScript* find_cmdscript_in_list(MkPtr** list) {
    MkPtr* ptr;
    MkHdr* hdr;

    if (list != 0) {
        ptr = *list;
        while (ptr != 0) {
            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                MkPtr* next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                if (hdr->vtbl == &vtbl_cmdscript) {
                    return (CmdScript*)hdr;
                }
                ptr = ptr->next;
            }
        }
    }
    return 0;
}

static inline void init_cmdscript_fields(CmdScript* cs) {
    cs->func_name = 0;
    cs->mko = 0;
    cs->pc = 0;
    cs->prev_pc = 0;
    cs->arg_word_count = 0;
    cs->state = 0;
    cs->stack_sp = cs->stack_mem;
    cs->stack_end = cs->stack_sp + 1;
    cs->arg_header = 0;
    cs->continuation = 0;
    memset(cs->stack_mem, 0, sizeof(cs->stack_mem));
    memset(cs->regs, 0, sizeof(cs->regs));
}

static inline void execute_cmdscript(ScriptSlot* slot) {
    CmdScript* cs;
    CmdScriptStackFrame* stack_base;
    unsigned int instruction;
    unsigned int header;
    ScriptBuiltinFn builtin;
    int stop;

    cs = active_cmdscript;
    if (cs == 0) {
        return;
    }
    cs->mko = slot;
    cs->state = 1;
    cs->unk28 = 0;
    stack_base = cs->stack_mem;
    stop = 0;

    while (cs->pc != (unsigned int*)slot->pad8c && stop == 0) {
        instruction = *cs->pc;
        while (instruction == 0 && cs->stack_sp != stack_base) {
            cs->stack_end = cs->stack_sp;
            cs->stack_sp--;
            if ((unsigned int)cs->stack_sp < (unsigned int)stack_base) {
                cs->state = 2;
            }
            cs->prev_pc = cs->stack_sp->saved_prev_pc;
            cs->pc = cs->stack_sp->return_pc;
            if (cs->stack_sp->keep_alive == 0) {
                stop = 1;
                break;
            }
            instruction = *cs->pc;
        }
        if (instruction == 0 || cs->state == 2 || stop != 0) {
            break;
        }

        cs->prev_pc = cs->pc;
        builtin = *(ScriptBuiltinFn*)cs->pc;
        cs->pc++;
        current_args = cs->pc;
        cs->pc = current_args + 1;
        cs->arg_header = current_args;
        header = *current_args;
        cs->arg_word_count = header >> 16;
        cs->pc += header & 0xffff;
        builtin();
    }

    cs->stack_sp = stack_base;
    cs->stack_end = stack_base + 1;
    cs->state = 0;
}

/* ---- 0x8001394C ---- */

void one_shot_script_func(void* a, unsigned int b, int wait) {
    MkProc* proc;
    OneShotScriptPdata* pdata;
    int instance;
    float one;

    proc = cmdscript_create_tinystack(0x9028, 0x1f, (MkProcEntryFn)p_run_one_shot_script, 0x10,
                                      (MkHdr**)&pdata);
    if (proc != 0) {
        set_process_as_scriptable(proc);
        pdata->script = (ScriptSlot*)a;
        pdata->func_index = b;
        instance = proc->instance;
        if (wait != 0) {
            one = kOne;
            while (proc != 0 && proc->instance == instance) {
                _mkproc_sleep_ticks = one;
                CMDSCRIPT_PROC_VTBL(aproc)->sleep();
            }
        }
    }
}

/* ---- 0x80013A28 ---- */

float p_run_one_shot_script(void) {
    OneShotScriptPdata* pdata;

    pdata = (OneShotScriptPdata*)apdata;
    if (pdata == 0) {
        return kNegOne;
    }
    cmdscript_setup_execution(pdata->script, pdata->func_index);
    execute_cmdscript(pdata->script);
    return kNegOne;
}

/* ---- 0x80013BD8 / 0x80013C00 (krypt-critical) ---- */

void load_string_bank_async(unsigned int bank, char* name) {
    cmdscript_loadfile_language_by_name_async((int)(bank >> 16) - 1, name);
}

void load_string_bank(unsigned int bank, char* name) {
    cmdscript_loadfile_language_by_name((int)(bank >> 16) - 1, name);
}

/* ---- 0x80013C28 ---- */

char* get_string_by_id(unsigned int id) {
    unsigned int bank;
    unsigned int index;
    ScriptSlotEntry* entry;
    ScriptSlot* slot;
    unsigned int* row;
    unsigned int row_id;
    ScriptTableDef* def;
    unsigned int nstrings;

    bank = id >> 16;
    index = id & 0xffff;
    if (bank == 0) {
        return 0;
    }
    entry = slot_entry_at((int)bank - 1);
    if (entry->state != 2) {
        return 0;
    }
    slot = &entry->body;
    row = (unsigned int*)resolve_table_row(slot, slot->table_count);
    if ((unsigned int)row < (unsigned int)slot->table_data) {
        return 0;
    }
    if ((unsigned int)row > (unsigned int)(slot->table_data + slot->data_words)) {
        return 0;
    }
    row_id = row[-1];
    if (row_id < 1 || row_id > slot->max_table) {
        return 0;
    }
    def = &slot->table_defs[row_id - 1];
    if (slot->table_data + def->data_index != row) {
        return 0;
    }
    nstrings = def->row_count;
    if (index >= nstrings) {
        return 0;
    }
    return ((char**)row)[index];
}

/* ---- stack helpers ---- */

void cmdscript_reset_stack(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    cs->stack_sp = cs->stack_mem;
    active_cmdscript->state = 0;
}

void cmdscript_step_backward(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    cs->pc = cs->prev_pc;
}

unsigned int get_script_stack_depth(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    return ((unsigned int)cs->stack_sp - (unsigned int)cs->stack_mem) /
           sizeof(CmdScriptStackFrame);
}

static inline void push_script_stack_frame_inline(int keep_alive) {
    active_cmdscript->stack_sp->return_pc = active_cmdscript->pc;
    active_cmdscript->stack_sp->keep_alive = keep_alive;
    active_cmdscript->stack_sp->saved_prev_pc = active_cmdscript->prev_pc;
    active_cmdscript->stack_sp = active_cmdscript->stack_end;
    active_cmdscript->stack_end++;
    if (active_cmdscript->stack_sp >= (CmdScriptStackFrame*)&active_cmdscript->stack_sp)
        active_cmdscript->state = 2;
}

void push_script_stack_frame(int keep_alive) {
    push_script_stack_frame_inline(keep_alive);
}

void register_c_table(const char* name, void* table) {
    hashtable_store(&c_table_list, name, table);
}

/* ---- 0x80013E44 parse_args (va_list lift) ---- */

void parse_args(const char* fmt, ...) {
    __va_list ap;
    unsigned int arg_offset;
    char c;
    unsigned int val;
    unsigned int base;
    unsigned int limit;
    ScriptSlot* mko;

    va_start(ap, fmt);
    arg_offset = sizeof(unsigned int);
    while ((c = *fmt) != '\0') {
        if (c == 'i') {
            int* out = *(int**)__va_arg(ap, 1);
            *out = *(int*)((unsigned char*)current_args + arg_offset);
        } else if (c == 'u') {
            unsigned int* out = *(unsigned int**)__va_arg(ap, 1);
            *out = *(unsigned int*)((unsigned char*)current_args + arg_offset);
        } else if (c == 'f') {
            float* out = *(float**)__va_arg(ap, 1);
            *out = *(float*)((unsigned char*)current_args + arg_offset);
        } else if (c == 's') {
            char** out = *(char***)__va_arg(ap, 1);
            val = *(unsigned int*)((unsigned char*)current_args + arg_offset);
            if (val != 0) {
                mko = active_cmdscript->mko;
                base = mko->string_base;
                limit = mko->string_limit;
                if (val < base && val < limit) {
                    val = base + (val - 1);
                }
            }
            *out = (char*)val;
        } else if (c == 'v') {
            void** out = *(void***)__va_arg(ap, 1);
            *out = (void*)*(unsigned int*)((unsigned char*)current_args + arg_offset);
        } else if (c == 'c') {
            ScriptBuiltinFn* out = *(ScriptBuiltinFn**)__va_arg(ap, 1);
            val = *(unsigned int*)((unsigned char*)current_args + arg_offset);
            if (val == 0)
                *out = 0;
            else if (val < (unsigned int)number_of_script_functions)
                *out = (ScriptBuiltinFn)script_callable_function_table[val - 1];
            else
                *out = (ScriptBuiltinFn)val;
        }
        arg_offset += sizeof(unsigned int);
        fmt++;
    }
    va_end(ap);
}

char* get_script_string_arg(int index) {
    unsigned int val;
    unsigned int base;
    unsigned int limit;
    ScriptSlot* mko;

    val = current_args[index];
    if (val == 0) {
        return 0;
    }
    mko = active_cmdscript->mko;
    base = mko->string_base;
    limit = mko->string_limit;
    if (val < base && val < limit) {
        return (char*)(base + (val - 1));
    }
    return (char*)val;
}

void* get_function_attributes_table(ScriptSlot* slot, int func_index) {
    unsigned int attrs_id;

    attrs_id = slot->func_defs[func_index - 1].attrs_id;
    return resolve_table_row(slot, attrs_id);
}

void* get_data_table_by_name(const char* name) {
    void* found;
    unsigned int i;
    ScriptSlotEntry* entry;
    ScriptSlot* slot;
    unsigned int t;
    ScriptTableDef* def;
    int cmp;

    found = hashtable_get(&c_table_list, name);
    if (found != 0) {
        return found;
    }
    for (i = 0; i < SCRIPT_SLOT_COUNT; i++) {
        entry = slot_entry_at((int)i);
        if (entry->state == 2) {
            slot = &entry->body;
            /* Soft ceiling: ~94.9% -- NV carousel (5 regs rotated), retail
             * applies the -1 after the add (subi last, un-reassociated) and
             * bumps t before the 0x10 stride; no C shape reproduces both. */
            for (t = 0; t < slot->max_table; t++) {
                def = &slot->table_defs[t];
                if (def->is_internal != 0) {
                    cmp = strcmp(name, def->name + slot->string_reloc - 1);
                    if (cmp == 0) {
                        return slot->table_data + def->data_index;
                    }
                }
            }
        }
    }
    return 0;
}

int get_script_function_by_name(ScriptSlot* slot, const char* name) {
    unsigned int i;
    ScriptFuncDef* def;
    char* function_name;

    i = 0;
    def = slot->func_defs;
    while (i < slot->func_count) {
        function_name = (char*)(def->name_offset + slot->string_reloc);
        if (strcmp(name, function_name - 1) == 0) {
            return (int)i + 1;
        }
        i++;
        def++;
    }
    return 0;
}

int check_script_function_exists(ScriptSlot* slot, const char* name) {
    unsigned int i;
    ScriptFuncDef* def;
    char* function_name;

    i = 0;
    def = slot->func_defs;
    while (i < slot->func_count) {
        function_name = (char*)(def->name_offset + slot->string_reloc);
        if (strcmp(name, function_name - 1) == 0) {
            return (int)i + 1;
        }
        i++;
        def++;
    }
    return 0;
}

char* get_name_of_table_by_pointer(ScriptSlot* slot, void* table) {
    unsigned int base;
    unsigned int id;
    ScriptTableDef* def;

    base = (unsigned int)slot->table_data;
    if ((unsigned int)table < base) {
        return 0;
    }
    if ((unsigned int)table > base + (unsigned int)(slot->data_words * 4)) {
        return 0;
    }
    id = ((unsigned int*)table)[-1];
    if (id != 0 && id <= slot->max_table) {
        def = &slot->table_defs[id - 1];
        if (base + (unsigned int)(def->data_index * 4) != (unsigned int)table) {
            return 0;
        }
        return def->name + slot->string_reloc - 1;
    }
    return 0;
}

char* get_name_of_table(ScriptSlot* slot, unsigned int index) {
    char* table_name;

    if (index > slot->max_table || index == 0) {
        return 0;
    }
    table_name = slot->table_defs[index - 1].name + slot->string_reloc;
    table_name--;
    return table_name;
}

unsigned int get_table_index_by_pointer(ScriptSlot* slot, void* table) {
    unsigned int base;
    unsigned int id;
    ScriptTableDef* def;

    base = (unsigned int)slot->table_data;
    if ((unsigned int)table < base || (unsigned int)table > base + (unsigned int)(slot->data_words * 4)) {
        return 0;
    }
    id = ((unsigned int*)table)[-1];
    if (id != 0 && id <= slot->max_table) {
        def = &slot->table_defs[id - 1];
        if (base + (unsigned int)(def->data_index * 4) != (unsigned int)table) {
            return 0;
        }
        return id;
    }
    return 0;
}

unsigned int get_row_count_for_table_by_pointer(ScriptSlot* slot, void* table) {
    unsigned int base;
    unsigned int id;
    ScriptTableDef* def;

    base = (unsigned int)slot->table_data;
    if ((unsigned int)table < base || (unsigned int)table > base + (unsigned int)(slot->data_words * 4)) {
        return 0;
    }
    id = ((unsigned int*)table)[-1];
    if (id != 0 && id <= slot->max_table) {
        def = &slot->table_defs[id - 1];
        if (base + (unsigned int)(def->data_index * 4) != (unsigned int)table) {
            return 0;
        }
        return def->row_count;
    }
    return 0;
}

unsigned int get_row_count_for_table(ScriptSlot* slot, unsigned int index) {
    if (index > slot->max_table || index == 0) {
        return 0;
    }
    return slot->table_defs[index - 1].row_count;
}

void* get_data_table(ScriptSlot* slot, unsigned int index) {
    return resolve_table_row(slot, index);
}

/* ---- execute / setup ---- */

void cmdscript_execute(ScriptSlot* slot) {
    CmdScript* cs;
    CmdScriptStackFrame* stack_base;
    unsigned int instruction;
    unsigned int header;
    ScriptBuiltinFn builtin;
    int stop;

    cs = active_cmdscript;
    if (cs == 0) {
        return;
    }
    cs->mko = slot;
    cs->state = 1;
    cs->unk28 = 0;
    stack_base = cs->stack_mem;
    stop = 0;

    while (cs->pc != (unsigned int*)slot->pad8c && stop == 0) {
        instruction = *cs->pc;
        while (instruction == 0 && cs->stack_sp != stack_base) {
            cs->stack_end = cs->stack_sp;
            cs->stack_sp--;
            if ((unsigned int)cs->stack_sp < (unsigned int)stack_base) {
                cs->state = 2;
            }
            cs->prev_pc = cs->stack_sp->saved_prev_pc;
            cs->pc = cs->stack_sp->return_pc;
            if (cs->stack_sp->keep_alive == 0) {
                stop = 1;
                break;
            }
            instruction = *cs->pc;
        }
        if (instruction == 0 || cs->state == 2 || stop != 0) {
            break;
        }

        cs->prev_pc = cs->pc;
        builtin = *(ScriptBuiltinFn*)cs->pc;
        cs->pc++;
        current_args = cs->pc;
        cs->pc = current_args + 1;
        cs->arg_header = current_args;
        header = *current_args;
        cs->arg_word_count = header >> 16;
        cs->pc += header & 0xffff;
        builtin();
    }

    cs->stack_sp = stack_base;
    cs->stack_end = stack_base + 1;
    cs->state = 0;
}

void cmdscript_setup_execution(ScriptSlot* slot, unsigned int func_index) {
    ScriptFuncDef* def;

    if (active_cmdscript != 0 && func_index <= slot->func_count) {
        if (func_index == 0) {
            return;
        }
        active_cmdscript->mko = slot;
        active_cmdscript->stack_end = active_cmdscript->stack_sp + 1;
        def = &slot->func_defs[func_index - 1];
        active_cmdscript->attrs_table = resolve_table_row(slot, def->attrs_id);
        trial_register_script_function(func_index);
        def = &slot->func_defs[func_index - 1];
        active_cmdscript->func_name =
            (char*)(def->name_offset + slot->string_reloc - 1);
        active_cmdscript->continuation = 0;
        active_cmdscript->pc = slot->bytecode + def->code_offset;
        active_cmdscript->prev_pc = active_cmdscript->pc;
    }
}

void cmdscript_set_parameters(CmdScript* script, unsigned int count, ...) {
    __va_list ap;
    unsigned int i;
    int* val;

    if (script != 0 && count < 6 && count != 0) {
        va_start(ap, count);
        for (i = 0; i < count; i++) {
            val = (int*)__va_arg(ap, 1);
            script->stack_sp->args[i] = *val;
        }
        va_end(ap);
    }
}

float call_player_script_function(ScriptSlot* slot) {
    execute_cmdscript(slot);
    if (active_cmdscript->continuation != 0) {
        CMDSCRIPT_PROC_VTBL(aproc)->jump_sleep(active_cmdscript->continuation, kZero);
    } else {
        CMDSCRIPT_PROC_VTBL(aproc)->jump_sleep(j_exit, kZero);
    }
    return kZero;
}

void cmdscript_unload(ScriptSlot* slot) {
    ScriptSlotEntry* entry;
    MkProc* saved;

    if (slot == 0) {
        return;
    }
    entry = slot_entry_at(slot->slot_index);
    entry->state = 0;
    if (entry->async_req != 0) {
        saved = aproc;
        if (aproc != 0) {
            aproc = 0;
        } else {
            saved = 0;
        }
        mk_hwfile_cancel(&entry->async_req);
        if (saved != 0) {
            aproc = saved;
        }
        mk_hwfile_free_request(entry->async_req);
        entry->async_req = 0;
    }
    if (entry->file != 0) {
        mk_file_close(entry->file);
        entry->file = 0;
    }
    entry->file_info = 0;
    if (slot->load_buf != 0) {
        _mwMemFree(slot->load_buf, 0, 0);
    }
    destroy_list(&slot->pdata_list);
    memset(slot, 0, sizeof(ScriptSlot));
}

/* Retail leaves r3 untouched after memfree (int slot, no explicit return). */
int vdestroy_cmdscript(CmdScript* script) {
    script->instance = 0;
    mkhdr_memfree((MkHdr*)script);
}

void unload_script(int slot_index) {
    ScriptSlotEntry* entry;
    int state;

    entry = slot_entry_at(slot_index);
    state = entry->state;
    if ((state == 2 || state == 1) && entry != 0) {
        cmdscript_unload(&entry->body);
    }
}

ScriptSlot* cmdscript_loadfile_language_by_name_async(int language, char* name) {
    MkFileInfo* section;

    section = find_section_by_name(name);
    if (section == 0) {
        return 0;
    }
    return cmdscript_loadfile_language_async(language, section);
}

ScriptSlot* cmdscript_loadfile_language_by_name(int language, char* name) {
    MkFileInfo* section;

    section = find_section_by_name(name);
    if (section == 0) {
        return 0;
    }
    return cmdscript_loadfile_language(language, section);
}

ScriptSlot* cmdscript_loadfile_by_name(int language, const char* name) {
    MkFileInfo* section;

    section = find_section_by_name(name);
    if (section == 0) {
        return 0;
    }
    return cmdscript_loadfile(language, section);
}

ScriptSlot* cmdscript_loadfile_language_async(int language, MkFileInfo* file_info) {
    MkFileInfo* info;
    ScriptSlotEntry* entry;
    ScriptSlot* body;
    MkProc* saved;
    MkFileEntry* file;
    int length;
    unsigned int size;
    void* buf;

    info = offset_mk_file_info(file_info, get_language());
    saved = aproc;
    aproc = 0;
    entry = slot_entry_at(language);
    body = &entry->body;
    if (info->name != (char*)body) {
        strcpy(body->name, info->name);
    }
    entry->state = 1;
    entry->file_info = info;
    body->slot_index = language;
    file = mk_file_open(info, "rb", (void*)3);
    entry->file = file;
    if (file == 0) {
        body = 0;
    } else {
        length = mk_file_length(file);
        size = (unsigned int)(length + 0x7ff) & 0xfffff800;
        buf = _mwMemMalloc(SystemSwappableHeap, size, 5, 0, 0, 0);
        body->load_buf = buf;
        body->load_size = size;
        entry->async_req = mk_file_read_async(buf, 1, size, file);
        mk_file_close(file);
        entry->file = 0;
    }
    aproc = saved;
    return body;
}

ScriptSlot* cmdscript_loadfile_language(int language, MkFileInfo* file_info) {
    MkFileInfo* info;

    info = offset_mk_file_info(file_info, get_language());
    return cmdscript_loadfile(language, info);
}

ScriptSlot* cmdscript_loadfile(int slot_index, MkFileInfo* file_info) {
    ScriptSlotEntry* entry;
    ScriptSlot* body;
    MkFileEntry* file;
    int length;
    unsigned int size;
    void* buf;

    entry = slot_entry_at(slot_index);
    body = &entry->body;
    if (entry->state == 2 || entry->state == 1) {
        if (entry->file_info == file_info) {
            cmdscript_finish_load(slot_index);
            if (entry->state == 2) {
                return body;
            }
        } else {
            unload_script(slot_index);
        }
    }
    if (file_info->name != (char*)body) {
        strcpy(body->name, file_info->name);
    }
    entry->state = 1;
    entry->file_info = file_info;
    body->slot_index = slot_index;
    file = mk_file_open(file_info, "rb", (void*)3);
    entry->file = file;
    if (file == 0) {
        body = 0;
    } else {
        length = mk_file_length(file);
        size = (unsigned int)(length + 0x7ff) & 0xfffff800;
        buf = _mwMemMalloc(SystemSwappableHeap, size, 5, 0, 0, 0);
        body->load_buf = buf;
        body->load_size = size;
        entry->async_req = mk_file_read_async(buf, 1, size, file);
        mk_file_close(file);
        entry->file = 0;
    }
    cmdscript_finish_load(slot_index);
    return body;
}

ScriptSlot* cmdscript_finish_load(int slot_index) {
    ScriptSlotEntry* entry;
    ScriptSlot* slot;
    unsigned int* header;
    unsigned int* walk;
    unsigned int function_id;
    ScriptBuiltinFn resolved;
    int state;

    entry = slot_entry_at(slot_index);
    slot = &entry->body;
    state = entry->state;
    if (state != 2 && state != 1) {
        return 0;
    }
    if (state == 1) {
        mk_hwfile_wait_for_completion(&entry->async_req);
        mk_hwfile_free_request(entry->async_req);
        if (entry->state != 2) {
            entry->state = 2;
            entry->async_req = 0;
            header = (unsigned int*)slot->load_buf;
            slot->func_count = header[0];
            slot->hdr_word0 = header[1];
            slot->pad48 = header[2];
            slot->string_limit = header[3];
            slot->pad50 = header[4];
            slot->table_count = header[5];
            slot->max_table = header[6];
            slot->data_words = header[7];
            slot->func_defs = (ScriptFuncDef*)(header + 8);
            slot->table_defs = (ScriptTableDef*)(slot->func_defs + slot->func_count);
            slot->string_reloc =
                (int)(slot->table_defs + slot->max_table);
            slot->string_base = slot->string_reloc + slot->pad48;
            slot->table_schema_base = slot->string_base + slot->string_limit;
            slot->table_data =
                (unsigned int*)(slot->table_schema_base + slot->pad50);
            slot->bytecode = slot->table_data + slot->data_words;
            slot->pad8c = (unsigned int)slot->bytecode + slot->hdr_word0;

            fixup_data_tables(slot);

            walk = slot->bytecode;
            while ((unsigned int)walk < slot->pad8c) {
                if (*walk == 0) {
                    walk++;
                    continue;
                }
                function_id = *walk - 1;
                if ((function_id & 0xff000000) == 0) {
                    if ((unsigned int)number_of_script_functions <= function_id) {
                        break;
                    }
                    resolved = (ScriptBuiltinFn)script_callable_function_table[function_id];
                } else {
                    resolved = builtin_script_function_table[function_id & 0x00ffffff];
                }
                *walk = (unsigned int)resolved;
                walk += (walk[1] & 0xffff) + 2;
            }
        }
    }
    return slot;
}

void deactivate_cmdscript(void) {
    active_cmdscript = 0;
}

void activate_cmdscript(void) {
    active_cmdscript = find_cmdscript_in_list(&aproc->pdata_list);
}

/* ---- 0x80015344 (krypt-critical) ---- */

void set_process_as_scriptable(MkProc* proc) {
    MkPtr* next;
    MkPtr* ptr;
    MkHdr* hdr;
    CmdScript* cs;
    int found;

    found = 0;
    if (proc != (MkProc*)-0xc0) {
        ptr = proc->pdata_list;
        while (ptr != 0) {
            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                next = ptr->next;
                ptr->hdr = 0;
                destroy_mkptr(ptr);
                ptr = next;
            } else {
                if (hdr->vtbl != &vtbl_cmdscript) {
                    hdr = 0;
                }
                if (hdr != 0) {
                    found = 1;
                    break;
                }
                ptr = ptr->next;
            }
        }
    }
    if (found == 0) {
        cs = (CmdScript*)get_mkhdr(&vtbl_cmdscript, 0x1b0);
        if (cs == 0) {
            cs = 0;
        } else {
            init_cmdscript_fields(cs);
        }
        if (cs != 0) {
            mk_append((MkHdr*)cs, &proc->pdata_list);
            proc->flags_bits.game_info = 1;
        }
    }
}

CmdScript* get_cmdscript_for_proc(MkProc* proc) {
    return find_cmdscript_in_list(&proc->pdata_list);
}

CmdScript* alloc_cmdscript(void) {
    CmdScript* cs;

    cs = (CmdScript*)get_mkhdr(&vtbl_cmdscript, 0x1b0);
    if (cs == 0) {
        return 0;
    }
    init_cmdscript_fields(cs);
    return cs;
}

/* Soft ceiling: exact retail size/CFG; residual is GPR coloring and scheduling. */
void fixup_data_tables(ScriptSlot* slot) {
    unsigned int table_index;

    if (slot->tables_fixed_up != 0) {
        return;
    }

    for (table_index = 0; table_index < slot->max_table; table_index++) {
        ScriptTableDef* def = &slot->table_defs[table_index];

        if (def->is_internal == 0) {
            const char* name = def->name + slot->string_reloc - 1;
            void* table = hashtable_get(&c_table_list, name);

            if (table == 0) {
                unsigned int slot_index;

                for (slot_index = 0; slot_index < SCRIPT_SLOT_COUNT; slot_index++) {
                    ScriptSlotEntry* candidate_entry = slot_entry_at((int)slot_index);
                    ScriptSlot* candidate;
                    unsigned int candidate_index;

                    if (candidate_entry->state != 2) {
                        continue;
                    }
                    candidate = &candidate_entry->body;
                    for (candidate_index = 0;
                         candidate_index < candidate->max_table;
                         candidate_index++) {
                        ScriptTableDef* candidate_def =
                            &candidate->table_defs[candidate_index];

                        if (candidate_def->is_internal != 0 &&
                            strcmp(name, candidate_def->name +
                                             candidate->string_reloc - 1) == 0) {
                            table = candidate->table_data + candidate_def->data_index;
                            break;
                        }
                    }
                    if (table != 0) {
                        break;
                    }
                }
            }
            def->data_index = (int)table;
        }
    }

    for (table_index = 0; table_index < slot->max_table; table_index++) {
        ScriptTableDef* def = &slot->table_defs[table_index];

        if (def->is_internal != 0) {
            const unsigned char* schema =
                (const unsigned char*)(slot->table_schema_base + def->is_internal);
            unsigned int column_count = schema[3];
            unsigned int* value = slot->table_data + def->data_index;
            unsigned int row;

            for (row = 0; row < def->row_count; row++) {
                const unsigned char* column_type = schema + 4;
                unsigned int column;

                for (column = 0; column < column_count; column++) {
                    switch (column_type[column]) {
                    case 4:
                        if (*value != 0) {
                            *value += slot->string_base - 1;
                        }
                        break;
                    case 0x85:
                        if (*value != 0) {
                            ScriptTableDef* referenced = &slot->table_defs[*value - 1];

                            if (referenced->is_internal != 0) {
                                *value = (unsigned int)(slot->table_data +
                                                        referenced->data_index);
                            } else {
                                *value = (unsigned int)def->data_index;
                            }
                        }
                        break;
                    case 6:
                        if (*value != 0) {
                            *value = (unsigned int)script_callable_function_table[*value - 1];
                        }
                        break;
                    default:
                        break;
                    }
                    value++;
                }
            }
        }
    }
    slot->tables_fixed_up = 1;
}

void script_system_reset(void) {
    int i;
    ScriptSlotEntry* entry;

    for (i = 1; i < SCRIPT_SLOT_COUNT; i++) {
        entry = slot_entry_at(i);
        unload_script(i);
        entry->state = 0;
    }
    init_cmdscript_fields(&global_script_interpreter);
}

void init_cmdscript_system(void) {
    hashtable_dynamic_init(&c_table_list, 0x17, SystemSwappableHeap);
    memset(script_slot_list, 0, sizeof(script_slot_list));
}

/* ---- builtins (local; retail .fn order after init_cmdscript_system) ---- */

void _set_bit_field(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int* dst;
    unsigned int src;
    unsigned int mask;
    unsigned int shift;

    args = current_args;
    cs = active_cmdscript;
    dst = (unsigned int*)cs->regs[args[1]];
    src = cs->regs[args[2]];
    mask = args[4];
    shift = args[3] >> 16;
    *dst = (*dst & ~mask) | (src << shift);
}

void _get_bit_field(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int val;
    unsigned int mask;
    unsigned int shift;

    args = current_args;
    cs = active_cmdscript;
    val = cs->regs[args[2]];
    mask = args[4];
    shift = args[3] >> 16;
    cs->regs[args[1]] = (val & mask) >> shift;
}

void _copy_stream_to_address(void) {
    unsigned int* args;
    CmdScript* cs;
    void* dst;
    unsigned int size;

    args = current_args;
    cs = active_cmdscript;
    dst = (void*)cs->regs[args[1]];
    size = args[2] * 4;
    memcpy(dst, &args[3], size);
}

void _call_script_function(void) {
    unsigned int* args;
    unsigned int func_index;
    ScriptSlot* slot;
    ScriptFuncDef* function;

    args = current_args;
    func_index = args[1];
    push_script_stack_frame_inline(1);
    memcpy(active_cmdscript->stack_sp->args, &args[2],
           ((unsigned short)args[0] - 1) * sizeof(unsigned int));
    slot = active_cmdscript->mko;
    function = &slot->func_defs[func_index - 1];
    active_cmdscript->pc = slot->bytecode + function->code_offset;
    active_cmdscript->func_name = (char*)(function->name_offset + slot->string_reloc - 1);
}

void _load_table_address(void) {
    unsigned int* args;
    CmdScript* cs;

    args = current_args;
    cs = active_cmdscript;
    cs->regs[args[1]] = (unsigned int)get_data_table(cs->mko, args[2]);
}

void _unconditional_branch(void) {
    unsigned int* args;
    CmdScript* cs;
    int rel;
    unsigned int* base;

    /* Retail load order: current_args then active_cmdscript; PC field @+0x14. */
    args = current_args;
    cs = active_cmdscript;
    rel = (int)args[1];
    base = cs->pc;
    cs->pc = base + rel;
}

void _conditional_branch(void) {
    CmdScript* cs;
    unsigned int* args;
    int rel;
    unsigned int* base;

    /* Retail: active_cmdscript then current_args; bnelr if regs[0]!=0; else pc+=args[1]. */
    cs = active_cmdscript;
    args = current_args;
    if (cs->regs[0] != 0) {
        return;
    }
    rel = (int)args[1];
    base = cs->pc;
    cs->pc = base + rel;
}

void _compare_float_float(void) {
    unsigned int* args;
    CmdScript* cs;
    float a;
    float b;
    int op;
    int result;

    args = current_args;
    result = 0;
    cs = active_cmdscript;
    a = *(float*)&cs->regs[args[1]];
    b = *(float*)&cs->regs[args[2]];
    op = args[3];
    switch (op) {
    case 18:
        if (a < b)
            result = 1;
        break;
    case 15:
        if (a == b)
            result = 1;
        break;
    case 16:
        if (a > b)
            result = 1;
        break;
    case 14:
        if (a != b)
            result = 1;
        break;
    case 19:
        if (a <= b)
            result = 1;
        break;
    case 17:
        if (a >= b)
            result = 1;
        break;
    }
    cs->regs[args[1]] = (unsigned int)result;
}

void _compare_uint_uint(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int a;
    unsigned int b;
    int op;
    int result;

    args = current_args;
    result = 0;
    cs = active_cmdscript;
    a = cs->regs[args[1]];
    b = cs->regs[args[2]];
    op = args[3];
    switch (op) {
    case 18:
        if (a < b) {
            result = 1;
        }
        break;
    case 15:
        if (a == b) {
            result = 1;
        }
        break;
    case 16:
        if (a > b) {
            result = 1;
        }
        break;
    case 14:
        if (a != b) {
            result = 1;
        }
        break;
    case 19:
        if (a <= b) {
            result = 1;
        }
        break;
    case 17:
        if (a >= b) {
            result = 1;
        }
        break;
    case 13:
        if (a != 0 || b != 0) {
            result = 1;
        }
        break;
    case 12:
        if (a != 0 && b != 0) {
            result = 1;
        }
        break;
    }
    cs->regs[args[1]] = (unsigned int)result;
}

void _compare_int_int(void) {
    unsigned int* args;
    CmdScript* cs;
    int a;
    int b;
    int op;
    int result;

    args = current_args;
    result = 0;
    op = args[3];
    cs = active_cmdscript;
    a = (int)cs->regs[args[1]];
    b = (int)cs->regs[args[2]];
    switch (op) {
    case 18:
        if (a < b)
            result = 1;
        break;
    case 15:
        if (a == b)
            result = 1;
        break;
    case 16:
        if (a > b)
            result = 1;
        break;
    case 14:
        if (a != b)
            result = 1;
        break;
    case 19:
        if (a <= b)
            result = 1;
        break;
    case 17:
        if (a >= b)
            result = 1;
        break;
    case 13:
        if (a != 0 || b != 0)
            result = 1;
        break;
    case 12:
        if (a != 0 && b != 0)
            result = 1;
        break;
    }
    cs->regs[args[1]] = (unsigned int)result;
}

void _combine_float_float(void) {
    unsigned int* args;
    CmdScript* cs;
    float a;
    float b;
    int op;
    float result;

    args = current_args;
    result = 0.0f;
    op = args[3];
    cs = active_cmdscript;
    a = *(float*)&cs->regs[args[1]];
    b = *(float*)&cs->regs[args[2]];
    switch (op) {
    case 0:
        result = a + b;
        break;
    case 1:
        result = a - b;
        break;
    case 2:
        result = a * b;
        break;
    case 3:
        result = a / b;
        break;
    }
    *(float*)&cs->regs[args[1]] = result;
}

void _combine_uint_uint(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int a;
    unsigned int b;
    int op;
    unsigned int result;

    args = current_args;
    result = 0;
    op = args[3];
    cs = active_cmdscript;
    a = cs->regs[args[1]];
    b = cs->regs[args[2]];
    switch (op) {
    case 0:
        result = a + b;
        break;
    case 1:
        result = a - b;
        break;
    case 2:
        result = a * b;
        break;
    case 3:
        result = a / b;
        break;
    case 4:
        result = a % b;
        break;
    case 5:
        result = a & b;
        break;
    case 6:
        result = a | b;
        break;
    case 7:
        result = a << b;
        break;
    case 8:
        result = a >> b;
        break;
    }
    cs->regs[args[1]] = result;
}

void _combine_int_int(void) {
    unsigned int* args;
    CmdScript* cs;
    int a;
    int b;
    int op;
    int result;

    args = current_args;
    result = 0;
    op = args[3];
    cs = active_cmdscript;
    a = (int)cs->regs[args[1]];
    b = (int)cs->regs[args[2]];
    switch (op) {
    case 0:
        result = a + b;
        break;
    case 1:
        result = a - b;
        break;
    case 2:
        result = a * b;
        break;
    case 3:
        result = a / b;
        break;
    case 4:
        result = a % b;
        break;
    case 5:
        result = a & b;
        break;
    case 6:
        result = a | b;
        break;
    case 7:
        result = a << b;
        break;
    case 8:
        result = a >> b;
        break;
    }
    cs->regs[args[1]] = (unsigned int)result;
}

void _copy_register_to_address(void) {
    CmdScript* cs;
    void* destination;

    cs = active_cmdscript;
    destination = (void*)cs->regs[current_args[1]];
    memcpy(destination, &cs->regs[current_args[2]], current_args[3]);
}

void _copy_column_address_to_register(void) {
    unsigned int* args;
    CmdScript* script;
    unsigned int table;
    unsigned int row;
    unsigned int destination;
    unsigned int offset;
    unsigned int stride;

    args = current_args;
    script = active_cmdscript;
    table = script->regs[args[2]];
    row = script->regs[args[3]];
    destination = args[1];
    offset = args[4];
    stride = args[5];
    if (table != 0) {
        script->regs[destination] = table + row * stride + offset;
    }
}

void _copy_column_to_register(void) {
    unsigned int value;
    unsigned int dest_reg;
    unsigned int table;

    value = 0;
    dest_reg = current_args[1];
    table = active_cmdscript->regs[current_args[2]];
    if (table != 0) {
        memcpy(&value,
               (void*)(table + active_cmdscript->regs[current_args[3]] * current_args[5] +
                       current_args[4]),
               current_args[6]);
        active_cmdscript->regs[dest_reg] = value;
    }
}

void _copy_constant_to_variable(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int* stack_words;

    args = current_args;
    cs = active_cmdscript;
    stack_words = (unsigned int*)cs->stack_sp;
    stack_words[args[1]] = args[2];
}

void _copy_register_to_variable(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int* stack_words;

    args = current_args;
    cs = active_cmdscript;
    stack_words = (unsigned int*)cs->stack_sp;
    stack_words[args[1]] = cs->regs[args[2]];
}

void _copy_variable_to_register(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int* stack_words;

    args = current_args;
    cs = active_cmdscript;
    stack_words = (unsigned int*)cs->stack_sp;
    cs->regs[args[1]] = stack_words[args[2]];
}

void _copy_register_to_register(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    cs->regs[current_args[1]] = cs->regs[current_args[2]];
}

void _copy_constant_to_register(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    cs->regs[current_args[1]] = current_args[2];
}

void _copy_register_to_instruction(void) {
    unsigned int* args;
    CmdScript* cs;

    args = current_args;
    cs = active_cmdscript;
    cs->pc[args[2]] = cs->regs[args[1]];
}

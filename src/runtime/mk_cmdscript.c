#include "runtime/mk_cmdscript.h"

#include "mw/mwMem.h"
#include "platform/gcARam.h"
#include "game/ejb.h"
#include "game/trial.h"
#include "runtime/utils.h"
#include "runtime/cstring.h"
#include "runtime/cstdarg.h"
#include "runtime/hashtable.h"
#include "runtime/mk_pdata.h"
#include "runtime/mk_hwfile.h"
#include "runtime/mk_struct.h"
#include "runtime/mk_vtbl.h"
#include "runtime/section.h"

extern void** script_callable_function_table;
extern int number_of_script_functions;

typedef void (*ScriptBuiltinFn)(void);

struct CmdScriptProcVtable {
    void* prefix[6];
    void (*sleep)(void);
    void* stack_ops[2];
    MkProcJumpSleepFn jump_sleep;
};

#define CMDSCRIPT_PROC_VTBL(proc_) ((struct CmdScriptProcVtable*)(proc_)->vtbl)

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
static void _combine_float_float(void);
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
    _copy_register_to_instruction,
    _copy_constant_to_register,
    _copy_register_to_register,
    _copy_variable_to_register,
    _copy_register_to_variable,
    _copy_constant_to_variable,
    _copy_column_to_register,
    _copy_column_address_to_register,
    _copy_register_to_address,
    _combine_int_int,
    _combine_uint_uint,
    _combine_float_float,
    _compare_int_int,
    _compare_uint_uint,
    _compare_float_float,
    _conditional_branch,
    _unconditional_branch,
    _load_table_address,
    _call_script_function,
    _copy_stream_to_address,
    _get_bit_field,
    _set_bit_field,
};

enum { SCRIPT_SLOT_COUNT = 20 };

static ScriptSlotEntry script_slot_list[SCRIPT_SLOT_COUNT];
static Hashtable c_table_list;
CmdScript global_script_interpreter;

CmdScript* active_cmdscript;
unsigned int* current_args;

static const float kOne = 1.0f;
static const float kNegOne = -1.0f;
static const float kZero = 0.0f;

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

static inline CmdScript* find_cmdscript_for_process(MkProc* proc) {
    MkPtr* ptr;
    MkHdr* hdr;

    if (&proc->pdata_list != 0) {
        ptr = proc->pdata_list;
        while (ptr != 0) {
            hdr = ptr->hdr;
            if (ptr->instance != hdr->instance) {
                MkPtr* next = ptr->next;
                discard_stale_mkptr(ptr);
                ptr = next;
            } else {
                hdr = hdr->vtbl == MK_VTABLE_ADDRESS(vtbl_cmdscript) ? hdr : 0;
                if (hdr != 0) {
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

void one_shot_script_func(ScriptSlot* script, unsigned int function, int wait) {
    unsigned int instance;
    MkProc* proc;
    OneShotScriptPdata* pdata;
    float one;

    proc = _create_mkproc_generic_tinystack(
        0x9028, 0x1f, p_run_one_shot_script,
        sizeof(OneShotScriptPdata), (MkHdr**)&pdata);
    if (proc != 0) {
        set_process_as_scriptable(proc);
        pdata->script = script;
        pdata->func_index = function;
        instance = proc->instance;
        if (wait != 0) {
            one = kOne;
            while (MK_HDR_LIVE(proc, instance) != 0) {
                _mkproc_sleep_ticks = one;
                CMDSCRIPT_PROC_VTBL(aproc)->sleep();
            }
        }
    }
}

/* TODO: [breakthrough needed] 63.31%; 101 rows differ; inspect retail CFG and operand types. */
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

void load_string_bank_async(unsigned int bank, char* name) {
    cmdscript_loadfile_language_by_name_async((int)(bank >> 16) - 1, name);
}

void load_string_bank(unsigned int bank, char* name) {
    cmdscript_loadfile_language_by_name((int)(bank >> 16) - 1, name);
}

static inline unsigned int validated_string_table_count(
    ScriptSlot* slot, unsigned int* row)
{
    unsigned int row_id;
    ScriptTableDef* def;
    if ((unsigned int)row < (unsigned int)slot->table_data ||
        (unsigned int)row > (unsigned int)(slot->table_data + slot->data_words)) {
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
    return def->row_count;
}
static inline ScriptSlot* loaded_string_bank(unsigned int bank)
{
    ScriptSlotEntry* entry = slot_entry_at((int)bank - 1);
    if (entry->state != 2) {
        return 0;
    }
    return &entry->body;
}
char* get_string_by_id(unsigned int id) {
    unsigned int bank;
    unsigned int index;
    ScriptSlot* slot;
    unsigned int* row;
    unsigned int nstrings;

    bank = id >> 16;
    index = id & 0xffff;
    if (bank == 0) {
        return 0;
    }
    slot = loaded_string_bank(bank);
    if (slot == 0) {
        return 0;
    }
    row = resolve_table_row(slot, slot->table_count);
    nstrings = validated_string_table_count(slot, row);
    if (index >= nstrings) {
        return 0;
    }
    return ((char**)row)[index];
}

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

/* TODO: [breakthrough needed] 86.75%; frame-pointer subtraction regresses;
 * retail unsigned divide/shift lowering needs compiler-mode evidence. */
unsigned int get_script_stack_depth(void) {
    return (unsigned int)((unsigned char*)active_cmdscript->stack_sp -
                          (unsigned char*)active_cmdscript->stack_mem) /
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

/* TODO: [breakthrough needed] 82.64%; 60 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [near miss] 88.61%; relative offset decode agrees;
 * subtraction is hoisted before the limit comparison and uses a separate register. */
char* get_script_string_arg(int index) {
    unsigned int value;
    ScriptSlot* script;
    char* strings;

    value = current_args[index];
    if (value == 0) {
        return 0;
    }
    script = active_cmdscript->mko;
    strings = (char*)script->string_base;
    if (value < (unsigned int)strings) {
        unsigned int offset = value - 1;

        if (value < script->string_limit) {
            return &strings[offset];
        }
    }
    return (char*)value;
}

/* TODO: [breakthrough needed] 62.96%; 13 rows differ; inspect retail CFG and operand types. */
void* get_function_attributes_table(ScriptSlot* slot, int func_index) {
    unsigned int attrs_id;

    attrs_id = slot->func_defs[func_index - 1].attrs_id;
    return resolve_table_row(slot, attrs_id);
}

/* TODO: [near miss] 99.79%; two string-address load operands differ; behavior and CFG agree. */
void* get_data_table_by_name(const char* name) {
    ScriptTableDef* def;
    ScriptSlot* slot;
    ScriptSlotEntry* entry;
    unsigned int i;
    unsigned int t;
    int cmp;
    void* found;

    found = hashtable_get(&c_table_list, name);
    if (found != 0) {
        return found;
    }
    for (i = 0; i < SCRIPT_SLOT_COUNT; i++) {
        entry = slot_entry_at(i);
        if (entry->state == 2) {
            slot = &entry->body;
            for (t = 0; t < slot->max_table; t++) {
                def = &slot->table_defs[t];
                if (def->is_internal != 0) {
                    cmp = strcmp(name, def->name + (slot->string_reloc - 1));
                    if (cmp == 0) {
                        return slot->table_data + def->data_index;
                    }
                }
            }
        }
    }
    return 0;
}

/* TODO: [near miss] 99.35%; native byte-pointer relocation restores retail arithmetic;
 * sum destination and operand allocation remain in two rows. */
int get_script_function_by_name(ScriptSlot* slot, const char* name) {
    unsigned int i;
    char* function_name;

    for (i = 0; i < slot->func_count; i++) {
        function_name = (char*)slot->string_reloc;
        function_name += slot->func_defs[i].name_offset;
        if (strcmp(name, function_name - 1) == 0) {
            return (int)i + 1;
        }
    }
    return 0;
}

/* TODO: [near miss] 99.35%; native byte-pointer relocation restores retail arithmetic;
 * sum destination and operand allocation remain in two rows. */
unsigned int check_script_function_exists(ScriptSlot* slot, const char* name) {
    unsigned int i;
    char* function_name;

    for (i = 0; i < slot->func_count; i++) {
        function_name = (char*)slot->string_reloc;
        function_name += slot->func_defs[i].name_offset;
        if (strcmp(name, function_name - 1) == 0) {
            return i + 1;
        }
    }
    return 0;
}

/* TODO: [breakthrough needed] 77.61%; 13 rows differ; inspect retail CFG and operand types. */
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
    int table_index;
    unsigned int name_offset;
    unsigned int name_address;

    if (index > slot->max_table || index == 0) {
        return 0;
    }
    table_index = index - 1;
    name_offset = (unsigned int)slot->table_defs[table_index].name;
    name_address = slot->string_reloc;
    name_address = name_offset + name_address;
    name_address -= 1;
    return (char*)name_address;
}

/* TODO: [breakthrough needed] 83.63%; 12 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 84.16%; 9 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 66.91%; 91 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 77.85%; 39 rows differ; inspect retail CFG and operand types. */
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

    if (script != 0 && count <= 5 && count != 0) {
        __builtin_va_info(&ap);
        for (i = 0; i < count; i++) {
            val = __va_arg(ap, 1);
            script->stack_sp->args[i] = *val;
        }
        va_end(ap);
    }
}

/* TODO: [breakthrough needed] 67.33%; 98 rows differ; inspect retail CFG and operand types. */
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
        saved = 0;
        if (aproc != 0) {
            saved = aproc;
            aproc = 0;
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

void vdestroy_cmdscript(CmdScript* script) {
    script->instance = 0;
    mkhdr_memfree((MkHdr*)script);
}

void unload_script(int slot_index) {
    ScriptSlotEntry* entry;
    int state;

    entry = &script_slot_list[slot_index];
    state = entry->state;
    if (state == 2 || state == 1) {
        cmdscript_unload(&entry->body);
    }
}

ScriptSlot* cmdscript_loadfile_language_by_name_async(int language, char* name) {
    MkFileInfo* section;

    section = find_section_by_name(name);
    if (section != 0) {
        return cmdscript_loadfile_language_async(language, section);
    }
    return 0;
}

ScriptSlot* cmdscript_loadfile_language_by_name(int language, char* name) {
    MkFileInfo* section;

    section = find_section_by_name(name);
    if (section != 0) {
        return cmdscript_loadfile_language(language, section);
    }
    return 0;
}

ScriptSlot* cmdscript_loadfile_by_name(int language, const char* name) {
    MkFileInfo* section = find_section_by_name(name);
    if (section != 0) {
        return cmdscript_loadfile(language, section);
    }
    return 0;
}

/* TODO: [near miss] 98.80%; entry/info register allocation differs in 16 rows. */
ScriptSlot* cmdscript_loadfile_language_async(int language, MkFileInfo* file_info) {
    ScriptSlotEntry* entry;
    ScriptSlot* body;
    MkFileInfo* info;
    MkProc* saved;
    int selected_language;

    selected_language = get_language();
    info = offset_mk_file_info(file_info, selected_language);
    saved = aproc;
    aproc = 0;
    saved_aproc = saved;
    entry = slot_entry_at(language);
    body = &entry->body;
    if (info->name != body->name) {
        strcpy(body->name, info->name);
    }
    entry->state = 1;
    entry->file_info = info;
    body->slot_index = language;
    entry->file = mk_file_open(info, "rb", (void*)3);
    if (entry->file == 0) {
        body = 0;
    } else {
        unsigned int size = ((unsigned int)mk_file_length(entry->file) + 0x7ff) & 0xfffff800;
        body->load_buf = _mwMemMalloc(SystemSwappableHeap, size, 5, 0, 0, 0);
        body->load_size = size;
        entry->async_req = mk_file_read_async(body->load_buf, 1, size, entry->file);
        mk_file_close(entry->file);
        entry->file = 0;
    }
    aproc = saved_aproc;
    return body;
}

ScriptSlot* cmdscript_loadfile_language(int language, MkFileInfo* file_info) {
    MkFileInfo* info;
    int selected_language;

    selected_language = get_language();
    info = offset_mk_file_info(file_info, selected_language);
    return cmdscript_loadfile(language, info);
}

/* TODO: [breakthrough] 86.77%; unload guards fixed; fresh file/buffer reloads remain. */
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

/* TODO: [breakthrough] 75.57%; byte-count arithmetic fixed; inspect slot layout and load CFG. */
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
            header = slot->load_buf;
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
            slot->bytecode = (unsigned int*)((unsigned char*)slot->table_data + slot->data_words);
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
    active_cmdscript = find_cmdscript_for_process(aproc);
}

void set_process_as_scriptable(MkProc* proc) {
    CmdScript* cs;

    if (find_cmdscript_for_process(proc) == 0) {
        cs = (CmdScript*)get_mkhdr(&vtbl_cmdscript, sizeof(CmdScript));
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

/* TODO: [breakthrough] 92.12%; class-validation diamond recovered; inspect nullable list input and return staging. */
CmdScript* get_cmdscript_for_proc(MkProc* proc) {
    return find_cmdscript_for_process(proc);
}

CmdScript* alloc_cmdscript(void) {
    CmdScript* cs;

    cs = (CmdScript*)get_mkhdr(&vtbl_cmdscript, sizeof(CmdScript));
    if (cs == 0) {
        return 0;
    }
    init_cmdscript_fields(cs);
    return cs;
}

/* TODO: [breakthrough needed] 86.14%; 92 rows differ; inspect retail CFG and operand types. */
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
                    ScriptSlotEntry* candidate_entry = slot_entry_at(slot_index);
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
                                *value = def->data_index;
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

/* TODO: [breakthrough] 98.98%; unload CFG and unsigned index fixed; slot/member grouping and owner GPR allocation remain. */
void script_system_reset(void) {
    unsigned int i;
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

/* TODO: [breakthrough needed] 79.21%; 10 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [near miss] 60.25%; owner-relative address grouping and scheduling differ;
 * direct expression and full operand capture are neutral; stop at compiler ceiling. */
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

static inline const unsigned int* cmdscript_register_slot(
    const CmdScript* script, unsigned int index) {
    return &script->regs[index];
}

void _copy_stream_to_address(void) {
    unsigned int* args;
    CmdScript* cs;
    void* dst;
    unsigned int size;

    args = current_args;
    cs = active_cmdscript;
    dst = (void*)*cmdscript_register_slot(cs, args[1]);
    size = args[2] * sizeof(*args);
    memcpy(dst, &args[3], size);
}

/* TODO: [breakthrough needed] 82.68%; 32 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [near miss] 95.68965%; lookup and ownership agree; final indexed-store lowering differs. */
void _load_table_address(void)
{
    unsigned int* args;
    CmdScript* cs;
    unsigned int table_id;
    unsigned int idx;

    args = current_args;
    cs = active_cmdscript;
    table_id = args[2];
    idx = args[1];
    cs->regs[idx] = (unsigned int)get_data_table(cs->mko, table_id);
}

void _unconditional_branch(void) {
    unsigned int* args;
    CmdScript* cs;
    int rel;
    unsigned int* base;

    args = current_args;
    cs = active_cmdscript;
    rel = args[1];
    base = cs->pc;
    cs->pc = base + rel;
}

void _conditional_branch(void) {
    CmdScript* cs;
    unsigned int* args;
    int rel;
    unsigned int* base;

    cs = active_cmdscript;
    args = current_args;
    rel = args[1];
    if (cs->regs[0] != 0) {
        return;
    }
    base = cs->pc;
    cs->pc = base + rel;
}

/* TODO: [near miss] 97.94%; typed operand slots agree; 19 rows differ only in volatile GPR homes. */
void _compare_float_float(void) {
    unsigned int* args;
    CmdScript* cs;
    CmdScriptRegister* destination;
    CmdScriptRegister* right;
    float a;
    float b;
    int op;
    unsigned int result;

    result = 0;
    cs = active_cmdscript;
    destination = &cs->registers[(args = current_args)[1]];
    right = &cs->registers[args[2]];
    op = args[3];
    a = destination->real;
    b = right->real;
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
    destination->word = result;
}

/* TODO: [breakthrough needed] 84.01%; 36 rows differ; inspect retail CFG and operand types. */
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
    cs->regs[args[1]] = result;
}

/* TODO: [breakthrough needed] 84.01%; 36 rows differ; inspect retail CFG and operand types. */
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
    a = cs->regs[args[1]];
    b = cs->regs[args[2]];
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
    cs->regs[args[1]] = result;
}

/* TODO: [near miss] 98.06%; typed operand slots agree; volatile GPR homes differ. */
static void _combine_float_float(void) {
    unsigned int* args;
    CmdScript* cs;
    CmdScriptRegister* destination;
    CmdScriptRegister* right;
    float a;
    float b;
    int op;
    float result;

    result = 0.0f;
    cs = active_cmdscript;
    destination = &cs->registers[(args = current_args)[1]];
    right = &cs->registers[args[2]];
    op = args[3];
    a = destination->real;
    b = right->real;
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
    destination->real = result;
}

/* TODO: [breakthrough needed] 79.75%; 27 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 79.75%; 27 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 79.66%; 12 rows differ; inspect retail CFG and operand types. */
void _copy_register_to_address(void) {
    CmdScript* cs;
    void* destination;

    cs = active_cmdscript;
    destination = (void*)cs->regs[current_args[1]];
    memcpy(destination, &cs->regs[current_args[2]], current_args[3]);
}

/* TODO: [breakthrough needed] 64.04%; 21 rows differ; inspect retail CFG and operand types. */
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

/* TODO: [breakthrough needed] 45.62%; 32 rows differ; inspect retail CFG and operand types. */
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
    CmdScript* script = active_cmdscript;
    int variable_index = current_args[1];
    int register_index = current_args[2];
    unsigned int* variables = (unsigned int*)script->stack_sp;
    const CmdScriptRegister* source = &script->registers[register_index];

    variables[variable_index] = source->word;
}

/* TODO: [breakthrough needed] 85.90%; 7 rows differ; inspect retail CFG and operand types. */
void _copy_variable_to_register(void) {
    unsigned int* args;
    CmdScript* cs;
    unsigned int* stack_words;

    args = current_args;
    cs = active_cmdscript;
    stack_words = (unsigned int*)cs->stack_sp;
    cs->regs[args[1]] = stack_words[args[2]];
}

/* TODO: [near miss] 80.45%; register-array base grouping and GPR roles differ;
 * signed index/value staging is byte-neutral; stop at compiler ceiling. */
void _copy_register_to_register(void) {
    CmdScript* cs;

    cs = active_cmdscript;
    cs->regs[current_args[1]] = cs->regs[current_args[2]];
}

/* TODO: [near miss] 81.25%; register-store address grouping/GPR roles differ;
 * frame, scalar and array staging are neutral; reject CSE/empty-guard search. */
void _copy_constant_to_register(void) {
    active_cmdscript->regs[current_args[1]] = current_args[2];
}

void _copy_register_to_instruction(void) {
    unsigned int* args;
    CmdScript* cs;

    args = current_args;
    cs = active_cmdscript;
    cs->pc[args[2]] = *cmdscript_register_slot(cs, args[1]);
}

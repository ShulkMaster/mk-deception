#ifndef RUNTIME_STRING_BANK_H
#define RUNTIME_STRING_BANK_H

void load_string_bank_async(unsigned int bank, const char* name);
void load_string_bank(unsigned int bank, const char* name);
char* get_string_by_id(unsigned int id);

#endif

#ifndef NBC_H
#define NBC_H

void set_u8_bit(unsigned char* bits, int num_bits, int bit, int value);
int get_u8_bit(unsigned char* bits, int num_bits, int bit);
const char* nbc_find_text(int index, int table);
int nbc_get_language(void);
void eat_switch_action(int player, int action);
int check_switch_action(int player, int action);

#endif

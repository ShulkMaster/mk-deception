#ifndef GAME_AI_H
#define GAME_AI_H

typedef struct PlyrPdata PlyrPdata;
typedef struct PlyrWeaponStyle PlyrWeaponStyle;

void drone_ai_finished_request(void);
float drone_start(void);
void cleanup_drone_ai(void);
int drone_ai_check_switching_to(int command);
void generate_ai_table_player(PlyrPdata* player);
void generate_ai_table_moveset(PlyrWeaponStyle* moveset);

#endif

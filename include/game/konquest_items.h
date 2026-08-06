#ifndef KONQUEST_ITEMS_H
#define KONQUEST_ITEMS_H

typedef struct RwTexture RwTexture;

typedef struct PuiItem {
    int type;       /* +0x00 */
    char pad04[8];  /* +0x04 */
    char* filename; /* +0x0C */
    char pad10[4];  /* +0x10 */
    int name_id;    /* +0x14 */
    int desc_id;    /* +0x18 */
} PuiItem;

int get_last_character_trained_with(void);
void set_last_character_trained_with(int character);
const char* get_konq_profile_value_item_name(int index);
const char* get_konq_profile_value_item_description(int index);
RwTexture* get_konq_profile_value_item_tga_alpha(int index);
RwTexture* get_konq_profile_value_item_tga(int index);
int find_next_item_in_inventory(int index);
int get_number_items_in_inventory(void);
void add_to_konq_profile_value(int type, int value);
int get_konq_profile_value(int type, int index);
void set_konq_profile_value(int type, int index, int value);
void set_pui_status(PuiItem* item, int status);
int get_pui_status(PuiItem* item);

#endif

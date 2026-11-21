#ifndef RUNTIME_ANIM_API_H
#define RUNTIME_ANIM_API_H

typedef struct AnimPdata AnimPdata;
typedef struct MkHdr MkHdr;
typedef struct MkObj MkObj;

struct MorphFrameHeader {
    unsigned short frame;
    unsigned char target;
    unsigned char next_target;
    float position;
};
typedef char MorphFrameHeaderSize[(sizeof(struct MorphFrameHeader) == 8) ? 1 : -1];

typedef struct MorphScript {
    unsigned int frame_count;
    unsigned short* frame_table;
} MorphScript;

float p_animate(void);
void reset_ani_data_space(void);
void start_morph_proc(void);
int pose_morph(MkHdr* hdr);

struct MorphState;
struct MorphState* obj_start_morph(
    MkObj* obj, unsigned int sobj_id, MorphScript* script, unsigned int flags);

void set_root_and_obj_movement_weights(
    AnimPdata* animation, float root_weight, float object_weight);

#endif

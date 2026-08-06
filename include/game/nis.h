#ifndef NIS_H
#define NIS_H

typedef struct MkObj MkObj;
typedef struct ScriptSlot ScriptSlot;

void show_shujinko_unlock_screen(int string_id);
void release_kamidogu(MkObj* owner, void* bonematcher);
void p_konquest_ending(void);
void nis_set_wait_override(int value);
void nis_clear_event_list(void);
void nis_show_cancel_message(void);
int nis_scene_done(void);
void nis_end(void);
void nis_signal_event(int event);
void nis_wait_for_event(int event, int timeout);
void nis_init(ScriptSlot* cmdscript, unsigned int scene_func, unsigned int cancel_func);

#endif

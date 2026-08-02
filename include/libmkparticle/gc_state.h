#ifndef LIBMKPARTICLE_GC_STATE_H
#define LIBMKPARTICLE_GC_STATE_H

/* 2D projection / TEV helpers shared by gc_2d and gcdisplay. */

void disable_vertex_lights(void);
void set_2d_projection(void);
void set_2d_position(int x, int y);
void save_projection_matrix(void);
void restore_projection_matrix(void);
void apply_single_texture(void);
void apply_texture_with_alphamap(void);
void reset_tev_stages(void);

#endif

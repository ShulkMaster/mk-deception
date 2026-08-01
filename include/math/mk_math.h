#ifndef MK_MATH_H
#define MK_MATH_H

#include "math/gxQuat.h"
#include "rw/rtquat.h"

/*
 * Midway's matrix type is layout-compatible with RwMatrix:
 * right/up/at/pos vectors, each followed by a flags/padding word (0x40 bytes).
 */
typedef RwMatrix MKMATRIX;

/* Shared retail constants and scratch storage from mk_math.o. */
extern Vec Xaxis;
extern Vec Yaxis;
extern Vec Zaxis;
extern Quat identity_quat;
extern MKMATRIX tmp_matrix;

/* XZ-plane geometry. Declarations follow retail mk_math.o .text order. */
int intersect_xz_lines(const Vec* p, const Vec* dir, Vec* out, float a, float b);
void parametric_ray_to_point(Vec* out, const Vec* origin, const Vec* dir, float t);
int ray_cyl_intersection(const Vec* origin, const Vec* dir, const Vec* cyl_pos,
                         const Vec* cyl_axis, float radius, float* t_near, float* t_far);
float dist2_xz_to_xz(const Vec* a, const Vec* b);
float dist_xz_to_xz(const Vec* a, const Vec* b);
void rotate_xz(Vec* out, const Vec* v, float angle);
void xz_x_v_add_xz(Vec* dst, const Vec* v, float s);
void normalize_xz(Vec* v);
float length_xz(const Vec* v);
float xz_dot_xz(const Vec* a, const Vec* b);
float xz_unit_vector_recip(Vec* out, const Vec* from, const Vec* to);
void xz_unit_vector(Vec* out, const Vec* from, const Vec* to);
float xz_to_y_ang(const Vec* v);
void scale_xz(Vec* out, const Vec* v, float s);

/* Three-dimensional vector and angle operations. */
void midpoint_v3(Vec* out, const Vec* a, const Vec* b);
float dist2_v3_to_v3(const Vec* a, const Vec* b);
float dist_v3_to_v3(const Vec* a, const Vec* b);
void uv_from_angle_y(Vec* out, float angle_y);
void uv_from_angles_xy(Vec* out, float angle_x, float angle_y);
float uv_v3_to_v3_dist(Vec* out, const Vec* from, const Vec* to);
void uv_v3_to_v3(Vec* out, const Vec* from, const Vec* to);
void v3_blend3(Vec* out, const Vec* weights, const Vec* a, const Vec* b, const Vec* c);
void normalize_v3_length(Vec* v);
void normalize_v3(Vec* v);
void zero_v3(Vec* v);
float length_v3(const Vec* v);
void v3_cross_v3(Vec* out, const Vec* a, const Vec* b);
float v3_dot_v3(const Vec* a, const Vec* b);
void v3_sub_v3(Vec* out, const Vec* a, const Vec* b);
void v3_add_v3_scaled(Vec* out, const Vec* a, const Vec* b, float s);
void v3_add_v3(Vec* out, const Vec* a, const Vec* b);
void v3_x_v_add_v3(Vec* dst, const Vec* v, float s);
void scale_v3(Vec* out, const Vec* v, float s);
/* Produces a * weight_a + b * (1.0f - weight_a). */
void interp_v3(Vec* out, const Vec* a, const Vec* b, float weight_a);
void norm_angles_v3(Vec* ang);
float norm_angle(float angle);
void v3_to_xz_ang(Vec* ang, const Vec* v);
void v3_to_xy_ang_high_freq(Vec* ang, const Vec* v);
void v3_to_xy_ang(Vec* ang, const Vec* v);

/* Matrix, quaternion, and angle conversion operations. */
void mat_scaled_by_v3(MKMATRIX* out, const MKMATRIX* m, const Vec* scale);
void v3_x_mat_sub_v3(Vec* out, const Vec* v, const MKMATRIX* m, const Vec* sub);
void v3_x_mat_add_v3(Vec* out, const Vec* v, const MKMATRIX* m, const Vec* add);
void v3_x_mat(Vec* out, const Vec* v, const MKMATRIX* m);
void p3_x_mat(Vec* out, const Vec* p, const MKMATRIX* m);
void mat_x_mat(MKMATRIX* out, const MKMATRIX* a, const MKMATRIX* b);
void set_mat(MKMATRIX* dst, const MKMATRIX* src);
float ang_sub_ang(float angle_a, float angle_b);
float quat_extract_ang_y(const Quat* q);
void interp_quat(Quat* out, const Quat* q1, const Quat* q2, float weight);
void quat_x_quat(Quat* out, const Quat* a, const Quat* b);
void v3_v3_to_quat(Quat* out, const Vec* v1, const Vec* v2);
void quat_to_mat(MKMATRIX* out, const Quat* q);
void YXZ_angles_to_quat(const Vec* angles, Quat* out);
void mat_to_quat(Quat* out, const MKMATRIX* m);
void XYZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m);
void ZYX_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m);
void YXZ_angles_to_MKMATRIX(const Vec* angles, MKMATRIX* m);
void y_angle_to_MKMATRIX(MKMATRIX* m, float angle_y);
MKMATRIX* MKMatrixRotateScaleTranslate(MKMATRIX* m, const Vec* axis, float angle, const Vec* scale,
                                       const Vec* translate);
MKMATRIX* MKMatrixRotatXZYScaleTranslate(MKMATRIX* m, float angle_x, float angle_z, float angle_y,
                                         const Vec* scale, const Vec* translate);
void MKMatrixSetIdentity(MKMATRIX* m);

/* combine uses the retail RenderWare operation values: 0 replace, 1 pre, 2 post. */
MKMATRIX* MKMatrixTranslate(MKMATRIX* m, const Vec* delta, int combine);
MKMATRIX* MKMatrixScale(MKMATRIX* m, const Vec* scale, int combine);
MKMATRIX* MKMatrixRotate(MKMATRIX* m, const Vec* axis, float angle, int combine);

#endif

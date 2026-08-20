#ifndef RUNTIME_CAM_API_H
#define RUNTIME_CAM_API_H

#include "math/gxVect.h"

void set_camera_angle(const Vec* angle);
void set_camera_position(const Vec* position);
void set_camera_focal_length(float focal_length);
void get_camera_angle(Vec* angle);
void get_camera_position(Vec* position);

#endif

#ifndef RUNTIME_CAM_API_H
#define RUNTIME_CAM_API_H

typedef struct CamVec3 {
    float x;
    float y;
    float z;
} CamVec3;

void set_camera_angle(CamVec3* angle);
void set_camera_position(CamVec3* position);
void set_camera_focal_length(float focal_length);
void get_camera_angle(CamVec3* angle);
void get_camera_position(CamVec3* position);

#endif

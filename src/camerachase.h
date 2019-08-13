#pragma once

#include <gthree/gthree.h>

typedef struct _CameraChase CameraChase;


enum {
      MODE_CHASE,
      MODE_ORBIT,
};

CameraChase *camera_chase_new (GthreeCamera *camera,
                               GthreeObject *target,
                               float y_offset,
                               float z_offset,
                               float view_offset);

void camera_chase_free (CameraChase *chase);
void camera_chase_update (CameraChase *chase,
                          float dt,
                          float ratio);

void camera_chase_set_mode (CameraChase *chase,
                            int mode);

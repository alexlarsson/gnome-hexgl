#include <gthree/gthree.h>

typedef struct _CameraChase CameraChase;

CameraChase *camera_chase_new (GthreeCamera *camera,
                               GthreeObject *target,
                               float y_offset,
                               float z_offset,
                               float view_offset);

void camera_chase_free (CameraChase *chase);
void camera_chase_update (CameraChase *chase,
                          float dt,
                          float ratio);

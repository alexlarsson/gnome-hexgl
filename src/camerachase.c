#include "camerachase.h"

struct _CameraChase {
  GthreeCamera *camera;
  GthreeObject *target;

  int mode;

  float y_offset; /* How much above is the camera */
  float z_offset; /* How much (initially) behind is the camera */
  float view_offset; /* How much ahead of the target do we look */

  float speed_offset; /* How much extra behind is the camera due to speed */
  float speed_offset_max;
  float speed_offset_step;

  float time; /* rotation time */
  float orbit_offset; /* How far from the object to orbit */
};

CameraChase *
camera_chase_new (GthreeCamera *camera,
                  GthreeObject *target,
                  float y_offset,
                  float z_offset,
                  float view_offset)
{
  CameraChase *chase = g_new0 (CameraChase, 1);

  chase->camera = g_object_ref (camera);
  chase->target = g_object_ref (target);

  chase->mode = MODE_CHASE;

  chase->speed_offset = 0;
  chase->speed_offset_max = 10;
  chase->speed_offset_step = 0.05;

  chase->y_offset = y_offset;
  chase->z_offset = z_offset;
  chase->view_offset = view_offset;
  chase->orbit_offset = 12;
  chase->time = 0;

  return chase;
}

void
camera_chase_free (CameraChase *chase)
{
  g_object_unref (chase->camera);
  g_object_unref (chase->target);
  g_free (chase);
}

void
camera_chase_update (CameraChase *chase,
                     float dt,
                     float ratio)
{
  gthree_object_update_matrix (chase->target);

  if (chase->mode == MODE_CHASE)
    {
      const graphene_matrix_t *m;
      graphene_vec3_t dir, dir2, up, up2, target, lookat;

      graphene_vec3_init (&dir, 0, 0, 1);
      graphene_vec3_init (&up, 0, 1, 0);

      m = gthree_object_get_matrix (chase->target);
      graphene_matrix_transform_vec3 (m, &dir, &dir);
      graphene_matrix_transform_vec3 (m, &up, &up);

      chase->speed_offset += (chase->speed_offset_max * ratio - chase->speed_offset) * fmin (1, 0.3 * dt);

      graphene_vec3_scale (&dir, chase->z_offset + chase->speed_offset, &dir2);
      graphene_vec3_subtract (gthree_object_get_position (chase->target), &dir2, &target);
      graphene_vec3_scale (&up, chase->y_offset, &up2);
      graphene_vec3_add (&target, &up2, &target);

      gthree_object_set_position_xyz (GTHREE_OBJECT (chase->camera),
                                      graphene_vec3_get_x (&target),
                                      graphene_vec3_get_y (&target) - graphene_vec3_get_y (&up2) + chase->y_offset,
                                      graphene_vec3_get_z (&target));

      graphene_vec3_scale (&dir, chase->view_offset, &dir2);
      graphene_vec3_add (gthree_object_get_position (chase->target), &dir2, &lookat);

      gthree_object_look_at (GTHREE_OBJECT (chase->camera), &lookat);
    }
  else if (chase->mode == MODE_ORBIT)
    {
      graphene_vec3_t dir, target;

      chase->time += dt;

      graphene_vec3_add (gthree_object_get_position (chase->target),
                         graphene_vec3_init (&dir,
                                             sinf (chase->time * .008) * chase->orbit_offset,
                                             chase->y_offset / 2,
                                             cosf (chase->time * .008) * chase->orbit_offset),
                         &target);

      gthree_object_set_position (GTHREE_OBJECT (chase->camera), &target);

      gthree_object_look_at (GTHREE_OBJECT (chase->camera), gthree_object_get_position (chase->target));
    }
}

void
camera_chase_set_mode (CameraChase *chase,
                       int mode)
{
  chase->mode = mode;
}

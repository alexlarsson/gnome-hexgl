#include "shipeffects.h"
#include "shipcontrols.h"

struct _ShipEffects {
  ShipControls *controls;
  GthreeMeshBasicMaterial *booster_material;
  GthreeMesh *booster;
  GthreePointLight *booster_light;
};

ShipEffects *
ship_effects_new (ShipControls *controls)
{
  ShipEffects *effects = g_new0 (ShipEffects, 1);
  GthreeObject *the_ship;
  GthreeMaterial *orig_booster_material;
  GthreeTexture *booster_texture;
  graphene_vec3_t light_pos;

  effects->controls = controls;

  the_ship = ship_controls_get_mesh (controls);
  effects->booster = GTHREE_MESH (gthree_object_find_first_by_name (the_ship, "BoosterMesh"));

  /* Rejig booster material */

  orig_booster_material = gthree_mesh_get_material (effects->booster, 0);
  booster_texture = gthree_mesh_standard_material_get_map (GTHREE_MESH_STANDARD_MATERIAL (orig_booster_material));

  effects->booster_material = gthree_mesh_basic_material_new ();
  gthree_mesh_basic_material_set_map (effects->booster_material, booster_texture);
  gthree_material_set_is_transparent (GTHREE_MATERIAL (effects->booster_material), TRUE);
  // TODO: For whatever reason this causes other things to render wrongly...
  //gthree_material_set_depth_write (GTHREE_MATERIAL (effects->booster_material), FALSE);

  gthree_mesh_set_material (effects->booster, 0, GTHREE_MATERIAL (effects->booster_material));

  /* Add booster light */
  GdkRGBA color = { 0.0, 0.64, 1.0, 1.0 };

  effects->booster_light = gthree_point_light_new (&color, 4.0, 60.0);
  gthree_point_light_set_decay (effects->booster_light, 0.5);


  gthree_object_add_child (the_ship, GTHREE_OBJECT (effects->booster_light));

  graphene_vec3_init (&light_pos, 0, 0, -0.2);
  graphene_vec3_add (&light_pos,
                     gthree_object_get_position (gthree_object_get_parent (GTHREE_OBJECT (effects->booster))),
                     &light_pos);

  gthree_object_set_position (GTHREE_OBJECT (effects->booster_light), &light_pos);

  return effects;
}

void
ship_effects_free (ShipEffects *effects)
{
  g_clear_object (&effects->booster);
  g_clear_object (&effects->booster_material);
  g_clear_object (&effects->booster_light);
  g_free (effects);
}

void
ship_effects_update (ShipEffects *effects,
                      float dt)
{
  float boost_ratio = 0, opacity = 0, scale = 0, random = 0, intensity = 0;

  if (!ship_controls_is_destroyed (effects->controls))
    {
      gboolean is_accel = ship_controls_is_accelerating (effects->controls);
      boost_ratio = ship_controls_get_boost_ratio (effects->controls);
      opacity =  (is_accel ? 0.8 : 0.3) + boost_ratio * 0.4;
      scale = (is_accel  ? 1.0 : 0.8) + boost_ratio * 0.5;
      intensity = is_accel ? 4.0 : 2.0;
      random = g_random_double_range (0, 0.2);
    }

  if (effects->booster)
    {
      const graphene_euler_t *old_rotation = gthree_object_get_rotation (GTHREE_OBJECT (effects->booster));
      graphene_euler_t r;
      graphene_point3d_t s;

      gthree_object_set_rotation (GTHREE_OBJECT (effects->booster),
                                  graphene_euler_init (&r,
                                                       graphene_euler_get_x (old_rotation),
                                                       graphene_euler_get_y (old_rotation) + 57,
                                                       graphene_euler_get_z (old_rotation)));
      gthree_object_set_scale_point3d (GTHREE_OBJECT (effects->booster),
                               graphene_point3d_init (&s, scale, scale, scale));
      gthree_material_set_opacity (GTHREE_MATERIAL (effects->booster_material), random + opacity);
      gthree_light_set_intensity (GTHREE_LIGHT (effects->booster_light), intensity * (random + 0.8));

      // this.boosterSprite.opacity = random+opacity;
    }

}

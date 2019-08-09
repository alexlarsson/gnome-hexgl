#include "shipeffects.h"
#include "shipcontrols.h"
#include "utils.h"
#include "particles.h"

struct _ShipEffects {
  GthreeScene *scene;
  ShipControls *controls;
  GthreeMeshBasicMaterial *booster_material;
  GthreeObject *booster;
  GthreeMesh *booster_mesh;
  GthreePointLight *booster_light;
  GthreeSprite *booster_sprite;

  graphene_vec3_t pVel;
  graphene_vec3_t pOffset;
  graphene_vec3_t pRad;
  float pVelS;
  float pOffsetS;
  float pRadS;

  Particles *right_sparks;
  Particles *left_sparks;
  Particles *right_clouds;
  Particles *left_clouds;
};

ShipEffects *
ship_effects_new (GthreeScene *scene,
                  ShipControls *controls)
{
  ShipEffects *effects = g_new0 (ShipEffects, 1);
  GthreeObject *the_ship;
  GthreeMaterial *orig_booster_material;
  GthreeTexture *booster_texture;
  graphene_vec3_t light_pos, s, v;

  effects->scene = scene;
  effects->controls = controls;

  the_ship = ship_controls_get_mesh (controls);
  effects->booster = gthree_object_find_first_by_name (the_ship, "booster");
  effects->booster_mesh = GTHREE_MESH (gthree_object_find_first_by_name (effects->booster, "BoosterMesh"));

  /* Rejig booster material */

  orig_booster_material = gthree_mesh_get_material (effects->booster_mesh, 0);
  booster_texture = gthree_mesh_standard_material_get_map (GTHREE_MESH_STANDARD_MATERIAL (orig_booster_material));

  effects->booster_material = gthree_mesh_basic_material_new ();
  gthree_mesh_basic_material_set_map (effects->booster_material, booster_texture);
  gthree_material_set_is_transparent (GTHREE_MATERIAL (effects->booster_material), TRUE);
  // TODO: For whatever reason this causes other things to render wrongly...
  //gthree_material_set_depth_write (GTHREE_MATERIAL (effects->booster_material), FALSE);

  gthree_mesh_set_material (effects->booster_mesh, 0, GTHREE_MATERIAL (effects->booster_material));

  /* Add booster light */
  GdkRGBA color = { 0.0, 0.64, 1.0, 1.0 };

  effects->booster_light = gthree_point_light_new (&color, 4.0, 60.0);
  gthree_point_light_set_decay (effects->booster_light, 0.5);


  gthree_object_add_child (the_ship, GTHREE_OBJECT (effects->booster_light));

  graphene_vec3_init (&light_pos, 0, 0, -0.2);
  graphene_vec3_add (&light_pos,
                     gthree_object_get_position (effects->booster),
                     &light_pos);

  gthree_object_set_position (GTHREE_OBJECT (effects->booster_light), &light_pos);

  g_autoptr(GthreeTexture) texture = load_texture ("boostersprite.jpg");

  effects->booster_sprite = gthree_sprite_new (NULL);
  GthreeMaterial *sprite_material = gthree_sprite_get_material (effects->booster_sprite);
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (sprite_material), texture);
  gthree_material_set_blend_mode (sprite_material,
                                  GTHREE_BLEND_ADDITIVE, 0, 0, 0);
  gthree_object_set_scale (GTHREE_OBJECT (effects->booster_sprite),
                           graphene_vec3_init (&s,
                                               20.0, 20.0, 1.0));
  gthree_material_set_depth_test (sprite_material, FALSE);

  gthree_object_add_child (GTHREE_OBJECT (effects->booster), GTHREE_OBJECT (effects->booster_sprite));

  /* Particles */

  graphene_vec3_init (&effects->pVel, 0.5, 0, 0);
  graphene_vec3_init (&effects->pOffset, -3, -0.3, 0);
  graphene_vec3_init (&effects->pRad, 0, 0, 1.5);
  effects->pVelS = graphene_vec3_length (&effects->pVel);
  effects->pOffsetS = graphene_vec3_length (&effects->pOffset);
  effects->pRadS = graphene_vec3_length (&effects->pRad);
  graphene_vec3_normalize (&effects->pVel, &effects->pVel);
  graphene_vec3_normalize (&effects->pOffset, &effects->pOffset);
  graphene_vec3_normalize (&effects->pRad, &effects->pRad);

  GdkRGBA spark_color1 = { 1.0, 1.0, 1.0, 1.0 };
  GdkRGBA spark_color2 = { 1.0, 0.7529411764705882, 0.0, 1.0 };

  g_autoptr(GthreeTexture) spark_texture = load_texture ("spark.png");

  effects->right_sparks = particles_new (200, 2);
  particles_set_velocity_randomness (effects->right_sparks, graphene_vec3_init (&v, 0.4, 0.4, 0.4));
  particles_set_life (effects->right_sparks, 60);
  particles_set_color1 (effects->right_sparks, &spark_color1);
  particles_set_color2 (effects->right_sparks, &spark_color2);
  particles_set_map (effects->right_sparks, spark_texture);

  gthree_object_add_child (GTHREE_OBJECT (scene), particles_get_object (effects->right_sparks));

  effects->left_sparks = particles_new (200, 2);
  particles_set_velocity_randomness (effects->left_sparks, graphene_vec3_init (&v, 0.4, 0.4, 0.4));
  particles_set_life (effects->left_sparks, 60);
  particles_set_color1 (effects->left_sparks, &spark_color1);
  particles_set_color2 (effects->left_sparks, &spark_color2);
  particles_set_map (effects->left_sparks, spark_texture);

  gthree_object_add_child (GTHREE_OBJECT (scene), particles_get_object (effects->left_sparks));

  g_autoptr(GthreeTexture) cloud_texture = load_texture ("cloud.png");
  GdkRGBA cloud_color1 = { 0.6431372549019608, 0.9450980392156862, 1.0 };
  GdkRGBA cloud_color2 = { 0.4, 0.4, 0.4, 1.0 };

  effects->right_clouds = particles_new (200, 6);
  particles_set_life (effects->right_clouds, 60);
  particles_set_spawn_point (effects->right_clouds, graphene_vec3_init (&v, -3, -0.3, 0));
  particles_set_spawn_radius (effects->right_clouds, graphene_vec3_init (&v, 1, 1, 2));
  particles_set_velocity (effects->right_clouds, graphene_vec3_init (&v, 0, 0, -0.4));
  particles_set_velocity_randomness (effects->right_clouds, graphene_vec3_init (&v, 0.05, 0.05, 0.1));
  particles_set_color1 (effects->right_clouds, &cloud_color1);
  particles_set_color2 (effects->right_clouds, &cloud_color2);
  particles_set_map (effects->right_clouds, cloud_texture);

  gthree_object_add_child (GTHREE_OBJECT (the_ship), particles_get_object (effects->right_clouds));

  effects->left_clouds = particles_new (200, 6);
  particles_set_life (effects->left_clouds, 60);
  particles_set_spawn_point (effects->left_clouds, graphene_vec3_init (&v, 3, -0.3, 0));
  particles_set_spawn_radius (effects->left_clouds, graphene_vec3_init (&v, 1, 1, 2));
  particles_set_velocity (effects->left_clouds, graphene_vec3_init (&v, 0, 0, -0.4));
  particles_set_velocity_randomness (effects->left_clouds, graphene_vec3_init (&v, 0.05, 0.05, 0.1));
  particles_set_color1 (effects->left_clouds, &cloud_color1);
  particles_set_color2 (effects->left_clouds, &cloud_color2);
  particles_set_map (effects->left_clouds, cloud_texture);

  gthree_object_add_child (GTHREE_OBJECT (the_ship), particles_get_object (effects->left_clouds));

  return effects;
}

void
ship_effects_free (ShipEffects *effects)
{
  g_clear_object (&effects->booster);
  g_clear_object (&effects->booster_mesh);
  g_clear_object (&effects->booster_material);
  g_clear_object (&effects->booster_light);
  g_clear_object (&effects->booster_sprite);
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
      const graphene_euler_t *old_rotation = gthree_object_get_rotation (GTHREE_OBJECT (effects->booster_mesh));
      graphene_euler_t r;
      graphene_vec3_t s;

      gthree_object_set_rotation (GTHREE_OBJECT (effects->booster_mesh),
                                  graphene_euler_init (&r,
                                                       graphene_euler_get_x (old_rotation),
                                                       graphene_euler_get_y (old_rotation) + 57,
                                                       graphene_euler_get_z (old_rotation)));
      gthree_object_set_scale (GTHREE_OBJECT (effects->booster_mesh),
                               graphene_vec3_init (&s, scale, scale, scale));
      gthree_material_set_opacity (GTHREE_MATERIAL (effects->booster_material), random + opacity);
      gthree_light_set_intensity (GTHREE_LIGHT (effects->booster_light), intensity * (random + 0.8));
      gthree_material_set_opacity (GTHREE_MATERIAL (gthree_sprite_get_material (effects->booster_sprite)), random + opacity);
    }

  /* Update particles */

  GthreeObject *dummy = ship_controls_get_dummy (effects->controls);
  GthreeObject *mesh = ship_controls_get_mesh (effects->controls);

  graphene_vec3_t shipVelocity;
  graphene_vec3_scale (ship_controls_get_current_velocity (effects->controls), 0.7, &shipVelocity);

  graphene_vec3_t spawn_point;
  graphene_vec3_t spawn_vel;
  graphene_vec3_t spawn_rad;
  graphene_vec3_t flip_x;

  graphene_vec3_init (&flip_x, -1, 1, 1);

  // right sparks

  graphene_matrix_transform_vec3 (gthree_object_get_matrix (mesh),
                                  &effects->pOffset, &spawn_point);
  graphene_vec3_scale (&spawn_point, effects->pOffsetS, &spawn_point);
  graphene_vec3_add (&spawn_point, gthree_object_get_position (dummy), &spawn_point);

  graphene_matrix_transform_vec3 (gthree_object_get_matrix (dummy),
                                  &effects->pVel, &spawn_vel);
  graphene_vec3_scale (&spawn_vel, effects->pVelS, &spawn_vel);
  graphene_vec3_add (&spawn_vel, &shipVelocity, &spawn_vel);

  graphene_matrix_transform_vec3 (gthree_object_get_matrix (mesh),
                                  &effects->pRad, &spawn_rad);
  graphene_vec3_scale (&spawn_rad, effects->pRadS, &spawn_rad);

  particles_set_spawn_point (effects->right_sparks, &spawn_point);
  particles_set_velocity (effects->right_sparks, &spawn_vel);
  particles_set_spawn_radius (effects->right_sparks, &spawn_rad);

  // left sparks

  graphene_vec3_multiply (&effects->pOffset, &flip_x, &spawn_point);
  graphene_matrix_transform_vec3 (gthree_object_get_matrix (mesh),
                                  &spawn_point, &spawn_point);
  graphene_vec3_scale (&spawn_point, effects->pOffsetS, &spawn_point);
  graphene_vec3_add (&spawn_point, gthree_object_get_position (dummy), &spawn_point);

  graphene_vec3_multiply (&effects->pVel, &flip_x, &spawn_vel);
  graphene_matrix_transform_vec3 (gthree_object_get_matrix (mesh),
                                  &spawn_vel, &spawn_vel);
  graphene_vec3_scale (&spawn_vel, effects->pVelS, &spawn_vel);
  graphene_vec3_add (&spawn_vel, &shipVelocity, &spawn_vel);

  particles_set_spawn_point (effects->left_sparks, &spawn_point);
  particles_set_velocity (effects->left_sparks, &spawn_vel);
  particles_set_spawn_radius (effects->left_sparks, &spawn_rad); // same as right

  if (ship_controls_get_collision_right (effects->controls))
    {
      particles_emit (effects->right_sparks, 20);
      particles_emit (effects->right_clouds, 5);
    }

  if (ship_controls_get_collision_left (effects->controls))
    {
      particles_emit (effects->left_sparks, 20);
      particles_emit (effects->left_clouds, 5);
    }

  particles_update (effects->right_sparks, dt);
  particles_update (effects->left_sparks, dt);
  particles_update (effects->right_clouds, dt);
  particles_update (effects->left_clouds, dt);
}

#include "shipcontrols.h"
#include <gtk/gtk.h>
#include "sounds.h"

#define RAD_TO_DEG(x)          ((x) * (180.f / GRAPHENE_PI))

#define EPSILON 0.00000001

struct _ShipControls {
  GthreeObject *mesh;
  GthreeObject *dummy;
  AnalysisMap *height_map;
  AnalysisMap *collision_map;

  gboolean active;
  gboolean destroyed;
  gboolean falling;
  gint64 fall_start_time;

  float airResist;
  float airDrift;
  float thrust;
  float airBrake;
  float maxSpeed;
  float boosterSpeed;
  float boosterDecay;
  float angularSpeed;
  float airAngularSpeed;
  float repulsionRatio;
  float repulsionCap;
  float repulsionLerp;
  float collisionSpeedDecrease;
  float collisionSpeedDecreaseCoef;
  float maxShield;
  float shieldDelay;
  float shieldTiming;
  float shieldDamage;
  float driftLerp;
  float angularLerp;
  float rollAngle;
  float rollLerp;
  float heightLerp;
  float heightOffset; // height above track
  float gradientLerp;
  float gradientScale;
  float tiltLerp;
  float tiltScale;
  float repulsionVScale;

  /* motion state */
  float drift;
  float angular;
  float speed;
  float speedRatio;
  float shield;
  float boost;
  float roll;
  graphene_point3d_t repulsionForce;
  float gradient;
  float gradientTarget;
  float tilt;
  float tiltTarget;
  int check_point;

  gboolean collision_left;
  gboolean collision_right;
  gboolean collision_front;

  gboolean key_forward;
  gboolean key_backward;
  gboolean key_left;
  gboolean key_right;
  gboolean key_ltrigger;
  gboolean key_rtrigger;
  gboolean key_use;

  graphene_vec3_t currentVelocity;
};

static void ship_controls_fall (ShipControls *controls);

ShipControls *
ship_controls_new (void)
{
  ShipControls *controls = g_new0 (ShipControls, 1);

  controls->active = FALSE;

  controls->airResist = 0.02;
  controls->airDrift = 0.1;
  controls->thrust = 0.02;
  controls->airBrake = 0.02;
  controls->maxSpeed = 7.0;
  controls->boosterSpeed = controls->maxSpeed * 0.2;
  controls->boosterDecay = 0.01;
  controls->angularSpeed = 0.005;
  controls->airAngularSpeed = 0.0065;
  controls->repulsionRatio = 0.5;
  controls->repulsionCap = 2.5;
  controls->repulsionLerp = 0.1;
  controls->collisionSpeedDecrease = 0.8;
  controls->collisionSpeedDecreaseCoef = 0.8;
  controls->maxShield = 1.0;
  controls->shieldDelay = 60;
  controls->shieldTiming = 0;
  controls->shieldDamage = 0.25;
  controls->driftLerp = 0.35;
  controls->angularLerp = 0.35;
  controls->rollAngle = 0.6;
  controls->rollLerp = 0.08;
  controls->heightLerp = 0.4;
  controls->heightOffset = 9;
  controls->gradientLerp = 0.05;
  controls->gradientScale = 4.0;
  controls->tiltLerp = 0.05;
  controls->tiltScale = 4.0;
  controls->repulsionVScale = 4.0;

  controls->drift = 0.0;
  controls->angular = 0.0;
  controls->speed = 0.0;
  controls->speedRatio = 0.0;
  controls->boost = 0.0;
  controls->roll = 0.0;
  controls->shield = 1.0;
  controls->check_point = -1;
  graphene_point3d_init (&controls->repulsionForce, 0, 0, 0);
  controls->gradient = 0.0;
  controls->gradientTarget = 0.0;
  controls->tilt = 0.0;
  controls->tiltTarget = 0.0;

  controls->dummy = gthree_object_new ();

  ship_controls_set_difficulty (controls, 0);

  return controls;
}

void
ship_controls_set_active (ShipControls *controls,
                          gboolean active)
{
  controls->active = active;
}

void
ship_controls_stop (ShipControls *controls)
{
  controls->speed = 0;
}

void
ship_controls_set_difficulty (ShipControls *controls,
                              int difficulty)
{
  if (difficulty == 1)
    {
      controls->airResist = 0.035;
      controls->airDrift = 0.07;
      controls->thrust = 0.035;
      controls->airBrake = 0.04;
      controls->maxSpeed = 9.6;
      controls->boosterSpeed = controls->maxSpeed * 0.35;
      controls->boosterDecay = 0.007;
      controls->angularSpeed = 0.0140;
      controls->airAngularSpeed = 0.0165;
      controls->rollAngle = 0.6;
      controls->shieldDamage = 0.03;
      controls->collisionSpeedDecrease = 0.8;
      controls->collisionSpeedDecreaseCoef = 0.5;
      controls->rollLerp = 0.1;
      controls->driftLerp = 0.4;
      controls->angularLerp = 0.4;
    }
  else if (difficulty == 0)
    {
      controls->airResist = 0.02;
      controls->airDrift = 0.06;
      controls->thrust = 0.02;
      controls->airBrake = 0.025;
      controls->maxSpeed = 7.0;
      controls->boosterSpeed = controls->maxSpeed * 0.5;
      controls->boosterDecay = 0.007;
      controls->angularSpeed = 0.0125;
      controls->airAngularSpeed = 0.0135;
      controls->rollAngle = 0.6;
      controls->shieldDamage = 0.06;
      controls->collisionSpeedDecrease = 0.8;
      controls->collisionSpeedDecreaseCoef = 0.5;
      controls->rollLerp = 0.07;
      controls->driftLerp = 0.3;
      controls->angularLerp = 0.4;
    }
}

int
ship_controls_get_check_point (ShipControls *controls)
{
  return controls->check_point;
}

gboolean
ship_controls_get_collision_left (ShipControls *controls)
{
  return controls->collision_left;
}

gboolean
ship_controls_get_collision_right (ShipControls *controls)
{
  return controls->collision_right;
}

const graphene_vec3_t *
ship_controls_get_current_velocity (ShipControls *controls)
{
  return &controls->currentVelocity;
}

int
ship_controls_get_real_speed (ShipControls *controls, float scale)
{
  return (int)roundf ((controls->speed + controls->boost) * scale);
}

float
ship_controls_get_real_speed_ratio (ShipControls *controls)
{
  return fmin (controls->maxSpeed, controls->speed + controls->boost) / controls->maxSpeed;
}

float
ship_controls_get_speed_ratio (ShipControls *controls)
{
  return (controls->speed + controls->boost) / controls->maxSpeed;
}

float
ship_controls_get_shield_ratio (ShipControls *controls)
{
  return controls->shield / controls->maxShield;
}

int
ship_controls_get_shield (ShipControls *controls, float scale)
{
  return (int)roundf (controls->shield * scale);
}

float
ship_controls_get_boost_ratio (ShipControls *controls)
{
  return controls->boost / controls->boosterSpeed;
}

gboolean
ship_controls_is_accelerating (ShipControls *controls)
{
  return controls->key_forward;
}

gboolean
ship_controls_is_destroyed (ShipControls *controls)
{
  return controls->destroyed;
}

void
ship_controls_reset (ShipControls *controls,
                     const graphene_vec3_t *position,
                     const graphene_vec3_t *rotation)
{
  graphene_quaternion_t quaternion;

  controls->check_point = -1;
  controls->roll = 0.0;
  controls->drift = 0.0;
  controls->angular = 0.0;
  controls->speed = 0.0;
  controls->speedRatio = 0.0;
  controls->boost = 0.0;
  controls->shield = controls->maxShield;
  controls->destroyed = FALSE;
  controls->falling = FALSE;
  controls->gradient = 0.0;
  controls->gradientTarget = 0.0;
  controls->tilt = 0.0;
  controls->tiltTarget = 0.0;
  graphene_point3d_init (&controls->repulsionForce, 0, 0, 0);

  gthree_object_set_position (controls->dummy,
                              position);
  graphene_quaternion_init (&quaternion,
                            graphene_vec3_get_x (rotation),
                            graphene_vec3_get_y (rotation),
                            graphene_vec3_get_z (rotation),
                            1);
  graphene_quaternion_normalize (&quaternion, &quaternion);
  gthree_object_set_quaternion (controls->dummy, &quaternion);
  gthree_object_update_matrix (controls->dummy);

  gthree_object_set_matrix (controls->mesh, gthree_object_get_matrix (controls->dummy));
  gthree_object_update_matrix_world (controls->mesh, TRUE);
}

void
ship_controls_control (ShipControls *controls,
                       GthreeObject *mesh)
{
  g_clear_object (&controls->mesh);
  controls->mesh = g_object_ref (mesh);

  gthree_object_set_matrix_auto_update (controls->mesh, FALSE);
  gthree_object_set_position (controls->dummy,
                              gthree_object_get_position (mesh));
}

GthreeObject *
ship_controls_get_mesh (ShipControls *controls)
{
  return controls->mesh;
}

GthreeObject *
ship_controls_get_dummy (ShipControls *controls)
{
  return controls->dummy;
}

void
ship_controls_set_height_map (ShipControls *controls,
                              AnalysisMap *map)
{
  controls->height_map = analysis_map_ref (map);
}

void
ship_controls_set_collision_map (ShipControls *controls,
                                 AnalysisMap *map)
{
  controls->collision_map = analysis_map_ref (map);
}

void
ship_controls_free (ShipControls *controls)
{
  g_clear_object (&controls->mesh);
  g_object_unref (controls->dummy);
  g_free (controls);
}

static void
ship_controls_height_check (ShipControls *controls,
                            graphene_point3d_t *movement,
                            float dt)
{
  if (controls->height_map == NULL)
    return;

  graphene_point3d_t pos;
  graphene_point3d_init_from_vec3 (&pos,
                                   gthree_object_get_position (controls->dummy));

  float height = analysis_map_lookup_depthmapped (controls->height_map, pos.x, pos.z);

  float delta = (height + controls->heightOffset - pos.y);

  if (delta > 0)
    movement->y += delta;
  else
    movement->y += delta * controls->heightLerp;

  // gradient
  graphene_vec3_t gradientVector;
  graphene_vec3_init (&gradientVector, 0, 0, 5);

  graphene_matrix_transform_vec3 (gthree_object_get_matrix (controls->dummy),
                                  &gradientVector, &gradientVector);
  graphene_vec3_add (&gradientVector,
                     gthree_object_get_position (controls->dummy),
                     &gradientVector);

  float nheight = analysis_map_lookup_depthmapped (controls->height_map,
                                                   graphene_vec3_get_x (&gradientVector),
                                                   graphene_vec3_get_z (&gradientVector));
  if (fabs (nheight - height) < 100)
      controls->gradientTarget = -atan2f (nheight - height, 5.0); //TODO: original had this???: * controls->gradientScale;

  // tilt
  graphene_vec3_t tiltVector;
  graphene_vec3_init (&tiltVector, 5, 0, 0);

  graphene_matrix_transform_vec3 (gthree_object_get_matrix (controls->dummy),
                                  &tiltVector, &tiltVector);
  graphene_vec3_add (&tiltVector,
                     gthree_object_get_position (controls->dummy),
                     &tiltVector);

  nheight = analysis_map_lookup_depthmapped (controls->height_map,
                                             graphene_vec3_get_x (&tiltVector),
                                             graphene_vec3_get_z (&tiltVector));

  if (fabs (nheight - height) > 100) // If right project out of bounds, try left projection
    {
      graphene_vec3_init (&tiltVector, -5, 0, 0);
      graphene_matrix_transform_vec3 (gthree_object_get_matrix (controls->dummy),
                                      &tiltVector, &tiltVector);
      graphene_vec3_add (&tiltVector,
                         gthree_object_get_position (controls->dummy),
                         &tiltVector);

      nheight = analysis_map_lookup_depthmapped (controls->height_map,
                                                 graphene_vec3_get_x (&tiltVector),
                                                 graphene_vec3_get_z (&tiltVector));
      // TODO: Shouldn't we flip tilt angle here ????
    }

  if (fabs (nheight - height) < 100)
    controls->tiltTarget = atan2f (nheight - height, 5.0); // *controls->tiltScale;
}

static void
ship_controls_booster_check (ShipControls *controls,
                             graphene_point3d_t *movement,
                             float dt)
{
  graphene_point3d_t pos;
  GdkRGBA collision;

  if (controls->collision_map == NULL)
    return;


  graphene_point3d_init_from_vec3 (&pos,
                                   gthree_object_get_position (controls->dummy));


  analysis_map_lookup_rgba_bilinear (controls->collision_map, pos.x, pos.z, &collision);


  controls->boost -= controls->boosterDecay * dt;
  if (controls->boost < 0)
    {
      controls->boost = 0.0;
      stop_sound ("boost");
    }

  if (collision.red >= 0.9 && collision.green < 0.5 && collision.blue < 0.5)
    {
      play_sound ("boost", FALSE);
      controls->boost = controls->boosterSpeed;
    }

  movement->z += controls->boost * dt;
}

static void
ship_controls_collision_check (ShipControls *controls,
                               float dt)
{
  graphene_point3d_t pos;
  GdkRGBA collision;

  if (controls->collision_map == NULL)
    return;

  if (controls->shieldDelay > 0)
    controls->shieldDelay -= dt;

  controls->collision_left = FALSE;
  controls->collision_right = FALSE;
  controls->collision_front = FALSE;


  graphene_point3d_init_from_vec3 (&pos,
                                   gthree_object_get_position (controls->dummy));

  analysis_map_lookup_rgba_nearest (controls->collision_map, pos.x, pos.z, &collision);

  if (collision.red < 1.0)
    {
      play_sound ("crash", FALSE);

      // Shield
      float sr = (float) ship_controls_get_real_speed (controls, 1) / controls->maxSpeed;
      controls->shield -= sr * sr * 0.8 * controls->shieldDamage;

      // Repulsion
      graphene_vec3_t repulsionVLeft;
      graphene_vec3_init (&repulsionVLeft, 1, 0, 0);
      graphene_matrix_transform_vec3 (gthree_object_get_matrix (controls->dummy),
                                      &repulsionVLeft, &repulsionVLeft);
      graphene_vec3_scale (&repulsionVLeft, controls->repulsionVScale, &repulsionVLeft);

      graphene_vec3_t repulsionVRight;
      graphene_vec3_init (&repulsionVRight, -1, 0, 0);
      graphene_matrix_transform_vec3 (gthree_object_get_matrix (controls->dummy),
                                      &repulsionVRight, &repulsionVRight);
      graphene_vec3_scale (&repulsionVRight, controls->repulsionVScale, &repulsionVRight);

      graphene_vec3_t lPos, rPos;
      graphene_vec3_add (gthree_object_get_position (controls->dummy),
                         &repulsionVLeft, &lPos);
      graphene_vec3_add (gthree_object_get_position (controls->dummy),
                         &repulsionVRight, &rPos);


      GdkRGBA lCol, rCol;
      analysis_map_lookup_rgba_bilinear (controls->collision_map, graphene_vec3_get_x (&lPos), graphene_vec3_get_z (&lPos), &lCol);
      analysis_map_lookup_rgba_bilinear (controls->collision_map, graphene_vec3_get_x (&rPos), graphene_vec3_get_z (&rPos), &rCol);

      float repulsionAmount = fmaxf (0.8,
                                     fminf (controls->repulsionCap,
                                            controls->speed * controls->repulsionRatio
                                            )
                                     );
      if (rCol.red > lCol.red)
        {
          // Repulse right
          controls->repulsionForce.x += -repulsionAmount;
          controls->collision_left = TRUE;
        }
      else if (rCol.red < lCol.red)
        {
          // Repulse left
          controls->repulsionForce.x += repulsionAmount;
          controls->collision_right = TRUE;
        }
      else
        {
          //console.log(collision.r+"  --  "+fCol+"  @  "+lCol+"  /  "+rCol);
          controls->repulsionForce.z += -repulsionAmount*4;
          controls->collision_front = TRUE;
          controls->speed = 0;
        }

      // DIRTY GAMEOVER
      if (rCol.red < 0.5 && lCol.red < 0.5)
        {
          GdkRGBA cCol;
          const graphene_vec3_t *cPos = gthree_object_get_position (controls->dummy);
          analysis_map_lookup_rgba_bilinear (controls->collision_map, graphene_vec3_get_x (cPos), graphene_vec3_get_z (cPos), &cCol);
          if (cCol.red < 0.1)
            ship_controls_fall (controls);
        }

      controls->speed *= controls->collisionSpeedDecrease;
      controls->speed *= (1-controls->collisionSpeedDecreaseCoef * (1 - collision.red));
      controls->boost = 0;
    }


  if (collision.red >= 0.9 && collision.green > 0.9 && collision.blue < 0.8)
    controls->check_point = (int)floor (collision.blue * 10);
}

static float
lerp (float a, float b, float alpha)
{
  return a * alpha + b * (1.0-alpha);
}

static void
lerp_point (const graphene_point3d_t *a, const graphene_point3d_t *b, float alpha, graphene_point3d_t *dest)
{
  dest->x = lerp (a->x, b->x, alpha);
  dest->y = lerp (a->y, b->y, alpha);
  dest->z = lerp (a->z, b->z, alpha);
}

static void
ship_controls_destroy (ShipControls *controls)
{
  play_sound ("destroyed", FALSE);
  stop_sound ("bg");
  //stop_sound ("wind");

  controls->active = FALSE;
  controls->destroyed = TRUE;
  controls->collision_front = FALSE;
  controls->collision_left = FALSE;
  controls->collision_right = FALSE;
}

static void
ship_controls_fall (ShipControls *controls)
{
  controls->active = FALSE;
  controls->collision_front = FALSE;
  controls->collision_left = FALSE;
  controls->collision_right = FALSE;
  controls->falling = TRUE;
  controls->fall_start_time = g_get_monotonic_time ();
}

void
ship_controls_update (ShipControls *controls,
                      float dt)
{
  graphene_point3d_t rotation = { 0, 0, 0 };
  graphene_point3d_t movement = { 0, 0, 0 };
  graphene_quaternion_t quaternion;

  if (controls->falling)
    {
      graphene_vec3_t pos, fall_vec;

      graphene_vec3_init (&fall_vec, 0, -20, 0);
      graphene_vec3_add (gthree_object_get_position (controls->mesh), &fall_vec, &pos);

      gthree_object_set_position (controls->mesh, &pos);

      if ((g_get_monotonic_time () - controls->fall_start_time) > 2 * G_USEC_PER_SEC)
        controls->destroyed = TRUE;

      return;
    }

  rotation.y = 0;
  controls->drift += (0.0 - controls->drift) * controls->driftLerp;
  controls->angular += (0.0 - controls->angular) * controls->angularLerp * 0.5;

  float rollAmount = 0.0;
  float angularAmount = 0.0;

  if (controls->active)
    {
      if (controls->key_left)
        {
          angularAmount += controls->angularSpeed * dt;
          rollAmount -= controls->rollAngle;
        }

      if (controls->key_right)
        {
          angularAmount -= controls->angularSpeed * dt;
          rollAmount += controls->rollAngle;
        }

      if (controls->key_forward)
        controls->speed += controls->thrust * dt;
      else
        controls->speed -= controls->airResist * dt;

      if (controls->key_ltrigger)
        {
          if (controls->key_left)
            angularAmount += controls->airAngularSpeed * dt;
          else
            angularAmount += controls->airAngularSpeed * 0.5 * dt;

          controls->speed -= controls->airBrake * dt;
          controls->drift += (controls->airDrift - controls->drift) * controls->driftLerp;
          movement.x += controls->speed * controls->drift * dt;

          if(controls->drift > 0.0)
            movement.z -= controls->speed * controls->drift * dt;

          rollAmount -= controls->rollAngle * 0.7;
        }

      if (controls->key_rtrigger)
        {
          if(controls->key_right)
            angularAmount -= controls->airAngularSpeed * dt;
          else
            angularAmount -= controls->airAngularSpeed * 0.5 * dt;
          controls->speed -= controls->airBrake * dt;
          controls->drift += (-controls->airDrift - controls->drift) * controls->driftLerp;
          movement.x += controls->speed * controls->drift * dt;
          if(controls->drift < 0.0)
            movement.z += controls->speed * controls->drift * dt;
          rollAmount += controls->rollAngle * 0.7;
        }
    }

  controls->angular += (angularAmount - controls->angular) * controls->angularLerp;
  rotation.y = controls->angular;

  controls->speed = fmax (0.0, fmin (controls->speed, controls->maxSpeed));
  controls->speedRatio = controls->speed / controls->maxSpeed;
  movement.z += controls->speed * dt;

  if (graphene_point3d_near (&controls->repulsionForce, graphene_point3d_zero (), EPSILON))
    graphene_point3d_init (&controls->repulsionForce, 0, 0, 0);
  else
    {
      if (controls->repulsionForce.z != 0.0)
        movement.z = 0;

      movement.x += controls->repulsionForce.x;
      movement.y += controls->repulsionForce.y;
      movement.z += controls->repulsionForce.z;

      lerp_point (graphene_point3d_zero (), &controls->repulsionForce,
                  dt > 1.5 ? controls->repulsionLerp*2 : controls->repulsionLerp,
                  &controls->repulsionForce);
    }

  graphene_vec3_t collisionPreviousPosition = *gthree_object_get_position (controls->dummy);

  ship_controls_booster_check (controls, &movement, dt);

  gthree_object_translate_x (controls->dummy, movement.x);
  gthree_object_translate_z (controls->dummy, movement.z);

  ship_controls_height_check (controls, &movement, dt);
  gthree_object_translate_y (controls->dummy, movement.y);

  graphene_vec3_subtract (gthree_object_get_position (controls->dummy), &collisionPreviousPosition,
                          &controls->currentVelocity);

  ship_controls_collision_check (controls, dt);

  graphene_quaternion_init (&quaternion,
                            rotation.x, rotation.y, rotation.z, 1);
  graphene_quaternion_normalize (&quaternion, &quaternion);
  graphene_quaternion_multiply (&quaternion,
                                gthree_object_get_quaternion (controls->dummy),
                                &quaternion);
  gthree_object_set_quaternion (controls->dummy, &quaternion);
  gthree_object_update_matrix (controls->dummy);

  if (controls->shield <= 0.0)
    {
      controls->shield = 0.0;
      ship_controls_destroy (controls);
    }

  if (controls->mesh)
    {
      graphene_matrix_t matrix;

      graphene_matrix_init_identity (&matrix);

      // Gradient (Mesh only, no dummy physics impact)
      float gradientDelta = (controls->gradientTarget - (controls->gradient)) * controls->gradientLerp;

      if (fabs (gradientDelta) > EPSILON)
        controls->gradient += gradientDelta;

      if (fabs(controls->gradient) > EPSILON)
        graphene_matrix_rotate_x (&matrix, RAD_TO_DEG (controls->gradient));

      // Tilting (Idem)
      float tiltDelta = (controls->tiltTarget - controls->tilt) * controls->tiltLerp;

      if (fabs (tiltDelta) > EPSILON)
        controls->tilt += tiltDelta;

      if (fabs (controls->tilt) > EPSILON)
        graphene_matrix_rotate_z (&matrix, RAD_TO_DEG (controls->tilt));

      // Rolling (Idem)
      float rollDelta = (rollAmount - controls->roll) * controls->rollLerp;
      if (fabs (rollDelta) > EPSILON)
        controls->roll += rollDelta;
      if (fabs (controls->roll) > EPSILON)
        {
          graphene_matrix_rotate_z (&matrix, RAD_TO_DEG (controls->roll));
        }

      graphene_matrix_multiply (&matrix,
                                gthree_object_get_matrix (controls->dummy),
                                &matrix);

      gthree_object_set_matrix (controls->mesh, &matrix);
      gthree_object_update_matrix_world (controls->mesh, TRUE);
    }

  //Update listener position
  //bkcore.Audio.setListenerPos(&movement);
  //bkcore.Audio.setListenerVelocity(&controls->currentVelocity);

}

static gboolean
handle_key (ShipControls *controls,
            GdkEventKey *event,
            gboolean down)
{
  switch (event->keyval)
    {
    case GDK_KEY_Left:
      controls->key_left = down;
      return TRUE;
    case GDK_KEY_Right:
      controls->key_right = down;
      return TRUE;
    case GDK_KEY_Up:
      controls->key_forward = down;
      return TRUE;
    case GDK_KEY_Down:
      controls->key_backward = down;
      return TRUE;
    case GDK_KEY_A:
    case GDK_KEY_a:
    case GDK_KEY_Q:
    case GDK_KEY_q:
      controls->key_ltrigger = down;
      return TRUE;
    case GDK_KEY_D:
    case GDK_KEY_d:
    case GDK_KEY_E:
    case GDK_KEY_e:
      controls->key_rtrigger = down;
      return TRUE;
    }

  return FALSE;
}

gboolean
ship_controls_key_press (ShipControls *controls,
                         GdkEventKey *event)
{
  return handle_key (controls, event, TRUE);
}

gboolean
ship_controls_key_release (ShipControls *controls,
                           GdkEventKey *event)
{
  return handle_key (controls, event, FALSE);
}

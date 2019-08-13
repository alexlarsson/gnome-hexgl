#include "gameplay.h"
#include "utils.h"
#include "sounds.h"

struct _Gameplay {
  ShipControls *controls;
  CameraChase *chase;
  HUD *hud;
  GTimer *timer;
  GCallback finished_cb;

  gboolean active;
  int result;
  int step;
  int lap;
  int previous_checkpoint;
  int last_checkpoint;
  int max_laps;
  float score;

  double message_end_time;
  HUDMessage *message;
};

enum {
      RESULT_NONE,
      RESULT_FINISHED,
      RESULT_DESTROYED,
};


Gameplay *
gameplay_new (ShipControls *controls,
              HUD *hud,
              CameraChase *chase,
              GCallback finished_cb)
{
  Gameplay *gameplay = g_new0 (Gameplay, 1);

  gameplay->active = FALSE;
  gameplay->controls = controls;
  gameplay->hud = hud;
  gameplay->chase = chase;
  gameplay->timer = g_timer_new ();
  gameplay->finished_cb = finished_cb;

  return gameplay;
}

void
gameplay_free (Gameplay *gameplay)
{
  g_free (gameplay);
}

void
gameplay_start (Gameplay *gameplay)
{
  graphene_vec3_t pos;
  graphene_vec3_t rot;

 if (gameplay->message)
    hud_remove_message (gameplay->hud, gameplay->message);
   gameplay->message = NULL;

  ship_controls_set_active (gameplay->controls, FALSE);
  camera_chase_set_mode (gameplay->chase, MODE_CHASE);

  gameplay->result = RESULT_NONE;
  gameplay->step = 0;
  gameplay->lap = 1;
  gameplay->max_laps = 3;
  gameplay->previous_checkpoint = -1;
  gameplay->last_checkpoint = 2;
  gameplay->score = -1;
  gameplay->active = TRUE;

  graphene_vec3_init (&pos,
                      -1134*2,
                      387,
                      -443*2);
  graphene_vec3_init (&rot, 0, 0, 0);

  ship_controls_reset (gameplay->controls,
                       &pos, &rot);

  gameplay->message = hud_show_message (gameplay->hud, "GET READY");

  play_sound ("bg", TRUE);
  //play_sound ("wind", TRUE);

  hud_set_lap (gameplay->hud, gameplay->lap, gameplay->max_laps);
  hud_set_time (gameplay->hud, -1.0);
  g_timer_start (gameplay->timer);
}

static void
gameplay_message (Gameplay *gameplay,
                  const char *message,
                  float end_time)
{
  if (gameplay->message != NULL)
    {
      hud_remove_message (gameplay->hud, gameplay->message);
      gameplay->message = NULL;
    }

  if (message)
    {
      gameplay->message = hud_show_message (gameplay->hud, message);
      gameplay->message_end_time = end_time;
    }
}


void
gameplay_end (Gameplay *gameplay,
              int result)
{
  gameplay->result = result;
  gameplay->score = g_timer_elapsed (gameplay->timer, NULL);
  ship_controls_set_active (gameplay->controls, FALSE);
  g_timer_start (gameplay->timer);

  if (result == RESULT_FINISHED)
    {
      // TODO: Go to replay
      camera_chase_set_mode (gameplay->chase, MODE_ORBIT);
      ship_controls_stop (gameplay->controls);
      gameplay_message (gameplay, "Finished", 2.0);
      gameplay->step = 100;
    }
  else if (result == RESULT_DESTROYED)
    {
      gameplay_message (gameplay, "Destroyed", 2.0);
      gameplay->step = 100;
    }
}

void
gameplay_update (Gameplay *gameplay,
                 float dt)
{
  if (!gameplay->active)
    return;

  double elapsed = g_timer_elapsed (gameplay->timer, NULL);

  // Clear old messages
  if (gameplay->message != NULL &&
      gameplay->message_end_time > 0 &&
      elapsed > gameplay->message_end_time)
    {
      hud_remove_message (gameplay->hud, gameplay->message);
      gameplay->message = NULL;
    }

  switch (gameplay->step)
    {
    case 0:
      if (elapsed > 1.0)
        {
          gameplay_message (gameplay, "3", -1);
          gameplay->step = 1;
        }
      break;

    case 1:
     if (elapsed > 1.0 + 1.0*1)
        {
          gameplay_message (gameplay, "2", -1);
          gameplay->step = 2;
        }
      break;

    case 2:
      if (elapsed > 1.0 + 1.0*2)
        {
          gameplay_message (gameplay, "1", -1);
          gameplay->step = 3;
        }
      break;

    case 3:
      if (elapsed > 1.0 + 1.0*3)
        {
          gameplay_message (gameplay, NULL, -1);
          gameplay->step = 4;

          g_timer_start (gameplay->timer);
          ship_controls_set_active (gameplay->controls, TRUE);
        }
      break;

    case 4:
      hud_set_time (gameplay->hud, elapsed);

      int cp = ship_controls_get_check_point (gameplay->controls);
      if (cp == 0 && gameplay->previous_checkpoint == gameplay->last_checkpoint)
        {
          gameplay->previous_checkpoint = cp;
          gameplay->lap++;

          if (gameplay->lap > gameplay->max_laps)
            {
              gameplay_end (gameplay, RESULT_FINISHED);
            }
          else
            {
              if (gameplay->lap == gameplay->max_laps)
                gameplay_message (gameplay, "Final lap", elapsed + 2.0);
              hud_set_lap (gameplay->hud, gameplay->lap, gameplay->max_laps);
            }
        }
      else if (cp != -1 && cp != gameplay->previous_checkpoint)
        {
          //gameplay_message (gameplay, "Checkpoint", elapsed + 1.0);
          gameplay->previous_checkpoint = cp;
        }

      if (gameplay->result == RESULT_NONE &&
          ship_controls_is_destroyed (gameplay->controls))
        {
          gameplay_end (gameplay, RESULT_DESTROYED);
        }

      break;

    case 100:
      if (elapsed > 2)
        {
          gameplay->active = FALSE;
          gameplay->finished_cb ();
        }
      break;

    }
}

gboolean
gameplay_key_press (Gameplay *gameplay,
                    GdkEventKey *event)
{
  switch (event->keyval)
    {
    case GDK_KEY_Escape:
      if (gameplay->active)
        gameplay_start (gameplay);
      break;
    }
  return FALSE;
}

gboolean
gameplay_key_release (Gameplay *gameplay,
                      GdkEventKey *event)
{
  return FALSE;
}

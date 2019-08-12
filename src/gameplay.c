#include "gameplay.h"
#include "utils.h"

struct _Gameplay {
  ShipControls *controls;
  HUD *hud;
  GTimer *timer;

  int step;
  int lap;
  int previous_checkpoint;
  int last_checkpoint;
  int max_laps;

  HUDMessage *message;
};


Gameplay *
gameplay_new (ShipControls *controls,
              HUD *hud)
{
  Gameplay *gameplay = g_new0 (Gameplay, 1);

  gameplay->controls = controls;
  gameplay->hud = hud;
  gameplay->timer = g_timer_new ();

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

  gameplay->step = 0;
  gameplay->lap = 1;
  gameplay->max_laps = 3;
  gameplay->previous_checkpoint = -1;
  gameplay->last_checkpoint = 2;

  graphene_vec3_init (&pos,
                      -1134*2,
                      387,
                      -443*2);
  graphene_vec3_init (&rot, 0, 0, 0);

  ship_controls_reset (gameplay->controls,
                       &pos, &rot);

  gameplay->message = hud_show_message (gameplay->hud, "GET READY");

  hud_set_lap (gameplay->hud, gameplay->lap, gameplay->max_laps);
  hud_set_time (gameplay->hud, -1.0);
  g_timer_start (gameplay->timer);
}

void
gameplay_update (Gameplay *gameplay,
                 float dt)
{
  double elapsed;
  switch (gameplay->step)
    {
    case 0:
      elapsed = g_timer_elapsed (gameplay->timer, NULL);
      if (elapsed > 1.0)
        {
          hud_remove_message (gameplay->hud, gameplay->message);
          gameplay->message = hud_show_message (gameplay->hud, "3");
          gameplay->step = 1;
        }
      break;

    case 1:
      elapsed = g_timer_elapsed (gameplay->timer, NULL);
      if (elapsed > 1.0 + 1.0*1)
        {
          hud_remove_message (gameplay->hud, gameplay->message);
          gameplay->message = hud_show_message (gameplay->hud, "2");
          gameplay->step = 2;
        }
      break;

    case 2:
      elapsed = g_timer_elapsed (gameplay->timer, NULL);
      if (elapsed > 1.0 + 1.0*2)
        {
          hud_remove_message (gameplay->hud, gameplay->message);
          gameplay->message = hud_show_message (gameplay->hud, "1");
          gameplay->step = 3;
        }
      break;

    case 3:
      elapsed = g_timer_elapsed (gameplay->timer, NULL);
      if (elapsed > 1.0 + 1.0*3)
        {
          hud_remove_message (gameplay->hud, gameplay->message);
          gameplay->message = NULL;
          gameplay->step = 4;

          g_timer_start (gameplay->timer);
          ship_controls_set_active (gameplay->controls, TRUE);
        }
      break;

    case 4:
      elapsed = g_timer_elapsed (gameplay->timer, NULL);
      hud_set_time (gameplay->hud, elapsed);

      int cp = ship_controls_get_check_point (gameplay->controls);
      if (cp == 0 && gameplay->previous_checkpoint == gameplay->last_checkpoint)
        {
          gameplay->previous_checkpoint = cp;
          gameplay->lap++;

          if (gameplay->lap > gameplay->max_laps)
            {
              g_print ("DONE!\n");
            }
          else
            {
              g_print ("LAP!\n");
              hud_set_lap (gameplay->hud, gameplay->lap, gameplay->max_laps);
            }
        }
      else if (cp != -1 && cp != gameplay->previous_checkpoint)
        {
          gameplay->previous_checkpoint = cp;
          g_print ("Checkpoint %d\n", cp);
        }

      break;

    }
}

gboolean
gameplay_key_press (Gameplay *gameplay,
                    GdkEventKey *event)
{
  return FALSE;
}

gboolean
gameplay_key_release (Gameplay *gameplay,
                      GdkEventKey *event)
{
  return FALSE;
}

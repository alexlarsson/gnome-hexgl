#include "shipcontrols.h"

struct _ShipControls {
  GthreeObject *mesh;
  GthreeObject *dummy;

  gboolean key_forward;
  gboolean key_backward;
  gboolean key_left;
  gboolean key_right;
  gboolean key_ltrigger;
  gboolean key_rtrigger;
  gboolean key_use;
};

ShipControls *
ship_controls_new (void)
{
  ShipControls *controls = g_new0 (ShipControls, 1);


  controls->dummy = gthree_object_new ();
  return controls;
}

void
ship_controls_control (ShipControls *controls,
                       GthreeObject *mesh)
{
  g_clear_object (&controls->mesh);
  controls->mesh = g_object_ref (mesh);

  gthree_object_set_matrix_auto_update (controls->mesh, FALSE);
  gthree_object_set_position_vec3 (controls->dummy,
                                   gthree_object_get_position (mesh));
}


void
ship_controls_free (ShipControls *controls)
{
  g_clear_object (&controls->mesh);
  g_object_unref (controls->dummy);
  g_free (controls);
}

void
ship_controls_update (ShipControls *controls,
                      float dt)
{
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
      controls->key_ltrigger = down;
      return TRUE;
    case GDK_KEY_S:
    case GDK_KEY_s:
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

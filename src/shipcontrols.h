#include <gthree/gthree.h>

typedef struct _ShipControls ShipControls;

ShipControls *ship_controls_new (void);

void ship_controls_control (ShipControls *controls,
                            GthreeObject *mesh);

void ship_controls_free (ShipControls *controls);
void ship_controls_update (ShipControls *controls,
                           float dt);

gboolean ship_controls_key_press (ShipControls *controls,
                                  GdkEventKey *event);
gboolean ship_controls_key_release (ShipControls *controls,
                                    GdkEventKey *event);

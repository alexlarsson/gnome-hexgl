#include <gthree/gthree.h>
#include "analysismap.h"

typedef struct _ShipControls ShipControls;

ShipControls *ship_controls_new (void);

void ship_controls_control (ShipControls *controls,
                            GthreeObject *mesh);
void ship_controls_set_height_map (ShipControls *controls,
                                   AnalysisMap *map);

void ship_controls_free (ShipControls *controls);
void ship_controls_update (ShipControls *controls,
                           float dt);

float ship_controls_get_speed_ratio (ShipControls *controls);

gboolean ship_controls_key_press (ShipControls *controls,
                                  GdkEventKey *event);
gboolean ship_controls_key_release (ShipControls *controls,
                                    GdkEventKey *event);

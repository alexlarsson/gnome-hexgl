#include <gthree/gthree.h>
#include "shipcontrols.h"

typedef struct _ShipEffects ShipEffects;

ShipEffects *ship_effects_new (GthreeScene *scene,
                               ShipControls *controls);

void ship_effects_free (ShipEffects *effects);
void ship_effects_update (ShipEffects *effects,
                           float dt);

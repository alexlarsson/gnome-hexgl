#include <gthree/gthree.h>
#include "shipcontrols.h"

typedef struct _HUD HUD;

HUD *hud_new (ShipControls *controls,
              GtkWidget *widget);
void hud_free (HUD *hud);
void hud_update (HUD *hud,
                 float dt);

void hud_add_passes (HUD *hud,
                     GthreeEffectComposer *composer);

void hud_update_screen_size (HUD *hud,
                             int width,
                             int height);

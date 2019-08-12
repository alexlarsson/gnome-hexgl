#include <gthree/gthree.h>
#include "shipcontrols.h"

typedef struct _HUD HUD;
typedef struct _HUDMessage HUDMessage;

HUD *       hud_new                (ShipControls         *controls,
                                    GtkWidget            *widget);
void        hud_free               (HUD                  *hud);
void        hud_update             (HUD                  *hud,
                                    float                 dt);
void        hud_set_lap            (HUD                  *hud,
                                    int                   lap,
                                    int                   max_laps);
void        hud_set_time           (HUD                  *hud,
                                    gdouble               time);
void        hud_add_passes         (HUD                  *hud,
                                    GthreeEffectComposer *composer);
void        hud_update_screen_size (HUD                  *hud,
                                    int                   width,
                                    int                   height);
HUDMessage *hud_show_message       (HUD                  *hud,
                                    const char           *text);
void        hud_remove_message     (HUD                  *hud,
                                    HUDMessage           *message);

#include <gthree/gthree.h>
#include "shipcontrols.h"
#include "hud.h"
#include "camerachase.h"

typedef struct _Gameplay Gameplay;

Gameplay *gameplay_new         (ShipControls *controls,
                                HUD          *hud,
                                CameraChase *chase,
                                GCallback     finished_cb);
void      gameplay_free        (Gameplay     *gameplay);
void      gameplay_update      (Gameplay     *gameplay,
                                float         dt);
gboolean  gameplay_key_press   (Gameplay     *gameplay,
                                GdkEventKey  *event);
gboolean  gameplay_key_release (Gameplay     *gameplay,
                                GdkEventKey  *event);
void      gameplay_start       (Gameplay     *gameplay);

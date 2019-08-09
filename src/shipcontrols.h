#include <gthree/gthree.h>
#include "analysismap.h"

typedef struct _ShipControls ShipControls;

ShipControls *         ship_controls_new                  (void);
void                   ship_controls_set_difficulty       (ShipControls *controls,
                                                           int           difficulty);
void                   ship_controls_control              (ShipControls *controls,
                                                           GthreeObject *mesh);
void                   ship_controls_set_height_map       (ShipControls *controls,
                                                           AnalysisMap  *map);
void                   ship_controls_set_collision_map    (ShipControls *controls,
                                                           AnalysisMap  *map);
GthreeObject *         ship_controls_get_mesh             (ShipControls *controls);
GthreeObject *         ship_controls_get_dummy            (ShipControls *controls);
const graphene_vec3_t *ship_controls_get_current_velocity (ShipControls *controls);
gboolean               ship_controls_get_collision_left   (ShipControls *controls);
gboolean               ship_controls_get_collision_right  (ShipControls *controls);
void                   ship_controls_free                 (ShipControls *controls);
void                   ship_controls_update               (ShipControls *controls,
                                                           float         dt);
float                  ship_controls_get_real_speed_ratio (ShipControls *controls);
float                  ship_controls_get_speed_ratio      (ShipControls *controls);
float                  ship_controls_get_boost_ratio      (ShipControls *controls);
gboolean               ship_controls_is_accelerating      (ShipControls *controls);
gboolean               ship_controls_is_destroyed         (ShipControls *controls);
float                  ship_controls_get_shield_ratio     (ShipControls *controls);
int                    ship_controls_get_shield           (ShipControls *controls,
                                                           float scale);
gboolean               ship_controls_key_press            (ShipControls *controls,
                                                           GdkEventKey  *event);
gboolean               ship_controls_key_release          (ShipControls *controls,
                                                           GdkEventKey  *event);

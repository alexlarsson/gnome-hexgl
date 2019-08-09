#include <gthree/gthree.h>

typedef struct _Particles Particles;

Particles *   particles_new                     (int                    max,
                                                 float                  size);
GthreeObject *particles_get_object              (Particles             *particles);
void          particles_free                    (Particles             *particles);
void          particles_emit                    (Particles             *particles,
                                                 int                    count);
void          particles_update                  (Particles             *particles,
                                                 float                  dt);
void          particles_set_spawn_point         (Particles             *particles,
                                                 const graphene_vec3_t *spawn_point);
void          particles_set_spawn_radius        (Particles             *particles,
                                                 const graphene_vec3_t *spawn_radius);
void          particles_set_velocity            (Particles             *particles,
                                                 const graphene_vec3_t *velocity);
void          particles_set_velocity_randomness (Particles             *particles,
                                                 const graphene_vec3_t *velocity_randomness);
void          particles_set_life                (Particles             *particles,
                                                 float                  life);
void          particles_set_size                (Particles             *particles,
                                                 float                  size);
void          particles_set_color1              (Particles             *particles,
                                                 GdkRGBA               *color);
void          particles_set_color2              (Particles             *particles,
                                                 GdkRGBA               *color);
void          particles_set_map                 (Particles             *particles,
                                                 GthreeTexture         *texture);

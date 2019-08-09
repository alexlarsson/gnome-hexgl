#include "particles.h"

typedef struct {
  int index;
  graphene_vec3_t position;
  graphene_vec3_t velocity;
  graphene_vec3_t force;
  graphene_vec3_t color;
  graphene_vec3_t base_color;
  float life;
}  Particle;

struct _Particles {
  GthreePointsMaterial *material;
  GthreePoints *points;
  GthreeTexture *map;
  GthreeGeometry *geometry;

  GthreeAttribute *position_attr;
  GthreeAttribute *color_attr;

  Particle *buffer;
  int buffer_size;
  int buffer_used;

  float size;
  float life;
  float ageing;
  float friction;
  float spawn_rate;
  float spawn_count;

  graphene_vec3_t color1;
  graphene_vec3_t color2;

  graphene_vec3_t spawn_point;
  graphene_vec3_t spawn_radius;
  graphene_vec3_t velocity;
  graphene_vec3_t velocity_randomness;
  graphene_vec3_t force;
};

static void
particle_reset (Particle *particle)
{
  graphene_vec3_init (&particle->position,
                      100000, 100000, 100000);
  graphene_vec3_init (&particle->color, 0, 0, 1.0);
 }

GthreeObject *
particles_get_object (Particles *particles)
{
  return GTHREE_OBJECT (particles->points);
}

void
particles_set_spawn_point (Particles *particles,
                           const graphene_vec3_t *spawn_point)
{
  particles->spawn_point = *spawn_point;
}

void
particles_set_spawn_radius (Particles *particles,
                            const graphene_vec3_t *spawn_radius)
{
  particles->spawn_radius = *spawn_radius;
}

void
particles_set_velocity (Particles *particles,
                        const graphene_vec3_t *velocity)
{
  particles->velocity = *velocity;
}

void
particles_set_velocity_randomness (Particles *particles,
                                   const graphene_vec3_t *velocity_randomness)
{
  particles->velocity_randomness = *velocity_randomness;
}

void
particles_set_life (Particles *particles, float life)
{
  particles->life = life;
  particles->ageing = 1 / particles->life;
}

void
particles_set_map (Particles             *particles,
                   GthreeTexture         *texture)
{
  g_set_object (&particles->map, texture);
  gthree_points_material_set_map (particles->material, particles->map);
}

void
particles_set_size (Particles *particles, float size)
{
  particles->size = size;
  gthree_points_material_set_size (particles->material, particles->size);
}

void
particles_set_color1 (Particles *particles, GdkRGBA *color)
{
  graphene_vec3_init (&particles->color1, color->red, color->green, color->blue);
}

void
particles_set_color2 (Particles *particles, GdkRGBA *color)
{
  graphene_vec3_init (&particles->color2, color->red, color->green, color->blue);
}

Particles *
particles_new (int max, float size)
{
  Particles *particles = g_new0 (Particles, 1);

  particles->buffer_size = max;
  particles->buffer_used = 0;
  particles->buffer = g_new0 (Particle, particles->buffer_size);

  for (int i = 0; i < particles->buffer_size; i++)
    {
      Particle *p = &particles->buffer[i];
      p->index = i;
      particle_reset (p);
    }

  particles->size = size;
  particles->life = 60;
  particles->ageing = 1 / particles->life;
  particles->friction = 1.0;
  particles->spawn_rate = 0;
  particles->spawn_count = 0;
  graphene_vec3_init (&particles->spawn_point, 0, 0, 0);
  graphene_vec3_init (&particles->spawn_radius, 0, 0, 0);
  graphene_vec3_init (&particles->velocity, 0, 0, 0);
  graphene_vec3_init (&particles->velocity_randomness, 0, 0, 0);
  graphene_vec3_init (&particles->force, 0, 0, 0);

  graphene_vec3_init (&particles->color1, 1.0, 0, 0);
  graphene_vec3_init (&particles->color2, 0.0, 1.0, 0);

  particles->position_attr = gthree_attribute_new ("position", GTHREE_ATTRIBUTE_TYPE_FLOAT,
                                                   particles->buffer_size, 3, FALSE);
  gthree_attribute_set_dynamic (particles->position_attr, TRUE);
  particles->color_attr = gthree_attribute_new ("color", GTHREE_ATTRIBUTE_TYPE_FLOAT,
                                                particles->buffer_size, 3, FALSE);
  gthree_attribute_set_dynamic (particles->color_attr, TRUE);

  particles->material = gthree_points_material_new ();
  gthree_points_material_set_size (particles->material, particles->size);
  gthree_material_set_vertex_colors (GTHREE_MATERIAL (particles->material), TRUE);
  gthree_material_set_depth_test (GTHREE_MATERIAL (particles->material), FALSE);

  particles->geometry = gthree_geometry_new ();
  gthree_geometry_add_attribute (particles->geometry, "position",   particles->position_attr);
  gthree_geometry_add_attribute (particles->geometry, "color", particles->color_attr);

  particles->points = gthree_points_new (particles->geometry, GTHREE_MATERIAL (particles->material));

  return particles;
}

void
particles_free (Particles *particles)
{
  g_clear_object (&particles->map);
  g_clear_object (&particles->geometry);
  g_free (particles->buffer);
  g_free (particles);
}

static void
init_random_vector (graphene_vec3_t *dst)
{
  graphene_vec3_init (dst,
                      g_random_double_range (-1, 1),
                      g_random_double_range (-1, 1),
                      g_random_double_range (-1, 1));
}

void
particles_emit (Particles *particles,
                int count)
{
  int emitable = MIN (count, particles->buffer_size - particles->buffer_used);

  for (int i = 0; i < emitable; i++)
    {
      int buffer_index = particles->buffer_used++;
      Particle *p = &particles->buffer[buffer_index];

      init_random_vector (&p->position);
      graphene_vec3_multiply (&p->position, &particles->spawn_radius, &p->position);
      graphene_vec3_add (&p->position, &particles->spawn_point, &p->position);

      init_random_vector (&p->velocity);
      graphene_vec3_multiply (&p->velocity, &particles->velocity_randomness, &p->velocity);
      graphene_vec3_add (&p->velocity, &particles->velocity, &p->velocity);

      p->force = particles->force;

      graphene_vec3_interpolate (&particles->color1,
                                 &particles->color2,
                                 g_random_double_range (0, 1),
                                 &p->base_color);

      p->life = 1;
    }
}

void
particles_update (Particles *particles,
                  float dt)
{
  float l;
  graphene_vec3_t df, dv;

  particles->spawn_count += particles->spawn_rate;
  if (particles->spawn_count >= 1.0f)
    {
      float count = floorf (particles->spawn_count);
      particles->spawn_count -= count;
      particles_emit (particles, count);
   }

  for (int i = 0; i < particles->buffer_used; i++)
    {
      Particle *p = &particles->buffer[i];

      p->life -= particles->ageing;

      if (p->life <= 0)
        {
          particle_reset (p);

          // Swap this and last used so we don't get a hole in the buffer array with unused particles
          if (i != particles->buffer_used -1)
            {
              Particle tmp = *p;
              particles->buffer[i] = particles->buffer[particles->buffer_used-1];
              particles->buffer[particles->buffer_used-1] = tmp;
            }

          particles->buffer_used--;

          i--; // compensate for the i++ in the loop since we deleted one
          continue;
        }

      l = p->life > 0.5 ? 1.0 : p->life + 0.5;

      graphene_vec3_scale (&p->base_color, l, &p->color);

      graphene_vec3_scale (&p->velocity, particles->friction, &p->velocity);

      graphene_vec3_scale (&p->force, dt, &df);
      graphene_vec3_add (&p->velocity, &df, &p->velocity);

      graphene_vec3_scale (&p->velocity, dt, &dv);
      graphene_vec3_add (&p->position, &dv, &p->position);
    }

  // Update buffer
  for (int i = 0; i < particles->buffer_size; i++)
    {
      Particle *p = &particles->buffer[i];

      gthree_attribute_set_vec3 (particles->position_attr, p->index, &p->position);
      gthree_attribute_set_vec3 (particles->color_attr, p->index, &p->color);
    }

  gthree_attribute_set_needs_update (particles->position_attr);
  gthree_attribute_set_needs_update (particles->color_attr);
  gthree_geometry_invalidate_bounds (particles->geometry);
}

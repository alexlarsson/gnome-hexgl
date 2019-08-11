#include "hud.h"
#include "utils.h"

static GthreeTexture *
cairo_texture_new (int width, int height)
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  GthreeTexture *texture = gthree_texture_new_from_surface (surface);
  gthree_texture_set_flip_y (texture, FALSE); // We'll draw upside down to avoid flip on upload

  return texture;
}

static cairo_t *
cairo_texture_begin_paint (GthreeTexture *texture)
{
  cairo_surface_t *surface = gthree_texture_get_surface (texture);
  cairo_t *cr = cairo_create (surface);
  int height = cairo_image_surface_get_height (surface);

  // Flip y for OpenGL
  cairo_scale (cr, 1, -1);
  cairo_translate (cr, 0, -height);

  // Clear to transparent
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  return cr;
}

static void
cairo_texture_end_paint (GthreeTexture *texture)
{
  gthree_texture_set_needs_update (texture, TRUE);
}

struct _HUD {
  ShipControls *controls;
  PangoContext *pango_context;

  int screen_width;
  int screen_height;

  GthreeSprite *bg_sprite;
  GthreeSprite *fg_speed_sprite;
  GthreeSprite *fg_shield_sprite;

  GthreeSprite *data_sprite;
  GthreeTexture *data_texture;

  PangoLayout *speed_layout;
  PangoLayout *shield_layout;

  GthreeSprite *lap_sprite;
  GthreeTexture *lap_texture;
  PangoLayout *lap_layout;

  GthreeSprite *time_sprite;
  GthreeTexture *time_texture;
  PangoLayout *time_layout;

  float aspect;

  char *lap_text;
  char *time_text;

  GthreeOrthographicCamera *camera;
  GArray *speed_clipping_planes;
  GArray *shield_clipping_planes;

  GthreePass *pass;
  GthreePass *pass2;
  GthreePass *pass3;
};

static void
hud_update_sprites (HUD *hud,
                    int width,
                    int height)
{
  graphene_vec3_t pos, s;
  float hud_width, hud_height;

  hud_width = width; // fullscreen width
  hud_height = hud_width * hud->aspect;

  gthree_object_set_position (GTHREE_OBJECT (hud->bg_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->bg_sprite),
                           graphene_vec3_init (&s,
                                               hud_width, hud_height, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud->fg_shield_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->fg_shield_sprite),
                           graphene_vec3_init (&s,
                                               hud_width, hud_height, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud->fg_speed_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->fg_speed_sprite),
                           graphene_vec3_init (&s,
                                               hud_width, hud_height, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud->data_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2 + hud_height * 0.6, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->data_sprite),
                           graphene_vec3_init (&s,
                                               hud_height * 0.6, hud_height * 0.6, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud->lap_sprite),
                              graphene_vec3_init (&pos, width / 2 - 20, height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->lap_sprite),
                           graphene_vec3_init (&s,
                                               256, 128, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud->time_sprite),
                              graphene_vec3_init (&pos, 0, height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud->time_sprite),
                           graphene_vec3_init (&s,
                                               512, 128, 1.0));
}

static PangoLayout *
hud_new_pango_layout (HUD *hud,
                      const char *font)
{
  PangoLayout *layout = pango_layout_new (hud->pango_context);
  PangoFontDescription *fd = pango_font_description_from_string (font);
  pango_layout_set_font_description (layout, fd);
  pango_font_description_free (fd);
  return layout;
}

static GthreeSprite *
hud_sprite_new (GthreeTexture *map)
{
  GthreeSprite *sprite = gthree_sprite_new (NULL);
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (sprite)), map);
  gthree_material_set_depth_test (gthree_sprite_get_material (sprite), FALSE);
  return sprite;
}

HUD *
hud_new (ShipControls *controls,
         GtkWidget *widget)
{
  HUD *hud = g_new0 (HUD, 1);
  GthreeScene *scene, *scene2, *scene3;
  graphene_vec2_t v2;
  graphene_vec3_t plane_normal;
  graphene_plane_t plane;
  int hud_width;
  int hud_height;
  graphene_vec3_t pos;

  hud->controls = controls;
  hud->pango_context = gtk_widget_create_pango_context (widget);
  hud->lap_text = g_strdup ("1/3");
  hud->time_text = g_strdup ("00:00:00");

  scene = gthree_scene_new ();

  // Just some values to start with, we'll do the right ones in the callback later
  int width = 100, height = 100;

  hud->camera = gthree_orthographic_camera_new ( -width / 2, width / 2, height / 2, -height / 2, 1, 10);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->camera));
  gthree_object_set_position (GTHREE_OBJECT (hud->camera),
                              graphene_vec3_init (&pos, 0, 0, 10));

  g_autoptr(GthreeTexture) hud_texture = load_texture ("hud-bg.png");
  g_autoptr(GthreeTexture) speed_texture = load_texture ("hud-fg-speed.png");
  g_autoptr(GthreeTexture) shield_texture = load_texture ("hud-fg-shield.png");

  hud_width = gdk_pixbuf_get_width (gthree_texture_get_pixbuf (hud_texture));
  hud_height = gdk_pixbuf_get_height (gthree_texture_get_pixbuf (hud_texture));
  hud->aspect = (float) hud_height / hud_width;

  hud->bg_sprite = hud_sprite_new (hud_texture);
  gthree_sprite_set_center (hud->bg_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->bg_sprite));


  scene2 = gthree_scene_new ();

  hud->fg_shield_sprite = hud_sprite_new (shield_texture);
  gthree_sprite_set_center (hud->fg_shield_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_object_add_child (GTHREE_OBJECT (scene2), GTHREE_OBJECT (hud->fg_shield_sprite));

  scene3 = gthree_scene_new ();

  graphene_vec3_init (&plane_normal, 0, 0, 0);
  graphene_plane_init (&plane, &plane_normal, 0);

  hud->speed_clipping_planes = g_array_new (FALSE, FALSE, sizeof (graphene_plane_t));
  g_array_append_val (hud->speed_clipping_planes, plane);
  g_array_append_val (hud->speed_clipping_planes, plane);

  hud->shield_clipping_planes = g_array_new (FALSE, FALSE, sizeof (graphene_plane_t));
  g_array_append_val (hud->shield_clipping_planes, plane);

  hud->fg_speed_sprite = hud_sprite_new (speed_texture);
  gthree_sprite_set_center (hud->fg_speed_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_object_add_child (GTHREE_OBJECT (scene3), GTHREE_OBJECT (hud->fg_speed_sprite));

  // text data for hud
  hud->data_texture = cairo_texture_new (128, 128);
  hud->data_sprite = hud_sprite_new (hud->data_texture);
  gthree_sprite_set_center (hud->data_sprite, graphene_vec2_init (&v2, 0.5, 0.5));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->data_sprite));

  hud->speed_layout = hud_new_pango_layout (hud, "Sans bold 50");
  pango_layout_set_alignment (hud->speed_layout, PANGO_ALIGN_CENTER);
  pango_layout_set_width (hud->speed_layout, 128);

  hud->shield_layout = hud_new_pango_layout (hud, "Sans bold 20");
  pango_layout_set_alignment (hud->shield_layout, PANGO_ALIGN_CENTER);
  pango_layout_set_width (hud->shield_layout, 128);

  hud->lap_texture = cairo_texture_new (256, 128);
  hud->lap_sprite = hud_sprite_new (hud->lap_texture);
  gthree_sprite_set_center (hud->lap_sprite, graphene_vec2_init (&v2, 1.0, 1.0));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->lap_sprite));

  hud->lap_layout = hud_new_pango_layout (hud, "Sans bold 60");
  pango_layout_set_alignment (hud->lap_layout, PANGO_ALIGN_RIGHT);
  pango_layout_set_width (hud->lap_layout, 256);

  hud->time_texture = cairo_texture_new (512, 128);
  hud->time_sprite = hud_sprite_new (hud->time_texture);
  gthree_sprite_set_center (hud->time_sprite, graphene_vec2_init (&v2, 0.5, 1.0));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->time_sprite));

  hud->time_layout = hud_new_pango_layout (hud, "Sans bold 60");
  pango_layout_set_alignment (hud->time_layout, PANGO_ALIGN_CENTER);
  pango_layout_set_width (hud->time_layout, 256);

  hud_update_sprites (hud, width, height);

  hud->pass = gthree_render_pass_new (scene, GTHREE_CAMERA (hud->camera), NULL);
  gthree_pass_set_clear (hud->pass, FALSE);

  hud->pass2 = gthree_render_pass_new (scene2, GTHREE_CAMERA (hud->camera), NULL);
  gthree_render_pass_set_clipping_planes (GTHREE_RENDER_PASS (hud->pass2), hud->shield_clipping_planes);
  gthree_pass_set_clear (hud->pass2, FALSE);

  hud->pass3 = gthree_render_pass_new (scene3, GTHREE_CAMERA (hud->camera), NULL);
  gthree_render_pass_set_clipping_planes (GTHREE_RENDER_PASS (hud->pass3), hud->speed_clipping_planes);
  gthree_pass_set_clear (hud->pass3, FALSE);

  return hud;
}

void
hud_free (HUD *hud)
{
  g_free (hud->time_text);
  g_free (hud->lap_text);
  g_free (hud);
}

void
hud_add_passes (HUD *hud,
                GthreeEffectComposer *composer)
{
  gthree_effect_composer_add_pass  (composer, hud->pass);
  gthree_effect_composer_add_pass  (composer, hud->pass2);
  gthree_effect_composer_add_pass  (composer, hud->pass3);
}

static void
hud_update_data_surface (HUD *hud)
{
  cairo_t *cr = cairo_texture_begin_paint (hud->data_texture);

  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_move_to (cr, 64, 20);

  g_autofree char *speed_text = g_strdup_printf ("%d", ship_controls_get_real_speed (hud->controls, 100));
  pango_layout_set_text (hud->speed_layout, speed_text, -1);
  pango_cairo_show_layout (cr, hud->speed_layout);

  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 1);
  cairo_move_to (cr, 64, 85);

  g_autofree char *shield_text = g_strdup_printf ("%d", ship_controls_get_shield (hud->controls, 100));
  pango_layout_set_text (hud->shield_layout, shield_text, -1);
  pango_cairo_show_layout (cr, hud->shield_layout);

  cairo_destroy (cr);

  cairo_texture_end_paint (hud->data_texture);
}

static void
hud_update_lap_surface (HUD *hud)
{
  cairo_t *cr = cairo_texture_begin_paint (hud->lap_texture);

  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_move_to (cr, 256, 0);

  pango_layout_set_text (hud->lap_layout, hud->lap_text, -1);
  pango_cairo_show_layout (cr, hud->lap_layout);

  cairo_destroy (cr);

  cairo_texture_end_paint (hud->lap_texture);
}

static void
hud_update_time_surface (HUD *hud)
{
  cairo_t *cr = cairo_texture_begin_paint (hud->time_texture);

  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_move_to (cr, 256, 0);

  pango_layout_set_text (hud->time_layout, hud->time_text, -1);
  pango_cairo_show_layout (cr, hud->time_layout);

  cairo_destroy (cr);

  cairo_texture_end_paint (hud->time_texture);
}

static void
hud_update_clipping_planes (HUD *hud)
{
  graphene_vec3_t plane_normal;
  graphene_plane_t plane;
  graphene_point3d_t p;

  int width = hud->screen_width / 2;
  int height = hud->screen_height / 2;

  float speed_ratio = ship_controls_get_real_speed_ratio (hud->controls);
  float speed_factor = speed_ratio * (0.75 - 0.07) + 0.07;

  graphene_vec3_init (&plane_normal, 1, 1, 0);
  graphene_vec3_normalize (&plane_normal, &plane_normal);
  graphene_plane_init_from_point (&plane, &plane_normal,
                                  graphene_point3d_init (&p,
                                                         -width * speed_factor ,
                                                         -height + width * hud->aspect / 2,
                                                         0));
  g_array_index (hud->speed_clipping_planes, graphene_plane_t, 0) = plane;

  graphene_vec3_init (&plane_normal, -1, 1, 0);
  graphene_vec3_normalize (&plane_normal, &plane_normal);
  graphene_plane_init_from_point (&plane, &plane_normal,
                                  graphene_point3d_init (&p,
                                                         width * speed_factor ,
                                                         -height + width * hud->aspect / 2,
                                                         0));
  g_array_index (hud->speed_clipping_planes, graphene_plane_t, 1) = plane;

  float shield_ratio = ship_controls_get_shield_ratio (hud->controls);
  float shield_factor = shield_ratio * (1.54 - 0.47) + 0.47;

  graphene_vec3_init (&plane_normal, 0, -1, 0);
  graphene_plane_init_from_point (&plane, &plane_normal,
                                  graphene_point3d_init (&p,
                                                         0,
                                                         -height + width * hud->aspect * shield_factor,
                                                         0));
  g_array_index (hud->shield_clipping_planes, graphene_plane_t, 0) = plane;
}

void
hud_update (HUD *hud,
            float dt)
{
  hud_update_data_surface (hud);
  hud_update_clipping_planes (hud);
  hud_update_lap_surface (hud);
  hud_update_time_surface (hud);
}

void
hud_update_screen_size (HUD *hud,
                        int width,
                        int height)
{
  hud->screen_width = width;
  hud->screen_height = height;

  gthree_orthographic_camera_set_left (hud->camera, -width / 2);
  gthree_orthographic_camera_set_right (hud->camera, width / 2);
  gthree_orthographic_camera_set_top (hud->camera, height / 2);
  gthree_orthographic_camera_set_bottom (hud->camera, -height / 2);

  hud_update_sprites (hud, width, height);
}

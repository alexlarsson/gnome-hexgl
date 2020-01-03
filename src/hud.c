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
  gthree_texture_set_needs_update (texture);
}

typedef struct {
  GthreeSprite *sprite;
  GthreeTexture *texture;
  PangoLayout *layout;
  gboolean dirty;
  float x;
  float y;
  float scale;
  int dx, dy;
  int width;
  int height;
  float x_alignment;
  float y_alignment;
  GdkRGBA color;
} TextSprite;

static TextSprite *
text_sprite_new (PangoContext *pango_context,
                 const char *font,
                 const char *initial_text, // Used for measuring texture size
                 float x_alignment,
                 float y_alignment)
{
  PangoFontDescription *fd;
  TextSprite *text;
  int layout_width, layout_height;

  text = g_new0 (TextSprite, 1);

  text->dirty = TRUE; // Need initial paint
  text->color.red = 1;
  text->color.green = 1;
  text->color.blue = 1;
  text->color.alpha = 1;
  text->scale = 1.0;

  text->x_alignment = x_alignment;
  text->y_alignment = y_alignment;

  text->layout = pango_layout_new (pango_context);

  fd = pango_font_description_from_string (font);
  pango_layout_set_font_description (text->layout, fd);
  pango_font_description_free (fd);

  pango_layout_set_text (text->layout, initial_text, -1);

  pango_layout_get_pixel_size (text->layout, &layout_width, &layout_height);

  // Round up to power of 2
  text->width = 1 << g_bit_storage (layout_width);
  text->height = 1 << g_bit_storage (layout_height);

  // Create texture and sprite
  text->texture = cairo_texture_new (text->width, text->height);

  text->sprite = gthree_sprite_new (NULL);
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (text->sprite)), text->texture);
  gthree_material_set_depth_test (gthree_sprite_get_material (text->sprite), FALSE);

  return text;
}

static void
text_sprite_set_text (TextSprite *text,
                      const char *str)
{
  pango_layout_set_text (text->layout, str, -1);
  text->dirty = TRUE;
}


static void
text_sprite_set_pos (TextSprite *text,
                     float x,
                     float y)
{
  text->x = x;
  text->y = y;
}

static void
text_sprite_set_scale (TextSprite *text,
                       float scale)
{
  text->scale = scale;
}

static void
text_sprite_set_pos_offset (TextSprite *text,
                            int dx,
                            int dy)
{
  text->dx = dx;
  text->dy = dy;
}

static void
text_sprite_update (TextSprite *text,
                    int screen_width,
                    int screen_height)
{
  graphene_vec3_t pos;
  graphene_vec2_t v2;
  graphene_vec3_t v3;

  if (text->dirty)
    {
      cairo_t *cr = cairo_texture_begin_paint (text->texture);

      cairo_set_source_rgba (cr, text->color.red, text->color.green, text->color.blue, text->color.alpha);

      PangoRectangle logical_rect;
      pango_layout_get_extents (text->layout, NULL, &logical_rect);

      cairo_move_to (cr,
                     round (text->x_alignment * (text->width - pango_units_to_double (logical_rect.width)) +
                            pango_units_to_double (logical_rect.x)),
                     round (text->y_alignment * (text->height - pango_units_to_double (logical_rect.height)) +
                            pango_units_to_double (logical_rect.y)));

      pango_cairo_show_layout (cr, text->layout);

      cairo_destroy (cr);

      cairo_texture_end_paint (text->texture);

      text->dirty = FALSE;
    }

  gthree_sprite_set_center (text->sprite,
                            graphene_vec2_init (&v2,
                                                text->x_alignment,
                                                (1 - text->y_alignment)));
  gthree_object_set_scale (GTHREE_OBJECT (text->sprite),
                           graphene_vec3_init (&v3,
                                               text->scale * text->width,
                                               text->scale * text->height,
                                               1.0));
  gthree_object_set_position (GTHREE_OBJECT (text->sprite),
                              graphene_vec3_init (&pos,
                                                  round (text->x * screen_width) + text->dx,
                                                  round (text->y * screen_height) + text->dy,
                                                  1));

}

static void
text_sprite_free (TextSprite *text)
{
  g_object_unref (text->layout);
  g_object_unref (text->texture);

  gthree_object_destroy (GTHREE_OBJECT (text->sprite));
  g_object_unref (text->sprite);

  g_free (text);
}


struct _HUDMessage {
  TextSprite *text;

  float animation_time;
  float animation_duration;

  float start_scale;
  float end_scale;
  graphene_vec2_t start_pos;
  graphene_vec2_t end_pos;
  graphene_vec3_t start_color;
  graphene_vec3_t end_color;

  gboolean removed;
};

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

  TextSprite *lap_text;
  TextSprite *time_text;

  float aspect;

  GthreeOrthographicCamera *camera;
  GArray *speed_clipping_planes;
  GArray *shield_clipping_planes;

  GthreeScene *scene;

  GthreePass *pass;
  GthreePass *pass2;
  GthreePass *pass3;

  GList *messages;
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

  hud->scene = scene = gthree_scene_new ();

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

  hud->lap_text =
    text_sprite_new (hud->pango_context,
                     "Sans bold 60",
                     "X/X",
                     1.0,
                     0.0);
  text_sprite_set_pos (hud->lap_text, 0.5, 0.5);
  text_sprite_set_pos_offset (hud->lap_text, -10, 0);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->lap_text->sprite));

  hud->time_text =
    text_sprite_new (hud->pango_context,
                     "Sans bold 60",
                     "99'99''99",
                     0.5,
                     0.0);
  text_sprite_set_pos (hud->time_text, 0.0, 0.5);
  text_sprite_set_pos_offset (hud->time_text, -10, 0);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (hud->time_text->sprite));

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


HUDMessage *
hud_show_message (HUD *hud,
                  const char *text)
{
  HUDMessage *message = g_new0 (HUDMessage, 1);

  message->text =
    text_sprite_new (hud->pango_context,
                     "Sans bold 100",
                     text,
                     0.5,
                     0.5);
  gthree_object_add_child (GTHREE_OBJECT (hud->scene), GTHREE_OBJECT (message->text->sprite));

  message->start_scale = 4;
  message->end_scale = 1;

  message->animation_time = 0;
  message->animation_duration = 16 * 1;

  graphene_vec2_init (&message->start_pos,
                      0.0, 0.3);
  graphene_vec2_init (&message->end_pos,
                      0.0, 0.0);

  graphene_vec3_init (&message->start_color,
                      0.2, 0.2, 0.2);
  graphene_vec3_init (&message->end_color,
                      1.0, 1.0, 1.0);

  hud->messages = g_list_append (hud->messages, message);

  return message;
}

void
hud_remove_message (HUD *hud,
                    HUDMessage *message)
{
  g_assert (g_list_find (hud->messages, message) != NULL);
  message->removed = TRUE;

  message->animation_time = 0;

  message->start_scale = 1;
  message->end_scale = 1;

  graphene_vec2_init (&message->start_pos,
                      0.0, 0.0);
  graphene_vec2_init (&message->end_pos,
                      0.0, -0.1);

  graphene_vec3_init (&message->start_color,
                      1.0, 1.0, 1.0);
  graphene_vec3_init (&message->end_color,
                      0.2, 0.2, 0.2);
}

void
hud_update_message (HUD *hud,
                    HUDMessage *message,
                    float dt)
{
  float animation_factor;
  graphene_vec2_t pos;
  graphene_vec3_t color;

  message->animation_time += dt;
  if (message->animation_time < message->animation_duration)
    animation_factor = message->animation_time / message->animation_duration;
  else
    {
      animation_factor = 1;

      if (message->removed)
        {
          hud->messages = g_list_remove (hud->messages, message);
          text_sprite_free (message->text);
          g_free (message);
          return;
        }
    }

  text_sprite_set_scale (message->text,
                         message->start_scale + (message->end_scale - message->start_scale) * animation_factor);

  graphene_vec2_interpolate (&message->start_pos,
                             &message->end_pos,
                             animation_factor,
                             &pos);

  text_sprite_set_pos (message->text,
                       graphene_vec2_get_x (&pos),
                       graphene_vec2_get_y (&pos));

  graphene_vec3_interpolate (&message->start_color,
                             &message->end_color,
                             animation_factor,
                             &color);

  gthree_sprite_material_set_color (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (message->text->sprite)), &color);

  text_sprite_update (message->text, hud->screen_width, hud->screen_height);
}

void
hud_update (HUD *hud,
            float dt)
{
  GList *l, *next;

  hud_update_data_surface (hud);
  hud_update_clipping_planes (hud);

  text_sprite_update (hud->lap_text, hud->screen_width, hud->screen_height);
  text_sprite_update (hud->time_text, hud->screen_width, hud->screen_height);

  for (l = hud->messages; l != NULL; l = next)
    {
      HUDMessage *message = l->data;
      next = l->next;

      hud_update_message (hud, message, dt);
    }
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

void
hud_set_lap (HUD *hud,
             int lap, int max_laps)
{
  g_autofree char *s = g_strdup_printf ("%d/%d", lap, max_laps);

  text_sprite_set_text (hud->lap_text, s);
}

void
hud_set_time (HUD *hud,
              gdouble time)
{
  if (time < 0)
    text_sprite_set_text (hud->time_text, "");
  else
    {
      double minutes = floor (time / 60);
      time -= minutes * 60;
      double seconds = floor (time);
      time -= seconds;
      double rest = floor (time * 100);

      g_autofree char *s = g_strdup_printf ("%.0f'%02.0f''%02.0f",
                                            minutes, seconds, rest);
      text_sprite_set_text (hud->time_text, s);
    }
}

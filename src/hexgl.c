#include <stdlib.h>
#include <gtk/gtk.h>

#include <epoxy/gl.h>
#include <gthree/gthree.h>
#include "camerachase.h"
#include "shipcontrols.h"
#include "shipeffects.h"
#include "analysismap.h"
#include "utils.h"
#include "shaders.h"

GthreeEffectComposer *composer;
GthreeObject *the_ship;
CameraChase *camera_chase;
ShipControls *ship_controls;
ShipEffects *ship_effects;
AnalysisMap *height_map;
AnalysisMap *collision_map;
GthreeSprite* hud_bg_sprite;
GthreeSprite* hud_fg_speed_sprite;
GthreeSprite* hud_fg_shield_sprite;
GthreeUniforms *hex_uniforms;

float hud_aspect;

GthreeOrthographicCamera *hud_camera;
GArray *speed_clipping_planes;
GArray *shield_clipping_planes;

static void
update_hud_sprites (int width,
                    int height)
{
  graphene_vec3_t pos, s;

  gthree_object_set_position (GTHREE_OBJECT (hud_bg_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud_bg_sprite),
                           graphene_vec3_init (&s,
                                               width, width * hud_aspect, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud_fg_shield_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud_fg_shield_sprite),
                           graphene_vec3_init (&s,
                                               width, width * hud_aspect, 1.0));

  gthree_object_set_position (GTHREE_OBJECT (hud_fg_speed_sprite),
                              graphene_vec3_init (&pos, 0, - height / 2, 1));
  gthree_object_set_scale (GTHREE_OBJECT (hud_fg_speed_sprite),
                           graphene_vec3_init (&s,
                                               width, width * hud_aspect, 1.0));
}

static void
resize_area (GthreeArea *area,
             gint width,
             gint height,
             GthreePerspectiveCamera *camera)
{
  gthree_perspective_camera_set_aspect (camera, (float)width / (float)(height));

  gthree_orthographic_camera_set_left (hud_camera, -width / 2);
  gthree_orthographic_camera_set_right (hud_camera, width / 2);
  gthree_orthographic_camera_set_top (hud_camera, height / 2);
  gthree_orthographic_camera_set_bottom (hud_camera, -height / 2);

  update_hud_sprites (width, height);

  gthree_uniforms_set_float (hex_uniforms, "size", 512.0 * (width ) / (1633.0 * gtk_widget_get_scale_factor (GTK_WIDGET (area))));
  gthree_uniforms_set_float (hex_uniforms, "rx", width / gtk_widget_get_scale_factor (GTK_WIDGET (area)));
  gthree_uniforms_set_float (hex_uniforms, "ry", height / gtk_widget_get_scale_factor (GTK_WIDGET (area)));
}

static void
init_scene (GthreeScene *scene)
{
  g_autoptr(GthreeObject) track = NULL;
  g_autoptr(GthreeObject) ship = NULL;
  g_autoptr(GthreeTexture) skybox = NULL;
  GthreeAmbientLight *ambient_light;
  GthreeDirectionalLight *sun;
  GdkRGBA white =  {1, 1, 1, 1};
  GdkRGBA grey =  {0.75, 0.75, 0.75, 1};
  graphene_point3d_t pos;

  track = load_model ("tracks/cityscape/cityscape.glb");
  ship = load_model ("ships/feisar/feisar.glb");

  skybox = load_skybox ("dawnclouds");
  gthree_scene_set_background_texture (scene, skybox);

  gthree_object_add_child (GTHREE_OBJECT (scene), track);

  gthree_object_add_child (GTHREE_OBJECT (scene), ship);
  gthree_object_set_matrix_auto_update (GTHREE_OBJECT (ship), TRUE);
  the_ship = ship;

  gthree_object_set_position_point3d (GTHREE_OBJECT (ship),
                              graphene_point3d_init (&pos,
                                                     -1134*2,
                                                     387,
                                                     -443*2));

  ambient_light = gthree_ambient_light_new (&grey);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (ambient_light));

  sun = gthree_directional_light_new (&white, 1.5);
  gthree_object_set_position_point3d (GTHREE_OBJECT (sun),
                              graphene_point3d_init (&pos,
                                                     -4000, 1200, 1800));
  gthree_object_look_at (GTHREE_OBJECT (sun),
                         graphene_point3d_init (&pos, 0, 0, 0));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (sun));
}

static gboolean
tick (GtkWidget     *widget,
      GdkFrameClock *frame_clock,
      gpointer       user_data)
{
  static gint64 last_frame_time_i = 0;
  float delta_time_sec = 0;
  gint64 frame_time_i;
  float dt;

  frame_time_i = gdk_frame_clock_get_frame_time (frame_clock);
  if (last_frame_time_i != 0)
    delta_time_sec = (frame_time_i - last_frame_time_i) / (float) G_USEC_PER_SEC;
  last_frame_time_i = frame_time_i;

  dt = delta_time_sec * 1000.0 / 16.6;

  ship_controls_update (ship_controls, dt);
  ship_effects_update (ship_effects, dt);

  camera_chase_update (camera_chase, dt, ship_controls_get_speed_ratio (ship_controls));

  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static gboolean
enable_layer_cb (GthreeObject                *object,
                 gpointer                     user_data)
{
  guint layer = GPOINTER_TO_UINT (user_data);
  gthree_object_enable_layer (object, layer);
  return TRUE;
}

static void
add_name_to_layer (GthreeObject *top,
                   const char *name,
                   guint layer)
{
  g_autoptr(GList) objects;
  GList *l;

  objects = gthree_object_find_by_name (top, name);
  for (l = objects; l != NULL; l = l->next)
    {
      GthreeObject *obj = l->data;
      gthree_object_traverse (GTHREE_OBJECT (obj), enable_layer_cb, GUINT_TO_POINTER(layer));
    }
}

static gboolean
disable_vertex_colors_cb (GthreeObject                *object,
                          gpointer                     user_data)
{
  if (GTHREE_IS_MESH (object))
    {
      GthreeMaterial *material = gthree_mesh_get_material (GTHREE_MESH (object), 0);
      gthree_material_set_vertex_colors (material, FALSE);
    }
  return TRUE;
}

static void
disable_vertex_colors (GthreeObject *top,
                       const char *name)
{
  g_autoptr(GList) objects;
  GList *l;

  objects = gthree_object_find_by_name (top, name);
  for (l = objects; l != NULL; l = l->next)
    {
      GthreeObject *obj = l->data;
      gthree_object_traverse (GTHREE_OBJECT (obj), disable_vertex_colors_cb, NULL);
    }
}


static gboolean
render_area (GtkGLArea    *gl_area,
             GdkGLContext *context)
{
  GthreeRenderer *renderer = gthree_area_get_renderer (GTHREE_AREA(gl_area));
  GthreeScene *scene = gthree_area_get_scene (GTHREE_AREA(gl_area));

  if (height_map == NULL)
    {
    graphene_box_t bounding_box;
    graphene_point3d_t min, max;
    graphene_point3d_t pos;
    graphene_euler_t e;
    GthreeOrthographicCamera *o_camera;
    g_autoptr(GthreeRenderTarget) render_target = NULL;
    g_autoptr(GthreeMeshDepthMaterial) depth_material = NULL;
    g_autoptr(GthreeMeshBasicMaterial) track_material = NULL;
    cairo_surface_t *surface;
    GdkRGBA white   = {1, 1, 1, 1};
    GdkRGBA black   = {0, 0, 0, 1};
    GdkRGBA red   = {1, 0, 0, 1};

    gthree_object_update_matrix_world (GTHREE_OBJECT (scene), FALSE);
    gthree_object_get_mesh_extents (GTHREE_OBJECT (scene), &bounding_box);

    graphene_box_get_min (&bounding_box, &min);
    graphene_box_get_max (&bounding_box, &max);

    o_camera = gthree_orthographic_camera_new (min.x, max.x,
                                               -min.z, -max.z,
                                               0, max.y - min.y);
    gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (o_camera));

    gthree_object_set_position_point3d (GTHREE_OBJECT (o_camera),
                                graphene_point3d_init (&pos, 0, max.y, 0));
    gthree_object_set_rotation (GTHREE_OBJECT (o_camera),
                                graphene_euler_init (&e, -90, 0, 0));

    depth_material = gthree_mesh_depth_material_new ();
    gthree_mesh_depth_material_set_depth_packing_format (depth_material,
                                                         GTHREE_DEPTH_PACKING_FORMAT_RGBA);
    gthree_material_set_blend_mode (GTHREE_MATERIAL (depth_material),
                                    GTHREE_BLEND_NO,
                                    GL_FUNC_ADD,
                                    GL_SRC_ALPHA,
                                    GL_ONE_MINUS_SRC_ALPHA);

    gthree_renderer_set_clear_color (renderer, &white);

    render_target = gthree_render_target_new (2048, 2048);
    gthree_render_target_set_stencil_buffer (render_target, FALSE);

    gthree_scene_set_override_material (scene, GTHREE_MATERIAL (depth_material));
    gthree_renderer_set_render_target (renderer, render_target, 0, 0);

    add_name_to_layer (GTHREE_OBJECT (scene), "tracks", 2);
    // Don't use vertex colors for the tracks object, as its used for collision info
    disable_vertex_colors (GTHREE_OBJECT (scene), "tracks");
    add_name_to_layer (GTHREE_OBJECT (scene), "bonus-base", 3);

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 2);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          gthree_render_target_get_width (render_target),
                                          gthree_render_target_get_height (render_target));
    gthree_render_target_download (render_target,
                                   cairo_image_surface_get_data (surface),
                                   cairo_image_surface_get_stride (surface));

    //cairo_surface_write_to_png (surface, "analysis-height.png");

    height_map = analysis_map_new (surface, &bounding_box);
    ship_controls_set_height_map (ship_controls, height_map);
    cairo_surface_destroy (surface);

    track_material = gthree_mesh_basic_material_new ();
    // We use white vertex color to mark the ridable part of the track
    gthree_material_set_vertex_colors (GTHREE_MATERIAL (track_material), TRUE);

    gthree_scene_set_override_material (scene, GTHREE_MATERIAL (track_material));

    gthree_renderer_set_clear_color (renderer, &black);

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 2);
    gthree_mesh_basic_material_set_color (track_material, &white);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    gthree_renderer_set_autoclear (renderer, FALSE);

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 3);
    gthree_material_set_vertex_colors (GTHREE_MATERIAL (track_material), FALSE);
    gthree_mesh_basic_material_set_color (track_material, &red);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          gthree_render_target_get_width (render_target),
                                          gthree_render_target_get_height (render_target));
    gthree_render_target_download (render_target,
                                   cairo_image_surface_get_data (surface),
                                   cairo_image_surface_get_stride (surface));

    //cairo_surface_write_to_png (surface, "analysis-collision.png");

    collision_map = analysis_map_new (surface, &bounding_box);
    ship_controls_set_collision_map (ship_controls, collision_map);
    cairo_surface_destroy (surface);

    gthree_renderer_set_autoclear (renderer, TRUE);
    gthree_renderer_set_render_target (renderer, NULL, 0, 0);
    gthree_scene_set_override_material (scene, NULL);
  }


  {
    graphene_vec3_t plane_normal;
    graphene_plane_t plane;
    int height = gtk_widget_get_allocated_height (GTK_WIDGET (gl_area));
    int width = gtk_widget_get_allocated_width (GTK_WIDGET (gl_area));
    graphene_point3d_t p;

    float speed_ratio = ship_controls_get_real_speed_ratio (ship_controls);
    float speed_factor = speed_ratio * (0.75 - 0.07) + 0.07;

    graphene_vec3_init (&plane_normal, 1, 1, 0);
    graphene_vec3_normalize (&plane_normal, &plane_normal);
    graphene_plane_init_from_point (&plane, &plane_normal,
                                    graphene_point3d_init (&p,
                                                           -width * speed_factor ,
                                                           -height + width * hud_aspect / 2,
                                                           0));
    g_array_index (speed_clipping_planes, graphene_plane_t, 0) = plane;

    graphene_vec3_init (&plane_normal, -1, 1, 0);
    graphene_vec3_normalize (&plane_normal, &plane_normal);
    graphene_plane_init_from_point (&plane, &plane_normal,
                                    graphene_point3d_init (&p,
                                                           width * speed_factor ,
                                                           -height + width * hud_aspect / 2,
                                                           0));
    g_array_index (speed_clipping_planes, graphene_plane_t, 1) = plane;

    float shield_ratio = ship_controls_get_shield_ratio (ship_controls);
    float shield_factor = shield_ratio * (1.54 - 0.47) + 0.47;

    graphene_vec3_init (&plane_normal, 0, -1, 0);
    graphene_plane_init_from_point (&plane, &plane_normal,
                                    graphene_point3d_init (&p,
                                                           0,
                                                           -height + width * hud_aspect * shield_factor,
                                                           0));
    g_array_index (shield_clipping_planes, graphene_plane_t, 0) = plane;

  }

  gthree_effect_composer_render (composer, gthree_area_get_renderer (GTHREE_AREA(gl_area)),
                                 0.1);
  return TRUE;
}

static gboolean
key_press (GtkWidget	     *widget,
           GdkEventKey	     *event)
{
  return ship_controls_key_press (ship_controls, event);
}

static gboolean
key_release (GtkWidget	     *widget,
             GdkEventKey	     *event)
{
  return ship_controls_key_release (ship_controls, event);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *hbox, *button, *area;
  GthreeScene *scene;
  GthreePerspectiveCamera *camera;
  GthreePass *clear_pass, *render_pass, *bloom_pass, *hex_pass;
  GthreePass *hud_pass, *hud_pass2, *hud_pass3;
  GthreeScene *hud_scene, *hud_scene2, *hud_scene3;
  GthreeShader *hex_shader;
  graphene_vec3_t pos;
  GdkRGBA black = {0, 0, 0, 1.0};
  graphene_vec2_t v2;
  graphene_vec3_t plane_normal;
  graphene_plane_t plane;
  int hud_width;
  int hud_height;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "HexGL");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_box_set_spacing (GTK_BOX (box), 6);
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  gtk_box_set_spacing (GTK_BOX (hbox), 6);
  gtk_container_add (GTK_CONTAINER (box), hbox);
  gtk_widget_show (hbox);

  composer = gthree_effect_composer_new  ();

  clear_pass = gthree_clear_pass_new (&black);

  scene = gthree_scene_new ();
  camera = gthree_perspective_camera_new (70, 1, 1, 6000);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (camera));

  render_pass = gthree_render_pass_new (scene, GTHREE_CAMERA (camera), NULL);
  gthree_pass_set_clear (render_pass, FALSE);

  bloom_pass = gthree_bloom_pass_new  (0.5, 4, 256);

  g_autoptr(GthreeTexture) hex_texture = load_texture ("hex.jpg");

  hex_shader = hexvignette_shader_clone ();
  hex_uniforms = gthree_shader_get_uniforms (hex_shader);
  gthree_uniforms_set_texture (hex_uniforms, "tHex", hex_texture);
  hex_pass = gthree_shader_pass_new (hex_shader, NULL);

  hud_scene = gthree_scene_new ();

  g_autoptr(GthreeTexture) hud_texture = load_texture ("hud-bg.png");
  g_autoptr(GthreeTexture) hud_speed_texture = load_texture ("hud-fg-speed.png");
  g_autoptr(GthreeTexture) hud_shield_texture = load_texture ("hud-fg-shield.png");

  hud_width = gdk_pixbuf_get_width (gthree_texture_get_pixbuf (hud_texture));
  hud_height = gdk_pixbuf_get_height (gthree_texture_get_pixbuf (hud_texture));
  hud_aspect = (float) hud_height / hud_width;

  // Just some values to start with, we'll do the right ones in the callback later
  int width = 100, height = 100;

  hud_bg_sprite = gthree_sprite_new (NULL);
  gthree_sprite_set_center (hud_bg_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (hud_bg_sprite)), hud_texture);
  gthree_material_set_depth_test (gthree_sprite_get_material (hud_bg_sprite), FALSE);
  gthree_object_add_child (GTHREE_OBJECT (hud_scene), GTHREE_OBJECT (hud_bg_sprite));

  hud_camera = gthree_orthographic_camera_new ( -width / 2, width / 2, height / 2, -height / 2, 1, 10);
  gthree_object_add_child (GTHREE_OBJECT (hud_scene), GTHREE_OBJECT (hud_camera));
  gthree_object_set_position (GTHREE_OBJECT (hud_camera),
                              graphene_vec3_init (&pos, 0, 0, 10));

  hud_scene2 = gthree_scene_new ();

  hud_fg_shield_sprite = gthree_sprite_new (NULL);
  gthree_sprite_set_center (hud_fg_shield_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (hud_fg_shield_sprite)), hud_shield_texture);
  gthree_material_set_depth_test (gthree_sprite_get_material (hud_fg_shield_sprite), FALSE);
  gthree_object_add_child (GTHREE_OBJECT (hud_scene2), GTHREE_OBJECT (hud_fg_shield_sprite));

  hud_scene3 = gthree_scene_new ();

  graphene_vec3_init (&plane_normal, 0, 0, 0);
  graphene_plane_init (&plane, &plane_normal, 0);

  speed_clipping_planes = g_array_new (FALSE, FALSE, sizeof (graphene_plane_t));
  g_array_append_val (speed_clipping_planes, plane);
  g_array_append_val (speed_clipping_planes, plane);

  shield_clipping_planes = g_array_new (FALSE, FALSE, sizeof (graphene_plane_t));
  g_array_append_val (shield_clipping_planes, plane);

  hud_fg_speed_sprite = gthree_sprite_new (NULL);
  gthree_sprite_set_center (hud_fg_speed_sprite,
                            graphene_vec2_init (&v2, 0.5, 0.0));
  gthree_sprite_material_set_map (GTHREE_SPRITE_MATERIAL (gthree_sprite_get_material (hud_fg_speed_sprite)), hud_speed_texture);
  gthree_material_set_depth_test (gthree_sprite_get_material (hud_fg_speed_sprite), FALSE);
  gthree_object_add_child (GTHREE_OBJECT (hud_scene3), GTHREE_OBJECT (hud_fg_speed_sprite));

  update_hud_sprites (width, height);

  hud_pass = gthree_render_pass_new (hud_scene, GTHREE_CAMERA (hud_camera), NULL);
  gthree_pass_set_clear (hud_pass, FALSE);

  hud_pass2 = gthree_render_pass_new (hud_scene2, GTHREE_CAMERA (hud_camera), NULL);
  gthree_render_pass_set_clipping_planes (GTHREE_RENDER_PASS (hud_pass2), shield_clipping_planes);
  gthree_pass_set_clear (hud_pass2, FALSE);

  hud_pass3 = gthree_render_pass_new (hud_scene3, GTHREE_CAMERA (hud_camera), NULL);
  gthree_render_pass_set_clipping_planes (GTHREE_RENDER_PASS (hud_pass3), speed_clipping_planes);
  gthree_pass_set_clear (hud_pass3, FALSE);

  gthree_effect_composer_add_pass  (composer, clear_pass);
  gthree_effect_composer_add_pass  (composer, render_pass);
  gthree_effect_composer_add_pass  (composer, bloom_pass);
  gthree_effect_composer_add_pass  (composer, hex_pass);
  gthree_effect_composer_add_pass  (composer, hud_pass);
  gthree_effect_composer_add_pass  (composer, hud_pass2);
  gthree_effect_composer_add_pass  (composer, hud_pass3);

  area = gthree_area_new (scene, GTHREE_CAMERA (camera));
  g_signal_connect (area, "resize", G_CALLBACK (resize_area), camera);
  g_signal_connect (area, "render", G_CALLBACK (render_area), NULL);
  gtk_widget_grab_focus (area);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);
  gtk_container_add (GTK_CONTAINER (hbox), area);
  gtk_widget_show (area);

  gtk_widget_add_tick_callback (GTK_WIDGET (area), tick, area, NULL);
  g_signal_connect (window, "key-press-event", G_CALLBACK (key_press), NULL);
  g_signal_connect (window, "key-release-event", G_CALLBACK (key_release), NULL);

  button = gtk_button_new_with_label ("Quit");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_container_add (GTK_CONTAINER (box), button);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_widget_show (button);

  ship_controls = ship_controls_new ();

  init_scene (scene);
  camera_chase = camera_chase_new (GTHREE_CAMERA (camera), the_ship, 8, 10, 10);

  ship_controls_control (ship_controls, the_ship);
  ship_effects = ship_effects_new (scene, ship_controls);

  gtk_widget_show (window);

  gtk_main ();

  return EXIT_SUCCESS;
}

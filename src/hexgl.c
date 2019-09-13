#include <stdlib.h>
#include <gtk/gtk.h>

#include <epoxy/gl.h>
#include <gthree/gthree.h>
#include <gthree/gthreearea.h>
#include "camerachase.h"
#include "shipcontrols.h"
#include "shipeffects.h"
#include "analysismap.h"
#include "utils.h"
#include "hud.h"
#include "shaders.h"
#include "gameplay.h"
#include "sounds.h"

GthreeEffectComposer *composer;
GthreeObject *the_ship;
CameraChase *camera_chase;
ShipControls *ship_controls;
ShipEffects *ship_effects;
AnalysisMap *height_map;
AnalysisMap *collision_map;
GthreeUniforms *hex_uniforms;

GtkWidget *the_stack;
GtkWidget *start_button;
HUD *hud;
Gameplay *gameplay;

static gboolean
_enable_shadow_cb (GthreeObject *object,
                   gpointer      user_data)
{
  if (GTHREE_IS_MESH (object))
    {
      gthree_object_set_cast_shadow (GTHREE_OBJECT (object), TRUE);
      gthree_object_set_receive_shadow (GTHREE_OBJECT (object), TRUE);
    }

  return TRUE;
}

static void
enable_shadows (GthreeObject *root)
{
  gthree_object_traverse (root, _enable_shadow_cb, NULL);
}

static void
resize_area (GthreeArea *area,
             gint width,
             gint height,
             GthreePerspectiveCamera *camera)
{
  gthree_perspective_camera_set_aspect (camera, (float)width / (float)(height));

  hud_update_screen_size (hud, width, height);

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
  graphene_vec3_t white, grey;
  graphene_point3d_t pos;
  GthreeLightShadow *shadow;
  GthreeCamera *shadow_camera;

  graphene_vec3_init (&white, 1, 1, 1);
  graphene_vec3_init (&grey, 0.75, 0.75, 0.75);

  track = load_model ("tracks/cityscape/cityscape.glb");
  ship = load_model ("ships/feisar/feisar.glb");

  enable_shadows (track);
  enable_shadows (ship);

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
  gthree_light_set_intensity (GTHREE_LIGHT (ambient_light), 0.7);

  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (ambient_light));

  sun = gthree_directional_light_new (&white, 1.5);
  gthree_object_set_position_point3d (GTHREE_OBJECT (sun),
                              graphene_point3d_init (&pos,
                                                     -4000, 1200, 1800));
  gthree_object_look_at (GTHREE_OBJECT (sun),
                         graphene_point3d_init (&pos, 0, 0, 0));
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (sun));

  /* Sun shadow */
  gthree_object_set_cast_shadow (GTHREE_OBJECT (sun), TRUE);

  shadow = gthree_light_get_shadow (GTHREE_LIGHT (sun));
  shadow_camera = gthree_light_shadow_get_camera (shadow);
  gthree_orthographic_camera_set_left (GTHREE_ORTHOGRAPHIC_CAMERA (shadow_camera), -3000);
  gthree_orthographic_camera_set_right (GTHREE_ORTHOGRAPHIC_CAMERA (shadow_camera), 3000);
  gthree_orthographic_camera_set_top (GTHREE_ORTHOGRAPHIC_CAMERA (shadow_camera), 3000);
  gthree_orthographic_camera_set_bottom (GTHREE_ORTHOGRAPHIC_CAMERA (shadow_camera), -3000);
  gthree_camera_set_near (shadow_camera, 50);
  gthree_camera_set_far (shadow_camera, 6000 * 2);
  gthree_light_shadow_set_bias (shadow, -0.002);
  gthree_light_shadow_set_map_size (shadow, 2048, 2048);


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
  graphene_vec3_t col;

  frame_time_i = gdk_frame_clock_get_frame_time (frame_clock);
  if (last_frame_time_i != 0)
    delta_time_sec = (frame_time_i - last_frame_time_i) / (float) G_USEC_PER_SEC;
  last_frame_time_i = frame_time_i;

  dt = delta_time_sec * 1000.0 / 16.6;

  ship_controls_update (ship_controls, dt);
  ship_effects_update (ship_effects, dt);

  camera_chase_update (camera_chase, dt, ship_controls_get_speed_ratio (ship_controls));

  gameplay_update (gameplay, dt);

  hud_update (hud, dt);

  if (ship_controls_get_shield_ratio (ship_controls) < 0.2)
    gthree_uniforms_set_vec3 (hex_uniforms, "color", graphene_vec3_init (&col, 0.6, 0.1255, 0.1255));
  else
    gthree_uniforms_set_vec3 (hex_uniforms, "color", graphene_vec3_init (&col,
                                                                         0.27059,
                                                                         0.54118,
                                                                         0.694118));

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
  g_autoptr(GList) objects = NULL;
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
  g_autoptr(GList) objects = NULL;
  GList *l;

  objects = gthree_object_find_by_name (top, name);
  for (l = objects; l != NULL; l = l->next)
    {
      GthreeObject *obj = l->data;
      gthree_object_traverse (GTHREE_OBJECT (obj), disable_vertex_colors_cb, NULL);
    }
}

static void
realize_area (GtkWidget *widget)
{
  GthreeRenderer *renderer = gthree_area_get_renderer (GTHREE_AREA (widget));
  GthreeScene *scene = gthree_area_get_scene (GTHREE_AREA (widget));
  graphene_box_t bounding_box;
  graphene_point3d_t min, max;
  graphene_point3d_t pos;
  graphene_euler_t e;
  GthreeOrthographicCamera *o_camera;
  g_autoptr(GthreeRenderTarget) render_target = NULL;
  g_autoptr(GthreeMeshDepthMaterial) depth_material = NULL;
  g_autoptr(GthreeMeshBasicMaterial) track_material = NULL;
  cairo_surface_t *surface;
  graphene_vec3_t white, black, red;

  gthree_renderer_set_shadow_map_enabled (renderer, TRUE);
  gthree_renderer_set_shadow_map_auto_update (renderer, FALSE);
  gthree_renderer_set_shadow_map_needs_update (renderer, TRUE);

  graphene_vec3_init (&white, 1, 1, 1);
  graphene_vec3_init (&black, 0, 0, 0);
  graphene_vec3_init (&red, 1, 0, 0);

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

static gboolean
render_area (GtkGLArea    *gl_area,
             GdkGLContext *context)
{
  gthree_effect_composer_render (composer, gthree_area_get_renderer (GTHREE_AREA(gl_area)),
                                 0.1);
  return TRUE;
}

static gboolean
key_press (GtkWidget	     *widget,
           GdkEventKey	     *event)
{
  if (gameplay_key_press (gameplay, event))
    return TRUE;
  return ship_controls_key_press (ship_controls, event);
}

static gboolean
key_release (GtkWidget	     *widget,
             GdkEventKey	     *event)
{
  if (gameplay_key_release (gameplay, event))
    return TRUE;
  return ship_controls_key_release (ship_controls, event);
}

static void
start_clicked (GtkButton  *button)
{
  gtk_stack_set_visible_child_name (GTK_STACK (the_stack), "game");

  gameplay_start (gameplay);
}

static void
game_finished (void)
{
  gtk_stack_set_visible_child_name (GTK_STACK (the_stack), "menu");
  gtk_widget_grab_focus (start_button);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *label, *button, *area, *stack, *logo;
  GthreeScene *scene;
  GthreePerspectiveCamera *camera;
  GthreePass *clear_pass, *render_pass, *bloom_pass, *hex_pass;
  GthreeShader *hex_shader;
  graphene_vec3_t black;
  GdkPixbuf *title_pixbuf = load_pixbuf ("title.png");

  graphene_vec3_init (&black, 0, 0, 0);

  init_sounds ();

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "HexGL");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* Set up game */

  ship_controls = ship_controls_new ();
  hud = hud_new (ship_controls, window);

  scene = gthree_scene_new ();
  init_scene (scene);

  camera = gthree_perspective_camera_new (70, 1, 1, 6000);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (camera));

  camera_chase = camera_chase_new (GTHREE_CAMERA (camera), the_ship, 8, 10, 10);

  ship_controls_control (ship_controls, the_ship);

  ship_effects = ship_effects_new (scene, ship_controls);

  gameplay = gameplay_new (ship_controls, hud, camera_chase, game_finished);

  composer = gthree_effect_composer_new  ();

  clear_pass = gthree_clear_pass_new (&black);

  render_pass = gthree_render_pass_new (scene, GTHREE_CAMERA (camera), NULL);
  gthree_pass_set_clear (render_pass, FALSE);

  bloom_pass = gthree_bloom_pass_new  (0.5, 4, 256);

  g_autoptr(GthreeTexture) hex_texture = load_texture ("hex.jpg");

  hex_shader = hexvignette_shader_clone ();
  hex_uniforms = gthree_shader_get_uniforms (hex_shader);
  gthree_uniforms_set_texture (hex_uniforms, "tHex", hex_texture);
  hex_pass = gthree_shader_pass_new (hex_shader, NULL);

  gthree_effect_composer_add_pass  (composer, clear_pass);
  gthree_effect_composer_add_pass  (composer, render_pass);
  gthree_effect_composer_add_pass  (composer, bloom_pass);
  gthree_effect_composer_add_pass  (composer, hex_pass);
  hud_add_passes (hud, composer);

  /* Set up ui */

  the_stack = stack = gtk_stack_new ();
  gtk_container_add (GTK_CONTAINER (window), stack);
  gtk_widget_show (stack);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  gtk_box_set_spacing (GTK_BOX (box), 6);
  gtk_stack_add_named (GTK_STACK (stack), box, "menu");
  gtk_widget_show (box);

  logo = gtk_image_new_from_pixbuf (title_pixbuf);
  gtk_container_add (GTK_CONTAINER (box), logo);
  gtk_widget_show (logo);

  label = gtk_label_new ("Gnome port by Alexander Larsson");
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_widget_show (label);

  label = gtk_label_new ("Controls: Turn: left/right, Accelerate: up, Air Brakes: A/D");
  gtk_container_add (GTK_CONTAINER (box), label);
  gtk_widget_show (label);

  start_button = button = gtk_button_new_with_label ("Start");
  gtk_box_pack_start (GTK_BOX (box), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
  g_signal_connect (button, "clicked", G_CALLBACK (start_clicked), NULL);

  area = gthree_area_new (scene, GTHREE_CAMERA (camera));
  g_signal_connect (area, "resize", G_CALLBACK (resize_area), camera);
  g_signal_connect (area, "render", G_CALLBACK (render_area), NULL);
  g_signal_connect (area, "realize", G_CALLBACK (realize_area), NULL);
  gtk_widget_grab_focus (area);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);
  gtk_stack_add_named (GTK_STACK (stack), area, "game");
  gtk_widget_show (area);

  gtk_widget_add_tick_callback (GTK_WIDGET (area), tick, area, NULL);

  g_signal_connect (window, "key-press-event", G_CALLBACK (key_press), NULL);
  g_signal_connect (window, "key-release-event", G_CALLBACK (key_release), NULL);

  gtk_widget_show (window);

  gtk_main ();

  return EXIT_SUCCESS;
}

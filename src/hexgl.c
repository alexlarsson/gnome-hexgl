#include <stdlib.h>
#include <gtk/gtk.h>

#include <epoxy/gl.h>
#include <gthree/gthree.h>
#include "camerachase.h"


GthreeObject *the_ship;
CameraChase *camera_chase;
cairo_surface_t *depth_map;
cairo_surface_t *collision_map;

static void
resize_area (GthreeArea *area,
             gint width,
             gint height,
             GthreePerspectiveCamera *camera)
{
  gthree_perspective_camera_set_aspect (camera, (float)width / (float)(height));
}

static GdkPixbuf *
load_pixbuf (GFile *file)
{
  g_autoptr(GFileInputStream) in = NULL;
  in = g_file_read (file, NULL, NULL);
  return gdk_pixbuf_new_from_stream (G_INPUT_STREAM (in), NULL, NULL);
}

static void
load_cube_pixbufs (GFile *dir,
                   GdkPixbuf *pixbufs[6])
{
  char *files[] = {"px.jpg", "nx.jpg",
                   "py.jpg", "ny.jpg",
                   "pz.jpg", "nz.jpg"};
  int i;

  for (i = 0 ; i < 6; i++)
    {
      g_autoptr(GFile) file = g_file_get_child (dir, files[i]);
      pixbufs[i] = load_pixbuf (file);
    }
}

static GthreeTexture *
load_skybox (const char *name)
{
  GdkPixbuf *pixbufs[6];
  GthreeCubeTexture *cube_texture;
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;
  int i;

  base = g_file_new_for_path ("../textures/skybox");
  file = g_file_resolve_relative_path (base, name);

  load_cube_pixbufs (file, pixbufs);
  cube_texture = gthree_cube_texture_new_from_array (pixbufs);
  for (i = 0; i < 6; i++)
    g_object_unref (pixbufs[i]);

  return GTHREE_TEXTURE (cube_texture);
}

static GthreeObject *
load_model (const char *name)
{
  GError *error = NULL;
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(GFile) parent = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GthreeLoader) loader = NULL;

  base = g_file_new_for_path ("../models");
  file = g_file_resolve_relative_path (base, name);
  parent = g_file_get_parent (file);
  bytes = g_file_load_bytes (file, NULL, NULL, &error);
  if (bytes == NULL)
    g_error ("Failed to load %s: %s\n", name, error->message);

  loader = gthree_loader_parse_gltf (bytes, parent, &error);
  if (loader == NULL)
    g_error ("Failed to %s: %s\n", name, error->message);

  return GTHREE_OBJECT (g_object_ref (gthree_loader_get_scene (loader, 0)));
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
  ship = load_model ("ships/feisar/feisar.gltf");

  skybox = load_skybox ("dawnclouds");
  gthree_scene_set_background_texture (scene, skybox);

  gthree_object_add_child (GTHREE_OBJECT (scene), track);

  gthree_object_add_child (GTHREE_OBJECT (scene), ship);
  gthree_object_set_matrix_auto_update (GTHREE_OBJECT (ship), TRUE);
  the_ship = ship;

  gthree_object_set_position (GTHREE_OBJECT (ship),
                              graphene_point3d_init (&pos,
                                                     -1134*2,
                                                     387,
                                                     -443*2));

  ambient_light = gthree_ambient_light_new (&grey);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (ambient_light));

  sun = gthree_directional_light_new (&white, 1.5);
  gthree_object_set_position (GTHREE_OBJECT (sun),
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
  graphene_point3d_t pos;
  const graphene_vec3_t *current_pos = gthree_object_get_position (GTHREE_OBJECT (the_ship));

  frame_time_i = gdk_frame_clock_get_frame_time (frame_clock);
  if (last_frame_time_i != 0)
    delta_time_sec = (frame_time_i - last_frame_time_i) / (float) G_USEC_PER_SEC;
  last_frame_time_i = frame_time_i;

  gthree_object_set_position (GTHREE_OBJECT (the_ship),
                              graphene_point3d_init (&pos,
                                                     graphene_vec3_get_x (current_pos),
                                                     graphene_vec3_get_y (current_pos),
                                                     graphene_vec3_get_z (current_pos) + 1));

  camera_chase_update (camera_chase, delta_time_sec, 1.0 /* This should be ShipControls.getSpeedRatio */);

  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

static void
enable_layer_cb (GthreeObject                *object,
                 gpointer                     user_data)
{
  guint layer = GPOINTER_TO_UINT (user_data);
  gthree_object_enable_layer (object, layer);
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
render_area (GtkGLArea    *gl_area,
             GdkGLContext *context)
{
  GthreeRenderer *renderer = gthree_area_get_renderer (GTHREE_AREA(gl_area));
  GthreeScene *scene = gthree_area_get_scene (GTHREE_AREA(gl_area));

  if (depth_map == NULL)
    {
    graphene_box_t bounding_box;
    graphene_point3d_t min, max;
    graphene_point3d_t pos;
    graphene_euler_t e;
    GthreeOrthographicCamera *o_camera;
    g_autoptr(GthreeRenderTarget) render_target = NULL;
    g_autoptr(GthreeMeshDepthMaterial) depth_material = NULL;
    g_autoptr(GthreeMeshBasicMaterial) track_material = NULL;
    GdkRGBA white   = {1, 1, 1, 1};
    GdkRGBA red   = {1, 0, 0, 1};

    gthree_object_update_matrix_world (GTHREE_OBJECT (scene), FALSE);
    gthree_object_get_mesh_extents (GTHREE_OBJECT (scene), &bounding_box);

    graphene_box_get_min (&bounding_box, &min);
    graphene_box_get_max (&bounding_box, &max);

    o_camera = gthree_orthographic_camera_new (min.x, max.x,
                                               max.z, min.z,
                                               0, max.y - min.y);
    gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (o_camera));

    gthree_object_set_position (GTHREE_OBJECT (o_camera),
                                graphene_point3d_init (&pos, 0, min.y, 0));
    gthree_object_set_rotation (GTHREE_OBJECT (o_camera),
                                graphene_euler_init (&e, 90, 0, 0));

    depth_material = gthree_mesh_depth_material_new ();
    gthree_mesh_depth_material_set_depth_packing_format (depth_material,
                                                         GTHREE_DEPTH_PACKING_FORMAT_RGBA);

    render_target = gthree_render_target_new (2048, 2048);
    gthree_render_target_set_stencil_buffer (render_target, FALSE);

    gthree_scene_set_override_material (scene, GTHREE_MATERIAL (depth_material));
    gthree_renderer_set_render_target (renderer, render_target, 0, 0);

    add_name_to_layer (GTHREE_OBJECT (scene), "tracks", 2);
    add_name_to_layer (GTHREE_OBJECT (scene), "bonus-base", 3);

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 2);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    depth_map = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            gthree_render_target_get_width (render_target),
                                            gthree_render_target_get_height (render_target));
    gthree_render_target_download (render_target,
                                   cairo_image_surface_get_data (depth_map),
                                   cairo_image_surface_get_stride (depth_map));

    //cairo_surface_write_to_png (depth_map, "analysis-height.png");

    track_material = gthree_mesh_basic_material_new ();

    gthree_scene_set_override_material (scene, GTHREE_MATERIAL (track_material));

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 2);
    gthree_mesh_basic_material_set_color (track_material, &white);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    gthree_renderer_clear_depth (renderer); // TODO: Why is this needed, do we look the wrong way?
    gthree_renderer_set_autoclear (renderer, FALSE);

    gthree_object_set_layer (GTHREE_OBJECT (o_camera), 3);
    gthree_mesh_basic_material_set_color (track_material, &red);
    gthree_renderer_render (renderer, scene, GTHREE_CAMERA (o_camera));

    collision_map = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                gthree_render_target_get_width (render_target),
                                                gthree_render_target_get_height (render_target));
    gthree_render_target_download (render_target,
                                   cairo_image_surface_get_data (collision_map),
                                   cairo_image_surface_get_stride (collision_map));

    //cairo_surface_write_to_png (collision_map, "analysis-collision.png");

    gthree_renderer_set_autoclear (renderer, TRUE);
    gthree_renderer_set_render_target (renderer, NULL, 0, 0);
    gthree_scene_set_override_material (scene, NULL);
  }

  gthree_renderer_render (renderer, scene,
                          gthree_area_get_camera (GTHREE_AREA(gl_area)));
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *box, *hbox, *button, *area;
  GthreeScene *scene;
  GthreePerspectiveCamera *camera;

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

  scene = gthree_scene_new ();
  camera = gthree_perspective_camera_new (70, 1, 1, 6000);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (camera));

  area = gthree_area_new (scene, GTHREE_CAMERA (camera));
  g_signal_connect (area, "resize", G_CALLBACK (resize_area), camera);
  g_signal_connect (area, "render", G_CALLBACK (render_area), NULL);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);
  gtk_container_add (GTK_CONTAINER (hbox), area);
  gtk_widget_show (area);

  gtk_widget_add_tick_callback (GTK_WIDGET (area), tick, area, NULL);

  button = gtk_button_new_with_label ("Quit");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_container_add (GTK_CONTAINER (box), button);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);
  gtk_widget_show (button);

  init_scene (scene);
  camera_chase = camera_chase_new (GTHREE_CAMERA (camera), the_ship, 8, 10, 10);

  gtk_widget_show (window);

  gtk_main ();

  return EXIT_SUCCESS;
}

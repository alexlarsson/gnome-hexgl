#include <stdlib.h>
#include <gtk/gtk.h>

#include <epoxy/gl.h>
#include <gthree/gthree.h>
#include "camerachase.h"

GthreeObject *the_ship;
CameraChase *camera_chase;

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

  init_scene (scene);

  camera = gthree_perspective_camera_new (70, 1, 1, 6000);
  gthree_object_add_child (GTHREE_OBJECT (scene), GTHREE_OBJECT (camera));

  camera_chase = camera_chase_new (GTHREE_CAMERA (camera), the_ship, 8, 10, 10);

  area = gthree_area_new (scene, GTHREE_CAMERA (camera));
  g_signal_connect (area, "resize", G_CALLBACK (resize_area), camera);
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

  gtk_widget_show (window);

  gtk_main ();

  return EXIT_SUCCESS;
}

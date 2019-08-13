#include "utils.h"

char *
get_sound_path (const char *name)
{
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;

  base = g_file_new_for_path ("../sounds");
  file = g_file_resolve_relative_path (base, name);
  return g_file_get_path (file);
}

GdkPixbuf *
load_pixbuf (const char *name)
{
  const char *path;
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(GFileInputStream) in = NULL;

  path = g_getenv ("GNOME_HEXGL_DATADIR");
  if (path == NULL)
    path = DATADIR;
  base = g_file_new_for_path (path);
  base = g_file_get_child (base, "textures");
  file = g_file_resolve_relative_path (base, name);

  in = g_file_read (file, NULL, NULL);
  return gdk_pixbuf_new_from_stream (G_INPUT_STREAM (in), NULL, NULL);
}

static void
load_cube_pixbufs (char *dir,
                   GdkPixbuf *pixbufs[6])
{
  char *files[] = {"px.jpg", "nx.jpg",
                   "py.jpg", "ny.jpg",
                   "pz.jpg", "nz.jpg"};
  int i;

  for (i = 0 ; i < 6; i++)
    {
      g_autofree char *name = g_build_filename (dir, files[i], NULL);
      pixbufs[i] = load_pixbuf (name);
    }
}

GthreeTexture *
load_texture (const char *name)
{
  g_autoptr(GdkPixbuf) pixbuf = load_pixbuf (name);
  return gthree_texture_new (pixbuf);
}

GthreeTexture *
load_skybox (const char *name)
{
  GdkPixbuf *pixbufs[6];
  GthreeCubeTexture *cube_texture;
  g_autofree char *dir = g_build_filename ("skybox", name, NULL);
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;
  int i;

  load_cube_pixbufs (dir, pixbufs);
  cube_texture = gthree_cube_texture_new_from_array (pixbufs);
  for (i = 0; i < 6; i++)
    g_object_unref (pixbufs[i]);

  return GTHREE_TEXTURE (cube_texture);
}

GthreeObject *
load_model (const char *name)
{
  GError *error = NULL;
  const char *path;
  g_autoptr(GFile) base = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(GFile) parent = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GthreeLoader) loader = NULL;

  path = g_getenv ("GNOME_HEXGL_DATADIR");
  if (path == NULL)
    path = DATADIR;
  base = g_file_new_for_path (path);
  base = g_file_get_child (base, "models");
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

#include <gthree/gthree.h>
#include <gtk/gtk.h>

char *get_sound_path (const char *name);
GdkPixbuf *load_pixbuf (const char *name);
GthreeTexture *load_texture (const char *name);
GthreeTexture *load_skybox (const char *name);
GthreeObject *load_model (const char *name);

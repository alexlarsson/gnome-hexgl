#include <gsound.h>

#include "sounds.h"
#include "utils.h"

static GSoundContext *sound_context;

typedef struct {
  char *name;
  char *path;
  GCancellable *cancellable;
  gboolean loop;
  gboolean playing;
} Sound;

static GHashTable *sounds;

void
init_sounds (void)
{
  g_autoptr(GError) error = NULL;

  sound_context = gsound_context_new (NULL, &error);
  if (sound_context == NULL)
    {
      g_warning ("Failed to initialize sound: %s", error->message);
      return;
    }

  sounds = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
sound_done (GObject *source_object,
            GAsyncResult *res,
            gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  Sound *sound = user_data;

  sound->playing = FALSE;

  if (gsound_context_play_full_finish  (sound_context, res, NULL))
    {
      if (sound->loop)
        {
          sound->playing = TRUE;
          gsound_context_play_full (sound_context, sound->cancellable,
                                    sound_done, sound,
                                    GSOUND_ATTR_CANBERRA_CACHE_CONTROL, "permanent",
                                    GSOUND_ATTR_MEDIA_FILENAME, sound->path,
                                    NULL);
        }
    }
}

void
stop_sound (const char *name)
{
  Sound *sound = g_hash_table_lookup (sounds, name);

  if (sound && sound->playing)
    {
      g_cancellable_cancel (sound->cancellable);
    }
}

void
play_sound (const char *name,
            gboolean loop)
{
  Sound *sound = g_hash_table_lookup (sounds, name);

  if (sound == NULL)
    {
      g_autofree char *filename = g_strconcat (name, ".ogg", NULL);

      sound = g_new0 (Sound, 1);
      sound->name = g_strdup (name);
      sound->path = get_sound_path (filename);
      sound->cancellable = g_cancellable_new ();
      g_hash_table_insert (sounds, sound->name, sound);
    }

  sound->loop = loop;

  g_cancellable_reset (sound->cancellable);
  sound->playing = TRUE;
  gsound_context_play_full (sound_context, sound->cancellable,
                            sound_done, sound,
                            GSOUND_ATTR_CANBERRA_CACHE_CONTROL, "permanent",
                            GSOUND_ATTR_MEDIA_FILENAME, sound->path,
                            NULL);
}

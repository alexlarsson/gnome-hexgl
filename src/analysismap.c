#include "analysismap.h"

#define EPSILON 0.00000001

struct _AnalysisMap {
  grefcount refcount;
  cairo_surface_t *surface;
  graphene_point3d_t min;
  graphene_point3d_t max;

  graphene_box_t bounding_box;
};

AnalysisMap *
analysis_map_new (cairo_surface_t *surface,
                  const graphene_box_t *bounding_box)
{
  AnalysisMap *map = g_new0 (AnalysisMap, 1);

  g_ref_count_init (&map->refcount);

  map->surface = cairo_surface_reference (surface);
  map->bounding_box = *bounding_box;

  graphene_box_get_min (bounding_box, &map->min);
  graphene_box_get_max (bounding_box, &map->max);

  return map;
}

AnalysisMap *
analysis_map_ref (AnalysisMap *map)
{
  g_ref_count_inc (&map->refcount);
  return map;
}

void
analysis_map_unref (AnalysisMap *map)
{
  if (g_ref_count_dec (&map->refcount))
    {
      cairo_surface_destroy (map->surface);
      g_free (map);
    }
}

static gboolean
analysis_map_lookup_pixel (AnalysisMap *map,
                           int x, int y,
                           GdkRGBA *color)
{
  if (x < 0 || y < 0 ||
      x >= cairo_image_surface_get_width (map->surface) ||
      x >= cairo_image_surface_get_height (map->surface))
    {
      color->red = color->green = color->blue = 0.0;
      color->alpha = 0.0;
      return FALSE;
    }

  unsigned char *data = cairo_image_surface_get_data (map->surface);
  int stride = cairo_image_surface_get_stride (map->surface);
  guint32 pixel = *(guint32 *)(data + x * 4 + y * stride);

  int a = (pixel & 0xff000000) >> 24;
  int r = (pixel & 0x00ff0000) >> 16;
  int g = (pixel & 0x0000ff00) >> 8;
  int b = (pixel & 0x000000ff);

  color->red = r / 255.0;
  color->green = g / 255.0;
  color->blue = b / 255.0;
  color->alpha = a / 255.0;

  return TRUE;
}

static float
decode_depth (GdkRGBA *color)
{
  return
    color->alpha  * (255.0 / 256.0) +
    color->red * (255.0 / 256.0)  / (256.0 * 256.0 * 256.0) +
    color->green  * (255.0 / 256.0)  / (256.0 * 256.0) +
    color->blue  * (255.0 / 256.0)  / (256.0);
}

static float
lerp (float a, float b, float alpha)
{
  return a * alpha + b * (1.0-alpha);
}

/* Returns height (==y) at x,z */
float
analysis_map_lookup_depthmapped (AnalysisMap *map,
                                 float x,
                                 float z)
{
  float sum1, sum2, depth;
  GdkRGBA c1, c2;

  x = cairo_image_surface_get_width (map->surface) * (x - map->min.x) / (map->max.x - map->min.x);
  z = cairo_image_surface_get_height (map->surface) * (map->max.z - z) / (map->max.z - map->min.z);

  /* y flip */
  z = cairo_image_surface_get_height (map->surface) - z;

  /* Nearest */
  if (0)
    {
      x = floor (x + 0.5);
      z = floor (z + 0.5);
      analysis_map_lookup_pixel (map, x, z, &c1);
      depth = decode_depth (&c1);

      return map->max.y - depth * (map->max.y - map->min.y);
    }

  /* Bilinear */
  analysis_map_lookup_pixel (map, floor (x), floor (z), &c1);
  analysis_map_lookup_pixel (map, ceil (x), floor (z), &c2);
  sum1 = lerp (decode_depth (&c1), decode_depth (&c2), (ceil (x) - x));

  analysis_map_lookup_pixel (map, floor (x), ceil (z), &c1);
  analysis_map_lookup_pixel (map, ceil (x), ceil (z), &c2);
  sum2 = lerp (decode_depth (&c1),
               decode_depth (&c2),
               (ceil (x) - x));

  depth = lerp (sum1, sum2, (ceil (z) - z));
  return map->max.y - depth * (map->max.y - map->min.y);
}

void
analysis_map_lookup_rgba (AnalysisMap *map,
                          float x,
                          float y,
                          GdkRGBA *color)
{
}

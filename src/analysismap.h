#ifndef ANALYSISMAP_H
#define ANALYSISMAP_H

#include <gthree/gthree.h>

typedef struct _AnalysisMap AnalysisMap;

AnalysisMap *analysis_map_new                  (cairo_surface_t      *surface,
                                                const graphene_box_t *bounding_box);
AnalysisMap *analysis_map_ref                  (AnalysisMap          *map);
void         analysis_map_unref                (AnalysisMap          *map);
float        analysis_map_lookup_depthmapped   (AnalysisMap          *map,
                                                float                 x,
                                                float                 z);
void         analysis_map_lookup_rgba_nearest  (AnalysisMap          *map,
                                                float                 x,
                                                float                 z,
                                                GdkRGBA              *color);
void         analysis_map_lookup_rgba_bilinear (AnalysisMap          *map,
                                                float                 x,
                                                float                 z,
                                                GdkRGBA              *color);

#endif

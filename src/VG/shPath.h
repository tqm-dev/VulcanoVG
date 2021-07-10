
#ifndef __SHPATH_H
#define __SHPATH_H

#include "shVectors.h"
#include "shArrays.h"

/* Helper structures for subdivision */
typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHVector2 p3;
} SHQuad;

typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHVector2 p3;
  SHVector2 p4;
} SHCubic;

typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHfloat a1;
  SHfloat a2;
} SHArc;

/* SHVertex */
typedef struct
{
  SHVector2 point;
  SHVector2 tangent;
  SHfloat length;
  SHuint flags;
  
} SHVertex;

/* Vertex flags for contour definition */
#define SH_VERTEX_FLAG_CLOSE   (1 << 0)
#define SH_VERTEX_FLAG_SEGEND  (1 << 1)
#define SH_SEGMENT_TYPE_COUNT  13

/* Vertex array */
#define _ITEM_T SHVertex
#define _ARRAY_T SHVertexArray
#define _FUNC_T shVertexArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"


/* SHPath */
typedef struct SHPath
{
  /* Properties */
  VGint format;
  SHfloat scale;
  SHfloat bias;
  SHint segHint;
  SHint dataHint;
  VGbitfield caps;
  VGPathDatatype datatype;
  
  /* Raw data */
  SHuint8 *segs;
  void *data;
  SHint segCount;
  SHint dataCount;

  /* Reduced data */
  reduced_path_vec reduced_paths;

  /* Geometries for counting pixcel coverage */

  // Fill
  struct geometry fill_geoms[4];  /* 0: front-solid
                                     1: back-solid
                                     2: front-quad
                                     3: back-quad    */
  size_t fill_counts[2];
  int    fill_starts[2];
  float  fill_bounds[4];

  // Stroke
  struct geometry stroke_geoms[2];/* 1: solid
                                     2: quad  */
  float  stroke_bounds[4];
  float  stroke_width;
  int    join_style;
  int    initial_end_cap;
  int    terminal_end_cap;
  float  miter_limit;
  int    num_dashes;
  float  *dashes;
  float  dash_length;

  /* Subdivision */
  SHVertexArray vertices;
  SHVector2 min, max;
  
  /* Additional stroke geometry (dash vertices if
     path dashed or triangle vertices if width > 1 */
  SHVector2Array stroke;

  /* Cache */
  VGboolean      cacheDataValid;

  VGboolean      cacheTransformInit;
  SHMatrix3x3    cacheTransform;

  VGboolean      cacheStrokeInit;
  VGboolean      cacheStrokeTessValid;
  SHfloat        cacheStrokeLineWidth;
  VGCapStyle     cacheStrokeCapStyle;
  VGJoinStyle    cacheStrokeJoinStyle;
  SHfloat        cacheStrokeMiterLimit;
  SHfloat        cacheStrokeDashPhase;
  VGboolean      cacheStrokeDashPhaseReset;

  VGboolean      cacheReducedPaths;
  VGboolean      cacheFillGeometries;
  VGboolean      cacheStrokeGeometries;
  
} SHPath;

void SHPath_ctor(SHPath *p);
void SHPath_dtor(SHPath *p);


/* Processing normalization flags */
#define SH_PROCESS_SIMPLIFY_LINES    (1 << 0)
#define SH_PROCESS_SIMPLIFY_CURVES   (1 << 1)
#define SH_PROCESS_CENTRALIZE_ARCS   (1 << 2)
#define SH_PROCESS_REPAIR_ENDS       (1 << 3)

/* Segment callback function type */
typedef void (*SegmentFunc) (SHPath *p, VGPathSegment segment,
                             VGPathCommand originalCommand,
                             SHfloat *data, void *userData);

/* Processes raw path data into normalized segments */
void shProcessPathData(SHPath *p, int flags,
                       SegmentFunc callback,
                       void *userData);


/* Pointer-to-path array */
#define _ITEM_T SHPath*
#define _ARRAY_T SHPathArray
#define _FUNC_T shPathArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

#endif /* __SHPATH_H */


#ifndef __SH_GEOMETRY_H

#include "shDefs.h"
#include "shContext.h"
#include "shVectors.h"
#include "shPath.h"

void shFlattenPath(SHPath *p, SHint surfaceSpace);
void shStrokePath(VGContext* c, SHPath *p);
void shTransformVertices(SHMatrix3x3 *m, SHPath *p);
void shReducePath(SHPath *p);
void shFindBoundbox(SHPath *p);
void shCreateFillGeometry(SHPath *path);
void shCreateStrokeGeometry(SHPath *path);

#endif /* __SH_GEOMETRY_H */

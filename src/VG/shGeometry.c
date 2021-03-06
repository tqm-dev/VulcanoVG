
#define VG_API_EXPORT
#include <VG/openvg.h>
#include "shContext.h"
#include "shGeometry.h"
#include "kvec.h"
#include <assert.h>


static int shAddVertex(SHPath *p, SHVertex *v, SHint *contourStart)
{
  /* Assert contour was open */
  SH_ASSERT((*contourStart) >= 0);
  
  /* Check vertex limit */
  if (p->vertices.size >= SH_MAX_VERTICES) return 0;
  
  /* Add vertex to subdivision */
  shVertexArrayPushBackP(&p->vertices, v);
  
  /* Increment contour size. Its stored in
     the flags of first contour vertex */
  p->vertices.items[*contourStart].flags++;
  
  return 1;
}

static void shSubrecurseQuad(SHPath *p, SHQuad *quad, SHint *contourStart)
{
  SHVertex v;
  SHVector2 mid, dif, c1, c2, c3;
  SHQuad quads[SH_MAX_RECURSE_DEPTH];
  SHQuad *q, *qleft, *qright;
  SHint qindex=0;
  quads[0] = *quad;
  
  while (qindex >= 0) {
    
    q = &quads[qindex];
    
    /* Calculate distance of control point from its
     counterpart on the line between end points */
    SET2V(mid, q->p1); ADD2V(mid, q->p3); DIV2(mid, 2);
    SET2V(dif, q->p2); SUB2V(dif, mid); ABS2(dif);
    
    /* Cancel if the curve is flat enough */
    if (dif.x + dif.y <= 1.0f || qindex == SH_MAX_RECURSE_DEPTH-1) {

      /* Add subdivision point */
      v.point = q->p3; v.flags = 0;
      if (qindex == 0) return; /* Skip last point */
      if (!shAddVertex(p, &v, contourStart)) return;
      --qindex;
      
    }else{
      
      /* Left recursion goes on top of stack! */
      qright = q; qleft = &quads[++qindex];
      
      /* Subdivide into 2 sub-curves */
      SET2V(c1, q->p1); ADD2V(c1, q->p2); DIV2(c1, 2);
      SET2V(c3, q->p2); ADD2V(c3, q->p3); DIV2(c3, 2);
      SET2V(c2, c1); ADD2V(c2, c3); DIV2(c2, 2);
  
      /* Add left recursion onto stack */
      qleft->p1 = q->p1;
      qleft->p2 = c1;
      qleft->p3 = c2;
      
      /* Add right recursion onto stack */
      qright->p1 = c2;
      qright->p2 = c3;
      qright->p3 = q->p3;
    }
  }
}

static void shSubrecurseCubic(SHPath *p, SHCubic *cubic, SHint *contourStart)
{
  SHVertex v;
  SHfloat dx1, dy1, dx2, dy2;
  SHVector2 mm, c1, c2, c3, c4, c5;
  SHCubic cubics[SH_MAX_RECURSE_DEPTH];
  SHCubic *c, *cleft, *cright;
  SHint cindex = 0;
  cubics[0] = *cubic;
  
  while (cindex >= 0) {
    
    c = &cubics[cindex];
    
    /* Calculate distance of control points from their
     counterparts on the line between end points */
    dx1 = 3.0f*c->p2.x - 2.0f*c->p1.x - c->p4.x; dx1 *= dx1;
    dy1 = 3.0f*c->p2.y - 2.0f*c->p1.y - c->p4.y; dy1 *= dy1;
    dx2 = 3.0f*c->p3.x - 2.0f*c->p4.x - c->p1.x; dx2 *= dx2;
    dy2 = 3.0f*c->p3.y - 2.0f*c->p4.y - c->p1.y; dy2 *= dy2;
    if (dx1 < dx2) dx1 = dx2;
    if (dy1 < dy2) dy1 = dy2;
    
    /* Cancel if the curve is flat enough */
    if (dx1+dy1 <= 1.0 || cindex == SH_MAX_RECURSE_DEPTH-1) {
      
      /* Add subdivision point */
      v.point = c->p4; v.flags = 0;
      if (cindex == 0) return; /* Skip last point */
      if (!shAddVertex(p, &v, contourStart)) return;
      --cindex;
      
    }else{
      
      /* Left recursion goes on top of stack! */
      cright = c; cleft = &cubics[++cindex];
      
      /* Subdivide into 2 sub-curves */
      SET2V(c1, c->p1); ADD2V(c1, c->p2); DIV2(c1, 2);
      SET2V(mm, c->p2); ADD2V(mm, c->p3); DIV2(mm, 2);
      SET2V(c5, c->p3); ADD2V(c5, c->p4); DIV2(c5, 2);
      
      SET2V(c2, c1); ADD2V(c2, mm); DIV2(c2, 2);
      SET2V(c4, mm); ADD2V(c4, c5); DIV2(c4, 2);
      
      SET2V(c3, c2); ADD2V(c3, c4); DIV2(c3, 2);
      
      /* Add left recursion to stack */
      cleft->p1 = c->p1;
      cleft->p2 = c1;
      cleft->p3 = c2;
      cleft->p4 = c3;
      
      /* Add right recursion to stack */
      cright->p1 = c3;
      cright->p2 = c4;
      cright->p3 = c5;
      cright->p4 = c->p4;
    }
  }
}

static void shSubrecurseArc(SHPath *p, SHArc *arc,
                            SHVector2 *c,SHVector2 *ux, SHVector2 *uy,
                            SHint *contourStart)
{
  SHVertex v;
  SHfloat am, cosa, sina, dx, dy;
  SHVector2 uux, uuy, c1, m;
  SHArc arcs[SH_MAX_RECURSE_DEPTH];
  SHArc *a, *aleft, *aright;
  SHint aindex=0;
  arcs[0] = *arc;
  
  while (aindex >= 0) {
    
    a = &arcs[aindex];
    
    /* Middle angle and its cos/sin */
    am = (a->a1 + a->a2)/2;
    cosa = SH_COS(am);
    sina = SH_SIN(am);
  
    /* New point */
    SET2V(uux, (*ux)); MUL2(uux, cosa);
    SET2V(uuy, (*uy)); MUL2(uuy, sina);
    SET2V(c1, (*c)); ADD2V(c1, uux); ADD2V(c1, uuy);
    
    /* Check distance from linear midpoint */
    SET2V(m, a->p1); ADD2V(m, a->p2); DIV2(m, 2);
    dx = c1.x - m.x; dy = c1.y - m.y;
    if (dx < 0.0f) dx = -dx;
    if (dy < 0.0f) dy = -dy;
    
    /* Stop if flat enough */
    if (dx+dy <= 1.0f || aindex == SH_MAX_RECURSE_DEPTH-1) {
      
      /* Add middle subdivision point */
      v.point = c1; v.flags = 0;
      if (!shAddVertex(p, &v, contourStart)) return;
      if (aindex == 0) return; /* Skip very last point */
      
      /* Add end subdivision point */
      v.point = a->p2; v.flags = 0;
      if (!shAddVertex(p, &v, contourStart)) return;
      --aindex;
      
    }else{
      
      /* Left subdivision goes on top of stack! */
      aright = a; aleft = &arcs[++aindex];
      
      /* Add left recursion to stack */
      aleft->p1 = a->p1;
      aleft->a1 = a->a1;
      aleft->p2 = c1;
      aleft->a2 = am;
      
      /* Add right recursion to stack */
      aright->p1 = c1;
      aright->a1 = am;
      aright->p2 = a->p2;
      aright->a2 = a->a2;
    }
  }
}

static void shSubdivideSegment(SHPath *p, VGPathSegment segment,
                               VGPathCommand originalCommand,
                               SHfloat *data, void *userData)
{
  SHVertex v;
  SHint *contourStart = ((SHint**)userData)[0];
  SHint *surfaceSpace = ((SHint**)userData)[1];
  SHQuad quad; SHCubic cubic; SHArc arc;
  SHVector2 c, ux, uy;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  switch (segment)
  {
  case VG_MOVE_TO:
    
    /* Set contour start here */
    (*contourStart) = p->vertices.size;
    
    /* First contour vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = 0;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_CLOSE_PATH:
    
    /* Last contour vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = SH_VERTEX_FLAG_SEGEND | SH_VERTEX_FLAG_CLOSE;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_LINE_TO:
    
    /* Last segment vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_QUAD_TO:
    
    /* Recurse subdivision */
    SET2(quad.p1, data[0], data[1]);
    SET2(quad.p2, data[2], data[3]);
    SET2(quad.p3, data[4], data[5]);
    if (*surfaceSpace) {
      TRANSFORM2(quad.p1, context->pathTransform);
      TRANSFORM2(quad.p2, context->pathTransform);
      TRANSFORM2(quad.p3, context->pathTransform); }
    shSubrecurseQuad(p, &quad, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[4];
    v.point.y = data[5];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_CUBIC_TO:
    
    /* Recurse subdivision */
    SET2(cubic.p1, data[0], data[1]);
    SET2(cubic.p2, data[2], data[3]);
    SET2(cubic.p3, data[4], data[5]);
    SET2(cubic.p4, data[6], data[7]);
    if (*surfaceSpace) {
      TRANSFORM2(cubic.p1, context->pathTransform);
      TRANSFORM2(cubic.p2, context->pathTransform);
      TRANSFORM2(cubic.p3, context->pathTransform);
      TRANSFORM2(cubic.p4, context->pathTransform); }
    shSubrecurseCubic(p, &cubic, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[6];
    v.point.y = data[7];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  default:
    
    SH_ASSERT(segment==VG_SCWARC_TO || segment==VG_SCCWARC_TO ||
              segment==VG_LCWARC_TO || segment==VG_LCCWARC_TO);
    
    /* Recurse subdivision */
    SET2(arc.p1, data[0], data[1]);
    SET2(arc.p2, data[10], data[11]);
    arc.a1 = data[8]; arc.a2 = data[9];
    SET2(c,  data[2], data[3]);
    SET2(ux, data[4], data[5]);
    SET2(uy, data[6], data[7]);
    if (*surfaceSpace) {
      TRANSFORM2(arc.p1, context->pathTransform);
      TRANSFORM2(arc.p2, context->pathTransform);
      TRANSFORM2(c, context->pathTransform);
      TRANSFORM2DIR(ux, context->pathTransform);
      TRANSFORM2DIR(uy, context->pathTransform); }
    shSubrecurseArc(p, &arc, &c, &ux, &uy, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[10];
    v.point.y = data[11];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace) {
      TRANSFORM2(v.point, context->pathTransform); }
    break;
  }
  
  /* Add subdivision vertex */
  shAddVertex(p, &v, contourStart);
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static void subdivide_cubic(const double c[8], double c1[8], double c2[8])
{
    double p1x = (c[0] + c[2]) / 2;
    double p1y = (c[1] + c[3]) / 2;
    double p2x = (c[2] + c[4]) / 2;
    double p2y = (c[3] + c[5]) / 2;
    double p3x = (c[4] + c[6]) / 2;
    double p3y = (c[5] + c[7]) / 2;
    double p4x = (p1x + p2x) / 2;
    double p4y = (p1y + p2y) / 2;
    double p5x = (p2x + p3x) / 2;
    double p5y = (p2y + p3y) / 2;
    double p6x = (p4x + p5x) / 2;
    double p6y = (p4y + p5y) / 2;

    double p0x = c[0];
    double p0y = c[1];
    double p7x = c[6];
    double p7y = c[7];

    c1[0] = p0x;
    c1[1] = p0y;
    c1[2] = p1x;
    c1[3] = p1y;
    c1[4] = p4x;
    c1[5] = p4y;
    c1[6] = p6x;
    c1[7] = p6y;

    c2[0] = p6x;
    c2[1] = p6y;
    c2[2] = p5x;
    c2[3] = p5y;
    c2[4] = p3x;
    c2[5] = p3y;
    c2[6] = p7x;
    c2[7] = p7y;
}

static void subdivide_cubic2(const double cin[8], double cout[16])
{
    subdivide_cubic(cin, cout, cout + 8);
}

static void subdivide_cubic4(const double cin[8], double cout[32])
{
    subdivide_cubic(cin, cout, cout + 16);
    subdivide_cubic2(cout, cout);
    subdivide_cubic2(cout + 16, cout + 16);
}

static void subdivide_cubic8(const double cin[8], double cout[64])
{
    subdivide_cubic(cin, cout, cout + 32);
    subdivide_cubic4(cout, cout);
    subdivide_cubic4(cout + 32, cout + 32);
}

static void cubic_to_quadratic(const double c[8], double q[6])
{
    q[0] = c[0];
    q[1] = c[1];
    q[2] = (3 * (c[2] + c[4]) - (c[0] + c[6])) / 4;
    q[3] = (3 * (c[3] + c[5]) - (c[1] + c[7])) / 4;
    q[4] = c[6];
    q[5] = c[7];
}

static void new_path(reduced_path_vec *paths)
{
    struct reduced_path rp = {0};
    kv_push_back(*paths, rp);
}

static void move_to(struct reduced_path *path, float x, float y)
{
    if (kv_empty(path->commands))
    {
        kv_push_back(path->commands, VG_MOVE_TO_ABS);
        kv_push_back(path->coords, x);
        kv_push_back(path->coords, y);
    }
    else
    {
        kv_a(path->coords, 0) = x;
        kv_a(path->coords, 1) = y;
    }
}

static void line_to(struct reduced_path *path, float x1, float y1, float x2, float y2)
{
    if (kv_empty(path->commands))
    {
        kv_push_back(path->commands, VG_MOVE_TO_ABS);
        kv_push_back(path->coords, x1);
        kv_push_back(path->coords, y1);
    }

    kv_push_back(path->commands, VG_LINE_TO_ABS);
    kv_push_back(path->coords, x2);
    kv_push_back(path->coords, y2);
}

static void quad_to(struct reduced_path *path, float x1, float y1, float x2, float y2, float x3, float y3)
{
    if (kv_empty(path->commands))
    {
        kv_push_back(path->commands, VG_MOVE_TO_ABS);
        kv_push_back(path->coords, x1);
        kv_push_back(path->coords, y1);
    }

    kv_push_back(path->commands, VG_QUAD_TO_ABS);
    kv_push_back(path->coords, x2);
    kv_push_back(path->coords, y2);
    kv_push_back(path->coords, x3);
    kv_push_back(path->coords, y3);
}

static void cubic_to(struct reduced_path *path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    int i;

    double cin[8] = { x1, y1, x2, y2, x3, y3, x4, y4 };
    double cout[64];
    subdivide_cubic8(cin, cout);

    if (kv_empty(path->commands))
    {
        kv_push_back(path->commands, VG_MOVE_TO_ABS);
        kv_push_back(path->coords, x1);
        kv_push_back(path->coords, y1);
    }

    for (i = 0; i < 8; ++i)
    {
        double q[6];
        cubic_to_quadratic(cout + i * 8, q);
        kv_push_back(path->commands, VG_QUAD_TO_ABS);
        kv_push_back(path->coords, q[2]);
        kv_push_back(path->coords, q[3]);
        kv_push_back(path->coords, q[4]);
        kv_push_back(path->coords, q[5]);
    }
}

static double angle(double ux, double uy, double vx, double vy)
{
    return atan2(ux * vy - uy * vx, ux * vx + uy * vy);
}

/* http://www.w3.org/TR/SVG/implnote.html#ArcConversionEndpointToCenter */
static void endpoint_to_center(double x1, double y1, double x2, double y2,
                               int fA, int fS, double *prx, double *pry, double phi,
                               double *cx, double *cy, double *theta1, double *dtheta)
{
    double x1p, y1p, rx, ry, lambda, fsgn, c1, cxp, cyp;

    x1p =  cos(phi) * (x1 - x2) / 2 + sin(phi) * (y1 - y2) / 2;
    y1p = -sin(phi) * (x1 - x2) / 2 + cos(phi) * (y1 - y2) / 2;

    rx = *prx;
    ry = *pry;

    lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1)
    {
        lambda = sqrt(lambda);
        rx *= lambda;
        ry *= lambda;
        *prx = rx;
        *pry = ry;
    }

    fA = !!fA;
    fS = !!fS;

    fsgn = (fA != fS) ? 1 : -1;

    c1 = (rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p) / (rx*rx*y1p*y1p + ry*ry*x1p*x1p);

    if (c1 < 0)	// because of floating point inaccuracies, c1 can be -epsilon.
        c1 = 0;
    else
        c1 = sqrt(c1);

    cxp = fsgn * c1 * (rx * y1p / ry);
    cyp = fsgn * c1 * (-ry * x1p / rx);

    *cx = cos(phi) * cxp - sin(phi) * cyp + (x1 + x2) / 2;
    *cy = sin(phi) * cxp + cos(phi) * cyp + (y1 + y2) / 2;

    *theta1 = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry);

    *dtheta = angle((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);

    if (!fS && *dtheta > 0)
        *dtheta -= 2 * M_PI;
    else if (fS && *dtheta < 0)
        *dtheta += 2 * M_PI;
}

static void arc_tod(struct reduced_path *path, double x1, double y1, double rh, double rv, double phi, int fA, int fS, double x2, double y2)
{
    double cx, cy, theta1, dtheta;

    int i, nquads;

    phi *= M_PI / 180;

    endpoint_to_center(x1, y1, x2, y2, fA, fS, &rh, &rv, phi, &cx, &cy, &theta1, &dtheta);

    nquads = ceil(fabs(dtheta) * 4 / M_PI);

    for (i = 0; i < nquads; ++i)
    {
        double t1 = theta1 + (i / (double)nquads) * dtheta;
        double t2 = theta1 + ((i + 1) / (double)nquads) * dtheta;
        double tm = (t1 + t2) / 2;

        double x1 = cos(phi)*rh*cos(t1) - sin(phi)*rv*sin(t1) + cx;
        double y1 = sin(phi)*rh*cos(t1) + cos(phi)*rv*sin(t1) + cy;

        double x2 = cos(phi)*rh*cos(t2) - sin(phi)*rv*sin(t2) + cx;
        double y2 = sin(phi)*rh*cos(t2) + cos(phi)*rv*sin(t2) + cy;

        double xm = cos(phi)*rh*cos(tm) - sin(phi)*rv*sin(tm) + cx;
        double ym = sin(phi)*rh*cos(tm) + cos(phi)*rv*sin(tm) + cy;

        double xc = (xm * 4 - (x1 + x2)) / 2;
        double yc = (ym * 4 - (y1 + y2)) / 2;

        kv_push_back(path->commands, VG_QUAD_TO_ABS);
        kv_push_back(path->coords, xc);
        kv_push_back(path->coords, yc);
        kv_push_back(path->coords, x2);
        kv_push_back(path->coords, y2);
    }
}

static void arc_to(struct reduced_path *path, float x1, float y1, float rh, float rv, float phi, int fA, int fS, float x2, float y2)
{
    if (kv_empty(path->commands))
    {
        kv_push_back(path->commands, VG_MOVE_TO_ABS);
        kv_push_back(path->coords, x1);
        kv_push_back(path->coords, y1);
    }

    arc_tod(path, x1, y1, rh, rv, phi, fA, fS, x2, y2);
}

static void close_path(struct reduced_path *path)
{
    if (kv_back(path->commands) != VG_CLOSE_PATH)
        kv_push_back(path->commands, VG_CLOSE_PATH);
}

// Place pen into first two coords
// which is same as last_path
//         data[0]
//         data[1]
#define c0 data[2]
#define c1 data[3]
#define c2 data[4]
#define c3 data[5]
#define c4 data[6]
#define c5 data[7]
#define c6 data[8]
#define set(x1, y1, x2, y2) ncpx = x1; ncpy = y1; npepx = x2; npepy = y2;
#define last_path &kv_back(*reduced_paths)
static float spx = 0, spy = 0;
static float cpx = 0, cpy = 0;
static float pepx = 0, pepy = 0;
static float ncpx = 0, ncpy = 0;
static float npepx = 0, npepy = 0;
static unsigned char prev_command = 2;

static void shReduceSegmentInit(reduced_path_vec* rpv){
	spx = 0; spy = 0;
	cpx = 0; cpy = 0;
	pepx = 0; pepy = 0;
	ncpx = 0; ncpy = 0;
	npepx = 0; npepy = 0;
	prev_command = 2;
    kv_init(*rpv);
}

static void shReduceSegmentDeinit(reduced_path_vec* rpv){
	spx = 0; spy = 0;
	cpx = 0; cpy = 0;
	pepx = 0; pepy = 0;
	ncpx = 0; ncpy = 0;
	npepx = 0; npepy = 0;
	prev_command = 2;
	// reduced path
	size_t i;
	for (i = 0; i < kv_size(*rpv); ++i)
	{
		kv_free(kv_a(*rpv, i).commands);
		kv_free(kv_a(*rpv, i).coords);
	}
	kv_free(*rpv);
}

/*
M: 0
L: 1
Z: 2

M -> M no
M -> L no
M -> Z no
L -> M yes
L -> L no
L -> Z no
Z -> M yes
Z -> L yes
Z -> Z no
*/
                                       /* Current */
                                       /* M  L  Z */  /* Prev */
static const int new_path_table[3][3] = {{0, 0, 0},   /* M    */
										 {1, 0, 0},   /* L    */
										 {1, 1, 0}};  /* Z    */

static void shReduceSegment(SHPath *p, VGPathSegment segment,
                               VGPathCommand originalCommand,
                               SHfloat *data, void *userData)
{
	reduced_path_vec* reduced_paths = &p->reduced_paths;
	
	switch(segment)
	{
	case VG_MOVE_TO:
		if (new_path_table[prev_command][0])
			new_path(reduced_paths);
		prev_command = 0;
		move_to(last_path, c0, c1);
		set(c0, c1, c0, c1);
		spx = ncpx;
		spy = ncpy;
		break;

	case VG_CLOSE_PATH:
		if (new_path_table[prev_command][2])
			new_path(reduced_paths);
		prev_command = 2;
		close_path(last_path);
		set(spx, spy, spx, spy);
		break;

	case VG_LINE_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		line_to(last_path, cpx, cpy, c0, c1);
		set(c0, c1, c0, c1);
		break;
		
	case VG_HLINE_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		line_to(last_path, cpx, cpy, c0, cpy);
		set(c0, cpy, c0, cpy);
		break;

	case VG_VLINE_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		line_to(last_path, cpx, cpy, cpx, c0);
		set(cpx, c0, cpx, c0);
		break;

	case VG_QUAD_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		quad_to(last_path, cpx, cpy, c0, c1, c2, c3);
		set(c2, c3, c0, c1);
		break;

	case VG_CUBIC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		cubic_to(last_path, cpx, cpy, c0, c1, c2, c3, c4, c5);
		set(c4, c5, c2, c3);
		break;

	case VG_SQUAD_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		quad_to(last_path, cpx, cpy, 2 * cpx - pepx, 2 * cpy - pepy, c0, c1);
		set(c0, c1, 2 * cpx - pepx, 2 * cpy - pepy);
		break;

	case VG_SCUBIC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		cubic_to(last_path, cpx, cpy, 2 * cpx - pepx, 2 * cpy - pepy, c0, c1, c2, c3);
		set(c2, c3, c0, c1);
		break;

	case VG_SCCWARC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		arc_to(last_path, cpx, cpy, c0, c1, c2, 0, 1, c3, c4);
		set(c3, c4, c3, c4);
		break;

	case VG_SCWARC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		arc_to(last_path, cpx, cpy, c0, c1, c2, 0, 0, c3, c4);
		set(c3, c4, c3, c4);
		break;

	case VG_LCCWARC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		arc_to(last_path, cpx, cpy, c0, c1, c2, 1, 1, c3, c4);
		set(c3, c4, c3, c4);
		break;

	case VG_LCWARC_TO:
		if (new_path_table[prev_command][1])
			new_path(reduced_paths);
		prev_command = 1;
		arc_to(last_path, cpx, cpy, c0, c1, c2, 1, 0, c3, c4);
		set(c3, c4, c3, c4);
		break;

	default:
		assert(0);
		break;
	}

	cpx = ncpx;
	cpy = ncpy;
	pepx = npepx;
	pepy = npepy;
}
#undef c0
#undef c1
#undef c2
#undef c3
#undef c4
#undef c5
#undef c6
#undef set
#undef last_path

static void add_stroke_line(SHPath* p, double x0, double y0, double x1, double y1)
{
    struct geometry *g = &p->stroke_geoms[0];

    double width = p->stroke_width;

    int index = kv_size(g->vertices) / 2;

    double dx = x1 - x0;
    double dy = y1 - y0;
    double len = sqrt(dx * dx + dy * dy);
    if (len == 0)
        return;

    dx /= len;
    dy /= len;

    kv_push_back(g->vertices, x0 + (dy * width * 0.5));
    kv_push_back(g->vertices, y0 - (dx * width * 0.5));

    kv_push_back(g->vertices, x1 + (dy * width * 0.5));
    kv_push_back(g->vertices, y1 - (dx * width * 0.5));

    kv_push_back(g->vertices, x1 - (dy * width * 0.5));
    kv_push_back(g->vertices, y1 + (dx * width * 0.5));

    kv_push_back(g->vertices, x0 - (dy * width * 0.5));
    kv_push_back(g->vertices, y0 + (dx * width * 0.5));

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 1);
    kv_push_back(g->indices, index + 2);

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 2);
    kv_push_back(g->indices, index + 3);
}

static void add_stroke_line_dashed(SHPath* path, double x0, double y0, double x1, double y1, double *dash_offset)
{
    if (path->num_dashes == 0)
    {
        add_stroke_line(path, x0, y0, x1, y1);
        return;
    }

    double length = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

    /* TODO: remove this check and make sure that 0 length lines are not passed to this function (while creaating reduced path and while closing the path) */
    if (length == 0)
        return;

    double offset = -fmod(*dash_offset, path->dash_length);

    int i = 0;
    while (offset < length)
    {
        double o0 = MAX(offset, 0);
        double o1 = MIN(offset + path->dashes[i], length);

        if (o1 >= 0)
        {
            double t0 = o0 / length;
            double t1 = o1 / length;

            add_stroke_line(path, (1 - t0) * x0 + t0 * x1, (1 - t0) * y0 + t0 * y1, (1 - t1) * x0 + t1 * x1, (1 - t1) * y0 + t1 * y1);
        }

        offset += path->dashes[i] + path->dashes[i + 1];
        i = (i + 2) % path->num_dashes;
    }

    *dash_offset = fmod(length + *dash_offset, path->dash_length);
}

static void evaluate_quadratic(double x0, double y0, double x1, double y1, double x2, double y2, double t, double *x, double *y)
{
    double Ax = x0 - 2 * x1 + x2;
    double Ay = y0 - 2 * y1 + y2;
    double Bx = 2 * (x1 - x0);
    double By = 2 * (y1 - y0);
    double Cx = x0;
    double Cy = y0;

    *x = Ax * t * t + Bx * t + Cx;
    *y = Ay * t * t + By * t + Cy;
}

static void get_quadratic_bounds(double x0, double y0, double x1, double y1, double x2, double y2,
                                 double stroke_width,
                                 double *minx, double *miny, double *maxx, double *maxy)
{
    // TODO: if control point is exactly between start and end, division by zero occurs
    double tx = (x0 - x1) / (x0 - 2 * x1 + x2);
    double ty = (y0 - y1) / (y0 - 2 * y1 + y2);

    *minx = MIN(x0, x2);
    *miny = MIN(y0, y2);
    *maxx = MAX(x0, x2);
    *maxy = MAX(y0, y2);

    if (0 < tx && tx < 1)
    {
        double x, y;
        evaluate_quadratic(x0, y0, x1, y1, x2, y2, tx, &x, &y);
        
        *minx = MIN(*minx, x);
        *miny = MIN(*miny, y);
        *maxx = MAX(*maxx, x);
        *maxy = MAX(*maxy, y);
    }

    if (0 < ty && ty < 1)
    {
        double x, y;
        evaluate_quadratic(x0, y0, x1, y1, x2, y2, ty, &x, &y);

        *minx = MIN(*minx, x);
        *miny = MIN(*miny, y);
        *maxx = MAX(*maxx, x);
        *maxy = MAX(*maxy, y);
    }

    *minx -= stroke_width * 0.5;
    *miny -= stroke_width * 0.5;
    *maxx += stroke_width * 0.5;
    *maxy += stroke_width * 0.5;
}

static double dot(double x0, double y0, double x1, double y1)
{
    return x0 * x1 + y0 * y1;
}

static void get_quadratic_bounds_oriented(double x0, double y0, double x1, double y1, double x2, double y2,
                                          double stroke_width,
                                          double *pcx, double *pcy, double *pux, double *puy, double *pvx, double *pvy)
{
    double minx, miny, maxx, maxy;
    double cx, cy;
    double ex, ey;

    double ux = x2 - x0;
    double uy = y2 - y0;

    double len = sqrt(ux * ux + uy * uy);
    if (len < 1e-6)
    {
        ux = 1;
        uy = 0;
    }
    else
    {
        ux /= len;
        uy /= len;
    }

    get_quadratic_bounds(0, 0,
        dot(ux, uy, x1 - x0, y1 - y0), dot(-uy, ux, x1 - x0, y1 - y0),
        dot(ux, uy, x2 - x0, y2 - y0), dot(-uy, ux, x2 - x0, y2 - y0),
        stroke_width,
        &minx, &miny, &maxx, &maxy);

    cx = (minx + maxx) / 2;
    cy = (miny + maxy) / 2;
    ex = (maxx - minx) / 2;
    ey = (maxy - miny) / 2;

    *pcx = x0 + ux * cx + -uy * cy;
    *pcy = y0 + uy * cx +  ux * cy;
    *pux = ux * ex;
    *puy = uy * ex;
    *pvx = -uy * ey;
    *pvy = ux * ey;
}

static void calculatepq(double Ax, double Ay, double Bx, double By, double Cx, double Cy,
                        double px, double py,
                        double *p, double *q)
{
    double a = -2 * dot(Ax, Ay, Ax, Ay);
    double b = -3 * dot(Ax, Ay, Bx, By);
    double c = 2 * dot(px, py, Ax, Ay) - 2 * dot(Cx, Cy, Ax, Ay) - dot(Bx, By, Bx, By);
    double d = dot(px, py, Bx, By) - dot(Cx, Cy, Bx, By);

    *p = (3 * a * c - b * b) / (3 * a * a);
    *q = (2 * b * b * b - 9 * a * b * c + 27 * a * a * d) / (27 * a * a * a);
}

static double arc_length_helper(double A, double B, double C, double t)
{
    /* Integrate[Sqrt[A t^2 + B t + C], t] */
    return (2 * sqrt(A) * (B + 2 * A * t) * sqrt(C + t * (B + A * t)) - (B * B - 4 * A * C) * log(B + 2 * A * t + 2 * sqrt(A) * sqrt(C + t * (B + A * t)))) / (8 * sqrt(A * A * A));
}

static double arc_length(double Ax, double Ay, double Bx, double By, double Cx, double Cy, double t)
{
    double A = 4 * (Ax * Ax + Ay * Ay);
    double B = 4 * (Ax * Bx + Ay * By);
    double C = Bx * Bx + By * By;

    return arc_length_helper(A, B, C, t) - arc_length_helper(A, B, C, 0);
}

static double inverse_arc_length(double Ax, double Ay, double Bx, double By, double Cx, double Cy, double u)
{
    int i;

    double A = 4 * (Ax * Ax + Ay * Ay);
    double B = 4 * (Ax * Bx + Ay * By);
    double C = Bx * Bx + By * By;

    if (u <= 0)
        return 0;

    double length = arc_length_helper(A, B, C, 1) - arc_length_helper(A, B, C, 0);

    if (u >= length)
        return 1;

    double u0 = arc_length_helper(A, B, C, 0) + u;

    double a = 0;
    double b = 1;
    double fa = arc_length_helper(A, B, C, a) - u0;
    double fb = arc_length_helper(A, B, C, b) - u0;

    for (i = 0; i < 20; ++i)
    {
        double c = (a + b) / 2;
        double fc = arc_length_helper(A, B, C, c) - u0;

        if (fc < 0)
        {
            a = c;
            fa = fc;
        }
        else if (fc > 0)
        {
            b = c;
            fb = fc;
        }
        else
            break;
    }

    return a - fa * (b - a) / (fb - fa);
}

static void quad_segment(const double qin[6], double t0, double t1, double qout[6])
{
    double u0 = 1 - t0;
    double u1 = 1 - t1;

    double x0 = qin[0];
    double y0 = qin[1];
    double x1 = qin[2];
    double y1 = qin[3];
    double x2 = qin[4];
    double y2 = qin[5];

    qout[0] = (u0 * u0) * x0 + (u0 * t0 + u0 * t0) * x1 + (t0 * t0) * x2;
    qout[1] = (u0 * u0) * y0 + (u0 * t0 + u0 * t0) * y1 + (t0 * t0) * y2;
    qout[2] = (u0 * u1) * x0 + (u0 * t1 + u1 * t0) * x1 + (t0 * t1) * x2;
    qout[3] = (u0 * u1) * y0 + (u0 * t1 + u1 * t0) * y1 + (t0 * t1) * y2;
    qout[4] = (u1 * u1) * x0 + (u1 * t1 + u1 * t1) * x1 + (t1 * t1) * x2;
    qout[5] = (u1 * u1) * y0 + (u1 * t1 + u1 * t1) * y1 + (t1 * t1) * y2;
}

static void add_stroke_quad(SHPath *path, double x0, double y0, double x1, double y1, double x2, double y2)
{
    int i;

    double Ax = x0 - 2 * x1 + x2;
    double Ay = y0 - 2 * y1 + y2;
    double Bx = 2 * (x1 - x0);
    double By = 2 * (y1 - y0);
    double Cx = x0;
    double Cy = y0;

    double cx, cy, ux, uy, vx, vy;
    get_quadratic_bounds_oriented(x0, y0, x1, y1, x2, y2, path->stroke_width + 1, &cx, &cy, &ux, &uy, &vx, &vy);

    double a = -2 * dot(Ax, Ay, Ax, Ay);
    double b = -3 * dot(Ax, Ay, Bx, By);

    double px[4], py[4];

    px[0] = cx - ux - vx;
    py[0] = cy - uy - vy;
    px[1] = cx + ux - vx;
    py[1] = cy + uy - vy;
    px[2] = cx + ux + vx;
    py[2] = cy + uy + vy;
    px[3] = cx - ux + vx;
    py[3] = cy - uy + vy;

    double p[4], q[4];
    for (i = 0; i < 4; ++i)
        calculatepq(Ax, Ay, Bx, By, Cx, Cy, px[i], py[i], &p[i], &q[i]);

    struct geometry *g = &path->stroke_geoms[1];

    int index = kv_size(g->vertices) / 12;

    for (i = 0; i < 4; ++i)
    {
        kv_push_back(g->vertices, px[i]);
        kv_push_back(g->vertices, py[i]);
        kv_push_back(g->vertices, p[i]);
        kv_push_back(g->vertices, q[i]);
        kv_push_back(g->vertices, Ax);
        kv_push_back(g->vertices, Ay);
        kv_push_back(g->vertices, Bx);
        kv_push_back(g->vertices, By);
        kv_push_back(g->vertices, Cx);
        kv_push_back(g->vertices, Cy);
        kv_push_back(g->vertices, -b / (3 * a));
        kv_push_back(g->vertices, path->stroke_width * path->stroke_width / 4);
    }

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 1);
    kv_push_back(g->indices, index + 2);

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 2);
    kv_push_back(g->indices, index + 3);
}

static void add_stroke_quad_dashed(SHPath *path, double x0, double y0, double x1, double y1, double x2, double y2, double *dash_offset)
{
    if (path->num_dashes == 0)
    {
        add_stroke_quad(path, x0, y0, x1, y1, x2, y2);
        return;
    }

    double Ax = x0 - 2 * x1 + x2;
    double Ay = y0 - 2 * y1 + y2;
    double Bx = 2 * (x1 - x0);
    double By = 2 * (y1 - y0);
    double Cx = x0;
    double Cy = y0;

    double q[6] = { x0, y0, x1, y1, x2, y2 };

    double length = arc_length(Ax, Ay, Bx, By, Cx, Cy, 1);

    double offset = -fmod(*dash_offset, path->dash_length);

    int i = 0;
    while (offset < length)
    {
        double o0 = MAX(offset, 0);
        double o1 = MIN(offset + path->dashes[i], length);

        if (o1 >= 0)
        {
            double t0 = inverse_arc_length(Ax, Ay, Bx, By, Cx, Cy, o0);
            double t1 = inverse_arc_length(Ax, Ay, Bx, By, Cx, Cy, o1);

            double qout[6];
            quad_segment(q, t0, t1, qout);
            add_stroke_quad(path, qout[0], qout[1], qout[2], qout[3], qout[4], qout[5]);
        }

        offset += path->dashes[i] + path->dashes[i + 1];
        i = (i + 2) % path->num_dashes;
    }

    *dash_offset = fmod(length + *dash_offset, path->dash_length);
}

static void add_join_bevel(SHPath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
    float v0x = x0 - x1;
    float v0y = y0 - y1;
    float v1x = x2 - x1;
    float v1y = y2 - y1;

    float len0 = sqrtf(v0x * v0x + v0y * v0y);
    float len1 = sqrtf(v1x * v1x + v1y * v1y);

    if (len0 == 0 || len1 == 0)
        return;

    struct geometry *g = &path->stroke_geoms[0];

    float width = path->stroke_width;

    int index = kv_size(g->vertices) / 2;

    float w0 = width / (2 * len0);
    float w1 = width / (2 * len1);

    if (v0x * v1y - v0y * v1x < 0)
    {
        kv_push_back(g->vertices, x1 + v1y * w1);
        kv_push_back(g->vertices, y1 + -v1x * w1);
        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, x1 + -v0y * w0);
        kv_push_back(g->vertices, y1 + v0x * w0);
    }
    else
    {
        kv_push_back(g->vertices, x1 + v0y * w0);
        kv_push_back(g->vertices, y1 + -v0x * w0);
        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, x1 + -v1y * w1);
        kv_push_back(g->vertices, y1 + v1x * w1);
    }

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 1);
    kv_push_back(g->indices, index + 2);
}

static int check_offset(SHPath *path, float of)
{
    int i;

    if (path->num_dashes == 0)
        return 1;

    float offset = 0;
    for (i = 0; i < path->num_dashes; i += 2)
    {
        float o0 = offset;
        float o1 = offset + path->dashes[i];

        if (o0 <= of && of <= o1)
            return 1;

        offset += path->dashes[i] + path->dashes[i + 1];
    }

    return 0;
}

static void add_join_miter_truncate(SHPath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
    //TODO
}

static void add_join_round(SHPath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
    //TODO
}

static void add_join(SHPath *path, float x0, float y0, float x1, float y1, float x2, float y2)
{
    switch (path->join_style)
    {
    case VG_JOIN_BEVEL:
        add_join_bevel(path, x0, y0, x1, y1, x2, y2);
        break;
    case VG_JOIN_MITER:
        add_join_miter_truncate(path, x0, y0, x1, y1, x2, y2);
        break;
    case VG_JOIN_ROUND:
        add_join_round(path, x0, y0, x1, y1, x2, y2);
        break;
    default:
        assert(0);
        break;
    }
}

typedef kvec_t(float) kvec_float_t;

static void corner_start(kvec_float_t *v, float x, float y, float offset)
{
    kv_push_back(*v, x);
    kv_push_back(*v, y);
    kv_push_back(*v, offset);
}

static void corner_continue(kvec_float_t *v, float x0, float y0, float x1, float y1, float offset)
{
    float x = kv_a(*v, kv_size(*v) - 3);
    float y = kv_a(*v, kv_size(*v) - 2);

    // TODO: check equality with epsilon
    if (x != x0 || x != x1 || x0 != x1 || y != y0 || y != y1 || y0 != y1)
    {
        kv_push_back(*v, x0);
        kv_push_back(*v, y0);
        kv_push_back(*v, x1);
        kv_push_back(*v, y1);
        kv_push_back(*v, offset);
    }
}

static void corner_end(kvec_float_t *v, float x, float y)
{
    float x0 = kv_a(*v, 0);
    float y0 = kv_a(*v, 1);

    float x1 = kv_a(*v, kv_size(*v) - 3);
    float y1 = kv_a(*v, kv_size(*v) - 2);

    // TODO: check equality with epsilon
    if (x != x0 || x != x1 || x0 != x1 || y != y0 || y != y1 || y0 != y1)
    {
        kv_push_back(*v, x);
        kv_push_back(*v, y);
    }
    else
    {
        kv_pop_back(*v);
        kv_pop_back(*v);
        kv_pop_back(*v);
    }
}

static void update_bounds(float bounds[4], size_t num_vertices, const float *vertices, int stride)
{
    size_t i;

    for (i = 0; i < num_vertices; i += stride)
    {
        float x = vertices[i];
        float y = vertices[i + 1];

        bounds[0] = MIN(bounds[0], x);
        bounds[1] = MIN(bounds[1], y);
        bounds[2] = MAX(bounds[2], x);
        bounds[3] = MAX(bounds[3], y);
    }
}

void shCreateStrokeGeometry(SHPath *path)
{
#define c0 coords[icoord]
#define c1 coords[icoord + 1]
#define c2 coords[icoord + 2]
#define c3 coords[icoord + 3]

#define set(x1, y1, x2, y2) ncpx = x1; ncpy = y1; npepx = x2; npepy = y2;

    reduced_path_vec *reduced_paths = &path->reduced_paths;

    size_t i, j;

    for (i = 0; i < 2; ++i)
    {
        kv_init(path->stroke_geoms[i].vertices);
        kv_init(path->stroke_geoms[i].indices);
    }

    double offset = 0;

    for (i = 0; i < kv_size(*reduced_paths); ++i)
    {
        struct reduced_path *p = &kv_a(*reduced_paths, i);

        kvec_float_t corners;
        kv_init(corners);

        size_t num_commands = kv_size(p->commands);
        unsigned char *commands = kv_data(p->commands);
        float *coords = kv_data(p->coords);

        int closed = 0;

        int icoord = 0;

        float spx = 0, spy = 0;
        float cpx = 0, cpy = 0;
        float pepx = 0, pepy = 0;
        float ncpx = 0, ncpy = 0;
        float npepx = 0, npepy = 0;

        for (j = 0; j < num_commands; ++j)
        {
            switch (commands[j])
            {
            case VG_MOVE_TO:
                corner_start(&corners, c0, c1, offset);
                set(c0, c1, c0, c1);
                spx = ncpx;
                spy = ncpy;
                icoord += 2;
                break;
            case VG_LINE_TO:
                add_stroke_line_dashed(path, cpx, cpy, c0, c1, &offset);
                corner_continue(&corners, (cpx + c0) / 2, (cpy + c1) / 2, c0, c1, offset);
                set(c0, c1, c0, c1);
                icoord += 2;
                break;
            case VG_QUAD_TO:
                add_stroke_quad_dashed(path, cpx, cpy, c0, c1, c2, c3, &offset);
                corner_continue(&corners, c0, c1, c2, c3, offset);
                set(c2, c3, c0, c1);
                icoord += 4;
                break;
            case VG_CLOSE_PATH:
                add_stroke_line_dashed(path, cpx, cpy, spx, spy, &offset);
                corner_end(&corners, (cpx + spx) / 2, (cpy + spy) / 2);
                set(spx, spy, spx, spy);
                closed = 1;
                break;
            }

            cpx = ncpx;
            cpy = ncpy;
            pepx = npepx;
            pepy = npepy;
        }

        size_t ncorners = (kv_size(corners) - (closed ? 0 : 7)) / 5;

        for (j = 0; j < ncorners; j++)
        {
            int j0 = j;
            int j1 = (j + 1) % ncorners;

            float x0 = kv_a(corners, j0 * 5 + 3);
            float y0 = kv_a(corners, j0 * 5 + 4);
            float x1 = kv_a(corners, j1 * 5 + 0);
            float y1 = kv_a(corners, j1 * 5 + 1);
            float of = kv_a(corners, j1 * 5 + 2);
            float x2 = kv_a(corners, j1 * 5 + 3);
            float y2 = kv_a(corners, j1 * 5 + 4);

            if (check_offset(path, of))
                add_join(path, x0, y0, x1, y1, x2, y2);
        }

        kv_free(corners);
    }

#undef c0
#undef c1
#undef c2
#undef c3

#undef set

    path->stroke_bounds[0] = 1e30f;
    path->stroke_bounds[1] = 1e30f;
    path->stroke_bounds[2] = -1e30f;
    path->stroke_bounds[3] = -1e30f;

    update_bounds(path->stroke_bounds, kv_size(path->stroke_geoms[0].vertices), kv_data(path->stroke_geoms[0].vertices), 2);
    update_bounds(path->stroke_bounds, kv_size(path->stroke_geoms[1].vertices), kv_data(path->stroke_geoms[1].vertices), 12);

    path->stroke_geoms[0].count = kv_size(path->stroke_geoms[0].indices);
    path->stroke_geoms[1].count = kv_size(path->stroke_geoms[1].indices);
}

static void add_fill_line(SHPath *p, float xc, float yc, float x0, float y0, float x1, float y1)
{
    float v0x = x0 - xc;
    float v0y = y0 - yc;
    float v1x = x1 - xc;
    float v1y = y1 - yc;

    struct geometry *g = NULL;
    int index;

    if (v0x * v1y - v0y * v1x < 0)
    {
        g = &p->fill_geoms[1];

        index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, xc);
        kv_push_back(g->vertices, yc);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);
    }
    else
    {
        g = &p->fill_geoms[0];

        index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, xc);
        kv_push_back(g->vertices, yc);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);
    }

    kv_push_back(g->indices, index);
    kv_push_back(g->indices, index + 1);
    kv_push_back(g->indices, index + 2);
}

static void add_fill_quad(SHPath *p, float xc, float yc, float x0, float y0, float x1, float y1, float x2, float y2)
{
    float v0x, v0y, v1x, v1y;

    v0x = x0 - xc;
    v0y = y0 - yc;
    v1x = x2 - xc;
    v1y = y2 - yc;

    if (v0x * v1y - v0y * v1x < 0)
    {
        struct geometry *g = &p->fill_geoms[1];

        int index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, xc);
        kv_push_back(g->vertices, yc);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x2);
        kv_push_back(g->vertices, y2);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->indices, index);
        kv_push_back(g->indices, index + 1);
        kv_push_back(g->indices, index + 2);
    }
    else
    {
        struct geometry *g = &p->fill_geoms[0];

        int index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, xc);
        kv_push_back(g->vertices, yc);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x2);
        kv_push_back(g->vertices, y2);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->indices, index);
        kv_push_back(g->indices, index + 1);
        kv_push_back(g->indices, index + 2);
    }

    v0x = x1 - x0;
    v0y = y1 - y0;
    v1x = x2 - x0;
    v1y = y2 - y0;

    if (v0x * v1y - v0y * v1x < 0)
    {
        struct geometry *g = &p->fill_geoms[3];

        int index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x2);
        kv_push_back(g->vertices, y2);
        kv_push_back(g->vertices, 1);
        kv_push_back(g->vertices, 1);

        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, 0.5f);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->indices, index);
        kv_push_back(g->indices, index + 1);
        kv_push_back(g->indices, index + 2);
    }
    else
    {
        struct geometry *g = &p->fill_geoms[2];

        int index = kv_size(g->vertices) / 4;

        kv_push_back(g->vertices, x2);
        kv_push_back(g->vertices, y2);
        kv_push_back(g->vertices, 1);
        kv_push_back(g->vertices, 1);

        kv_push_back(g->vertices, x0);
        kv_push_back(g->vertices, y0);
        kv_push_back(g->vertices, 0);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->vertices, x1);
        kv_push_back(g->vertices, y1);
        kv_push_back(g->vertices, 0.5f);
        kv_push_back(g->vertices, 0);

        kv_push_back(g->indices, index);
        kv_push_back(g->indices, index + 1);
        kv_push_back(g->indices, index + 2);
    }
}

void shCreateFillGeometry(SHPath *path)
{
#define c0 coords[icoord]
#define c1 coords[icoord + 1]
#define c2 coords[icoord + 2]
#define c3 coords[icoord + 3]

#define set(x1, y1, x2, y2) ncpx = x1; ncpy = y1; npepx = x2; npepy = y2;

    reduced_path_vec *reduced_paths = &path->reduced_paths;

    size_t i, j;

    for (i = 0; i < 4; ++i)
    {
        kv_init(path->fill_geoms[i].vertices);
        kv_init(path->fill_geoms[i].indices);
    }

    for (i = 0; i < kv_size(*reduced_paths); ++i)
    {
        struct reduced_path *p = &kv_a(*reduced_paths, i);

        size_t num_commands = kv_size(p->commands);
        unsigned char *commands = kv_data(p->commands);
        float *coords = kv_data(p->coords);

        int closed = 0;

        int icoord = 0;

        float spx = 0, spy = 0;
        float cpx = 0, cpy = 0;
        float pepx = 0, pepy = 0;
        float ncpx = 0, ncpy = 0;
        float npepx = 0, npepy = 0;

        float xc, yc;

        for (j = 0; j < num_commands; ++j)
        {
            switch (commands[j])
            {
            case VG_MOVE_TO:
                set(c0, c1, c0, c1);
                spx = ncpx;
                spy = ncpy;
                xc = spx;
                yc = spy;
                icoord += 2;
                break;
            case VG_LINE_TO:
                add_fill_line(path, xc, yc, cpx, cpy, c0, c1);
                set(c0, c1, c0, c1);
                icoord += 2;
                break;
            case VG_QUAD_TO:
                add_fill_quad(path, xc, yc, cpx, cpy, c0, c1, c2, c3);
                set(c2, c3, c0, c1);
                icoord += 4;
                break;
            case VG_CLOSE_PATH:
                add_fill_line(path, xc, yc, cpx, cpy, spx, spy);
                set(spx, spy, spx, spy);
                closed = 1;
                break;
            }

            cpx = ncpx;
            cpy = ncpy;
            pepx = npepx;
            pepy = npepy;
        }

        if (closed == 0)
        {
            // TODO: close the fill geometry
        }
    }

#undef c0
#undef c1
#undef c2
#undef c3

#undef set

    path->fill_bounds[0] = 1e30f;
    path->fill_bounds[1] = 1e30f;
    path->fill_bounds[2] = -1e30f;
    path->fill_bounds[3] = -1e30f;

    update_bounds(path->fill_bounds, kv_size(path->fill_geoms[0].vertices), kv_data(path->fill_geoms[0].vertices), 4);
    update_bounds(path->fill_bounds, kv_size(path->fill_geoms[1].vertices), kv_data(path->fill_geoms[1].vertices), 4);
    update_bounds(path->fill_bounds, kv_size(path->fill_geoms[2].vertices), kv_data(path->fill_geoms[2].vertices), 4);
    update_bounds(path->fill_bounds, kv_size(path->fill_geoms[3].vertices), kv_data(path->fill_geoms[3].vertices), 4);

    path->fill_geoms[0].count = kv_size(path->fill_geoms[0].indices);
    path->fill_geoms[1].count = kv_size(path->fill_geoms[1].indices);
    path->fill_geoms[2].count = kv_size(path->fill_geoms[2].indices);
    path->fill_geoms[3].count = kv_size(path->fill_geoms[3].indices);
}

/*--------------------------------------------------
 * Processes path data by simplfying it and sending
 * each segment to subdivision callback function
 *--------------------------------------------------*/

void shFlattenPath(SHPath *p, SHint surfaceSpace)
{
  SHint contourStart = -1;
  SHint surfSpace = surfaceSpace;
  SHint *userData[2];
  SHint processFlags =
    SH_PROCESS_SIMPLIFY_LINES |
    SH_PROCESS_SIMPLIFY_CURVES |
    SH_PROCESS_CENTRALIZE_ARCS |
    SH_PROCESS_REPAIR_ENDS;
  
  userData[0] = &contourStart;
  userData[1] = &surfaceSpace;
  
  shVertexArrayClear(&p->vertices);
  shProcessPathData(p, processFlags, shSubdivideSegment, userData);
}

/*--------------------------------------------------
 * Processes path data by simplfying it and sending
 * each segment to reducer callback function
 *--------------------------------------------------*/
void shReducePath(SHPath *p)
{
  // Reduce paths
  shReduceSegmentInit(&p->reduced_paths); // for test
  shProcessPathData(p, 0, shReduceSegment, NULL);
  //shReduceSegmentDeinit(&p->reduced_paths); // for test
}

/*-------------------------------------------
 * Adds a rectangle to the path's stroke.
 *-------------------------------------------*/

static void shPushStrokeQuad(SHPath *p, SHVector2 *p1, SHVector2 *p2,
                             SHVector2 *p3, SHVector2 *p4)
{
  shVector2ArrayPushBackP(&p->stroke, p1);
  shVector2ArrayPushBackP(&p->stroke, p2);
  shVector2ArrayPushBackP(&p->stroke, p3);
  shVector2ArrayPushBackP(&p->stroke, p3);
  shVector2ArrayPushBackP(&p->stroke, p4);
  shVector2ArrayPushBackP(&p->stroke, p1);
}

/*-------------------------------------------
 * Adds a triangle to the path's stroke.
 *-------------------------------------------*/

static void shPushStrokeTri(SHPath *p, SHVector2 *p1,
                            SHVector2 *p2, SHVector2 *p3)
{
  shVector2ArrayPushBackP(&p->stroke, p1);
  shVector2ArrayPushBackP(&p->stroke, p2);
  shVector2ArrayPushBackP(&p->stroke, p3);
}

/*-----------------------------------------------------------
 * Adds a miter join to the path's stroke at the given
 * turn point [c], with the end of the previous segment
 * outset [o1] and the beginning of the next segment
 * outset [o2], transiting from tangent [d1] to [d2].
 *-----------------------------------------------------------*/

static void shStrokeJoinMiter(SHPath *p, SHVector2 *c,
                              SHVector2 *o1, SHVector2 *d1,
                              SHVector2 *o2, SHVector2 *d2)
{
  /* Init miter top to first point in case lines are colinear */
  SHVector2 x; SET2V(x,(*o1));
  
  /* Find intersection of two outer turn edges
     (lines defined by origin and direction) */
  shLineLineXsection(o1, d1, o2, d2, &x);
  
  /* Add a "diamond" quad with top on intersected point
     and bottom on center of turn (on the line) */
  shPushStrokeQuad(p, &x, o1, c, o2);
}

/*-----------------------------------------------------------
 * Adds a round join to the path's stroke at the given
 * turn point [c], with the end of the previous segment
 * outset [pstart] and the beginning of the next segment
 * outset [pend], transiting from perpendicular vector
 * [tstart] to [tend].
 *-----------------------------------------------------------*/

static void shStrokeJoinRound(SHPath *p, SHVector2 *c,
                              SHVector2 *pstart, SHVector2 *tstart, 
                              SHVector2 *pend, SHVector2 *tend)
{
  SHVector2 p1, p2;
  SHfloat a, ang, cosa, sina;
  
  /* Find angle between lines */
  ang = ANGLE2((*tstart),(*tend));
  
  /* Begin with start point */
  SET2V(p1,(*pstart));
  for (a=0.0f; a<ang; a+=PI/12) {
    
    /* Rotate perpendicular vector around and
       find next offset point from center */
    cosa = SH_COS(-a);
    sina = SH_SIN(-a);
    SET2(p2, tstart->x*cosa - tstart->y*sina,
         tstart->x*sina + tstart->y*cosa);
    ADD2V(p2, (*c));
    
    /* Add triangle, save previous */
    shPushStrokeTri(p, &p1, &p2, c);
    SET2V(p1, p2);
  }
  
  /* Add last triangle */
  shPushStrokeTri(p, &p1, pend, c);
}

static void shStrokeCapRound(SHPath *p, SHVector2 *c, SHVector2 *t, SHint start)
{
  SHint a;
  SHfloat ang, cosa, sina;
  SHVector2 p1, p2;
  SHint steps = 12;
  SHVector2 tt;
  
  /* Revert perpendicular vector if start cap */
  SET2V(tt, (*t));
  if (start) MUL2(tt, -1);
  
  /* Find start point */
  SET2V(p1, (*c));
  ADD2V(p1, tt);
  
  for (a = 1; a<=steps; ++a) {
    
    /* Rotate perpendicular vector around and
       find next offset point from center */
    ang = (SHfloat)a * PI / steps;
    cosa = SH_COS(-ang);
    sina = SH_SIN(-ang);
    SET2(p2, tt.x*cosa - tt.y*sina,
         tt.x*sina + tt.y*cosa);
    ADD2V(p2, (*c));
    
    /* Add triangle, save previous */
    shPushStrokeTri(p, &p1, &p2, c);
    SET2V(p1, p2);
  }
}

static void shStrokeCapSquare(SHPath *p, SHVector2 *c, SHVector2 *t, SHint start)
{
  SHVector2 tt, p1, p2, p3, p4;
  
  /* Revert perpendicular vector if start cap */
  SET2V(tt, (*t));
  if (start) MUL2(tt, -1);
  
  /* Find four corners of the quad */
  SET2V(p1, (*c));
  ADD2V(p1, tt);
  
  SET2V(p2, p1);
  ADD2(p2, tt.y, -tt.x);
  
  SET2V(p3, p2);
  ADD2(p3, -2*tt.x, -2*tt.y);
  
  SET2V(p4, p3);
  ADD2(p4, -tt.y, tt.x);
  
  shPushStrokeQuad(p, &p1, &p2, &p3, &p4);
}

/*-----------------------------------------------------------
 * Generates stroke of a path according to VGContext state.
 * Produces quads for every linear subdivision segment or
 * dash "on" segment, handles line caps and joins.
 *-----------------------------------------------------------*/

void shStrokePath(VGContext* c, SHPath *p)
{
  /* Line width and vertex count */
  SHfloat w = c->strokeLineWidth / 2;
  SHfloat mlimit = c->strokeMiterLimit;
  SHint vertsize = p->vertices.size;
  
  /* Contour state */
  SHint contourStart = 0;
  SHint contourLength = 0;
  SHint start = 0;
  SHint end = 0;
  SHint loop = 0;
  SHint close = 0;
  SHint segend = 0;
  
  /* Current vertices */
  SHint i1=0, i2=0;
  SHVertex *v1, *v2;
  SHVector2 *p1, *p2;
  SHVector2 d, t, dprev, tprev;
  SHfloat norm, cross, mlength;
  
  /* Stroke edge points */
  SHVector2 l1, r1, l2, r2, lprev, rprev;
  
  /* Dash state */
  SHint dashIndex = 0;
  SHfloat dashLength = 0.0f, strokeLength = 0.0f;
  SHint dashSize = c->strokeDashPattern.size;
  SHfloat *dashPattern = c->strokeDashPattern.items;
  SHint dashOn = 1;
  
  /* Dash edge points */
  SHVector2 dash1, dash2;
  SHVector2 dashL1, dashR1;
  SHVector2 dashL2, dashR2;
  SHfloat nextDashLength, dashOffset;
  
  /* Discard odd dash segment */
  dashSize -= dashSize % 2;

  /* Init previous so compiler doesn't warn
     for uninitialized usage */
  SET2(tprev, 0,0); SET2(dprev, 0,0);
  SET2(lprev, 0,0); SET2(rprev, 0,0);

  
  /* Walk over subdivision vertices */
  for (i1=0; i1<vertsize; ++i1) {
    
    if (loop) {
      /* Start new contour if exists */
      if (contourStart < vertsize)
        i1 = contourStart;
      else break;
    }
    
    start = end = loop = close = segend = 0;
    i2 = i1 + 1;
    
    if (i1 == contourStart) {
      /* Contour has started. Get length */
      contourLength = p->vertices.items[i1].flags;
      start = 1;
    }
    
    if (contourLength <= 1) {
      /* Discard empty contours. */
      contourStart = i1 + 1;
      loop = 1;
      continue;
    }
    
    v1 = &p->vertices.items[i1];
    v2 = &p->vertices.items[i2];
    
    if (i2 == contourStart + contourLength-1) {
      /* Contour has ended. Check close */
      close = v2->flags & SH_VERTEX_FLAG_CLOSE;
      end = 1;
    }
    
    if (i1 == contourStart + contourLength-1) {
      /* Loop back to first edge. Check close */
      close = v1->flags & SH_VERTEX_FLAG_CLOSE;
      i2 = contourStart+1;
      contourStart = i1 + 1;
      i1 = i2 - 1;
      loop = 1;
    }
    
    if (!start && !loop) {
      /* We are inside a contour. Check segment end. */
      segend = (v1->flags & SH_VERTEX_FLAG_SEGEND);
    }
    
    if (dashSize > 0 && start &&
        (contourStart == 0 || c->strokeDashPhaseReset)) {
      
      /* Reset pattern phase at contour start */
      dashLength = -c->strokeDashPhase;
      strokeLength = 0.0f;
      dashIndex = 0;
      dashOn = 1;
      
      if (dashLength < 0.0f) {
        /* Consume dash segments forward to reach stroke start */
        while (dashLength + dashPattern[dashIndex] <= 0.0f) {
          dashLength += dashPattern[dashIndex];
          dashIndex = (dashIndex + 1) % dashSize;
          dashOn = !dashOn; }
        
      }else if (dashLength > 0.0f) {
        /* Consume dash segments backward to return to stroke start */
        dashIndex = dashSize;
        while (dashLength > 0.0f) {
          dashIndex = dashIndex ? (dashIndex-1) : (dashSize-1);
          dashLength -= dashPattern[dashIndex];
          dashOn = !dashOn; }
      }
    }
    
    /* Subdiv segment vertices and points */
    v1 = &p->vertices.items[i1];
    v2 = &p->vertices.items[i2];
    p1 = &v1->point;
    p2 = &v2->point;
    
    /* Direction vector */
    SET2(d, p2->x-p1->x, p2->y-p1->y);
    norm = NORM2(d);
    if (norm == 0.0f) d = dprev;
    else DIV2(d, norm);
    
    /* Perpendicular vector */
    SET2(t, -d.y, d.x);
    MUL2(t, w);
    cross = CROSS2(t,tprev);
    
    /* Left and right edge points */
    SET2V(l1, (*p1)); ADD2V(l1, t);
    SET2V(r1, (*p1)); SUB2V(r1, t);
    SET2V(l2, (*p2)); ADD2V(l2, t);
    SET2V(r2, (*p2)); SUB2V(r2, t);
    
    /* Check if join needed */
    if ((segend || (loop && close)) && dashOn) {
      
      switch (c->strokeJoinStyle) {
      case VG_JOIN_ROUND:
        
        /* Add a round join to stroke */
        if (cross >= 0.0f)
          shStrokeJoinRound(p, p1, &lprev, &tprev, &l1, &t);
        else{
          SHVector2 _t, _tprev;
          SET2(_t, -t.x, -t.y);
          SET2(_tprev, -tprev.x, -tprev.y);
          shStrokeJoinRound(p, p1,  &r1, &_t, &rprev, &_tprev);
        }
        
        break;
      case VG_JOIN_MITER:
        
        /* Add a miter join to stroke */
        mlength = 1/SH_COS((ANGLE2(t, tprev))/2);
        if (mlength <= mlimit) {
          if (cross > 0.0f)
            shStrokeJoinMiter(p, p1, &lprev, &dprev, &l1, &d);
          else if (cross < 0.0f)
            shStrokeJoinMiter(p, p1, &rprev, &dprev, &r1, &d);
          break;
        }/* Else fall down to bevel */
        
      case VG_JOIN_BEVEL:
        
        /* Add a bevel join to stroke */
        if (cross > 0.0f)
          shPushStrokeTri(p, &l1, &lprev, p1);
        else if (cross < 0.0f)
          shPushStrokeTri(p, &r1, &rprev, p1);
        
        break;
      }
    }else if (!start && !loop && dashOn) {
      
      /* Fill gap with previous (= bevel join) */
      if (cross > 0.0f)
        shPushStrokeTri(p, &l1, &lprev, p1);
      else if (cross < 0.0f)
        shPushStrokeTri(p, &r1, &rprev, p1);
    }
    
    
    /* Apply cap to start of a non-closed contour or
       if we are dashing and dash segment is on */
    if ((dashSize == 0 && loop && !close) ||
        (dashSize > 0 && start && dashOn)) {
      switch (c->strokeCapStyle) {
      case VG_CAP_ROUND:
        shStrokeCapRound(p, p1, &t, 1); break;
      case VG_CAP_SQUARE:
        shStrokeCapSquare(p, p1, &t, 1); break;
      default: break;
      }
    }
    
    if (loop)
      continue;
    
    /* Handle dashing */
    if (dashSize > 0) {
      
      /* Start with beginning of subdiv segment */
      SET2V(dash1, (*p1)); SET2V(dashL1, l1); SET2V(dashR1, r1);
      
      do {
        /* Interpolate point on the current subdiv segment */
        nextDashLength = dashLength + dashPattern[dashIndex];
        dashOffset = (nextDashLength - strokeLength) / norm;
        if (dashOffset > 1.0f) dashOffset = 1;
        SET2V(dash2, (*p2)); SUB2V(dash2, (*p1));
        MUL2(dash2, dashOffset); ADD2V(dash2, (*p1));
        
        /* Left and right edge points */
        SET2V(dashL2, dash2); ADD2V(dashL2, t);
        SET2V(dashR2, dash2); SUB2V(dashR2, t);
        
        /* Add quad for this dash segment */
        if (dashOn) shPushStrokeQuad(p, &dashL2, &dashL1, &dashR1, &dashR2);

        /* Move to next dash segment if inside this subdiv segment */
        if (nextDashLength <= strokeLength + norm) {
          dashIndex = (dashIndex + 1) % dashSize;
          dashLength = nextDashLength;
          dashOn = !dashOn;
          SET2V(dash1, dash2);
          SET2V(dashL1, dashL2);
          SET2V(dashR1, dashR2);
          
          /* Apply cap to dash segment */
          switch (c->strokeCapStyle) {
          case VG_CAP_ROUND:
            shStrokeCapRound(p, &dash1, &t, dashOn); break;
          case VG_CAP_SQUARE:
            shStrokeCapSquare(p, &dash1, &t, dashOn); break;
          default: break;
          }
        }
        
        /* Consume dash segments until subdiv end met */
      } while (nextDashLength < strokeLength + norm);
      
    }else{
      
      /* Add quad for this line segment */
      shPushStrokeQuad(p, &l2, &l1, &r1, &r2);
    }
    
    
    /* Apply cap to end of a non-closed contour or
       if we are dashing and dash segment is on */
    if ((dashSize == 0 && end && !close) ||
        (dashSize > 0 && end && dashOn)) {
      switch (c->strokeCapStyle) {
      case VG_CAP_ROUND:
        shStrokeCapRound(p, p2, &t, 0); break;
      case VG_CAP_SQUARE:
        shStrokeCapSquare(p, p2, &t, 0); break;
      default: break;
      }
    }
    
    /* Save previous edge */
    strokeLength += norm;
    SET2V(lprev, l2);
    SET2V(rprev, r2);
    dprev = d;
    tprev = t;
  }
}


/*-------------------------------------------------------------
 * Transforms the tessellation vertices using the given matrix
 *-------------------------------------------------------------*/

void shTransformVertices(SHMatrix3x3 *m, SHPath *p)
{
  SHVector2 *v;
  int i = 0;
  
  for (i = p->vertices.size-1; i>=0; --i) {
    v = (&p->vertices.items[i].point);
    TRANSFORM2((*v), (*m)); }
}

/*--------------------------------------------------------
 * Finds the tight bounding box of path's tesselation
 * vertices. Depends on whether the path had been
 * tesselated in user or surface space.
 *--------------------------------------------------------*/

void shFindBoundbox(SHPath *p)
{
  int i;
  
  if (p->vertices.size == 0) {
    SET2(p->min, 0,0);
    SET2(p->max, 0,0);
    return;
  }
  
  p->min.x = p->max.x = p->vertices.items[0].point.x;
  p->min.y = p->max.y = p->vertices.items[0].point.y;
  
  for (i=0; i<p->vertices.size; ++i) {
    
    SHVector2 *v = &p->vertices.items[i].point;
    if (v->x < p->min.x) p->min.x = v->x;
    if (v->x > p->max.x) p->max.x = v->x;
    if (v->y < p->min.y) p->min.y = v->y;
    if (v->y > p->max.y) p->max.y = v->y;
  }
}

/*--------------------------------------------------------
 * Outputs a tight bounding box of a path in path's own
 * coordinate system.
 *--------------------------------------------------------*/

VG_API_CALL void vgPathBounds(VGPath path,
                              VGfloat * minX, VGfloat * minY,
                              VGfloat * width, VGfloat * height)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);

  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

  VG_RETURN_ERR_IF(minX == NULL || minY == NULL ||
                   width == NULL || height == NULL,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* TODO: check output pointer alignment */

  p = (SHPath*)path;
  VG_RETURN_ERR_IF(!(p->caps & VG_PATH_CAPABILITY_PATH_BOUNDS),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);

  /* Update path geometry */
  shFlattenPath(p, 0);
  shFindBoundbox(p);

  /* Output bounds */
  *minX = p->min.x;
  *minY = p->min.y;
  *width = p->max.x - p->min.x;
  *height = p->max.y - p->min.y;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------
 * Outputs a bounding box of a path defined by its control
 * points that is guaranteed to enclose the path geometry
 * after applying the current path-user-to-surface transform
 *------------------------------------------------------------*/

VG_API_CALL void vgPathTransformedBounds(VGPath path,
                                         VGfloat * minX, VGfloat * minY,
                                         VGfloat * width, VGfloat * height)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);

  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

  VG_RETURN_ERR_IF(minX == NULL || minY == NULL ||
                   width == NULL || height == NULL,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* TODO: check output pointer alignment */

  p = (SHPath*)path;
  VG_RETURN_ERR_IF(!(p->caps & VG_PATH_CAPABILITY_PATH_BOUNDS),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);

  /* Update path geometry */
  shFlattenPath(p, 1);
  shFindBoundbox(p);

  /* Output bounds */
  *minX = p->min.x;
  *minY = p->min.y;
  *width = p->max.x - p->min.x;
  *height = p->max.y - p->min.y;

  /* Invalidate subdivision for rendering */
  p->cacheDataValid = VG_FALSE;
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGfloat vgPathLength(VGPath path,
                                 VGint startSegment, VGint numSegments)
{
  return 0.0f;
}

VG_API_CALL void vgPointAlongPath(VGPath path,
                                  VGint startSegment, VGint numSegments,
                                  VGfloat distance,
                                  VGfloat * x, VGfloat * y,
                                  VGfloat * tangentX, VGfloat * tangentY)
{
}

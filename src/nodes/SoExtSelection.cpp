/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation. See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin for applications not compatible with the
 *  LGPL, please contact SIM to acquire a Professional Edition license.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

/*!
  \class SoExtSelection SoExtSelection.h Inventor/nodes/SoExtSelection.h
  \brief The SoExtSelection class can be used for extended selection functionality.
  \ingroup nodes

  This class enables you to select geometry by specifying a lasso (a
  polygon) or a rectangle on screen. When objects are selected, you'll
  receive the same callbacks as for the SoSelection node.
*/

#include <Inventor/nodes/SoExtSelection.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <coindefs.h> // COIN_STUB()

#include <Inventor/events/SoEvent.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SbTime.h>

#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoCamera.h>

#include <Inventor/SbBox3f.h>
#include <Inventor/SbBox2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/lists/SoCallbackList.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/SbMatrix.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <GL/gl.h>
#include <float.h>


/*!
  \enum SoExtSelection::LassoType
  Enum for type of lasso selection.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::NOLASSO
  Makes this node behave like a normal SoSelection node.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::LASSO
  Selecti objects using a lasso.
*/
/*!
  \var SoExtSelection::LassoType SoExtSelection::RECTANGLE
  Select objects using a rectangle.
*/

/*!
  \enum SoExtSelection::LassoPolicy
  Enum for specifying how objects are selected.
*/

/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::FULL_BBOX
  The entire bounding box must be inside the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::PART_BBOX
  Some part of the bounding box must intersect the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::FULL
  All primitives must be completely inside the lasso/rectangle.
*/
/*!
  \var SoExtSelection::LassoPolicy SoExtSelection::PART
  Some primitive must intersect the lasso/rectangle.
*/


/*!
  \var SoSFEnum SoExtSelection::lassoType
  Field for lasso type. Default value is NOLASSO.
*/
/*!
  \var SoSFEnum SoExtSelection::lassoPolicy
  Field for lasso policy. Default value is FULL_BBOX.
*/

// *************************************************************************

#ifndef DOXYGEN_SKIP_THIS

class SoExtSelectionP {
public:
  SoExtSelectionP(SoExtSelection * master) {
    this->master = master;
  }

  SbColor lassocolor;
  float lassowidth;
  SbBool lassopatternanimate;
  unsigned short lassopattern;

  enum SelectionState {
    NONE,
    RECTANGLE,
    LASSO
  };

  SelectionState selectionstate;
  SbBool isDragging;  // 0=no, 1=currently dragging a new point (mouse = last pos)

  SbList <SbVec2s> coords;
  SoTimerSensor * timersensor;
  SoCallbackAction * cbaction;

  const SbList <SbVec2s> & getCoords(void) const {
    return coords;
  }

  SoExtSelection * master;
  SbViewportRegion curvp;

  static void timercallback(void *data, SoSensor *sensor);
  static SoCallbackAction::Response preShapeCallback(void *data,
                                                     SoCallbackAction *action,
                                                     const SoNode *node);
  static SoCallbackAction::Response postShapeCallback(void *data,
                                                      SoCallbackAction *action,
                                                      const SoNode *node);

  static SoCallbackAction::Response cameraCB(void * data,
                                             SoCallbackAction * action,
                                             const SoNode * node);

  SoCallbackAction::Response testShape(SoCallbackAction * action, const SoShape * shape);

  SoCallbackAction::Response testBBox(SoCallbackAction * action,
                                      const SbMatrix & projmatrix,
                                      const SoShape * shape,
                                      const SbBox2s & lassorect,
                                      const SbBool full);

  SoCallbackAction::Response testPrimitives(SoCallbackAction * action,
                                            const SbMatrix & projmatrix,
                                            const SoShape * shape,
                                            const SbBox2s & lassorect,
                                            const SbBool full);

  static void triangleCB(void *userData,
                         SoCallbackAction *action,
                         const SoPrimitiveVertex *v1,
                         const SoPrimitiveVertex *v2,
                         const SoPrimitiveVertex *v3);
  static void lineSegmentCB(void *userData,
                            SoCallbackAction *action,
                            const SoPrimitiveVertex *v1,
                            const SoPrimitiveVertex *v2);
  static void pointCB(void *userData,
                      SoCallbackAction *action,
                      const SoPrimitiveVertex *v);

  struct {
    SbMatrix projmatrix;
    SbBool fulltest;
    SbBox2s lassorect;
    SbBool hit;
    SbBool allhit;
    SbVec2s vporg;
    SbVec2s vpsize;
    SbBool abort;
  } primcbdata;

  void doSelect(const SoPath * path);
  SoLassoSelectionFilterCB * filterCB;
  void * filterCBData;
  SbBool callfiltercbonlyifselectable;
};

#endif // DOXYGEN_SKIP_THIS


//
// Fast line intersection by Mukesh Prasad, from Graphics Gems II.
//
static SbBool
linelineintersect(const SbVec2s &p00, const SbVec2s & p01,
                  const SbVec2s &p10, const SbVec2s & p11)
{
#define DO_INTERSECT TRUE
#define DONT_INTERSECT FALSE
#define COLINEAR TRUE

#define SAME_SIGNS( a, b )	\
		(((long) ((unsigned long) a ^ (unsigned long) b)) >= 0 )

  int x1 = p00[0];
  int y1 = p00[1];

  int x2 = p01[0];
  int y2 = p01[1];

  int x3 = p10[0];
  int y3 = p10[1];

  int x4 = p11[0];
  int y4 = p11[1];

  int a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
  int r1, r2, r3, r4;         /* 'Sign' values */
  int denom;
#if 0 // not used
  int offset, num;     /* Intermediate values */
#endif

  /* Compute a1, b1, c1, where line joining points 1 and 2
   * is "a1 x  +  b1 y  +  c1  =  0".
   */

  a1 = y2 - y1;
  b1 = x1 - x2;
  c1 = x2 * y1 - x1 * y2;

  /* Compute r3 and r4.
   */

  r3 = a1 * x3 + b1 * y3 + c1;
  r4 = a1 * x4 + b1 * y4 + c1;

  /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
   * same side of line 1, the line segments do not intersect.
   */

  if ( r3 != 0 &&
       r4 != 0 &&
       SAME_SIGNS( r3, r4 ))
    return ( DONT_INTERSECT );

  /* Compute a2, b2, c2 */

  a2 = y4 - y3;
  b2 = x3 - x4;
  c2 = x4 * y3 - x3 * y4;

  /* Compute r1 and r2 */

  r1 = a2 * x1 + b2 * y1 + c2;
  r2 = a2 * x2 + b2 * y2 + c2;

  /* Check signs of r1 and r2.  If both point 1 and point 2 lie
   * on same side of second line segment, the line segments do
   * not intersect.
   */

  if ( r1 != 0 &&
       r2 != 0 &&
       SAME_SIGNS( r1, r2 ))
    return ( DONT_INTERSECT );

  /* Line segments intersect: compute intersection point.
   */

  denom = a1 * b2 - a2 * b1;
  if ( denom == 0 )
    return ( COLINEAR );
#if 0 // we don't need the intersection point

  offset = denom < 0 ? - denom / 2 : denom / 2;

  /* The denom/2 is to get rounding instead of truncating.  It
   * is added or subtracted to the numerator, depending upon the
   * sign of the numerator.
   */
  num = b1 * c2 - b2 * c1;
  *x = ( num < 0 ? num - offset : num + offset ) / denom;

  num = a2 * c1 - a1 * c2;
  *y = ( num < 0 ? num - offset : num + offset ) / denom;
#endif // disabled code

  return ( DO_INTERSECT );
#undef SAME_SIGN
#undef DO_INTERSECT
#undef DONT_INTERSECT
#undef COLINEAR
}


// The following code is by Randolph Franklin,
// it returns 1 for interior points and 0 for exterior points.
// http://astronomy.swin.edu.au/pbourke/geometry/insidepoly/

static SbBool
pointinpoly(const SbList <SbVec2s> & coords, const SbVec2s & point)
{
  int i, j;
  SbBool c = FALSE;
  int npol = coords.getLength();
  float x = (float) point[0];
  float y = (float) point[1];
  SbVec2f pi, pj;

  for (i = 0, j = npol-1; i < npol; j = i++) {

    pi[0] = (float) coords[i][0];
    pi[1] = (float) coords[i][1];
    pj[0] = (float) coords[j][0];
    pj[1] = (float) coords[j][1];

    if ((((pi[1] <= y) && (y < pj[1])) ||
	 ((pj[1] <= y) && (y < pi[1]))) &&
	(x < (pj[0] - pi[0]) * (y - pi[1]) / (pj[1] - pi[1]) + pi[0]))
      c = !c;
  }
  return c;
}

// do a bbox rejection test before calling this method. It's not fast,
// but testing will usually (always) be done on polygon vs triangle in
// which case it should be pretty fast.
static SbBool
polypolyintersect(const SbList <SbVec2s> & poly1,
                  const SbList <SbVec2s> & poly2)
{
  int i;
  int n1 = poly1.getLength();
  int n2 = poly2.getLength();

  if (n1 < n2) {
    for (i = 0; i < n1; i++) {
      if (pointinpoly(poly2, poly1[i])) return TRUE;
    }
    for (i = 0; i < n2; i++) {
      if (pointinpoly(poly1, poly2[i])) return TRUE;
    }
  }
  else {
    for (i = 0; i < n2; i++) {
      if (pointinpoly(poly1, poly2[i])) return TRUE;
    }
    for (i = 0; i < n1; i++) {
      if (pointinpoly(poly2, poly1[i])) return TRUE;
    }
  }
  // warning O(n^2)
  SbVec2s prev1 = poly1[n1-1];
  for (i = 0; i < n1; i++) {
    SbVec2s prev2 = poly2[n2-1];
    for (int j = 0; j < n2; j++) {
      if (linelineintersect(prev1, poly1[i], prev2, poly2[j])) return TRUE;
      prev2 = poly2[j];
    }
    prev1 = poly1[i];
  }
  return FALSE;
}

static SbBool
polylineintersect(const SbList <SbVec2s> & poly,
                  const SbVec2s & p0,
                  const SbVec2s & p1)
{
  if (pointinpoly(poly, p0)) return TRUE;
  if (pointinpoly(poly, p1)) return TRUE;

  int n = poly.getLength();
  SbVec2s prev = poly[n-1];
  for (int i = 0; i < n; i++) {
    if (linelineintersect(prev, poly[i], p0, p1)) return TRUE;
  }
  return FALSE;
}

// do a bbox rejection test before calling this method
static SbBool
polytriintersect(const SbList <SbVec2s> & poly,
                 const SbVec2s & v0,
                 const SbVec2s & v1,
                 const SbVec2s & v2)
{
  SbList <SbVec2s> poly2;
  poly2.append(v0);
  poly2.append(v1);
  poly2.append(v2);
  return polypolyintersect(poly, poly2);
}

// only used by polyprojboxintersect()
static SbBool
test_quad_intersect(const SbList <SbVec2s> & poly,
                    const SbVec2s & p0,
                    const SbVec2s & p1,
                    const SbVec2s & p2,
                    const SbVec2s & p3)
{
  // test if front facing:
  SbVec2s v0 = p1-p0;
  SbVec2s v1 = p3-p0;
  int crossz = v0[0]*v1[1] - v0[1]*v1[0];
  if (crossz > 0) {
    SbList <SbVec2s> poly2;
    poly2.append(p0);
    poly2.append(p1);
    poly2.append(p2);
    poly2.append(p3);
    return polypolyintersect(poly, poly2);
  }
  return FALSE;
}

// do a bbox rejection test before calling this method
static SbBool
polyprojboxintersect(const SbList <SbVec2s> & poly,
                     const SbVec2s * projpts)
{
  // test all size quads in the box
  if (test_quad_intersect(poly, projpts[0], projpts[1],
                          projpts[3], projpts[2])) return TRUE;
  if (test_quad_intersect(poly, projpts[1], projpts[5],
                          projpts[7], projpts[3])) return TRUE;
  if (test_quad_intersect(poly, projpts[2], projpts[3],
                          projpts[7], projpts[6])) return TRUE;
  if (test_quad_intersect(poly, projpts[4], projpts[0],
                          projpts[2], projpts[6])) return TRUE;
  if (test_quad_intersect(poly, projpts[4], projpts[5],
                          projpts[1], projpts[0])) return TRUE;
  if (test_quad_intersect(poly, projpts[6], projpts[7],
                          projpts[5], projpts[4])) return TRUE;

  return FALSE;
}

#undef THIS
#define THIS this->pimpl

SO_NODE_SOURCE(SoExtSelection);

/*!
  Constructor.
*/
SoExtSelection::SoExtSelection(void)
{
  THIS = new SoExtSelectionP(this);

  SO_NODE_INTERNAL_CONSTRUCTOR(SoExtSelection);

  SO_NODE_ADD_FIELD(lassoType, (SoExtSelection::NOLASSO));
  SO_NODE_ADD_FIELD(lassoPolicy, (SoExtSelection::FULL_BBOX));

  SO_NODE_DEFINE_ENUM_VALUE(LassoType, NOLASSO);
  SO_NODE_DEFINE_ENUM_VALUE(LassoType, LASSO);
  SO_NODE_DEFINE_ENUM_VALUE(LassoType, RECTANGLE);
  SO_NODE_SET_SF_ENUM_TYPE(lassoType, LassoType);

  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, FULL_BBOX);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, PART_BBOX);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, FULL);
  SO_NODE_DEFINE_ENUM_VALUE(LassoPolicy, PART);
  SO_NODE_SET_SF_ENUM_TYPE(lassoPolicy, LassoPolicy);

  // setup timer
  THIS->timersensor = new SoTimerSensor(&SoExtSelectionP::timercallback,
                                        (void *)this);
  THIS->timersensor->setBaseTime(SbTime(0.0));
  THIS->timersensor->setInterval(SbTime(0.3));

  THIS->cbaction = new SoCallbackAction();
  THIS->cbaction->addPreCallback(SoShape::getClassTypeId(), SoExtSelectionP::preShapeCallback,
                                 (void *) this);
  THIS->cbaction->addPostCallback(SoShape::getClassTypeId(), SoExtSelectionP::postShapeCallback,
                                  (void *) this);
  THIS->cbaction->addTriangleCallback(SoShape::getClassTypeId(), SoExtSelectionP::triangleCB,
                                      (void*) this);
  THIS->cbaction->addLineSegmentCallback(SoShape::getClassTypeId(), SoExtSelectionP::lineSegmentCB,
                                         (void*) this);
  THIS->cbaction->addPointCallback(SoShape::getClassTypeId(), SoExtSelectionP::pointCB,
                                   (void*) this);
  THIS->cbaction->addPostCallback(SoCamera::getClassTypeId(), SoExtSelectionP::cameraCB,
                                  (void *) this);

  // some init (just to be sure?)
  THIS->lassocolor = SbColor(1.0f, 1.0f, 1.0f);
  THIS->lassowidth = 1.0f;
  THIS->lassopatternanimate = TRUE;
  THIS->lassopattern = 0xf0f0;

  THIS->selectionstate = SoExtSelectionP::NONE;
  THIS->isDragging = FALSE;
  THIS->coords.truncate(0);

  THIS->filterCB = NULL;
}

/*!
  Destructor.
*/
SoExtSelection::~SoExtSelection()
{
  if (THIS->timersensor->isScheduled()) THIS->timersensor->unschedule();
  delete THIS->timersensor;
  delete THIS->cbaction;
  delete THIS;
}

// doc in parent
void
SoExtSelection::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoExtSelection);
}


/*!
  Specifies whether the overlay planes should be used to render the
  lasso.  Not supporte in Coin yet, as overlay planes are
  hard to come by these days.
*/
void
SoExtSelection::useOverlay(const SbBool /* overlay */)
{
  COIN_STUB();
}

/*!
  Returns whether overlay planes are used to draw the lasso.

  \sa useOverlay().
*/
SbBool
SoExtSelection::isUsingOverlay(void)
{
  COIN_STUB();
  return FALSE;
}

/*!
  Returns the scene graph for overlay rendering. Will always return NULL
  in Coin.
*/
SoSeparator *
SoExtSelection::getOverlaySceneGraph(void)
{
  COIN_STUB();
  return NULL;
}

/*!
  Set the color for the overlay lasso. Not supported in Coin.
*/
void
SoExtSelection::setOverlayLassoColorIndex(const int /* index */)
{
  COIN_STUB();
}

/*!
  Returns the overlay lasso color index.

  \sa setOverlayLassoColorIndex().
*/
int
SoExtSelection::getOverlayLassoColorIndex(void)
{
  COIN_STUB();
  return 0;
}

/*!
  Sets the lasso/rectangle line color. Default value 
  is (1.0, 1.0, 1.0).
*/
void
SoExtSelection::setLassoColor(const SbColor & color)
{
  THIS->lassocolor = color;
}

/*!
  Returns the lasso color.
*/
const SbColor &
SoExtSelection::getLassoColor(void)
{
  return THIS->lassocolor;
}

/*!
  Sets the lasso line width. Default value is 1.0.
*/
void
SoExtSelection::setLassoWidth(const float width)
{
  THIS->lassowidth = width;
}

/*!
  Returns the lasso line width.
*/
float
SoExtSelection::getLassoWidth(void)
{
  return THIS->lassowidth;
}

/*!
  Sets the lasso line pattern. Default value is 0xf0f0.
*/
void
SoExtSelection::setOverlayLassoPattern(const unsigned short pattern)
{
  THIS->lassopattern = pattern;
}

/*!
  Returns the lasso line pattern.
*/
unsigned short
SoExtSelection::getOverlayLassoPattern(void)
{
  return THIS->lassopattern;
}

/*!
  Sets whether the lasso should be animated by scrolling
  the line pattern.
*/
void
SoExtSelection::animateOverlayLasso(const SbBool animate)
{
  THIS->lassopatternanimate = animate;
}

/*!
  Returns whether the lasso is set to animate or not.
*/
SbBool
SoExtSelection::isOverlayLassoAnimated(void)
{
  return THIS->lassopatternanimate;
}

/*!
  Overloaded to handle lasso selection.
*/
void
SoExtSelection::handleEvent(SoHandleEventAction * action)
{
  if (this->lassoType.getValue() == NOLASSO) {
    inherited::handleEvent(action);
    return;
  }
  SoSeparator::handleEvent(action);
  if (action->isHandled()) return;

  const SoEvent *event = action->getEvent();
  const SbVec2s mousecoords = event->getPosition();

  switch (this->lassoType.getValue()) {

    // ---------- NO LASSO ----------

  case SoExtSelection::NOLASSO:
    // nothing to do here..
    break;

    // ---------- RECTANGLE ----------

  case SoExtSelection::RECTANGLE:
    // mouse click
    if (SO_MOUSE_PRESS_EVENT(event,BUTTON1)) {
      THIS->isDragging = TRUE;
      THIS->selectionstate = SoExtSelectionP::RECTANGLE;
      THIS->coords.truncate(0);
      THIS->coords.append(mousecoords);
      THIS->coords.append(mousecoords);
      if (!THIS->timersensor->isScheduled()) THIS->timersensor->schedule();
    }
    // mouse release
    else if (SO_MOUSE_RELEASE_EVENT(event,BUTTON1)) {
      THIS->timersensor->unschedule();
      THIS->isDragging = FALSE;
      THIS->selectionstate = SoExtSelectionP::NONE;
      THIS->curvp = action->getViewportRegion();
      THIS->cbaction->setViewportRegion(THIS->curvp);

      this->deselectAll();
      THIS->cbaction->apply(action->getCurPath()->getHead());
      this->touch();
    }
    // mouse move
    else if ((event->isOfType( SoLocation2Event::getClassTypeId()))) {
      if (THIS->isDragging == TRUE) {
	assert(THIS->coords.getLength() >= 2);
	THIS->coords[1] = mousecoords;
	this->touch();
      }
    }

    break;

    // ---------- LASSO ----------

  case SoExtSelection::LASSO:
    // mouse click
    if (SO_MOUSE_PRESS_EVENT(event,BUTTON1)) {
      if (event->wasShiftDown()) {
	if (THIS->selectionstate == SoExtSelectionP::NONE) {
	  THIS->coords.truncate(0);
	  THIS->coords.append(mousecoords);
	  THIS->selectionstate = SoExtSelectionP::LASSO;
	}
	THIS->isDragging = TRUE;
	THIS->coords.append(mousecoords);
        if (!THIS->timersensor->isScheduled()) THIS->timersensor->schedule();
	this->touch();
      }
    }
    // mouse release
    else if (SO_MOUSE_RELEASE_EVENT(event,BUTTON1)) {
      THIS->isDragging = FALSE;
    }
    // mouse move
    else if ( ( event->isOfType( SoLocation2Event::getClassTypeId() ) ) ) {
      if (THIS->isDragging == TRUE) {
	assert(THIS->coords.getLength());
	THIS->coords[THIS->coords.getLength()-1] = mousecoords;
	this->touch();
      }
    }
    // SHIFT press
    else if (SO_KEY_PRESS_EVENT(event,LEFT_SHIFT)) {
    }
    // SHIFT release
    else if (SO_KEY_RELEASE_EVENT(event,LEFT_SHIFT)) {
      THIS->timersensor->unschedule();
      if (THIS->selectionstate == SoExtSelectionP::LASSO) {
	THIS->curvp = action->getViewportRegion();
	THIS->cbaction->setViewportRegion(THIS->curvp);

        this->deselectAll();
	THIS->cbaction->apply(action->getCurPath()->getHead());
	this->touch();
      }
      THIS->isDragging = FALSE;
      THIS->selectionstate = SoExtSelectionP::NONE;
      THIS->coords.truncate(0);
    }
    break;
  }
}

// internal method for drawing lasso
void
SoExtSelection::draw(SoGLRenderAction *action)
{
  SbViewportRegion vp = action->getViewportRegion();
  SbVec2s vpo = vp.getViewportOriginPixels();
  SbVec2s vps = vp.getViewportSizePixels();

  // matrices
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(vpo[0], vpo[0]+vps[0]-1,
	  vpo[1], vpo[0]+vps[1]-1,
	  -1, 1);

  // attributes
  glPushAttrib(GL_LIGHTING_BIT|
	       GL_FOG_BIT|
	       GL_DEPTH_BUFFER_BIT|
	       GL_TEXTURE_BIT|
	       GL_LINE_BIT|
	       GL_CURRENT_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_FOG);
  glDisable(GL_DEPTH_TEST);

  // line color & width
  glColor3f(THIS->lassocolor[0],THIS->lassocolor[1],THIS->lassocolor[2]);
  glLineWidth(THIS->lassowidth);

  // stipple pattern
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, THIS->lassopattern);

  // --- RECTANGLE ---

  if (THIS->selectionstate == SoExtSelectionP::RECTANGLE) {
    assert(THIS->coords.getLength() >= 2);
    SbVec2s c1 = THIS->coords[0];
    SbVec2s c2 = THIS->coords[1];
    glBegin(GL_LINE_LOOP);
    glVertex2s(c1[0], c1[1]);
    glVertex2s(c2[0], c1[1]);
    glVertex2s(c2[0], c2[1]);
    glVertex2s(c1[0], c2[1]);
    glEnd();
  }

  // --- LASSO ---

  else if (THIS->selectionstate == SoExtSelectionP::LASSO) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < THIS->coords.getLength(); i++) {
      SbVec2s temp = THIS->coords[i];
      glVertex2s(temp[0],temp[1]);
    }
    glEnd();
  }

  // finish - restore state
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
}

/*!
  Overloaded to render lasso.
*/
void
SoExtSelection::GLRenderBelowPath(SoGLRenderAction * action)
{
  inherited::GLRenderBelowPath(action);
  SoState *state = action->getState();
  state->push();

  if (action->isRenderingDelayedPaths()) {

    SbViewportRegion vp = SoViewportRegionElement::get(state);
    SbVec2s vpo = vp.getViewportOriginPixels();
    SbVec2s vps = vp.getViewportSizePixels();
    this->draw(action);
  }
  // render this path after all other (delayed)
  else {
    if (THIS->selectionstate != SoExtSelectionP::NONE) {
      action->addDelayedPath(action->getCurPath()->copy());
    }
  }
  state->pop();
}

/*!

  The lasso selection filter callback is called when a node is about
  to be selected, and enables the application programmer to return a
  new path to be used when selecting. The new returned path should
  not be ref'd. SoExtSelection will ref() and unref() it.

  To cancel the selection, return NULL from the callback.

  if \a callonlyifselectable is TRUE, the callback will only be
  invoked when the path to the new node pass through the
  SoExtSelection node.
  
  This method is specific to Coin, and is not part of TGS OIV.
*/

void 
SoExtSelection::setLassoFilterCallback(SoLassoSelectionFilterCB * f, void * userdata,
                                       const SbBool callonlyifselectable)
{
  THIS->filterCB = f;
  THIS->filterCBData = userdata;
  THIS->callfiltercbonlyifselectable = callonlyifselectable;
}

#undef THIS

#ifndef DOXYGEN_SKIP_THIS

void
SoExtSelectionP::timercallback(void *data, SoSensor *sensor)
{
  SoExtSelection *ext = (SoExtSelection *)data;
  if (ext == NULL) return;
  if (ext->isOverlayLassoAnimated()) {
    int pat = ext->getOverlayLassoPattern();
    int pat2 = pat << 1;
    if ((pat & 0x8000) != 0) pat2 |= 1;
    ext->setOverlayLassoPattern(pat2 & 0xffff);
    ext->touch();
  }
}

SoCallbackAction::Response
SoExtSelectionP::preShapeCallback(void *data, SoCallbackAction *action, const SoNode *node)
{
  SoExtSelection * ext = (SoExtSelection*)data;
  assert(node->isOfType(SoShape::getClassTypeId()));
  return ext->pimpl->testShape(action, (const SoShape*) node);
}

SoCallbackAction::Response
SoExtSelectionP::postShapeCallback(void *data, SoCallbackAction *action, const SoNode *node)
{
  SoExtSelection * ext = (SoExtSelection*)data;
  SbBool hit = FALSE;
  switch (ext->lassoPolicy.getValue()) {
  case SoExtSelection::FULL:
    hit = ext->pimpl->primcbdata.allhit;
    break;
  case SoExtSelection::PART:
    hit = ext->pimpl->primcbdata.hit;
    break;
  default:
    break;
  }
  if (hit) ext->pimpl->doSelect(action->getCurPath());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoExtSelectionP::cameraCB(void * data,
                          SoCallbackAction * action,
                          const SoNode * node)
{
  SoExtSelection * thisp = (SoExtSelection*) data;

  SoState * state = action->getState();
  SbViewVolume vv = SoViewVolumeElement::get(state);
  const SbViewportRegion & vp = SoViewportRegionElement::get(state);

  SbBox2s rectbbox;
  for (int i = 0; i < thisp->pimpl->coords.getLength(); i++) {
    rectbbox.extendBy(thisp->pimpl->coords[i]);
  }

  SbVec2s org = vp.getViewportOriginPixels();
  SbVec2s siz = vp.getViewportSizePixels();
  float left = float(rectbbox.getMin()[0] - org[0]) / float(siz[0]);
  float bottom = float(rectbbox.getMin()[1] - org[1]) / float(siz[1]);

  float right = float(rectbbox.getMax()[0] - org[0]) / float(siz[0]);
  float top = float(rectbbox.getMax()[1] - org[1]) / float(siz[1]);

  // increment to avoid empty view volume
  if (left == right) right += 0.001f;
  if (top == bottom) top += 0.001f;

  vv = vv.narrow(left, bottom, right, top);
  SoCullElement::setViewVolume(state, vv);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoExtSelectionP::testShape(SoCallbackAction * action, const SoShape * shape)
{
  int i;
  SoState * state = action->getState();

  SbBox2s rectbbox;
  for (i = 0; i < this->coords.getLength(); i++) {
    rectbbox.extendBy(this->coords[i]);
  }

  SbMatrix projmatrix;
  projmatrix = (SoModelMatrixElement::get(state) *
                SoViewingMatrixElement::get(state) *
                SoProjectionMatrixElement::get(state));

  SbBool full = FALSE;
  switch (this->master->lassoPolicy.getValue()) {
  case SoExtSelection::FULL_BBOX:
    full = TRUE;
  case SoExtSelection::PART_BBOX:
    return testBBox(action, projmatrix, shape, rectbbox, full);
  case SoExtSelection::FULL:
    full = TRUE;
  case SoExtSelection::PART:
    return testPrimitives(action, projmatrix, shape, rectbbox, full);
  default:
    assert(0 && "unknown lasso policy");
    break;
  }
  return SoCallbackAction::CONTINUE;
}

static SbVec2s
project_pt(const SbMatrix & projmatrix, const SbVec3f & v,
           const SbVec2s & vporg, const SbVec2s & vpsize)
{
  SbVec3f normpt;
  projmatrix.multVecMatrix(v, normpt);
  normpt[0] += 1.0f;
  normpt[1] += 1.0f;
  normpt[0] *= 0.5f;
  normpt[1] *= 0.5f;

  normpt[0] *= (float) vpsize[0];
  normpt[1] *= (float) vpsize[1];
  normpt[0] += (float) vporg[0];
  normpt[1] += (float) vporg[1];

  return SbVec2s((short) SbClamp(normpt[0], -32768.0f, 32767.0f),
                 (short) SbClamp(normpt[1], -32768.0f, 32767.0f));
}


SoCallbackAction::Response
SoExtSelectionP::testBBox(SoCallbackAction * action,
                          const SbMatrix & projmatrix,
                          const SoShape * shape,
                          const SbBox2s & lassorect,
                          const SbBool full)
{
  SbBox3f bbox;
  SbVec3f center;
  ((SoShape*)shape)->computeBBox(action, bbox, center);

  SbVec3f mincorner = bbox.getMin();
  SbVec3f maxcorner = bbox.getMax();

  SbBox2s shapebbox;

  SbVec2s vppt;
  SbVec3f normpt;
  SbVec2s vpo = this->curvp.getViewportOriginPixels();
  SbVec2s vps = this->curvp.getViewportSizePixels();

  SbVec2s projpts[8];

  for (int i = 0; i < 8; i++) {
    SbVec3f corner(i & 1 ? maxcorner[0] : mincorner[0],
                   i & 2 ? maxcorner[1] : mincorner[1],
                   i & 4 ? maxcorner[2] : mincorner[2]);
    vppt = project_pt(projmatrix, corner, vpo, vps);
    projpts[i] = vppt;
    shapebbox.extendBy(vppt);
  }
  if (lassorect.intersect(shapebbox)) { // quick reject
    int i;
    int hit = 0;
    switch (this->master->lassoType.getValue()) {
    case SoExtSelection::LASSO:
      if (full) {
        for (i = 0; i < 8; i++) {
          if (!pointinpoly(this->coords, projpts[i])) break;
        }
        if (i == 8) hit = TRUE;
      }
      else {
        hit = polyprojboxintersect(this->coords, projpts);
      }
      break;
    case SoExtSelection::RECTANGLE:
      if (full) {
        for (i = 0; i < 8; i++) {
          if (!lassorect.intersect(projpts[i])) break;
        }
        if (i == 8) hit = TRUE;
      }
      else {
        for (i = 0; i < 8; i++) {
          if (lassorect.intersect(projpts[i])) { hit = TRUE; break; }
        }
      }
      break;
    default:
      break;
    }
    if (hit) this->doSelect(action->getCurPath());
  }
  return SoCallbackAction::PRUNE; // we don't need do callbacks for primitives
}

SoCallbackAction::Response
SoExtSelectionP::testPrimitives(SoCallbackAction * action,
                                const SbMatrix & projmatrix,
                                const SoShape * /* shape */,
                                const SbBox2s & lassorect,
                                const SbBool full)
{
  // FIXME: consider quick reject based on bounding box for now we
  // just initialize some variables, and trust that the user has a
  // sensible scene graph so that shapes are culled in the separators.

  this->primcbdata.abort = FALSE;
  this->primcbdata.fulltest = full;
  this->primcbdata.projmatrix = projmatrix;
  this->primcbdata.lassorect = lassorect;
  this->primcbdata.hit = FALSE;
  this->primcbdata.allhit = TRUE;
  this->primcbdata.vporg = SoViewportRegionElement::get(action->getState()).getViewportOriginPixels();
  this->primcbdata.vpsize = SoViewportRegionElement::get(action->getState()).getViewportSizePixels();

  // signal to callback action that we want to generate primitives for
  // this shape
  return SoCallbackAction::CONTINUE;
}


void
SoExtSelectionP::triangleCB(void * userData,
                            SoCallbackAction * action,
                            const SoPrimitiveVertex * v1,
                            const SoPrimitiveVertex * v2,
                            const SoPrimitiveVertex * v3)
{
  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;
  if (thisp->primcbdata.abort) return;

  if (thisp->primcbdata.fulltest) {
    SbVec2s p = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                           thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    if (!thisp->primcbdata.lassorect.intersect(p) || !pointinpoly(thisp->coords, p)) {
      thisp->primcbdata.abort = TRUE;
      thisp->primcbdata.allhit = FALSE;
      return;
    }

    p = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                   thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    if (!thisp->primcbdata.lassorect.intersect(p) || !pointinpoly(thisp->coords, p)) {
      thisp->primcbdata.abort = TRUE;
      thisp->primcbdata.allhit = FALSE;
      return;
    }
    p = project_pt(thisp->primcbdata.projmatrix, v3->getPoint(),
                   thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    if (!thisp->primcbdata.lassorect.intersect(p) || !pointinpoly(thisp->coords, p)) {
      thisp->primcbdata.abort = TRUE;
      thisp->primcbdata.allhit = FALSE;
      return;
    }
    // FIXME: to be 100% correct, we should check for intersections
    // between the three edges in the triangle and the edges in the lasso.
    // this is not too difficult, but might be pretty slow...
  }
  else {
    SbVec2s p0 = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                            thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    SbVec2s p1 = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                            thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    SbVec2s p2 = project_pt(thisp->primcbdata.projmatrix, v3->getPoint(),
                            thisp->primcbdata.vporg, thisp->primcbdata.vpsize);

    SbBox2s bbox;
    bbox.extendBy(p0);
    bbox.extendBy(p1);
    bbox.extendBy(p2);
    if (bbox.intersect(thisp->primcbdata.lassorect) &&
        polytriintersect(thisp->coords, p0, p1, p2)) {
      thisp->primcbdata.hit = TRUE;
      thisp->primcbdata.abort = TRUE;
    }
  }
}


void
SoExtSelectionP::lineSegmentCB(void *userData,
                               SoCallbackAction * action,
                               const SoPrimitiveVertex * v1,
                               const SoPrimitiveVertex * v2)
{
  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;
  if (thisp->primcbdata.abort) return;

  if (thisp->primcbdata.fulltest) {
    SbVec2s p = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                           thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    if (!thisp->primcbdata.lassorect.intersect(p) || !pointinpoly(thisp->coords, p)) {
      thisp->primcbdata.abort = TRUE;
      thisp->primcbdata.allhit = FALSE;
      return;
    }

    p = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                   thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    if (!thisp->primcbdata.lassorect.intersect(p) || !pointinpoly(thisp->coords, p)) {
      thisp->primcbdata.abort = TRUE;
      thisp->primcbdata.allhit = FALSE;
      return;
    }
    // FIXME: to be 100% correct, we should check for intersections
    // between line segment and all the edges in the lasso. This is
    // not too difficult, but might be pretty slow...
  }
  else {
    SbVec2s p0 = project_pt(thisp->primcbdata.projmatrix, v1->getPoint(),
                            thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    SbVec2s p1 = project_pt(thisp->primcbdata.projmatrix, v2->getPoint(),
                            thisp->primcbdata.vporg, thisp->primcbdata.vpsize);
    SbBox2s bbox;
    bbox.extendBy(p0);
    bbox.extendBy(p1);
    if (bbox.intersect(thisp->primcbdata.lassorect) &&
        polylineintersect(thisp->coords, p0, p1)) {
      thisp->primcbdata.hit = TRUE;
      thisp->primcbdata.abort = TRUE;
    }
  }
}

void
SoExtSelectionP::pointCB(void *userData,
                         SoCallbackAction *action,
                         const SoPrimitiveVertex * v)
{
  SoExtSelectionP * thisp = ((SoExtSelection*)userData)->pimpl;
  if (thisp->primcbdata.abort) return;
  SbVec2s p = project_pt(thisp->primcbdata.projmatrix, v->getPoint(),
                         thisp->primcbdata.vporg, thisp->primcbdata.vpsize);

  SbBool hit = thisp->primcbdata.lassorect.intersect(p) &&
    pointinpoly(thisp->coords, p);

  if (thisp->primcbdata.fulltest && !hit) {
    thisp->primcbdata.abort = TRUE;
    thisp->primcbdata.allhit = FALSE;
  }
  else if (!thisp->primcbdata.fulltest && hit) {
    thisp->primcbdata.hit = TRUE;
    thisp->primcbdata.abort = TRUE;
  }
}

void
SoExtSelectionP::doSelect(const SoPath * path)
{
  SoPath * newpath = (SoPath*) path;
  
  if (this->filterCB && (!this->callfiltercbonlyifselectable ||
                         path->findNode(this->master) >= 0)) {
    newpath = this->filterCB(this->filterCBData, path);
  }
  if (newpath == NULL) return;
  
  if (newpath != path) newpath->ref();
  
  this->master->startCBList->invokeCallbacks(this->master);
  this->master->invokeSelectionPolicy(newpath, TRUE);
  this->master->finishCBList->invokeCallbacks(this->master);
  
  if (newpath != path) newpath->unref();
}

#endif // DOXYGEN_SKIP_THIS

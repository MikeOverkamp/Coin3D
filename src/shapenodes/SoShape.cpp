/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class SoShape SoShape.h Inventor/nodes/SoShape.h
  \brief The SoShape class is the superclass for geometry shapes.
  \ingroup nodes

  The node types which have actual geometry to render inherits this
  class. For convenince, the SoShape class contains various common
  code used by the subclasses.
*/


#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <coindefs.h> // COIN_OBSOLETED()
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoVertexShape.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/caches/SoPrimitiveVertexCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoBumpMapElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/nodes/SoLight.h>

#include <Inventor/misc/SoGL.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/C/threads/threadsutilp.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/SbClip.h>
#include <Inventor/SbTime.h>
#include <Inventor/C/tidbitsp.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/C/glue/glp.h>

// SoShape.cpp grew too big, so I had to move some code into new
// files. pederb, 2001-07-18
#include "soshape_primdata.h"
#include "soshape_trianglesort.h"
#include "soshape_bigtexture.h"
#include "soshape_bumprender.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97
#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>
#include <Inventor/VRMLnodes/SoVRMLExtrusion.h>
#include <Inventor/VRMLnodes/SoVRMLElevationGrid.h>
#endif // HAVE_VRML97

#include <Inventor/system/gl.h>
#include <string.h>
#include <stdlib.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbStorage.h>
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

/*!
  \enum SoShape::TriangleShape
  \COININTERNAL
*/

/*!
  \fn void SoShape::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)

  Implemented by SoShape subclasses to let the SoShape superclass know
  the exact size and weighted center point of the shape's bounding box.

  The bounding box and center point should be calculated and returned
  in the local coordinate system.

  The method implements action behavior for shape nodes for
  SoGetBoundingBoxAction. It is invoked from
  SoShape::getBoundingBox(). (Subclasses should \e not override
  SoNode::getBoundingBox().)

  The \a box parameter sent in is guaranteed to be an empty box, while
  \a center is undefined upon function entry.
*/

/*!
  \fn void SoShape::generatePrimitives(SoAction * action)

  The method implements action behavior for shape nodes for
  SoCallbackAction. It is invoked from
  SoShape::callback(). (Subclasses should \e not override
  SoNode::callback().)

  The subclass implementations uses the convenience methods
  SoShape::beginShape(), SoShape::shapeVertex(), and
  SoShape::endShape(), with SoDetail instances, to pass the primitives
  making up the shape back to the caller.
*/

// *************************************************************************

class SoShapeP {
public:
  SoShapeP() {
    this->bboxcache = NULL;
    this->pvcache = NULL;
    this->bumprender = NULL;
    this->flags = 0;
  }
  ~SoShapeP() {
    if (this->bboxcache) { this->bboxcache->unref(); }
    if (this->pvcache) { this->pvcache->unref(); }
    delete this->bumprender;
  }
  enum Flags {
    SHOULD_BBOX_CACHE = 0x1,
    NEED_SETUP_SHAPE_HINTS = 0x2,
    DISABLE_VERTEX_ARRAY_CACHE = 0x4,
  };

  static void calibrateBBoxCache(void);
  static double bboxcachetimelimit;
  SoBoundingBoxCache * bboxcache;
  SoPrimitiveVertexCache * pvcache;
  soshape_bumprender * bumprender;
  uint32_t flags;
#ifdef COIN_THREADSAFE
  SbMutex mutex;
#endif // COIN_THREADSAFE
  
  // needed since some VRML97 nodes change the GL state inside the node
  void testSetupShapeHints(SoShape * shape) {
#ifdef HAVE_VRML97
    if ((this->flags & SoShapeP::NEED_SETUP_SHAPE_HINTS) == 0) {
      if (shape->isOfType(SoVRMLIndexedFaceSet::getClassTypeId()) ||
          shape->isOfType(SoVRMLExtrusion::getClassTypeId()) ||
          shape->isOfType(SoVRMLElevationGrid::getClassTypeId())) {
        this->flags |= SoShapeP::NEED_SETUP_SHAPE_HINTS;
      }
    }
#endif // HAVE_VRML97
  }
  void setupShapeHints(SoShape * shape, SoState * state) {
#ifdef HAVE_VRML97
    if (this->flags & SoShapeP::NEED_SETUP_SHAPE_HINTS) {
      SbBool ccw = ((SoSFBool*)(shape->getField("ccw")))->getValue();
      SbBool solid = ((SoSFBool*)(shape->getField("solid")))->getValue();
      SoGLShapeHintsElement::forceSend(state, ccw, solid, !solid);
    }
#endif // HAVE_VRML97
  }
  void lock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.lock();
#endif // COIN_THREADSAFE
  }
  void unlock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.unlock();
#endif // COIN_THREADSAFE
  }
};

double SoShapeP::bboxcachetimelimit;

#undef PRIVATE
#define PRIVATE(_thisp_) ((_thisp_)->pimpl)

// *************************************************************************
// code/structures to handle static and/or thread safe data

typedef struct {
  soshape_primdata * primdata;
  SbList <soshape_bigtexture*> * bigtexturelist;
  SbList <uint32_t> * bigtexturecontext;
  soshape_trianglesort * trianglesort;

  soshape_bigtexture * currentbigtexture;
  // used in generatePrimitives() callbacks to set correct material
  SoMaterialBundle * currentbundle;

  // need these in invokeTriangleCallbacks()
  SbBool is_doing_sorted_rendering;
  SbBool is_doing_bigtexture_rendering;
  SbBool is_doing_pvcache_rendering;
} soshape_staticdata;

static soshape_bigtexture *
soshape_get_bigtexture(soshape_staticdata * data, uint32_t context)
{
  for (int i = 0; i < data->bigtexturecontext->getLength(); i++) {
    if ((*(data->bigtexturecontext))[i] == context) {
      return (*(data->bigtexturelist))[i];
    }
  }
  soshape_bigtexture * newtex = new soshape_bigtexture;
  data->bigtexturelist->append(newtex);
  data->bigtexturecontext->append(context);
  return newtex;
}

static void
soshape_construct_staticdata(void * closure)
{
  soshape_staticdata * data = (soshape_staticdata*) closure;

  data->bigtexturelist = new SbList <soshape_bigtexture*>;
  data->bigtexturecontext = new SbList <uint32_t>;
  data->primdata = new soshape_primdata();
  data->trianglesort = new soshape_trianglesort();
  data->is_doing_sorted_rendering = FALSE;
  data->is_doing_bigtexture_rendering = FALSE;
  data->is_doing_pvcache_rendering = FALSE;
}

static void
soshape_destruct_staticdata(void * closure)
{
  soshape_staticdata * data = (soshape_staticdata*) closure;
  for (int i = 0; i < data->bigtexturelist->getLength(); i++) {
    delete (*(data->bigtexturelist))[i];
  }
  delete data->bigtexturelist;
  delete data->bigtexturecontext;
  delete data->primdata;
  delete data->trianglesort;
}

#ifdef COIN_THREADSAFE
static SbStorage * soshape_staticstorage;
#else // COIN_THREADSAFE
static soshape_staticdata * soshape_single_staticdata;
#endif // ! COIN_THREADSAFE

static soshape_staticdata *
soshape_get_staticdata(void)
{
#ifdef COIN_THREADSAFE
  return (soshape_staticdata*) soshape_staticstorage->get();
#else // COIN_THREADSAFE
  return soshape_single_staticdata;
#endif // !COIN_THREADSAFE
}

// called by atexit
static void
soshape_cleanup(void)
{
#ifdef COIN_THREADSAFE
  delete soshape_staticstorage;
#else // COIN_THREADSAFE
  soshape_destruct_staticdata(soshape_single_staticdata);
  delete soshape_single_staticdata;
#endif // ! COIN_THREADSAFE
}

// *************************************************************************

SO_NODE_ABSTRACT_SOURCE(SoShape);


/*!
  Constructor.
*/
SoShape::SoShape(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShape);
  PRIVATE(this) = new SoShapeP;
}

/*!
  Destructor.
*/
SoShape::~SoShape()
{
  delete PRIVATE(this);
}

static int soshape_use_gl_vertex_arrays = 0;

// Doc in parent.
void
SoShape::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoShape, SO_FROM_INVENTOR_1);

#ifdef COIN_THREADSAFE
  soshape_staticstorage =
    new SbStorage(sizeof(soshape_staticdata),
                  soshape_construct_staticdata,
                  soshape_destruct_staticdata);
#else // COIN_THREADSAFE
  soshape_single_staticdata = new soshape_staticdata;
  soshape_construct_staticdata((void*) soshape_single_staticdata);
#endif // ! COIN_THREADSAFE

  const char * env = coin_getenv("COIN_USE_GL_VERTEX_ARRAYS");
  if (env) {
    soshape_use_gl_vertex_arrays = atoi(env);
  }
  SoShapeP::calibrateBBoxCache();
  coin_atexit((coin_atexit_f*) soshape_cleanup, 0);
}

// Doc in parent.
void
SoShape::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SbBox3f box;
  SbVec3f center;
  this->getBBox(action, box, center);
  if (!box.isEmpty()) {
    action->extendBy(box);
    action->setCenter(center, TRUE);
  }
}

// Doc in parent.
void
SoShape::GLRender(SoGLRenderAction * action)
{
  // if we get here, the shape do not have a render method and
  // generatePrimitives should therefore be used to render the
  // shape. This is probably painfully slow, so if you want speed,
  // implement the GLRender() method.  pederb, 20000612

  if (!this->shouldGLRender(action)) return;
  SoMaterialBundle mb(action);
  mb.sendFirst();
  soshape_get_staticdata()->currentbundle = &mb;  // needed in the primitive callbacks
  this->generatePrimitives(action);
}

// Doc in parent.
void
SoShape::callback(SoCallbackAction * action)
{
  if (action->shouldGeneratePrimitives(this)) {
    soshape_staticdata * shapedata = soshape_get_staticdata();
    shapedata->primdata->faceCounter = 0;
    this->generatePrimitives(action);
  }
}

// test bbox intersection
static SbBool
soshape_ray_intersect(SoRayPickAction * action, const SbBox3f & box)
{
  if (box.isEmpty()) return FALSE;
  return action->intersect(box, TRUE);
}


/*!
  Calculates picked point based on primitives generated by subclasses.
*/
void
SoShape::rayPick(SoRayPickAction * action)
{
  if (this->shouldRayPick(action)) {
    this->computeObjectSpaceRay(action);

    if (!PRIVATE(this)->bboxcache ||
        !PRIVATE(this)->bboxcache->isValid(action->getState()) ||
        soshape_ray_intersect(action, PRIVATE(this)->bboxcache->getProjectedBox())) {
      this->generatePrimitives(action);
    }
  }
}

/*!
  A convenience function that returns the size of a \a boundingbox
  projected onto the screen. Useful for \c SCREEN_SPACE complexity
  geometry.
*/
void
SoShape::getScreenSize(SoState * const state, const SbBox3f & boundingbox,
                       SbVec2s & rectsize)
{
  SbMatrix projmatrix;
  projmatrix = (SoModelMatrixElement::get(state) *
                SoViewingMatrixElement::get(state) *
                SoProjectionMatrixElement::get(state));

  SbVec2s vpsize = SoViewportRegionElement::get(state).getViewportSizePixels();
  SbVec3f bmin, bmax;
  boundingbox.getBounds(bmin, bmax);

  SbVec3f v;
  SbBox2f normbox;
  normbox.makeEmpty();
  for (int i = 0; i < 8; i++) {
    v.setValue(i&1 ? bmin[0] : bmax[0],
               i&2 ? bmin[1] : bmax[1],
               i&4 ? bmin[2] : bmax[2]);
    projmatrix.multVecMatrix(v, v);
    normbox.extendBy(SbVec2f(v[0], v[1]));
  }
  float nx, ny;
  normbox.getSize(nx, ny);

  // restrict size of projection. It is often way off when object
  // intersects the near plane. We should probably do clipping against
  // the view volume do be 100% correct, but that would be too slow.
  // pederb, 2001-05-20
  if (nx > 10.0f) nx = 10.0f;
  if (ny > 10.0f) ny = 10.0f;

  rectsize[0] = (short) SbMin(32767.0f, float(vpsize[0])*0.5f*nx);
  rectsize[1] = (short) SbMin(32767.0f, float(vpsize[1])*0.5f*ny);
}

/*!
  Returns the complexity value to be used by subclasses. Considers
  complexity type. For \c OBJECT_SPACE complexity this will be a
  number between 0 and 1. For \c SCREEN_SPACE complexity it is a
  number from 0 and up.
*/
float
SoShape::getComplexityValue(SoAction * action)
{
  SoState * state = action->getState();
  switch (SoComplexityTypeElement::get(state)) {
  case SoComplexityTypeElement::SCREEN_SPACE:
    {
      SbBox3f box;
      SbVec3f center;
      this->getBBox(action, box, center);
      SbVec2s size;
      SoShape::getScreenSize(state, box, size);
      // FIXME: probably needs calibration.

#if 1 // testing new complexity code
      // The cast within the sqrt() is done to avoid ambigouity error
      // from HPUX aCC, as sqrt() can be either "long double sqrt(long
      // double)" or "float sqrt(float)". mortene.
      return float(sqrt((float)SbMax(size[0], size[1]))) * 0.4f *
        SoComplexityElement::get(state);
#else // first version
      float numPixels = float(size[0])* float(size[1]);
      return numPixels * 0.0001f * SoComplexityElement::get(state);
#endif
    }
  case SoComplexityTypeElement::OBJECT_SPACE:
    return SoComplexityElement::get(state);
  case SoComplexityTypeElement::BOUNDING_BOX:
    // return default value. We might get here when generating
    // primitives, not when rendering.
    return 0.5f;
  default:
    assert(0 && "unknown complexity type");
    return 0.5f;
  }
}

/*!
  \COININTERNAL
*/
SbBool
SoShape::shouldGLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  if (SoDrawStyleElement::get(state) == SoDrawStyleElement::INVISIBLE)
    return FALSE;

  if (!state->isCacheOpen() && !SoCullElement::completelyInside(state)) {
    if (PRIVATE(this)->bboxcache && PRIVATE(this)->bboxcache->isValid(state)) {
      if (SoCullElement::cullTest(state, PRIVATE(this)->bboxcache->getProjectedBox())) {
        return FALSE;
      }
    }
  }

  SbBool transparent = SoTextureImageElement::containsTransparency(state);
  if (!transparent) {
    transparent = SoLazyElement::getInstance(state)->isTransparent();
  }

  if (action->handleTransparency(transparent))
    return FALSE;

  if (SoComplexityTypeElement::get(state) ==
      SoComplexityTypeElement::BOUNDING_BOX) {


    this->GLRenderBoundingBox(action);
    return FALSE;
  }

  // test if we should sort triangles before rendering
  if (transparent &&
      ((action->getTransparencyType() ==
        SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND) ||
       (action->getTransparencyType() ==
        SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_ADD))) {

    soshape_staticdata * shapedata = soshape_get_staticdata();

    // do this before generating triangles to get correct
    // material for lines and point (only triangles are sorted).
    SoMaterialBundle mb(action);
    mb.sendFirst();
    shapedata->currentbundle = &mb;

    shapedata->trianglesort->beginShape(state);

    // when setting this flag, the triangles will be sent to the
    // trianglesort handler in invokeTriangleCallbacks().
    shapedata->is_doing_sorted_rendering = TRUE;
    this->generatePrimitives(action);
    shapedata->is_doing_sorted_rendering = FALSE;

    shapedata->trianglesort->endShape(state, mb); // this will render the triangles
    return FALSE; // tell shape _not_ to render
  }

  SoGLTextureImageElement::Model model;
  SbColor blendcolor;
  SoGLImage * glimage = SoGLTextureImageElement::get(state, model, blendcolor);
  if (glimage &&
      glimage->isOfType(SoGLBigImage::getClassTypeId()) &&
      SoGLTextureEnabledElement::get(state)) {

    soshape_staticdata * shapedata = soshape_get_staticdata();

    // do this before generating triangles to get correct
    // material for lines and point (only triangles are handled for now).
    SoMaterialBundle mb(action);
    mb.sendFirst();
    shapedata->currentbundle = &mb;

    SoGLBigImage * big = (SoGLBigImage*) glimage;

    shapedata->is_doing_bigtexture_rendering = TRUE;

    soshape_bigtexture * bigtex = soshape_get_bigtexture(shapedata, action->getCacheContext());
    shapedata->currentbigtexture = bigtex;
    bigtex->beginShape(big, SoTextureQualityElement::get(state));
    this->generatePrimitives(action);
    // endShape() returns whether more/less detailed textures need to be
    // fetched. We force a redraw if this is needed.
    if (bigtex->endShape(state, this, mb) == FALSE) {
      action->getCurPath()->getHead()->touch();
    }
    shapedata->is_doing_bigtexture_rendering = FALSE;

    return FALSE;
  }

  if (SoBumpMapElement::get(state)) {
    const SoNodeList & lights = SoLightElement::getLights(state);
    if (lights.getLength()) {

      soshape_staticdata * shapedata = soshape_get_staticdata();
      // lock mutex since bumprender and pvcache is shared among all threads
      PRIVATE(this)->lock();
      if (PRIVATE(this)->bumprender == NULL) {
        PRIVATE(this)->bumprender = new soshape_bumprender;
      }
      if (PRIVATE(this)->pvcache == NULL ||
          !PRIVATE(this)->pvcache->isValid(state)) {
        if (PRIVATE(this)->pvcache) {
          PRIVATE(this)->pvcache->unref();
        }
        SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
        // must push state to make cache dependencies work
        state->push();
        PRIVATE(this)->pvcache = new SoPrimitiveVertexCache(state);
        PRIVATE(this)->pvcache->ref();
        SoCacheElement::set(state, PRIVATE(this)->pvcache);
        shapedata->is_doing_pvcache_rendering = TRUE;
        this->generatePrimitives(action);
        shapedata->is_doing_pvcache_rendering = FALSE;
        // this _must_ be called after creating the pvcache
        PRIVATE(this)->bumprender->calcTangentSpace(PRIVATE(this)->pvcache);
        state->pop();
        SoCacheElement::setInvalid(storedinvalid);
        PRIVATE(this)->pvcache->fit();
        PRIVATE(this)->testSetupShapeHints(this);
      }
      if (PRIVATE(this)->pvcache->getNumIndices() == 0) {
        PRIVATE(this)->unlock();
        return TRUE;
      }

      SoGLLazyElement::getInstance(state)->send(state, SoLazyElement::ALL_MASK);

      glPushAttrib(GL_DEPTH_BUFFER_BIT);
      glDepthFunc(GL_LEQUAL);
      glDisable(GL_LIGHTING);
      
      glColor3f(1.0f, 1.0f, 1.0f);      
      PRIVATE(this)->setupShapeHints(this, state);
      const int numlights = lights.getLength();      
      for (int i = 0; i < numlights; i++) {      
        // fetch matrix that convert the light from its object space
        // to the OpenGL world space
        SbMatrix lm = SoLightElement::getMatrix(state, i);

        // convert light back to this objects' object space
        SbMatrix m = SoModelMatrixElement::get(state) *
          SoViewingMatrixElement::get(state);
        m = m.inverse();
        m.multLeft(lm);


        // bumprender is shared among all threads, so the mutex needs to
        // be locked when we get here since some internal arrays are
        // used while rendering
        PRIVATE(this)->bumprender->renderBump(state, PRIVATE(this)->pvcache, 
                                              (SoLight*) lights[i], m);
        
        if (i == 0) glEnable(GL_BLEND);
        if (i == numlights-1) {
          glBlendFunc(GL_DST_COLOR, GL_ZERO);
        }
        else if (i == 0) {
          glBlendFunc(GL_ONE, GL_ONE);
        }
      }
      
      
      SoGLLazyElement::getInstance(state)->reset(state, 
                                                 SoLazyElement::DIFFUSE_MASK |
                                                 SoLazyElement::GLIMAGE_MASK);
      SoMaterialBundle mb(action);
      mb.sendFirst();
      PRIVATE(this)->setupShapeHints(this, state);
      PRIVATE(this)->bumprender->renderNormal(state, PRIVATE(this)->pvcache);
               
      const SbColor spec = SoLazyElement::getSpecular(state); 
      if (spec[0] != 0 || spec[1] != 0 || spec[2] != 0) { // Is the spec. color black?

        const cc_glglue * glue = sogl_glue_instance(state);
        // Can the hardware do specular bump maps?
        if (glue->has_arb_fragment_program && 
            glue->has_arb_vertex_program) {
          
          SoGLLazyElement::getInstance(state)->reset(state, 
                                                     SoLazyElement::DIFFUSE_MASK |
                                                     SoLazyElement::GLIMAGE_MASK);     
          glEnable(GL_BLEND);
          glBlendFunc(GL_ONE, GL_ONE);
          
          for (int i = 0; i < numlights; i++) {      
            SbMatrix lm = SoLightElement::getMatrix(state, i);
            SbMatrix m = SoModelMatrixElement::get(state) *
              SoViewingMatrixElement::get(state);
            m = m.inverse();
            m.multLeft(lm);                   
            PRIVATE(this)->bumprender->renderBumpSpecular(state, PRIVATE(this)->pvcache, 
                                                          (SoLight*) lights[i], m);          
          }
        }
        
      }     
      
      
      PRIVATE(this)->unlock();
      
      glPopAttrib();
      glDisable(GL_BLEND); // FIXME: temporary for FPS-counter
      
      SoGLLazyElement::getInstance(state)->reset(state, 
                                                 SoLazyElement::LIGHT_MODEL_MASK|
                                                 SoLazyElement::BLENDING_MASK);
      
      return FALSE;
    }
  }

  if (soshape_use_gl_vertex_arrays && ((PRIVATE(this)->flags & SoShapeP::DISABLE_VERTEX_ARRAY_CACHE) == 0)) {
    // only create cache for built in Coin shapes. Make it possible to
    // override by setting COIN_USE_GL_VERTEX_ARRAYS=-1
    if (!this->isBuiltIn && (soshape_use_gl_vertex_arrays != -1)) return TRUE;
    soshape_staticdata * shapedata = soshape_get_staticdata();
    // lock mutex since pvcache is shared among all threads
    PRIVATE(this)->lock();
    if (PRIVATE(this)->pvcache == NULL ||
        !PRIVATE(this)->pvcache->isValid(state)) {
      if (PRIVATE(this)->pvcache) {
        // invalid cache. Don't try to create it again
        PRIVATE(this)->flags |= SoShapeP::DISABLE_VERTEX_ARRAY_CACHE;
        PRIVATE(this)->pvcache->unref();
        PRIVATE(this)->pvcache = NULL;
        PRIVATE(this)->unlock();
        // let shape render
        return TRUE;
      }
      SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
      // must push state to make cache dependencies work
      state->push();
      PRIVATE(this)->pvcache = new SoPrimitiveVertexCache(state);
      PRIVATE(this)->pvcache->ref();
      SoCacheElement::set(state, PRIVATE(this)->pvcache);
      shapedata->is_doing_pvcache_rendering = TRUE;
      this->generatePrimitives(action);
      shapedata->is_doing_pvcache_rendering = FALSE;
      // this _must_ be called after creating the pvcache
      state->pop();
      SoCacheElement::setInvalid(storedinvalid);
      PRIVATE(this)->pvcache->fit();
      PRIVATE(this)->testSetupShapeHints(this);
    }
    
    if ((PRIVATE(this)->pvcache->getNumIndices() < 100) &&
        (PRIVATE(this)->pvcache->getNumLineIndices() < 100) &&
        (PRIVATE(this)->pvcache->getNumPointIndices() < 100)) {
      PRIVATE(this)->pvcache->unref();
      PRIVATE(this)->pvcache = NULL;
      // don't try to create this cache again
      PRIVATE(this)->flags |= SoShapeP::DISABLE_VERTEX_ARRAY_CACHE;
      PRIVATE(this)->unlock();
      // let shape render
      return TRUE;
    }
    PRIVATE(this)->unlock();    

    int arrays = SoPrimitiveVertexCache::NORMAL|SoPrimitiveVertexCache::COLOR;
    if (glimage) arrays |= SoPrimitiveVertexCache::TEXCOORD;
    SoMaterialBundle mb(action);
    mb.sendFirst();
    PRIVATE(this)->setupShapeHints(this, state);
    PRIVATE(this)->pvcache->renderTriangles(state, arrays);
    if (PRIVATE(this)->pvcache->getNumLineIndices() ||
        PRIVATE(this)->pvcache->getNumPointIndices()) {
      const SoNormalElement * nelem = SoNormalElement::getInstance(state);
      if (nelem->getNum() == 0) {
        glPushAttrib(GL_LIGHTING_BIT);
        glDisable(GL_LIGHTING);
        arrays &= SoPrimitiveVertexCache::NORMAL;
      }
      PRIVATE(this)->pvcache->renderLines(state, arrays);
      PRIVATE(this)->pvcache->renderPoints(state, arrays);

      if (nelem->getNum() == 0) {
        glPopAttrib();
      }
    }
    // we have rendered, return FALSE
    return FALSE;
  }

#if COIN_DEBUG && 0 // enable this to test generatePrimitives() rendering
  SoMaterialBundle mb(action);
  mb.sendFirst();
  soshape_get_staticdata()->currentbundle = &mb;  // needed in the primitive callbacks
  this->generatePrimitives(action);
  return FALSE;
#else // generatePrimitives() rendering
  return TRUE; // let the shape node render the geometry using OpenGL
#endif // ! generatePrimitives() rendering
}

/*!
  \COININTERNAL
*/
SbBool
SoShape::shouldRayPick(SoRayPickAction * const action)
{
  switch (SoPickStyleElement::get(action->getState())) {
  case SoPickStyleElement::SHAPE:
    return TRUE;
  case SoPickStyleElement::BOUNDING_BOX:
    this->rayPickBoundingBox(action);
    return FALSE;
  case SoPickStyleElement::UNPICKABLE:
    return FALSE;
  default:
    assert(0 && "unknown pick style");
    return TRUE;
  }
}

/*!
  \COININTERNAL
*/
void
SoShape::beginSolidShape(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();

  SoShapeHintsElement::set(state,
                           SoShapeHintsElement::COUNTERCLOCKWISE,
                           SoShapeHintsElement::SOLID,
                           SoShapeHintsElement::FACE_TYPE_AS_IS);
}

/*!
  \COININTERNAL
*/
void
SoShape::endSolidShape(SoGLRenderAction * action)
{
  action->getState()->pop();
}

/*!
  \COININTERNAL
*/
void
SoShape::computeObjectSpaceRay(SoRayPickAction * const action)
{
  action->setObjectSpace();
}

/*!
  \COININTERNAL
*/
void
SoShape::computeObjectSpaceRay(SoRayPickAction * const action,
                               const SbMatrix & matrix)
{
  action->setObjectSpace(matrix);
}

/*!
  Will create triangle detail for a SoPickedPoint. This method will
  only be called internally, when generatePrimitives() is used for
  picking (SoShape::rayPick() is not overridden).

  This method returns \c NULL in Open Inventor, and subclasses will
  need to override this method to create details for a SoPickedPoint.

  This is not necessary with Coin. Of course, if you choose to
  override it, it will work in the same way as Open Inventor.

  For this to work, you must supply a face or line detail when
  generating primitives. If you supply \c NULL for the detail argument in
  SoShape::beginShape(), you'll have to override this method.
*/
SoDetail *
SoShape::createTriangleDetail(SoRayPickAction * action,
                              const SoPrimitiveVertex * /*v1*/,
                              const SoPrimitiveVertex * /*v2*/,
                              const SoPrimitiveVertex * /*v3*/,
                              SoPickedPoint * pp)
{
  soshape_staticdata * shapedata = soshape_get_staticdata();

  if (shapedata->primdata->faceDetail) {
    return shapedata->primdata->createPickDetail();
  }
#if COIN_DEBUG
  SoDebugError::postInfo("SoShape::createTriangleDetail",
                         "Unable to create triangle detail.");
#endif // COIN_DEBUG
  return NULL;
}

/*!
  Will create line detail for a SoPickedPoint. This method will only
  be called internally, when generatePrimitives() is used for picking
  (SoShape::rayPick() is not overridden).

  This method returns \c NULL in Open Inventor, and subclasses will
  need to override this method to create details for a SoPickedPoint.

  This is not necessary with Coin. Of course, if you choose to
  override it, it will work in the same way as Open Inventor.

  For this to work, you must supply a face or line detail when
  generating primitives. If you supply \c NULL for the detail argument in
  SoShape::beginShape(), you'll have to override this method.
*/
SoDetail *
SoShape::createLineSegmentDetail(SoRayPickAction * action,
                                 const SoPrimitiveVertex * /* v1 */,
                                 const SoPrimitiveVertex * /* v2 */,
                                 SoPickedPoint * pp)
{
  soshape_staticdata * shapedata = soshape_get_staticdata();

  if (shapedata->primdata->lineDetail) {
    return shapedata->primdata->createPickDetail();
  }
#if COIN_DEBUG
  SoDebugError::postInfo("SoShape::createLineSegmentDetail",
                         "Unable to create line segment detail.");
#endif // COIN_DEBUG
  return NULL;
}

/*!
  Will create point detail for a SoPickedPoint. This method will only
  be called internally, when generatePrimitives() is used for picking
  (SoShape::rayPick() is not overridden).

  This method returns \c NULL in Open Inventor, and subclasses will
  need to override this method to create details for a SoPickedPoint.

  This is not necessary with Coin. Of course, if you choose to
  override it, it will work in the same way as Open Inventor.

  For this to work, you must supply a point detail in the
  SoPrimitiveVertex in generatePrimitives().
*/
SoDetail *
SoShape::createPointDetail(SoRayPickAction * /* action */,
                           const SoPrimitiveVertex * v,
                           SoPickedPoint * /* pp */)
{
  if (v->getDetail()) return v->getDetail()->copy();
  return NULL;
}

/*!
  \COININTERNAL
*/
void
SoShape::invokeTriangleCallbacks(SoAction * const action,
                                 const SoPrimitiveVertex * const v1,
                                 const SoPrimitiveVertex * const v2,
                                 const SoPrimitiveVertex * const v3)
{
  if (action->getTypeId().isDerivedFrom(SoRayPickAction::getClassTypeId())) {
    SoRayPickAction * ra = (SoRayPickAction *) action;

    SbVec3f intersection;
    SbVec3f barycentric;
    SbBool front;

    if (ra->intersect(v1->getPoint(), v2->getPoint(), v3->getPoint(),
                      intersection, barycentric, front)) {

      if (ra->isBetweenPlanes(intersection)) {
        SoPickedPoint * pp = ra->addIntersection(intersection);
        if (pp) {
          pp->setDetail(this->createTriangleDetail(ra, v1, v2, v3, pp), this);
          // calculate normal at picked point
          SbVec3f n =
            v1->getNormal() * barycentric[0] +
            v2->getNormal() * barycentric[1] +
            v3->getNormal() * barycentric[2];
          n.normalize();
          pp->setObjectNormal(n);

          // calculate texture coordinate at picked point
          SbVec4f tc =
            v1->getTextureCoords() * barycentric[0] +
            v2->getTextureCoords() * barycentric[1] +
            v3->getTextureCoords() * barycentric[2];
          pp->setObjectTextureCoords(tc);

          // material index need to be approximated, since there is no
          // way to average material indices :( This makes it
          // impossible to fully support color per vertex. An
          // extension to the OIV API would perhaps be a good idea
          // here? Maybe calculate the rgba value for diffuse and
          // transparency and set it in SoPickedPoint?
          float maxval = barycentric[0];
          const SoPrimitiveVertex * maxv = v1;
          if (barycentric[1] > maxval) {
            maxv = v2;
            maxval = barycentric[1];
          }
          if (barycentric[2] > maxval) {
            maxv = v3;
          }
          pp->setMaterialIndex(maxv->getMaterialIndex());
        }
      }
    }
  }
  else if (action->getTypeId().isDerivedFrom(SoCallbackAction::getClassTypeId())) {
    SoCallbackAction * ca = (SoCallbackAction *) action;
    ca->invokeTriangleCallbacks(this, v1, v2, v3);
  }
  else if (action->getTypeId().isDerivedFrom(SoGetPrimitiveCountAction::getClassTypeId())) {
    SoGetPrimitiveCountAction * ga = (SoGetPrimitiveCountAction *) action;
    ga->incNumTriangles();
  }
  else if (action->getTypeId().isDerivedFrom(SoGLRenderAction::getClassTypeId())) {
    soshape_staticdata * shapedata = soshape_get_staticdata();

    if (shapedata->is_doing_sorted_rendering) {
      shapedata->trianglesort->triangle(action->getState(), v1, v2, v3);
    }
    else if (shapedata->is_doing_bigtexture_rendering) {
      shapedata->currentbigtexture->triangle(action->getState(), v1, v2, v3);
    }
    else if (shapedata->is_doing_pvcache_rendering) {
      int pdidx[3];
      pdidx[0] = shapedata->primdata->getPointDetailIndex(v1);
      pdidx[1] = shapedata->primdata->getPointDetailIndex(v2);
      pdidx[2] = shapedata->primdata->getPointDetailIndex(v3);
      PRIVATE(this)->pvcache->addTriangle(v1, v2, v3, pdidx);
    }
    else {
      glBegin(GL_TRIANGLES);
      glTexCoord4fv(v1->getTextureCoords().getValue());
      glNormal3fv(v1->getNormal().getValue());
      shapedata->currentbundle->send(v1->getMaterialIndex(), TRUE);
      glVertex3fv(v1->getPoint().getValue());

      glTexCoord4fv(v2->getTextureCoords().getValue());
      glNormal3fv(v2->getNormal().getValue());
      shapedata->currentbundle->send(v2->getMaterialIndex(), TRUE);
      glVertex3fv(v2->getPoint().getValue());

      glTexCoord4fv(v3->getTextureCoords().getValue());
      glNormal3fv(v3->getNormal().getValue());
      shapedata->currentbundle->send(v3->getMaterialIndex(), TRUE);
      glVertex3fv(v3->getPoint().getValue());
      glEnd();
    }
  }
}

/*!
  \COININTERNAL
*/
void
SoShape::invokeLineSegmentCallbacks(SoAction * const action,
                                    const SoPrimitiveVertex * const v1,
                                    const SoPrimitiveVertex * const v2)
{
  if (action->getTypeId().isDerivedFrom(SoRayPickAction::getClassTypeId())) {
    SoRayPickAction * ra = (SoRayPickAction *) action;

    SbVec3f intersection;
    if (ra->intersect(v1->getPoint(), v2->getPoint(), intersection)) {
      if (ra->isBetweenPlanes(intersection)) {
        SoPickedPoint * pp = ra->addIntersection(intersection);
        if (pp) {
          pp->setDetail(this->createLineSegmentDetail(ra, v1, v2, pp), this);
          float total = (v2->getPoint()-v1->getPoint()).length();
          float len1 = 1.0f;
          float len2 = 0.0f;
          if (total >= 0.0f) {
            len1 = (intersection-v1->getPoint()).length();
            len2 = (intersection-v2->getPoint()).length();
            len1 /= total;
            len2 /= total;
          }
          SbVec3f n =
            v1->getNormal() * len1 +
            v2->getNormal() * len2;
          n.normalize();
          pp->setObjectNormal(n);

          SbVec4f tc =
            v1->getTextureCoords() * len1 +
            v2->getTextureCoords() * len2;
          pp->setObjectTextureCoords(tc);
          pp->setMaterialIndex(len1 >= len2 ?
                               v1->getMaterialIndex() :
                               v2->getMaterialIndex());

        }
      }
    }
  }
  else if (action->getTypeId().isDerivedFrom(SoCallbackAction::getClassTypeId())) {
    SoCallbackAction * ca = (SoCallbackAction *) action;
    ca->invokeLineSegmentCallbacks(this, v1, v2);
  }
  else if (action->getTypeId().isDerivedFrom(SoGetPrimitiveCountAction::getClassTypeId())) {
    SoGetPrimitiveCountAction * ga = (SoGetPrimitiveCountAction *) action;
    ga->incNumLines();
  }
  else if (action->getTypeId().isDerivedFrom(SoGLRenderAction::getClassTypeId())) {
    soshape_staticdata * shapedata = soshape_get_staticdata();

    if (shapedata->is_doing_pvcache_rendering) {      
      PRIVATE(this)->pvcache->addLine(v1, v2);
    }
    else {
      glBegin(GL_LINES);
      glTexCoord4fv(v1->getTextureCoords().getValue());
      glNormal3fv(v1->getNormal().getValue());
      shapedata->currentbundle->send(v1->getMaterialIndex(), TRUE);
      glVertex3fv(v1->getPoint().getValue());
      
      glTexCoord4fv(v2->getTextureCoords().getValue());
      glNormal3fv(v2->getNormal().getValue());
      shapedata->currentbundle->send(v2->getMaterialIndex(), TRUE);
      glVertex3fv(v2->getPoint().getValue());
      glEnd();
    }
  }
}

/*!
  \COININTERNAL
*/
void
SoShape::invokePointCallbacks(SoAction * const action,
                              const SoPrimitiveVertex * const v)
{
  if (action->getTypeId().isDerivedFrom(SoRayPickAction::getClassTypeId())) {
    SoRayPickAction * ra = (SoRayPickAction *) action;

    SbVec3f intersection = v->getPoint();
    if (ra->intersect(intersection)) {
      if (ra->isBetweenPlanes(intersection)) {
        SoPickedPoint * pp = ra->addIntersection(intersection);
        if (pp) {
          pp->setDetail(this->createPointDetail(ra, v, pp), this);
          pp->setObjectNormal(v->getNormal());
          pp->setObjectTextureCoords(v->getTextureCoords());
          pp->setMaterialIndex(v->getMaterialIndex());
        }
      }
    }
  }
  else if (action->getTypeId().isDerivedFrom(SoCallbackAction::getClassTypeId())) {
    SoCallbackAction * ca = (SoCallbackAction *) action;
    ca->invokePointCallbacks(this, v);
  }
  else if (action->getTypeId().isDerivedFrom(SoGetPrimitiveCountAction::getClassTypeId())) {
    SoGetPrimitiveCountAction * ga = (SoGetPrimitiveCountAction *) action;
    ga->incNumPoints();
  }
  else if (action->getTypeId().isDerivedFrom(SoGLRenderAction::getClassTypeId())) {
    soshape_staticdata * shapedata = soshape_get_staticdata();

    if (shapedata->is_doing_pvcache_rendering) {      
      PRIVATE(this)->pvcache->addPoint(v);
    }
    else {
      glBegin(GL_POINTS);
      glTexCoord4fv(v->getTextureCoords().getValue());
      glNormal3fv(v->getNormal().getValue());
      shapedata->currentbundle->send(v->getMaterialIndex(), TRUE);
      glVertex3fv(v->getPoint().getValue());
      glEnd();
    }
  }
}

/*!

  This method is used to generate primitives for a shape. It's
  typically called from a node's generatePrimitives() method. If you
  have your own shape and want to write a generatePrimitives() method
  for that shape, it's probably a good idea to take a peek in the
  generatePrimitives() method for a similar shape in Coin.

  generatePrimitives() can contain several beginShape()/endShape()
  sequences. shapeVertex() is used for each vertex between
  beginShape() and endShape(). For instance, to generate primitives
  for a triangle you'd do something like this:

  \begin verbatim
  SoPrimitiveVertex vertex;

  this->beginShape(action, SoShape::POLYGON);
  vertex.setPoint(SbVec3f(0.0f, 0.0f, 0.0f));
  this->shapeVertex(&vertex);
  vertex.setPoint(SbVec3f(1.0f, 0.0f, 0.0f));
  this->shapeVertex(&vertex);
  vertex.setPoint(SbVec3f(1.0f, 1.0f, 0.0f));
  this->shapeVertex(&vertex);
  this->endShape();
  \end verbatim

  Note that the SoPrimitiveVertex instance can simply be placed on the
  stack and not allocated. SoShape will copy the needed information
  when you call shapeVertex().

  Before calling shapeVertex(), you can set extra information for the
  SoPrimitiveVertex, including normal, material index, and texture
  coordinates.

  This method is slightly different from its counterpart from the
  original Open Inventor library, as this method has an SoDetail as
  the last argument, and not an SoFaceDetail. This is because we
  accept more TriangleShape types, and the detail might be a
  SoFaceDetail or a SoLineDetail. There is no use sending in a
  SoPointDetail, as nothing will be done with it.
*/
void
SoShape::beginShape(SoAction * const action, const TriangleShape shapetype,
                    SoDetail * const detail)
{
  soshape_get_staticdata()->primdata->beginShape(this, action, shapetype, detail);
}


/*!

  This method is used while generating primitives for a shape. See
  beginShape() for more details.

  \sa beginShape(), endShape()
*/
void
SoShape::shapeVertex(const SoPrimitiveVertex * const v)
{
  soshape_get_staticdata()->primdata->shapeVertex(v);
}

/*!

  This method is used while generating primitives for a shape. See
  beginShape() for more details.

  \sa beginShape(), shapeVertex()
*/
void
SoShape::endShape(void)
{
  soshape_get_staticdata()->primdata->endShape();
}

/*!
  Convenience function which sets up an SoPrimitiveVertex, and sends
  it using the SoShape::shapeVertex() function. 2D version
*/
void
SoShape::generateVertex(SoPrimitiveVertex * const pv,
                        const SbVec3f & point,
                        const SbBool usetexfunc,
                        const SoTextureCoordinateElement * const tce,
                        const float s,
                        const float t,
                        const SbVec3f & normal)
{
  this->generateVertex(pv, point, usetexfunc, tce, s, t, 0.0f, normal);
}

/*!
  Convenience function which sets up an SoPrimitiveVertex, and sends
  it using the SoShape::shapeVertex() function. 3D version.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoShape::generateVertex(SoPrimitiveVertex * const pv,
                        const SbVec3f & point,
                        const SbBool usetexfunc,
                        const SoTextureCoordinateElement * const tce,
                        const float s,
                        const float t,
                        const float r,
                        const SbVec3f & normal)
{
  SbVec4f texCoord;
  if (usetexfunc)
    texCoord = tce->get(point, normal);
  else
    texCoord.setValue(s, t, r, 1.0f);
  pv->setPoint(point);
  pv->setNormal(normal);
  pv->setTextureCoords(texCoord);
  shapeVertex(pv);
}

// Doc in superclass.
SbBool
SoShape::affectsState(void) const
{
  // Overridden from default setting in SoNode to return FALSE instead
  // of TRUE, as we know for certain that no node classes derived from
  // SoShape will affect the rendering state.
  return FALSE;
}

// Doc in superclass.
void
SoShape::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (this->shouldPrimitiveCount(action)) this->generatePrimitives(action);
}

/*!
  Not implemented in Coin. Should probably have been private in TGS
  Inventor API.
 */
float
SoShape::getDecimatedComplexity(SoState * state, float complexity)
{
  COIN_OBSOLETED();
  return 1.0f * complexity;
}

/*!
  Render a bounding box.
*/
void
SoShape::GLRenderBoundingBox(SoGLRenderAction * action)
{
  SbBox3f box;
  SbVec3f center;
  this->getBBox(action, box, center);
  center = (box.getMin() + box.getMax()) * 0.5f;
  SbVec3f size = box.getMax()  - box.getMin();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  {
    SoGLShapeHintsElement::forceSend(action->getState(), TRUE, FALSE, FALSE);
  }

  glPushMatrix();
  glTranslatef(center[0], center[1], center[2]);
  sogl_render_cube(size[0], size[1], size[2], &mb,
                   SOGL_NEED_NORMALS | SOGL_NEED_TEXCOORDS, NULL);
  glPopMatrix();
}

/*!
  \COININTERNAL
 */
SbBool
SoShape::shouldPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  return TRUE; // FIXME: what to do here? pederb 1999-11-25
}

//
// used when pickStyle == BOUNDING_BOX
//
void
SoShape::rayPickBoundingBox(SoRayPickAction * action)
{
  SbBox3f box;
  SbVec3f center;
  this->getBBox(action, box, center);
  if (box.isEmpty()) return;
  this->computeObjectSpaceRay(action);
  SbVec3f isect;
  if (action->intersect(box, isect, FALSE)) {
    if (action->isBetweenPlanes(isect)) {
      action->addIntersection(isect);
    }
  }
}

// Doc from superclass.
void
SoShape::notify(SoNotList * nl)
{
  inherited::notify(nl);
  PRIVATE(this)->lock();
  if (PRIVATE(this)->bboxcache) {
    PRIVATE(this)->bboxcache->invalidate();
  }
  if (PRIVATE(this)->pvcache) {
    PRIVATE(this)->pvcache->invalidate();
  }
  PRIVATE(this)->flags &= ~SoShapeP::SHOULD_BBOX_CACHE;
  PRIVATE(this)->flags |= SoShapeP::DISABLE_VERTEX_ARRAY_CACHE;
  PRIVATE(this)->unlock();
}

/*!
  Return the bounding box cache for this shape. It might return
  NULL if no bounding box cache has been created. If not NULL, the
  caller must check if the cache is valid before using it. This
  can be done using SoCache::isValid().

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
const SoBoundingBoxCache *
SoShape::getBoundingBoxCache(void) const
{
  return PRIVATE(this)->bboxcache;
}

// return the bbox for this shape, using the cache if valid,
// calculating it if not.
void
SoShape::getBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();
  SbBool isvalid = PRIVATE(this)->bboxcache && PRIVATE(this)->bboxcache->isValid(state);
  if (isvalid) {
    box = PRIVATE(this)->bboxcache->getProjectedBox();
    // we know center will be set, so just fetch it from the cache
    center = PRIVATE(this)->bboxcache->getCenter();
  }
  if (isvalid) {
    return;
  }

  // destroy the old cache if we have one
  if (PRIVATE(this)->bboxcache) {
    PRIVATE(this)->lock();
    PRIVATE(this)->bboxcache->unref();
    PRIVATE(this)->bboxcache = NULL;
    PRIVATE(this)->unlock();
  }
  
  SbBool shouldcache = (PRIVATE(this)->flags & SoShapeP::SHOULD_BBOX_CACHE) != 0;
  SbBool storedinvalid = FALSE;
  if (shouldcache) {
    // must push state to make cache dependencies work
    state->push();
    storedinvalid = SoCacheElement::setInvalid(FALSE);
    assert(PRIVATE(this)->bboxcache == NULL);
    PRIVATE(this)->lock();
    PRIVATE(this)->bboxcache = new SoBoundingBoxCache(state);
    PRIVATE(this)->bboxcache->ref();
    PRIVATE(this)->unlock();
    SoCacheElement::set(state, PRIVATE(this)->bboxcache);
  }
  SbTime begin = SbTime::getTimeOfDay();
  this->computeBBox(action, box, center);
  SbTime end = SbTime::getTimeOfDay();
  if (shouldcache) {
    PRIVATE(this)->bboxcache->set(box, TRUE, center);
    // pop state since we pushed it
    state->pop();
    SoCacheElement::setInvalid(storedinvalid);
  }
  // only create cache if calculating it took longer than the limit
  else if ((end.getValue() - begin.getValue()) >= SoShapeP::bboxcachetimelimit) {
    PRIVATE(this)->flags |= SoShapeP::SHOULD_BBOX_CACHE;
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      // just recalculate the bbox so that the cache is created at
      // once. SoGLRenderAction and SoRayPickAction might need it.
      state->push();
      storedinvalid = SoCacheElement::setInvalid(FALSE);
      assert(PRIVATE(this)->bboxcache == NULL);
      PRIVATE(this)->lock();
      PRIVATE(this)->bboxcache = new SoBoundingBoxCache(state);
      PRIVATE(this)->bboxcache->ref();
      PRIVATE(this)->unlock();
      SoCacheElement::set(state, PRIVATE(this)->bboxcache);
      box.makeEmpty();
      this->computeBBox(action, box, center);
      PRIVATE(this)->bboxcache->set(box, TRUE, center);
      // pop state since we pushed it
      state->pop();
      SoCacheElement::setInvalid(storedinvalid);
    }
  }
}

void
SoShapeP::calibrateBBoxCache(void)
{
  int i;
  const int ARRAYSIZE = 100;

  // just create 100 random vertices
  SbVec3f vecarray[ARRAYSIZE];
  for (i = 0; i < ARRAYSIZE; i++) {
    for (int j = 0; j < 3; j++) {
      vecarray[i][j] = ((float) rand()) / ((float) RAND_MAX);
    }
  }

  // FIXME: should really measure CPU time spent, and not just wall
  // time. See the item in Coin/docs/todo.txt on implementing a
  // "stopwatch" ADT. 20021111 mortene.
  SbTime begin = SbTime::getTimeOfDay();
  SbBox3f bbox;
  bbox.makeEmpty();
  for (i = 0; i < ARRAYSIZE; i++) {
    bbox.extendBy(vecarray[i]);
  }
  SbTime end = SbTime::getTimeOfDay();
  SoShapeP::bboxcachetimelimit = end.getValue() - begin.getValue();
}

#undef PRIVATE

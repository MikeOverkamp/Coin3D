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
  \class SoIndexedFaceSet SoIndexedFaceSet.h Inventor/nodes/SoIndexedFaceSet.h
  \brief The SoIndexedFaceSet class is used to handle generic indexed facesets.
  \ingroup nodes

  Faces are specified using the coordIndex field. Each face must be
  terminated by a negative (-1) index. Coordinates, normals, materials
  and texture coordinates from the current state (or from the
  vertexProperty node if set), can be indexed to create triangles,
  quads or polygons.

  Binding PER_VERTEX_INDEXED, PER_VERTEX, PER_FACE_INDEXED, PER_FACE
  or OVERALL can be set for material, and normals. The default
  material binding is OVERALL. The default normal binding is
  PER_VERTEX_INDEXED. When PER_VERTEX_INDEXED binding is used and the
  corresponding materialIndex, normalIndex, texCoordIndex field is
  empty, the coordIndex field will be used to index material, normal
  or texture coordinate. If you do specify indices for material,
  normals or texture coordinates for PER_VERTEX_INDEXED binding, make
  sure your index array matches the coordIndex array (there should be
  -1 wherever there is a -1 in the coordIndex field. This is done to
  make this node more human readable.)

*/

#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/caches/SoConvexDataCache.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoGL.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <Inventor/system/gl.h>

#include <Inventor/actions/SoGetPrimitiveCountAction.h>

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/misc/SoGL.h>
#include <assert.h>

#include <Inventor/bundles/SoTextureCoordinateBundle.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbRWMutex.h>
#endif // COIN_THREADSAFE

// for concavestatus
#define STATUS_UNKNOWN 0
#define STATUS_CONVEX  1
#define STATUS_CONCAVE 2


#ifndef DOXYGEN_SKIP_THIS
class SoIndexedFaceSetP {
public:
  SoIndexedFaceSetP(void) 
#ifdef COIN_THREADSAFE
    : convexmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE 
  { }
  SoConvexDataCache * convexCache;
  int concavestatus;
#ifdef COIN_THREADSAFE
  SbRWMutex convexmutex;
#endif // COIN_THREADSAFE

  void readLockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.readLock();
#endif // COIN_THREADSAFE
  }
  void readUnlockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.readUnlock();
#endif // COIN_THREADSAFE
  }
  void writeLockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.writeLock();
#endif // COIN_THREADSAFE
  }
  void writeUnlockConvexCache(void) {
#ifdef COIN_THREADSAFE
    this->convexmutex.writeUnlock();
#endif // COIN_THREADSAFE
  }
};
#endif // DOXYGEN_SKIP_THIS

#undef THIS
#define THIS this->pimpl

SO_NODE_SOURCE(SoIndexedFaceSet);

/*!
  Constructor.
*/
SoIndexedFaceSet::SoIndexedFaceSet()
{
  THIS = new SoIndexedFaceSetP;
  THIS->convexCache = NULL;
  THIS->concavestatus = STATUS_UNKNOWN;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoIndexedFaceSet);
}


/*!
  Destructor.
*/
SoIndexedFaceSet::~SoIndexedFaceSet()
{
  if (THIS->convexCache) THIS->convexCache->unref();
  delete THIS;
}

// doc from parent
void
SoIndexedFaceSet::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoIndexedFaceSet, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

//
// translates current material binding into the internal Binding enum.
//
SoIndexedFaceSet::Binding
SoIndexedFaceSet::findMaterialBinding(SoState * const state) const
{
  Binding binding = OVERALL;
  SoMaterialBindingElement::Binding matbind =
    SoMaterialBindingElement::get(state);

  switch (matbind) {
  case SoMaterialBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoMaterialBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoMaterialBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoMaterialBindingElement::PER_PART:
  case SoMaterialBindingElement::PER_FACE:
    binding = PER_FACE;
    break;
  case SoMaterialBindingElement::PER_PART_INDEXED:
  case SoMaterialBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedFaceSet::findMaterialBinding",
                              "unknown material binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}


//
// translates current normal binding into the internal Binding enum.
//
SoIndexedFaceSet::Binding
SoIndexedFaceSet::findNormalBinding(SoState * const state) const
{
  Binding binding = PER_VERTEX_INDEXED;
  SoNormalBindingElement::Binding normbind =
    (SoNormalBindingElement::Binding) SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::OVERALL:
    binding = OVERALL;
    break;
  case SoNormalBindingElement::PER_VERTEX:
    binding = PER_VERTEX;
    break;
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    binding = PER_VERTEX_INDEXED;
    break;
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_FACE:
    binding = PER_FACE;
    break;
  case SoNormalBindingElement::PER_PART_INDEXED:
  case SoNormalBindingElement::PER_FACE_INDEXED:
    binding = PER_FACE_INDEXED;
    break;
  default:
#if COIN_DEBUG
    SoDebugError::postWarning("SoIndexedFaceSet::findNormalBinding",
                              "unknown normal binding setting");
#endif // COIN_DEBUG
    break;
  }
  return binding;
}

// Documented in superclass.
void
SoIndexedFaceSet::notify(SoNotList * list)
{
  // Overridden to invalidate convex cache.

  if (THIS->convexCache) THIS->convexCache->invalidate();
  SoField *f = list->getLastField();
  if (f == &this->coordIndex) THIS->concavestatus = STATUS_UNKNOWN;
  inherited::notify(list);
}

// doc from parent
void
SoIndexedFaceSet::GLRender(SoGLRenderAction * action)
{
  if (this->coordIndex.getNum() < 3) return;
  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->GLRender(action);
  }

  if (!this->shouldGLRender(action)) {
    if (this->vertexProperty.getValue())
      state->pop();
    return;
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int32_t numindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool normalCacheUsed;

  SoMaterialBundle mb(action);

  SbBool sendNormals = !mb.isColorOnly();

  this->getVertexData(state, coords, normals, cindices,
                      nindices, tindices, mindices, numindices,
                      sendNormals, normalCacheUsed);

  SoTextureCoordinateBundle tb(action, TRUE, FALSE);
  doTextures = tb.needCoordinates();

  if (!sendNormals) nbind = OVERALL;
  else if (nbind == OVERALL) {
    if (normals) glNormal3fv(normals[0].getValue());
    else glNormal3f(0.0f, 0.0f, 1.0f);
  }
  else if (normalCacheUsed && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }
  else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
    nbind = PER_FACE;
  }

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction()) {
      tbind = NONE;
      tindices = NULL;
    }
    // FIXME: just call inherited::areTexCoordsIndexed() instead of
    // the if-check? 20020110 mortene.
    else if (SoTextureCoordinateBindingElement::get(state) ==
             SoTextureCoordinateBindingElement::PER_VERTEX) {
      tbind = PER_VERTEX;
      tindices = NULL;
    }
    else {
      tbind = PER_VERTEX_INDEXED;
      if (tindices == NULL) tindices = cindices;
    }
  }

  SbBool convexcacheused = FALSE;
  if (this->useConvexCache(action, normals, nindices, normalCacheUsed)) {
    cindices = THIS->convexCache->getCoordIndices();
    numindices = THIS->convexCache->getNumCoordIndices();
    mindices = THIS->convexCache->getMaterialIndices();
    nindices = THIS->convexCache->getNormalIndices();
    tindices = THIS->convexCache->getTexIndices();

    if (mbind == PER_VERTEX) mbind = PER_VERTEX_INDEXED;
    else if (mbind == PER_FACE) mbind = PER_FACE_INDEXED;
    if (nbind == PER_VERTEX) nbind = PER_VERTEX_INDEXED;
    else if (nbind == PER_FACE) nbind = PER_FACE_INDEXED;

    if (tbind != NONE) tbind = PER_VERTEX_INDEXED;
    convexcacheused = TRUE;
  }

  mb.sendFirst(); // make sure we have the correct material

  sogl_render_faceset((SoGLCoordinateElement *)coords,
                      cindices,
                      numindices,
                      normals,
                      nindices,
                      &mb,
                      mindices,
                      &tb,
                      tindices,
                      (int)nbind,
                      (int)mbind,
                      doTextures?1:0);

  if (normalCacheUsed) {
    this->readUnlockNormalCache();
  }

  if (convexcacheused) {
    THIS->readUnlockConvexCache();
  }

  if (this->vertexProperty.getValue()) {
    state->pop();
  }
  // send approx number of triangles for autocache handling
  sogl_autocache_update(state, this->coordIndex.getNum() / 4);
}

  // this macro actually makes the code below more readable  :-)
#define DO_VERTEX(idx) \
  if (mbind == PER_VERTEX) {                  \
    pointDetail.setMaterialIndex(matnr);      \
    vertex.setMaterialIndex(matnr++);         \
  }                                           \
  else if (mbind == PER_VERTEX_INDEXED) {     \
    pointDetail.setMaterialIndex(*mindices); \
    vertex.setMaterialIndex(*mindices++); \
  }                                         \
  if (nbind == PER_VERTEX) {                \
    pointDetail.setNormalIndex(normnr);     \
    currnormal = &normals[normnr++];        \
    vertex.setNormal(*currnormal);          \
  }                                         \
  else if (nbind == PER_VERTEX_INDEXED) {   \
    pointDetail.setNormalIndex(*nindices);  \
    currnormal = &normals[*nindices++];     \
    vertex.setNormal(*currnormal);          \
  }                                         \
  if (tbind != NONE) {                      \
    pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx); \
    vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++)); \
  }                                         \
  else if (tb.isFunction()) {               \
    vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal)); \
  }                                         \
  vertex.setPoint(coords->get3(idx));        \
  pointDetail.setCoordinateIndex(idx);      \
  this->shapeVertex(&vertex);

// doc from parent
void
SoIndexedFaceSet::generatePrimitives(SoAction *action)
{
  if (this->coordIndex.getNum() < 3) return;

  SoState * state = action->getState();

  if (this->vertexProperty.getValue()) {
    state->push();
    this->vertexProperty.getValue()->doAction(action);
  }

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  const SoCoordinateElement * coords;
  const SbVec3f * normals;
  const int32_t * cindices;
  int32_t numindices;
  const int32_t * nindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool doTextures;
  SbBool sendNormals;
  SbBool normalCacheUsed;

  sendNormals = TRUE; // always generate normals

  getVertexData(state, coords, normals, cindices,
                nindices, tindices, mindices, numindices,
                sendNormals, normalCacheUsed);

  SoTextureCoordinateBundle tb(action, FALSE, FALSE);
  doTextures = tb.needCoordinates();

  if (!sendNormals) nbind = OVERALL;
  else if (normalCacheUsed && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }
  else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
    nbind = PER_FACE;
  }

  Binding tbind = NONE;
  if (doTextures) {
    if (tb.isFunction()) {
      tbind = NONE;
      tindices = NULL;
    }
    // FIXME: just call inherited::areTexCoordsIndexed() instead of
    // the if-check? 20020110 mortene.
    else if (SoTextureCoordinateBindingElement::get(state) ==
             SoTextureCoordinateBindingElement::PER_VERTEX) {
      tbind = PER_VERTEX;
      tindices = NULL;
    }
    else {
      tbind = PER_VERTEX_INDEXED;
      if (tindices == NULL) tindices = cindices;
    }
  }

  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) {
    nindices = cindices;
  }
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) {
    mindices = cindices;
  }

  SbBool convexcacheused = FALSE;
  if (this->useConvexCache(action, normals, nindices, normalCacheUsed)) {
    cindices = THIS->convexCache->getCoordIndices();
    numindices = THIS->convexCache->getNumCoordIndices();
    mindices = THIS->convexCache->getMaterialIndices();
    nindices = THIS->convexCache->getNormalIndices();
    tindices = THIS->convexCache->getTexIndices();

    if (mbind == PER_VERTEX) mbind = PER_VERTEX_INDEXED;
    else if (mbind == PER_FACE) mbind = PER_FACE_INDEXED;
    if (nbind == PER_VERTEX) nbind = PER_VERTEX_INDEXED;
    else if (nbind == PER_FACE) nbind = PER_FACE_INDEXED;

    if (tbind != NONE) tbind = PER_VERTEX_INDEXED;
    convexcacheused = TRUE;
  }

  int texidx = 0;
  TriangleShape mode = POLYGON;
  TriangleShape newmode;
  const int32_t *viptr = cindices;
  const int32_t *viendptr = viptr + numindices;
  int32_t v1, v2, v3, v4, v5 = 0; // v5 init unnecessary, but kills a compiler warning.

  SoPrimitiveVertex vertex;
  SoPointDetail pointDetail;
  SoFaceDetail faceDetail;

  vertex.setDetail(&pointDetail);

  SbVec3f dummynormal(0,0,1);
  const SbVec3f *currnormal = &dummynormal;
  if (normals) currnormal = normals;
  vertex.setNormal(*currnormal);

  int matnr = 0;
  int normnr = 0;

  while (viptr + 2 < viendptr) {
    v1 = *viptr++;
    v2 = *viptr++;
    v3 = *viptr++;
    if (v1 < 0 || v2 < 0 || v3 < 0) {
#if COIN_DEBUG
      SoDebugError::postInfo("SoIndexedFaceSet::generatePrimitives",
                             "Polygon with less than three vertices detected. "
                             "Aborting current shape.");
#endif // COIN_DEBUG
      
      break;
    } 
    v4 = viptr < viendptr ? *viptr++ : -1;
    if (v4  < 0) newmode = TRIANGLES;
    else {
      v5 = viptr < viendptr ? *viptr++ : -1;
      if (v5 < 0) newmode = QUADS;
      else newmode = POLYGON;
    }
    if (newmode != mode) {
      if (mode != POLYGON) this->endShape();
      mode = newmode;
      this->beginShape(action, mode, &faceDetail);
    }
    else if (mode == POLYGON) this->beginShape(action, POLYGON, &faceDetail);

    // vertex 1 can't use DO_VERTEX
    if (mbind == PER_VERTEX || mbind == PER_FACE) {
      pointDetail.setMaterialIndex(matnr);
      vertex.setMaterialIndex(matnr++);
    }
    else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
      pointDetail.setMaterialIndex(*mindices);
      vertex.setMaterialIndex(*mindices++);
    }
    if (nbind == PER_VERTEX || nbind == PER_FACE) {
      pointDetail.setNormalIndex(normnr);
      currnormal = &normals[normnr++];
      vertex.setNormal(*currnormal);
    }
    else if (nbind == PER_VERTEX || nbind == PER_VERTEX_INDEXED) {
      pointDetail.setNormalIndex(*nindices);
      currnormal = &normals[*nindices++];
      vertex.setNormal(*currnormal);
    }

    if (tbind != NONE) {
      pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx);
      vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++));
    }
    else if (tb.isFunction()) {
      vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
    }
    pointDetail.setCoordinateIndex(v1);
    vertex.setPoint(coords->get3(v1));
    this->shapeVertex(&vertex);

    DO_VERTEX(v2);
    DO_VERTEX(v3);

    if (mode != TRIANGLES) {
      DO_VERTEX(v4);
      if (mode == POLYGON) {
        DO_VERTEX(v5);
        v1 = viptr < viendptr ? *viptr++ : -1;
        while (v1 >= 0) {
          DO_VERTEX(v1);
          v1 = viptr < viendptr ? *viptr++ : -1;
        }
        this->endShape();
      }
    }
    faceDetail.incFaceIndex();
    if (mbind == PER_VERTEX_INDEXED) {
      mindices++;
    }
    if (nbind == PER_VERTEX_INDEXED) {
      nindices++;
    }
    if (tindices) tindices++;
  }
  if (mode != POLYGON) this->endShape();

  if (normalCacheUsed) {
    this->readUnlockNormalCache();
  }
  if (convexcacheused) {
    THIS->readUnlockConvexCache();
  }

  if (this->vertexProperty.getValue()) {
    state->pop();
  }
}

#undef DO_VERTEX

// doc from parent
void
SoIndexedFaceSet::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  int n = this->coordIndex.getNum();
  if (n < 3) return;

  if (action->canApproximateCount()) {
    action->addNumTriangles(n/4);
  }
  else {
    const int32_t * ptr = coordIndex.getValues(0);
    const int32_t * endptr = ptr + n;
    int cnt = 0;
    int add = 0;
    while (ptr < endptr) {
      if (*ptr++ >= 0) cnt++;
      else {
        add += cnt-2;
        cnt = 0;
      }
    }
    // in case index array wasn't terminated with a -1
    if (cnt >= 3) add += cnt-2;
    action->addNumTriangles(add);
  }
}

//
// internal method which checks if convex cache needs to be
// used or (re)created. Returns TRUE if convex cache must be
// used. this->convexCache is then guaranteed to be != NULL.
//
SbBool
SoIndexedFaceSet::useConvexCache(SoAction * action,
                                 const SbVec3f * normals,
                                 const int32_t * nindices,
                                 const SbBool normalsfromcache)
{
  SoState * state = action->getState();
  if (SoShapeHintsElement::getFaceType(state) == SoShapeHintsElement::CONVEX)
    return FALSE;
  
  if (THIS->concavestatus == STATUS_UNKNOWN) {
    const int32_t * ptr = this->coordIndex.getValues(0);
    const int32_t * endptr = ptr + this->coordIndex.getNum();
    int cnt = 0;
    THIS->concavestatus = STATUS_CONVEX;
    while (ptr < endptr) {
      if (*ptr++ >= 0) cnt++;
      else {
        if (cnt > 3) { THIS->concavestatus = STATUS_CONCAVE; break; }
        cnt = 0;
      }
    }
  }
  if (THIS->concavestatus == STATUS_CONVEX) return FALSE;

  THIS->readLockConvexCache();

  if (THIS->convexCache && THIS->convexCache->isValid(state))
    return TRUE;

  THIS->readUnlockConvexCache();
  THIS->writeLockConvexCache();

  if (THIS->convexCache) THIS->convexCache->unref();
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);

  // need to send matrix if we have some weird transformation
  SbMatrix modelmatrix = SoModelMatrixElement::get(state);
  if (modelmatrix[3][0] == 0.0f &&
      modelmatrix[3][1] == 0.0f &&
      modelmatrix[3][2] == 0.0f &&
      modelmatrix[3][3] == 1.0f) modelmatrix = SbMatrix::identity();

  // push to create cache dependencies
  state->push();
  THIS->convexCache = new SoConvexDataCache(state);
  THIS->convexCache->ref();
  SoCacheElement::set(state, THIS->convexCache);
  if (this->vertexProperty.getValue()) this->vertexProperty.getValue()->doAction(action);
  const SoCoordinateElement * coords;
  const SbVec3f * dummynormals;
  const int32_t * cindices;
  int32_t numindices;
  const int32_t * dummynindices;
  const int32_t * tindices;
  const int32_t * mindices;
  SbBool dummy;

  // normals was included as parameters to this function (to avoid
  // a double readLock on the normal cache), so tell getVertexData()
  // not to return normals.
  this->getVertexData(state, coords, dummynormals, cindices,
                      dummynindices, tindices, mindices, numindices,
                      FALSE, dummy);
  
  // force a cache-dependency on SoNormalElement
  (void) SoNormalElement::getInstance(state);

  Binding mbind = this->findMaterialBinding(state);
  Binding nbind = this->findNormalBinding(state);

  if (normalsfromcache && nbind == PER_VERTEX) {
    nbind = PER_VERTEX_INDEXED;
  }

  Binding tbind = NONE;
  // FIXME: just call inherited::areTexCoordsIndexed() instead of
  // the if-check? 20020110 mortene.
  if (SoTextureCoordinateBindingElement::get(state) ==
      SoTextureCoordinateBindingElement::PER_VERTEX) {
    tbind = PER_VERTEX;
  }
  else {
    tbind = PER_VERTEX_INDEXED;
    if (tindices == NULL) tindices = cindices;
  }

  if (nbind == PER_VERTEX_INDEXED && nindices == NULL) {
    nindices = cindices;
  }
  if (mbind == PER_VERTEX_INDEXED && mindices == NULL) {
    mindices = cindices;
  }
  THIS->convexCache->generate(coords, modelmatrix,
                              cindices, numindices,
                              mindices, nindices, tindices,
                              (SoConvexDataCache::Binding)mbind,
                              (SoConvexDataCache::Binding)nbind,
                              (SoConvexDataCache::Binding)tbind);

  THIS->writeUnlockConvexCache();

  state->pop();
  SoCacheElement::setInvalid(storedinvalid);

  THIS->readLockConvexCache();

  return TRUE;
}

// Documented in superclass.
SbBool
SoIndexedFaceSet::generateDefaultNormals(SoState *, SoNormalBundle *)
{
  // Normals are generated in normal cache.
  return FALSE;
}

// Documented in superclass.
SbBool
SoIndexedFaceSet::generateDefaultNormals(SoState * state,
                                       SoNormalCache * nc)
{
  SbBool ccw = TRUE;
  if (SoShapeHintsElement::getVertexOrdering(state) ==
      SoShapeHintsElement::CLOCKWISE) ccw = FALSE;

  const SbVec3f * coords = SoCoordinateElement::getInstance(state)->getArrayPtr3();
  assert(coords);

  SoNormalBindingElement::Binding normbind =
    SoNormalBindingElement::get(state);

  switch (normbind) {
  case SoNormalBindingElement::PER_VERTEX:
  case SoNormalBindingElement::PER_VERTEX_INDEXED:
    nc->generatePerVertex(coords,
                          coordIndex.getValues(0),
                          coordIndex.getNum(),
                          SoCreaseAngleElement::get(state, this->getNodeType() == SoNode::VRML1),
                          NULL,
                          ccw);
    break;
  case SoNormalBindingElement::PER_FACE:
  case SoNormalBindingElement::PER_FACE_INDEXED:
  case SoNormalBindingElement::PER_PART:
  case SoNormalBindingElement::PER_PART_INDEXED:
    nc->generatePerFace(coords,
                        coordIndex.getValues(0),
                        coordIndex.getNum(),
                        ccw);
    break;
  default:
    break;
  }
  return TRUE;
}

#undef THIS
#undef STATUS_UNKNOWN
#undef STATUS_CONVEX
#undef STATUS_CONCAVE

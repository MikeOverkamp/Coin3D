/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 2001 by Systems in Motion. All rights reserved.
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
  \class SoVRMLShape SoVRMLShape.h Inventor/VRMLnodes/SoVRMLShape.h
  \brief The SoVRMLShape class holds geometry and geometry appearance nodes.
*/

/*!
  \enum SoVRMLShape::CacheEnabled
  Used to enumerate cache strategies.
*/

/*!
  \var SoVRMLShape::CacheEnabled SoVRMLShape::OFF
  Never cache
*/

/*!
  \var SoVRMLShape::CacheEnabled SoVRMLShape::ON
  Always cache
*/

/*!
  \var SoVRMLShape::CacheEnabled SoVRMLShape::AUTO
  Use heuristics to try to figure out the optimal caching policy.
*/

/*!
  \var SoSFNode SoVRMLShape::appearance
  Can store an SoVRMLAppearance node, or NULL.
*/

/*!
  \var SoSFNode SoVRMLShape::geometry
  Can store any SoVRMLGeometry subclass, or NULL.
*/

/*!
  \var SoSFEnum SoVRMLShape::renderCaching
  Render caching strategy. Default value is AUTO.
*/

/*!
  \var SoSFEnum SoVRMLShape::boundingBoxCaching
  Bounding box caching strategy. Default value is AUTO.
*/




#include <Inventor/VRMLnodes/SoVRMLShape.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLAppearance.h>
#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/caches/SoGLCacheList.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/caches/SoGLCacheList.h>

#include <stddef.h>

#ifndef DOXYGEN_SKIP_THIS

class SoVRMLShapeP {
public:
  SoBoundingBoxCache * bboxcache;
  SoGLCacheList * cachelist;
  SoChildList * childlist;
  SbBool childlistvalid;
};

#endif // DOXYGEN_SKIP_THIS


SO_NODE_SOURCE(SoVRMLShape);

int SoVRMLShape::numrendercaches;

void
SoVRMLShape::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLShape, SO_VRML97_NODE_TYPE);

  SoType type = SoVRMLShape::getClassTypeId();
  SoRayPickAction::addMethod(type, SoNode::rayPickS);
}

#undef THIS
#define THIS this->pimpl
#undef THISP
#define THISP thisp->pimpl

SoVRMLShape::SoVRMLShape(void)
{
  THIS = new SoVRMLShapeP;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoVRMLShape);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(appearance, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(geometry, (NULL));

  SO_NODE_ADD_FIELD(renderCaching, (AUTO));
  SO_NODE_ADD_FIELD(boundingBoxCaching, (AUTO));

  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, ON);
  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, OFF);
  SO_NODE_DEFINE_ENUM_VALUE(CacheEnabled, AUTO);

  SO_NODE_SET_SF_ENUM_TYPE(renderCaching, CacheEnabled);
  SO_NODE_SET_SF_ENUM_TYPE(boundingBoxCaching, CacheEnabled);

  THIS->childlist = new SoChildList(this);
  THIS->childlistvalid = FALSE;
  THIS->bboxcache = NULL;
  THIS->cachelist = NULL;
}

SoVRMLShape::~SoVRMLShape()
{
  delete THIS->childlist;
  if (THIS->bboxcache) THIS->bboxcache->unref();
  delete THIS->cachelist;
  delete THIS;
}

void
SoVRMLShape::setNumRenderCaches(int num)
{
  SoVRMLShape::numrendercaches = num;
}

int
SoVRMLShape::getNumRenderCaches(void)
{
  return SoVRMLShape::numrendercaches;
}

SbBool
SoVRMLShape::affectsState(void) const
{
  return FALSE;
}

void
SoVRMLShape::doAction(SoAction * action)
{
  SoState * state = action->getState();

  if (state->isElementEnabled(SoLightModelElement::getClassStackIndex())) {
    if ((this->appearance.getValue() == NULL) ||
        (((SoVRMLAppearance*)this->appearance.getValue())->material.getValue() == NULL)) {
      SoLightModelElement::set(state, SoLightModelElement::BASE_COLOR);
    }
  }

  state->push();
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->getChildren()->traverseInPath(action, numindices, indices);
  }
  else {
    this->getChildren()->traverse(action); // traverse all children
  }
  state->pop();
}

void
SoVRMLShape::callback(SoCallbackAction * action)
{

  SoVRMLShape::doAction((SoAction*) action);
}

void
SoVRMLShape::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  state->push();

  if ((this->appearance.getValue() == NULL) ||
      (((SoVRMLAppearance*)this->appearance.getValue())->material.getValue() == NULL)) {
    SoLightModelElement::set(state, SoLightModelElement::BASE_COLOR);
  }

  int numindices;
  const int * indices;
  SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

  SoNode ** childarray = (SoNode**) this->getChildren()->getArrayPtr();

  if (pathcode == SoAction::IN_PATH) {
    int lastchild = indices[numindices - 1];
    for (int i = 0; i <= lastchild && !action->hasTerminated(); i++) {
      SoNode * child = childarray[i];
      action->pushCurPath(i, child);
      if (action->getCurPathCode() != SoAction::OFF_PATH ||
          child->affectsState()) {
        if (!action->abortNow()) {
          child->GLRender(action);
        }
        else {
          SoCacheElement::invalidate(state);
        }
      }
      action->popCurPath(pathcode);
    }
  }
  else {
    action->pushCurPath();
    int n = this->getChildren()->getLength();
    for (int i = 0; i < n && !action->hasTerminated(); i++) {
      if (action->abortNow()) {
        // only cache if we do a full traversal
        SoCacheElement::invalidate(state);
        break;
      }
      action->popPushCurPath(i, childarray[i]);
      childarray[i]->GLRender(action);
    }
    action->popCurPath();
  }
  state->pop();
}

void
SoVRMLShape::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoState * state = action->getState();

  SbXfBox3f childrenbbox;
  SbBool childrencenterset;
  SbVec3f childrencenter;

  // FIXME: AUTO is interpreted as ON for the boundingBoxCaching
  // field, but we should trigger some heuristics based on scene graph
  // "behavior" in the children subgraphs if the value is set to
  // AUTO. 19990513 mortene.
  SbBool iscaching = this->boundingBoxCaching.getValue() != OFF;

  switch (action->getCurPathCode()) {
  case SoAction::IN_PATH:
    // can't cache if we're not traversing all children
    iscaching = FALSE;
    break;
  case SoAction::OFF_PATH:
    return; // no need to do any more work
  case SoAction::BELOW_PATH:
  case SoAction::NO_PATH:
    // check if this is a normal traversal
    if (action->isInCameraSpace() || action->isResetPath()) iscaching = FALSE;
    break;
  default:
    iscaching = FALSE;
    assert(0 && "unknown path code");
    break;
  }

  SbBool validcache = THIS->bboxcache && THIS->bboxcache->isValid(state);

  if (iscaching && validcache) {
    SoCacheElement::addCacheDependency(state, THIS->bboxcache);
    childrenbbox = THIS->bboxcache->getBox();
    childrencenterset = THIS->bboxcache->isCenterSet();
    childrencenter = THIS->bboxcache->getCenter();
    if (THIS->bboxcache->hasLinesOrPoints()) {
      SoBoundingBoxCache::setHasLinesOrPoints(state);
    }
  }
  else {
    SbXfBox3f abox = action->getXfBoundingBox();

    SbBool storedinvalid = FALSE;
    if (iscaching) {
      storedinvalid = SoCacheElement::setInvalid(FALSE);
    }
    state->push();

    if (iscaching) {
      // if we get here, we know bbox cache is not created or is invalid
      if (THIS->bboxcache) THIS->bboxcache->unref();
      THIS->bboxcache = new SoBoundingBoxCache(state);
      THIS->bboxcache->ref();
      // set active cache to record cache dependencies
      SoCacheElement::set(state, THIS->bboxcache);
    }

    SoLocalBBoxMatrixElement::makeIdentity(state);
    action->getXfBoundingBox().makeEmpty();

    SoNode * shape = this->geometry.getValue();
    if (shape) shape->getBoundingBox(action);

    childrenbbox = action->getXfBoundingBox();
    childrencenterset = action->isCenterSet();
    if (childrencenterset) childrencenter = action->getCenter();

    action->getXfBoundingBox() = abox; // reset action bbox

    if (iscaching) {
      THIS->bboxcache->set(childrenbbox, childrencenterset, childrencenter);
    }
    state->pop();
    if (iscaching) SoCacheElement::setInvalid(storedinvalid);
  }

  if (!childrenbbox.isEmpty()) {
    action->extendBy(childrenbbox);
    if (childrencenterset) {
      // FIXME: shouldn't this assert() hold up? Investigate. 19990422 mortene.
#if 0 // disabled
      assert(!action->isCenterSet());
#else
      action->resetCenter();
#endif
      action->setCenter(childrencenter, TRUE);
    }
  }
}

void
SoVRMLShape::rayPick(SoRayPickAction * action)
{
  SoVRMLShape::doAction((SoAction*) action);
}

void
SoVRMLShape::search(SoSearchAction * action)
{
  // Include this node in the search.
  SoNode::search(action);
  if (action->isFound()) return;

  SoVRMLShape::doAction(action);
}

void
SoVRMLShape::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoVRMLShape::doAction((SoAction*) action);
}

SoChildList *
SoVRMLShape::getChildren(void) const
{
  if (!THIS->childlistvalid) {
    SoVRMLShape * thisp = (SoVRMLShape*) this;
    SoVRMLParent::updateChildList(thisp, *(THISP->childlist));
    THISP->childlistvalid = TRUE;
  }
  return THIS->childlist;
}

void
SoVRMLShape::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f && f->getTypeId() == SoSFNode::getClassTypeId()) {
    THIS->childlistvalid = FALSE;
  }
  inherited::notify(list);
}

void
SoVRMLShape::copyContents(const SoFieldContainer * from,
                          SbBool copyConn)
{
  inherited::copyContents(from, copyConn);
  THIS->childlistvalid = FALSE;
  THIS->childlist->truncate(0);
}

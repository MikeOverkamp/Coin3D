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
  \class SoVRMLParent SoVRMLParent.h Inventor/VRMLnodes/SoVRMLParent.h
  \brief The SoVRMLParent node is a superclass for all VRML grouping nodes.
*/

#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/sensors/SoFieldSensor.h>

/*!
  \var SoMFNode SoVRMLParent::children
  The children nodes.
*/

/*!
  \var SoMFNode SoVRMLParent::addChildren
  An event in that is used to add children to this node.
*/

/*!
  \var SoMFNode SoVRMLParent::removeChildren
  An event in that is used to remove children from this node.
*/

#ifndef DOXYGEN_SKIP_THIS

class SoVRMLParentP {
public:
  SbBool childlistvalid;
  SoFieldSensor * addsensor;
  SoFieldSensor * removesensor;
};

#endif // DOXYGEN_SKIP_THIS


SO_NODE_ABSTRACT_SOURCE(SoVRMLParent);

// Doc in parent
void
SoVRMLParent::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLParent, SO_VRML97_NODE_TYPE);
}

#undef THIS
#undef THISP
#define THIS this->pimpl
#define THISP thisp->pimpl

/*!
  Constructor.
*/
SoVRMLParent::SoVRMLParent(void)
{
  this->commonConstructor();
}

/*!
  Destructor. \a numchildren is the expected number of children.
*/
SoVRMLParent::SoVRMLParent(int numchildren)
  : SoGroup(numchildren)
{
  this->commonConstructor();
}

void
SoVRMLParent::commonConstructor(void)
{
  THIS = new SoVRMLParentP;
  THIS->childlistvalid = FALSE;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoVRMLParent);

  SO_VRMLNODE_ADD_EMPTY_EXPOSED_MFIELD(children);
  SO_VRMLNODE_ADD_EVENT_IN(addChildren);
  SO_VRMLNODE_ADD_EVENT_IN(removeChildren);

  THIS->addsensor = new SoFieldSensor(field_sensor_cb, this);
  THIS->removesensor = new SoFieldSensor(field_sensor_cb, this);
  THIS->addsensor->attach(&this->addChildren);
  THIS->removesensor->attach(&this->removeChildren);
}

/*!
  Destructor.
*/
SoVRMLParent::~SoVRMLParent()
{
  THIS->addsensor->detach();
  THIS->removesensor->detach();
  delete THIS->addsensor;
  delete THIS->removesensor;
  delete THIS;
}

// Doc in parent
SbBool
SoVRMLParent::affectsState(void) const
{
  return FALSE;
}

// Doc in parent
void
SoVRMLParent::addChild(SoNode * child)
{
  this->children.addNode(child);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLParent::insertChild(SoNode * child, int idx)
{
  this->children.insertNode(child, idx);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
SoNode *
SoVRMLParent::getChild(int idx) const
{
  return this->children.getNode(idx);
}

// Doc in parent
int
SoVRMLParent::findChild(const SoNode * child) const
{
  return this->children.findNode(child);
}

// Doc in parent
int
SoVRMLParent::getNumChildren(void) const
{
  return this->children.getNumNodes();
}

// Doc in parent
void
SoVRMLParent::removeChild(int idx)
{
  this->children.removeNode(idx);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLParent::removeChild(SoNode * child)
{
  this->children.removeNode(child);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLParent::removeAllChildren(void)
{
  this->children.removeAllNodes();
  SoGroup::children->truncate(0);
  THIS->childlistvalid = TRUE;
}

// Doc in parent
void
SoVRMLParent::replaceChild(int idx, SoNode * child)
{
  this->children.replaceNode(idx, child);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
void
SoVRMLParent::replaceChild(SoNode * old,
                           SoNode * child)
{
  this->children.replaceNode(old, child);
  THIS->childlistvalid = FALSE;
}

// Doc in parent
SoChildList *
SoVRMLParent::getChildren(void) const
{
  if (!THIS->childlistvalid) {
    SoVRMLParent::updateChildList(this->children.getValues(0),
                                  this->children.getNum(),
                                  *SoGroup::children);
    ((SoVRMLParent*)this)->pimpl->childlistvalid = TRUE;
  }
  return SoGroup::children;
}

// Doc in parent
void
SoVRMLParent::doAction(SoAction * action)
{
  inherited::doAction(action);
}

// Doc in parent
void
SoVRMLParent::search(SoSearchAction * action)
{
  inherited::search(action);
}

// Doc in parent
void
SoVRMLParent::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  inherited::getPrimitiveCount(action);
}

// Doc in parent
void
SoVRMLParent::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->children) {
    THIS->childlistvalid = FALSE;
  }
  inherited::notify(list);
}

// Doc in parent
SbBool
SoVRMLParent::readInstance(SoInput * in,
                           unsigned short flags)
{
  SbBool oldnot = this->children.enableNotify(FALSE);
  SbBool ret = inherited::readInstance(in, flags);
  if (oldnot) this->children.enableNotify(TRUE);
  THIS->childlistvalid = FALSE;
  return ret;
}

// Doc in parent
void
SoVRMLParent::copyContents(const SoFieldContainer * from,
                           SbBool copyConn)
{
  SoGroup::children->truncate(0);
  SoNode::copyContents(from, copyConn);
  THIS->childlistvalid = FALSE;
}

/*!

  A convenience method that can be used to update \a cl to match the
  list of nodes in \a nodes.

*/
void
SoVRMLParent::updateChildList(const SoNode * const * nodes,
                              const int numnodes,
                              SoChildList & cl)
{
  int i;
  SbBool needcopy = TRUE;
  if (numnodes == cl.getLength()) {
    const SoNode ** clarr = (const SoNode**) cl.getArrayPtr();
    for (i = 0; i < numnodes; i++) {
      if (clarr[i] != nodes[i]) break;
    }
    if (i == numnodes) needcopy = FALSE;
  }
  if (needcopy) {
    cl.truncate(0);
    for (i = 0; i < numnodes; i++) {
      if (nodes[i]) {
        cl.append((SoNode*) nodes[i]);
      }
    }
  }
}

/*!

  A convenience method that is used to sync the nodes in \a cl with
  all nodes in SoSFNode fields in \a nodewithsfnode.

*/
void
SoVRMLParent::updateChildList(SoNode * nodewithsfnode,
                              SoChildList & cl)
{
  SbList <SoNode*> nodelist;

  const SoFieldData * fd = nodewithsfnode->getFieldData();
  const int n = fd->getNumFields();

  SoType sosftype = SoSFNode::getClassTypeId();
  for (int i = 0; i < n; i++) {
    SoField * f = fd->getField(nodewithsfnode, i);
    if (f->getTypeId() == sosftype) {
      SoNode * node = ((SoSFNode*) f)->getValue();
      if (node) nodelist.append(node);
    }
  }
  SoVRMLParent::updateChildList(nodelist.getArrayPtr(),
                                nodelist.getLength(),
                                cl);
}

// add children in addChildren field
void
SoVRMLParent::processAddChildren(void)
{
  const int n = this->addChildren.getNum();
  const SoNode ** nodes = this->addChildren.getValues(0);
  for (int i = 0; i < n; i++) {
    const SoNode * node = nodes[i];
    if (this->findChild(node) < 0) {
      this->addChild((SoNode*)node);
    }
  }
}

// remove children in removeChildren field
void
SoVRMLParent::processRemoveChildren(void)
{
  const int n = this->removeChildren.getNum();
  const SoNode ** nodes = this->removeChildren.getValues(0);
  for (int i = 0; i < n; i++) {
    int idx = this->findChild(nodes[i]);
    if (idx >= 0) {
      this->removeChild(idx);
    }
  }
}

// callback from the addChildren/removeChildren sensors
void
SoVRMLParent::field_sensor_cb(void * data, SoSensor * sensor)
{
  SoVRMLParent * thisp = (SoVRMLParent*) data;
  if (sensor == THISP->addsensor) {
    thisp->processAddChildren();
  }
  else {
    assert(sensor == THISP->removesensor);
    thisp->processRemoveChildren();
  }
}

#undef THIS
#undef THISP

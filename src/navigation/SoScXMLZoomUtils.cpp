/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2008 by Kongsberg SIM.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Kongsberg SIM about acquiring
 *  a Coin Professional Edition License.
 *
 *  See http://www.coin3d.org/ for more information.
 *
 *  Kongsberg SIM, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  http://www.sim.no/  sales@sim.no  coin-support@coin3d.org
 *
\**************************************************************************/

#include "navigation/SoScXMLZoomUtils.h"

#include <Inventor/SbVec2f.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/SoScXMLEvent.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>

class ZoomData {
public:
  SbVec2f lastposn;

};

// *************************************************************************

SCXML_OBJECT_ABSTRACT_SOURCE(SoScXMLZoomInvoke);

void
SoScXMLZoomInvoke::initClass(void)
{
  SCXML_OBJECT_INIT_ABSTRACT_CLASS(SoScXMLZoomInvoke, SoScXMLNavigationInvoke);
}

ZoomData *
SoScXMLZoomInvoke::getZoomData(const ScXMLStateMachine * statemachine)
{
  ZoomData * data = NULL;
  ZoomDataMap::iterator findit =
    SoScXMLZoomInvoke::zoomdatamap.find(statemachine);
  if (findit == SoScXMLZoomInvoke::zoomdatamap.end()) {
    data = new ZoomData;
    ZoomDataEntry entry(statemachine, data);
    SoScXMLZoomInvoke::zoomdatamap.insert(entry);
  } else {
    data = findit->second;
  }
  return data;
}

void
SoScXMLZoomInvoke::freeZoomData(const ScXMLStateMachine * statemachine)
{
  ZoomDataMap::iterator findit =
    SoScXMLZoomInvoke::zoomdatamap.find(statemachine);
  if (findit != SoScXMLZoomInvoke::zoomdatamap.end()) {
    ZoomData * data = findit->second;
    delete data;
    SoScXMLZoomInvoke::zoomdatamap.erase(findit);
  }
}

SoScXMLZoomInvoke::ZoomDataMap SoScXMLZoomInvoke::zoomdatamap;

// *************************************************************************

SCXML_OBJECT_SOURCE(SoScXMLSetZoom);

void
SoScXMLSetZoom::initClass(void)
{
  SCXML_INVOKE_INIT_CLASS(SoScXMLSetZoom, SoScXMLZoomInvoke,
                           SCXML_COIN_NS, "sim.coin3d.coin", "SetZoom");
}

void
SoScXMLSetZoom::invoke(const ScXMLStateMachine * statemachinearg) const
{
  if (!statemachinearg->isOfType(SoScXMLStateMachine::getClassTypeId())) {
    SoDebugError::post("SetZoom",
                       "No support for non-SoScXMLStateMachine objects");
    return;
  }

  const SoScXMLStateMachine * statemachine =
    static_cast<const SoScXMLStateMachine *>(statemachinearg);

  ZoomData * data = SoScXMLZoomInvoke::getZoomData(statemachine);
  assert(data);
  data->lastposn.setValue(0.0f, 0.0f);

  // get mouse position
  const ScXMLEvent * ev = statemachine->getCurrentEvent();
  if (!ev || !ev->isOfType(SoScXMLEvent::getClassTypeId())) {
    SoDebugError::post("SetZoom", "Need SoEvent but statemachine has none.");
    return;
  }
  const SoEvent * soev = static_cast<const SoScXMLEvent *>(ev)->getSoEvent();
  if (!soev) {
    SoDebugError::post("SetZoom", "Need SoEvent.");
    return;
  }

  const SbViewportRegion & vp = statemachine->getViewportRegion();

  if (soev->isOfType(SoMouseButtonEvent::getClassTypeId())) {
    const SoMouseButtonEvent * mbevent =
      static_cast<const SoMouseButtonEvent *>(soev);
    data->lastposn = mbevent->getNormalizedPosition(vp);
  }
  else if (soev->isOfType(SoKeyboardEvent::getClassTypeId())) {
    const SoKeyboardEvent * kbevent =
      static_cast<const SoKeyboardEvent *>(soev);
    data->lastposn = kbevent->getNormalizedPosition(vp);
  }
  else if (soev->isOfType(SoLocation2Event::getClassTypeId())) {
    const SoLocation2Event * l2event =
      static_cast<const SoLocation2Event *>(soev);
    data->lastposn = l2event->getNormalizedPosition(vp);
  }
  else {
    return;
  }

}

// *************************************************************************

SCXML_OBJECT_SOURCE(SoScXMLUpdateZoom);

void
SoScXMLUpdateZoom::initClass(void)
{
  SCXML_INVOKE_INIT_CLASS(SoScXMLUpdateZoom, SoScXMLZoomInvoke,
                           SCXML_COIN_NS, "sim.coin3d.coin", "UpdateZoom");
 
}

void
SoScXMLUpdateZoom::invoke(const ScXMLStateMachine * statemachinearg) const
{
  if (!statemachinearg->isOfType(SoScXMLStateMachine::getClassTypeId())) {
    SoDebugError::post("SetZoom",
                       "No support for non-SoScXMLStateMachine objects");
    return;
  }

  const SoScXMLStateMachine * statemachine =
    static_cast<const SoScXMLStateMachine *>(statemachinearg);

  ZoomData * data = SoScXMLZoomInvoke::getZoomData(statemachine);
  assert(data);

  // get mouse position
  const ScXMLEvent * ev = statemachine->getCurrentEvent();
  if (!ev || !ev->isOfType(SoScXMLEvent::getClassTypeId())) {
    SoDebugError::post("SetZoom", "Need SoEvent but statemachine has none.");
    return;
  }
  const SoEvent * soev = static_cast<const SoScXMLEvent *>(ev)->getSoEvent();
  if (!soev) {
    SoDebugError::post("SetZoom", "Need SoEvent.");
    return;
  }

  SbVec2f prevposn = data->lastposn;
  SbVec2f thisposn = data->lastposn;
  const SbViewportRegion & vp = statemachine->getViewportRegion();

  if (soev->isOfType(SoLocation2Event::getClassTypeId())) {
    const SoLocation2Event * l2event =
      static_cast<const SoLocation2Event *>(soev);
    thisposn = l2event->getNormalizedPosition(vp);
    data->lastposn = thisposn;
  }

  SoCamera * camera = statemachine->getActiveCamera();
  // The value 20.0 is just a value found by trial.
  SoScXMLNavigationInvoke::zoom(camera, (thisposn[1] - prevposn[1]) * 20.0f);
}

// *************************************************************************

SCXML_OBJECT_SOURCE(SoScXMLEndZoom);

void
SoScXMLEndZoom::initClass(void)
{
  SCXML_INVOKE_INIT_CLASS(SoScXMLEndZoom, SoScXMLZoomInvoke,
                           SCXML_COIN_NS, "sim.coin3d.coin", "EndZoom");
}

void
SoScXMLEndZoom::invoke(const ScXMLStateMachine * statemachine) const
{
  SoScXMLZoomInvoke::freeZoomData(statemachine);
}

// *************************************************************************

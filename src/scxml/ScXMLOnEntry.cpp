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

#include <Inventor/scxml/ScXMLOnEntry.h>

#include <assert.h>
#include <algorithm>

#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/ScXMLInvoke.h>

#include "scxml/ScXMLCommonP.h"

// *************************************************************************

/*!
  \class ScXMLOnEntry ScXMLOnEntry.h Inventor/scxml/ScXMLOnEntry.h
  \brief Implementation of the <onentry> SCXML element.

  \since Coin 3.0
  \ingroup scxml
*/

SCXML_OBJECT_SOURCE(ScXMLOnEntry);

void
ScXMLOnEntry::initClass(void)
{
  SCXML_OBJECT_INIT_CLASS(ScXMLOnEntry, ScXMLObject, SCXML_DEFAULT_NS, "onentry");
}

ScXMLOnEntry::ScXMLOnEntry(void)
{
}

ScXMLOnEntry::~ScXMLOnEntry(void)
{
  {
    std::vector<ScXMLInvoke *>::iterator invokeit = this->invokelist.begin();
    while (invokeit != this->invokelist.end()) {
      delete *invokeit;
      ++invokeit;
    }
    this->invokelist.clear();
  }
}

// *************************************************************************
// executable content

SCXML_LIST_OBJECT_API_IMPL(ScXMLOnEntry, ScXMLInvoke, invokelist, Invoke, Invokes);

void
ScXMLOnEntry::invoke(ScXMLStateMachine * statemachine)
{
  std::vector<ScXMLInvoke *>::const_iterator it = this->invokelist.begin();
  while (it != this->invokelist.end()) {
    (*it)->invoke(statemachine);
    ++it;
  }
}

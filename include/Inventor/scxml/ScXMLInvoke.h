#ifndef COIN_SCXMLINVOKE_H
#define COIN_SCXMLINVOKE_H

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

#include <Inventor/scxml/ScXMLObject.h>

class ScXMLEvent;
class ScXMLStateMachine;

class COIN_DLL_API ScXMLInvoke : public ScXMLObject {
  typedef ScXMLObject inherited;
  SCXML_OBJECT_ABSTRACT_HEADER(ScXMLInvoke);

public:
  static void initClass(void);

  ScXMLInvoke(void);
  virtual ~ScXMLInvoke(void);

  // XML attributes
  virtual void setTargetTypeXMLAttr(const char * id);
  const char * getTargetTypeXMLAttr(void) const { return this->targettype; }

  virtual void setSrcXMLAttr(const char * id);
  const char * getSrcXMLAttr(void) const { return this->src; }

  virtual void setSrcExprXMLAttr(const char * id);
  const char * getSrcExprXMLAttr(void) const { return this->srcexpr; }

  virtual SbBool handleXMLAttributes(void);

  // 
  virtual void invoke(ScXMLStateMachine * statemachine) = 0;

protected:
  char * targettype;
  char * src;
  char * srcexpr;

}; // ScXMLInvoke

#endif // !COIN_SCXMLINVOKE_H

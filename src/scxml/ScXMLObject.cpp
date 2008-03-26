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

#include <string.h>
#include <assert.h>

#include <Inventor/SbName.h>
#include <Inventor/scxml/ScXML.h>

#include "scxml/ScXMLP.h"

SCXML_OBJECT_ABSTRACT_SOURCE(ScXMLObject);

void
ScXMLObject::initClass(void)
{
  ScXMLObject::classTypeId =
    SoType::createType(SoType::badType(), SbName("ScXMLObject"));
  // not registered with ScXML
}

ScXMLObject::ScXMLObject(void)
  : containerptr(NULL)
{
}

ScXMLObject::~ScXMLObject(void)
{
  {
    AttrDict::iterator it = this->attributedict.begin();
    while (it != this->attributedict.end()) {
      char * str = const_cast<char *>(it->second);
      delete [] str;
      ++it;
    }
    this->attributedict.clear();
  }
}

SbBool
ScXMLObject::isOfType(SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

/*!
  This method associates a value with an attribute name.  This is
  used for parameter passing while reading by the SCXML reader, but
  can be used as a general-purpose attribute dictionary outside of
  that.

  If NULL is passed as the value, the attribute is removed.
*/
void
ScXMLObject::setAttribute(const char * attribute, const char * value)
{
  assert(attribute);
  const SbName attrname(attribute); // uniqify on string pointer
  AttrDict::iterator it = this->attributedict.find(attrname.getString());
  if (it == this->attributedict.end()) {
    if (value) {
      char * valuedup = new char [ strlen(value) + 1 ];
      strcpy(valuedup, value);
      this->attributedict.insert(AttrEntry(attrname.getString(), valuedup));
    }
  } else {
    char * str = const_cast<char *>(it->second);
    delete [] str;
    if (!value) {
      this->attributedict.erase(it);
    } else {
      char * buf = new char [ strlen(value) + 1 ];
      strcpy(buf, value);
      it->second = buf;
    }
  }
}

/*!
  This method returns the string value set for an attribute, or
  NULL if not set.
*/
const char *
ScXMLObject::getAttribute(const char * attribute) const
{
  const SbName attrname(attribute);
  AttrDict::const_iterator it = this->attributedict.find(attrname.getString());
  if (it != this->attributedict.end()) {
    return it->second;
  }
  return NULL;
}

/*!
  This method is caled when the file reader has set all the XML
  attributes on the object, and wants the object to handle them and
  report if the values were ok or if a read error should be produced.

  \return TRUE if the attributes are ok, and FALSE on error.

  This base class implementation does nothing and just returns TRUE.
*/
SbBool
ScXMLObject::handleXMLAttributes(void)
{
  return TRUE;
}

/*!
  Set the pointer to the parent ScXML object.
*/
void
ScXMLObject::setContainer(const ScXMLObject * container)
{
  this->containerptr = container;
}

/*!
  This method returns TRUE if the object is contained within the given
  \a object argument. Also if they are the same, TRUE will be returned.
  Otherwise, FALSE is retured.
*/
SbBool
ScXMLObject::isContainedIn(const ScXMLObject * object) const
{
  const ScXMLObject * lineageobj = this;
  while (lineageobj) {
    if (lineageobj == object) return TRUE;
    lineageobj = lineageobj->getContainer();
  }
  return FALSE;
}

// *************************************************************************
/*!
  Internal method.
  Friendly forwarding for ScXMLObject subclasses.
*/
void
ScXMLObject::registerClassType(const char * xmlns, const char * classname, SoType type)
{
  ScXMLP::registerClassType(xmlns, classname, type);
}

/*!
  Internal method.
  Friendly forwarding for ScXMLObject subclasses.
*/
void
ScXMLObject::registerInvokeClassType(const char * xmlns, const char * targettype, const char * source, SoType type)
{
  ScXMLP::registerInvokeClassType(xmlns, targettype, source, type);
}


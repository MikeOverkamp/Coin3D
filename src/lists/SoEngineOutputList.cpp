/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2002 by Systems in Motion.  All rights reserved.
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
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class SoEngineOutputList SoEngineOutputList.h Inventor/lists/SoEngineOutputList.h
  \brief The SoEngineOutputList class is a container for SoEngineOutput objects.
  \ingroup engines

  \sa SbList
*/

// SoEngineOutputList was moved from being a subclass of SbPList to
// being a subclass of SbList. This removed the need to do lots of
// ugly casts in overridden methods, with the subsequent removal of
// most of the code in this file. 20000228 mortene.

/*!
  \fn SoEngineOutputList::SoEngineOutputList(void)

  Default constructor.
*/

/*!
  \fn SoEngineOutputList::SoEngineOutputList(const int sizehint)

  This constructor initializes the internal allocated size for the
  list to \a sizehint. Note that the list will still initially contain
  zero items.

  \sa SbList::SbList(const int)
*/

/*!
  \fn SoEngineOutputList::SoEngineOutputList(const SoEngineOutputList & l)

  Copy constructor.

  \sa SbList::SbList(const SbList<Type> & l)
*/

/*!
  \fn void SoEngineOutputList::set(const int index, SoEngineOutput * item)

  This method sets the element at \a index to \a item. Does the same
  thing as SbList::operator[](). This method is only present for
  compatibility with the original Inventor API.
*/

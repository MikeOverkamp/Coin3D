/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion.  All rights reserved.
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.  See the
 *  file LICENSE.GPL at the root directory of this source distribution
 *  for more details.
 *
 *  If you desire to use Coin with software that is incompatible
 *  licensewise with the GPL, and / or you would like to take
 *  advantage of the additional benefits with regard to our support
 *  services, please contact Systems in Motion about acquiring a Coin
 *  Professional Edition License.  See <URL:http://www.coin3d.org> for
 *  more information.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  <URL:http://www.sim.no>, <mailto:support@sim.no>
 *
\**************************************************************************/

/*!
  \class SbVec3fList SbVec3fList.h Inventor/lists/SbVec3fList.h
  \brief The SbVec3fList class is a container for arrays of SbVec3f pointers.
  \ingroup base

  Note that upon using the equality and inequality operators, the
  SbVec3f objects themselves are not compared, only the pointer
  values.

  Note also that all calls to append() and insert() will cause the
  list to allocate a new SbVec3f object. These objects are not freed
  automatically, but are the responsibility of the user.

  \sa SbList
*/



/*!
  \fn SbVec3fList::SbVec3fList(void)

  Default constructor.
*/

/*!
  \fn SbVec3fList::SbVec3fList(const int sizehint)

  This constructor initializes the internal allocated size for the
  list to \a sizehint. Note that the list will still initially contain
  zero items.

  \sa SbList::SbList(const int sizehint)
*/

/*!
  \fn SbVec3fList::SbVec3fList(const SbVec3fList & l)

  Copy constructor.

  \sa SbList::SbList(const SbList<Type> & l)
*/

/*!
  \fn SbVec3f * SbVec3fList::get(const int index) const

  This method returns the element at \a index. Does the same thing as
  SbList::operator[](). This method is only present for compatibility
  with the original Inventor API.
*/

/*!
  \fn void SbVec3fList::set(const int index, SbVec3f * const item)

  This method sets the element at \a index to \a item. Does the same
  thing as SbList::operator[](). This method is only present for
  compatibility with the original Inventor API.
*/

/*!
  \fn void SbVec3fList::append(const SbVec3f * item)

  Overridden from parent to allocate a new SbVec3f instance when
  called.

  \sa SbList::append()
 */

/*!
  \fn void SbVec3fList::insert(const SbVec3f * item, const int insertbefore)

  Overridden from parent to allocate a new SbVec3f instance when
  called.

  \sa SbList::insert()
 */

#ifndef COIN_SODIFFUSECOLORELEMENT_H
#define COIN_SODIFFUSECOLORELEMENT_H

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

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/elements/SoLazyElement.h>

class SoColorPacker;

class COIN_DLL_API SoDiffuseColorElement : public SoReplacedElement {
  typedef SoReplacedElement inherited;

  SO_ELEMENT_HEADER(SoDiffuseColorElement);
public:
  static void initClass(void);
protected:
  virtual ~SoDiffuseColorElement();

public:
  virtual void init(SoState * state);

  static void set(SoState * const state, SoNode * const node,
                  const int32_t numcolors, const SbColor * const colors);

  static void set(SoState * const state, SoNode * const node,
                  const int32_t numcolors, const uint32_t * const colors,
                  const SbBool packedtransparency = FALSE);

  static const SoDiffuseColorElement * getInstance(SoState *state);

  int32_t getNum(void) const;
  const SbColor &get(const int index) const;

  SbBool isPacked(void) const;
  SbBool hasPackedTransparency(void) const;

  const SbColor *getColorArrayPtr() const;
  const uint32_t *getPackedArrayPtr() const;

private:
  SoState * state;
  SoColorPacker colorpacker;
};

#endif // !COIN_SODIFFUSECOLORELEMENT_H

#ifndef CC_HASHP_H
#define CC_HASHP_H

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.  See the file LICENSE.GPL
 *  at the root directory of this source distribution for more details.
 *
 *  If you desire to use Coin with software that is incompatible
 *  licensewise with the GPL, and / or you would like to take
 *  advantage of the additional benefits with regard to our support
 *  services, please contact Systems in Motion about acquiring a Coin
 *  Professional Edition License.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  www.sim.no, support@sim.no, Voice: +47 22114160, Fax: +47 22207097
 *
\**************************************************************************/

#ifndef COIN_INTERNAL
#error You have tried to use one of the private Coin header files
#endif /* ! COIN_INTERNAL */

#include <Inventor/C/base/memalloc.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  struct cc_hash_entry {
    unsigned long key;
    void * val;
    struct cc_hash_entry * next;
  };
  typedef struct cc_hash_entry cc_hash_entry;

  struct cc_hash {
    unsigned int size;
    unsigned int elements;
    float loadfactor;
    unsigned int threshold;
    cc_hash_entry ** buckets;
    cc_hash_func * hashfunc;
    cc_memalloc * memalloc;
  };

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* ! CC_HASHP_H */

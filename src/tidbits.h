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

#ifndef COIN_TIDBITS_H
#define COIN_TIDBITS_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus */

/* ********************************************************************** */

#include <Inventor/system/inttypes.h>
#include <stdarg.h>

int coin_snprintf(char * dst, unsigned int n, const char * fmtstr, ...);
int coin_vsnprintf(char * dst, unsigned int n, const char * fmtstr, va_list args);

const char * coin_getenv(const char *);
int coin_strncasecmp(const char *, const char *, int);

int coin_host_is_bigendian(void);

uint16_t coin_hton_uint16(uint16_t value);
uint16_t coin_ntoh_uint16(uint16_t value);
uint32_t coin_hton_uint32(uint32_t value);
uint32_t coin_ntoh_uint32(uint32_t value);

int coin_isascii(int c);

typedef void coin_atexit_f(void);
void coin_atexit(coin_atexit_f *);

/* ********************************************************************** */

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* COIN_TIDBITS_H */

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

#include <Inventor/C/base/string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "../tidbits.h" /* snprintf() and vsnprintf() definitions. */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* ********************************************************************** */

cc_string *
cc_string_struct_malloc(void)
{
  cc_string * string_struct;
  string_struct = (cc_string *) malloc(sizeof(cc_string));
  assert(string_struct != NULL);
  return string_struct;
} /* cc_string_struct_malloc() */

void
cc_string_struct_init(cc_string * string_struct)
{
  string_struct->pointer = string_struct->buffer;
  string_struct->bufsize = CC_STRING_MIN_SIZE;
  string_struct->buffer[0] = '\0';
} /* cc_string_struct_init() */

void
cc_string_struct_clean(cc_string * string_struct)
{
  if ( string_struct->pointer != string_struct->buffer )
    free(string_struct->pointer);
} /* cc_string_struct_clean() */

void
cc_string_struct_free(cc_string * string_struct)
{
  if ( string_struct ) free(string_struct);
} /* cc_string_struct_free() */

/* ********************************************************************** */

void
cc_string_remove_substring(cc_string * me, int start, int end)
{
  int len;
  len = strlen(me->pointer);
  if ( end == -1 ) end = len - 1;

#if COIN_DEBUG
  if ( start < 0 || start >= len || end < 0 || end >= len || start > end ) {
    /*
    SoDebugError::postWarning("cc_string_remove_substring",
                              "invalid arguments [%d, %d] for string ``%s''",
                              startidx, endidx, me->sstring);
    */
    return;
  }
#endif /* COIN_DEBUG */
  (void) memmove(me->pointer + start, me->pointer + end + 1, len - end);
} /* cc_string_remove_substring() */

static
void
cc_string_expand_buffer(cc_string * me, int additional)
{
  int newsize;
  char * newbuf;
  newsize = strlen(me->pointer) + additional + 1;
  if ( newsize > me->bufsize ) {
    newbuf = (char *) malloc(newsize);
    assert(newbuf != NULL);
    (void) strcpy(newbuf, me->pointer);
    if ( me->pointer != me->buffer )
      free(me->pointer);
    me->pointer = newbuf;
    me->bufsize = newsize;
  }
} /* cc_string_expand_buffer() */

/* ********************************************************************** */

cc_string *
cc_string_construct(const char * text)
{
  cc_string * me;
  me = cc_string_struct_malloc();
  cc_string_struct_init(me);
  cc_string_set_text(me, text);
  return me;
} /* cc_string_construct() */

cc_string *
cc_string_construct_new(void)
{
  cc_string * me;
  me = cc_string_struct_malloc();
  cc_string_struct_init(me);
  return me;
} /* cc_string_construct_new() */

cc_string *
cc_string_construct_subtext(const char * text, int start, int end)
{
  cc_string * me;
  me = cc_string_struct_malloc();
  cc_string_struct_init(me);
  cc_string_set_subtext(me, text, start, end);
  return me;
} /* cc_string_construct_subtext() */

cc_string *
cc_string_clone(cc_string * string)
{
  cc_string * me;
  me = cc_string_struct_malloc();
  cc_string_struct_init(me);
  cc_string_set_text(me, string->pointer);
  return me;
} /* cc_string_clone() */

void
cc_string_destruct(cc_string * me)
{
  cc_string_struct_clean(me);
  cc_string_struct_free(me);
} /* cc_string_destruct() */

/* ********************************************************************** */

void
cc_string_set_text(cc_string * me, const char * text)
{
  static char emptystring[] = "";
  int size;
  if ( text == NULL ) text = emptystring;

  if ( text >= me->pointer && text < (me->pointer + me->bufsize) ) {
    /* text is within own buffer */
    cc_string_remove_substring(me, 0, text - me->pointer);
    return;
  }
  size = strlen(text) + 1;
  if ( size > me->bufsize )
    cc_string_expand_buffer(me, size - strlen(me->pointer) - 1);
  (void) strcpy(me->pointer, text);
} /* cc_string_set_text() */

void
cc_string_set_subtext(cc_string * me, const char * text, int start, int end)
{
  static char * emptystring = "";
  int len, size;
  if ( text == NULL ) text = emptystring;
  len = strlen(text);
  if ( end == -1 ) end = len - 1;

#if COIN_DEBUG && 0 /* FIXME */
  if (start<0) {
    // SoDebugError::postWarning("SbString::SbString",
    fprintf(stderr, 
                              "start index (%d) should be >= 0. Clamped to 0.",
                              start);
    start=0;
  }
  else if (start>len) {
    // SoDebugError::postWarning("SbString::SbString",
    fprintf(stderr, 
                              "start index (%d) is out of bounds [0, %d>. "
                              "Clamped to %d.", start, len, len-1);
    start=len;
  }
  if (end<0) {
    // SoDebugError::postWarning("SbString::SbString",
    fprintf(stderr, 
                              "end index (%d) should be >= 0. Clamped to 0.",
                              end);
    end=0;
  }
  else if (end>len) {
    // SoDebugError::postWarning("SbString::SbString",
    fprintf(stderr, 
                              "end index (%d) is out of bounds [0, %d>. "
                              "Clamped to %d.", end, len, len-1);
    end=len;
  }
  if (start>end+1) {
    // SoDebugError::postWarning("SbString::SbString",
    fprintf(stderr, 
                              "start index (%d) is greater than end index "
                              "(%d). Empty string created.", start, end);
    start=0;
    end=-1;
  }
#endif /* COIN_DEBUG */

  /* if new substring fits in internal buffer, freeing allocated
     buffer will not happen - should this be changed? */
  size = end - start + 1;
  if ( size >= me->bufsize ) {
    if ( me->pointer != me->buffer )
      free(me->pointer);
    me->pointer = (char *) malloc(size + 1);
    me->bufsize = size + 1;
  }
  (void) strncpy(me->pointer, text + start, size);
  me->pointer[size] = '\0';
} /* cc_string_set_subtext() */

void
cc_string_set_integer(cc_string * me, int integer)
{
  me->pointer[0] = '\0';
  cc_string_append_integer(me, integer);
} /* cc_string_set_integer() */

void
cc_string_set_string(cc_string * me, const cc_string * string)
{
  cc_string_set_text(me, string->pointer);
} /* cc_string_set_string() */

/* ********************************************************************** */

void
cc_string_append_string(cc_string * me, const cc_string * string)
{
  cc_string_append_text(me, string->pointer);
} /* cc_string_append_string() */

void
cc_string_append_text(cc_string * me, const char * text)
{
  if ( text ) {
    cc_string_expand_buffer(me, strlen(text));
    (void) strcat(me->pointer, text);
  }
} /* cc_string_append_text() */

void
cc_string_append_integer(cc_string * me, const int digits)
{
  int i, f, s, w, len, tmp;

  /* FIXME: Cheesus, this is lame. Use log10 instead. 19991216 mortene. */
  w = 1;
  if ( digits < 0 ) w++; /* sign */
  tmp = digits;
  while ( tmp >= 10 ) {
    tmp /= 10;
    w++;
  }

  cc_string_expand_buffer(me, w);

  tmp = digits;
  len = strlen(me->pointer);
  if ( tmp < 0 ) {
    tmp = -tmp;
    me->pointer[len++] = '-';
    w--;
  }

  f = 1;
  for ( i = 0; i < w - 1; i++ ) f *= 10;
  while ( w ) {
    s = tmp / f;
    tmp = tmp % f;
    f /= 10;
    w--;
    me->pointer[len++] = (char) (s + 0x30);
  }
  me->pointer[len] = '\0';
} /* cc_string_append_integer() */

void
cc_string_append_char(cc_string * me, const char c)
{
  int pos;
  cc_string_expand_buffer(me, 1);
  pos = strlen(me->pointer);
  me->pointer[pos] = c;
  me->pointer[pos+1] = '\0';
} /* cc_string_append_char() */

/* ********************************************************************** */

unsigned int
cc_string_length(const cc_string * me)
{
  return strlen(me->pointer);
} /* cc_string_length() */

void
cc_string_clear(cc_string * me)
{
  if ( me->pointer != me->buffer ) {
    free(me->pointer);
    me->pointer = me->buffer;
    me->bufsize = CC_STRING_MIN_SIZE;
  }
  me->pointer[0] = '\0';
} /* cc_string_clear() */

void
cc_string_clear_no_free(cc_string * me)
{
  me->pointer[0] = '\0';
} /* cc_string_clear_no_free() */

uint32_t
cc_string_hash_text(const char * text)
{
  uint32_t total, shift;
  total = shift = 0;
  while ( *text ) {
    total = total ^ ((*text) << shift);
    shift += 5;
    if ( shift > 24 ) shift -= 24;
    text++;
  }
  return total;
}

uint32_t
cc_string_hash(const cc_string * me)
{
  return cc_string_hash_text(me->pointer);
} /* cc_string_hash() */

const char *
cc_string_get_text(const cc_string * me)
{
  return me->pointer;
} /* cc_string_get_text() */

cc_string *
cc_string_get_substring(cc_string * me, int start, int end)
{
  return cc_string_construct_subtext(me->pointer, start, end);
} /* cc_string_get_substring() */

/* ********************************************************************** */

int
cc_string_is(const cc_string * me)
{
  return (me->pointer[0] != '\0');
} /* cc_string_is() */

int
cc_string_compare(const cc_string * lhs, const cc_string * rhs)
{
  return cc_string_compare_text(lhs->pointer, rhs->pointer);
} /* cc_string_compare() */

int
cc_string_compare_text(const char * lhs, const char * rhs)
{
  return lhs && rhs && strcmp(lhs, rhs);
} /* cc_string_compare_text() */

int
cc_string_compare_subtext(const cc_string * me, const char * text, int offset)
{
  /* FIXME: assert on invalid offset */
  return strncmp(me->pointer + offset, text, strlen(text));
} /* cc_string_compare_prefix() */

/* ********************************************************************** */

void
cc_string_apply(cc_string * string, char (*function)(char input))
{
  int len, i;
  assert(function != NULL);
  len = cc_string_length(string);
  for ( i = 0; i < len; i++ )
    string->pointer[i] = function(string->pointer[i]);
}

/* ********************************************************************** */

void
cc_string_sprintf(cc_string * me, const char * formatstr, ...)
{
  va_list args;
  va_start(args, formatstr);
  cc_string_vsprintf(me, formatstr, args);
  va_end(args);
} /* cc_string_sprintf() */

void
cc_string_vsprintf(cc_string * me, const char * formatstr, va_list args)
{
  int length;
  SbBool expand;
  do {
    length = vsnprintf(me->pointer, me->bufsize, formatstr, args);

    /* Some vsnprintf() implementations returns -1 upon failure (this
       is what vsnprintf() from GNU libc is documented to do). */
    expand = (length == -1);
    /* At least with GNU libc 2.1.1, vsnprintf() does _not_ return -1
       (as documented in the snprintf(3) man-page) when we can't fit
       the constructed string within the given buffer, but rather the
       number of characters needed. */
    if (!expand) { expand = (length > me->bufsize); }
    /* IRIX 6.5 vsnprintf() just returns the number of characters
       until clipped. */
    if (!expand) { expand = (length == me->bufsize - 1); }

    /* FIXME: all the vsnprintf() differences should be hidden in a
       tidbits.c coin_vsnprintf() implementation. Or perhaps just make
       our own vsnprintf() implementation from scratch (could perhaps
       grab something from the Public Domain? This wheel must have
       been invented many times over now.) 20011116 mortene. */


    if ( expand )
      cc_string_expand_buffer(me, 1024); /* increase linearly in 1Kb intervals */
  } while ( expand );
} /* cc_string_vsprintf() */

/* ********************************************************************** */

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
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
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <assert.h>

#include "win32.h"

#include <Inventor/C/glue/GLUWrapper.h>

/* ************************************************************************* */

/*
  Implementation note: no part of the code has to be reentrant, as the
  complete interface is protected from multiple threads accessing it
  at the same time by locking in the cc_flw_* functions (which should
  be the only callers).
*/

/*
  Resources:

  - <URL:http://www.codeproject.com/gdi/> (contains many programming
  examples for font queries, rendering and other handling)
*/

/* ************************************************************************* */

#ifndef HAVE_WIN32_API

/* Dummy versions of all functions. Only cc_flww32_initialize() will be
   used from generic wrapper. */

SbBool cc_flww32_initialize(void) { return FALSE; }
void cc_flww32_exit(void) { }

void * cc_flww32_get_font(const char * fontname, int sizex, int sizey) { assert(FALSE); return NULL; }
void cc_flww32_get_font_name(void * font, cc_string * str) { assert(FALSE); }
void cc_flww32_done_font(void * font) { assert(FALSE); }

int cc_flww32_get_num_charmaps(void * font) { assert(FALSE); return 0; }
const char * cc_flww32_get_charmap_name(void * font, int charmap) { assert(FALSE); return NULL; }
void cc_flww32_set_charmap(void * font, int charmap) { assert(FALSE); }

void cc_flww32_set_char_size(void * font, int width, int height) { assert(FALSE); }
void cc_flww32_set_font_rotation(void * font, float angle) { assert(FALSE); }
  
int cc_flww32_get_glyph(void * font, unsigned int charidx) { assert(FALSE); return 0; }
void cc_flww32_get_bitmap_advance(void * font, int glyph, int *x, int *y) { assert(FALSE); }
void cc_flww32_get_vector_advance(void * font, int glyph, float *x, float *y) { assert(FALSE); }
void cc_flww32_get_bitmap_kerning(void * font, int glyph1, int glyph2, int *x, int *y) { assert(FALSE); }
void cc_flww32_get_vector_kerning(void * font, int glyph1, int glyph2, float *x, float *y) { assert(FALSE); }
void cc_flww32_done_glyph(void * font, int glyph) { assert(FALSE); }
  
struct cc_flw_bitmap * cc_flww32_get_bitmap(void * font, int glyph) { assert(FALSE); return NULL; }
struct cc_flw_vector_glyph * cc_flww32_get_vector_glyph(void * font, unsigned int glyph, float complexity){ assert(FALSE); return NULL; }

const float * cc_flww32_get_vector_glyph_coords(struct cc_flw_vector_glyph * vecglyph) { assert(FALSE); return NULL; }
const int * cc_flww32_get_vector_glyph_faceidx(struct cc_flw_vector_glyph * vecglyph) { assert(FALSE); return NULL; }
const int * cc_flww32_get_vector_glyph_edgeidx(struct cc_flw_vector_glyph * vecglyph) { assert(FALSE); return NULL; }


#else /* HAVE_WIN32_API */


static void CALLBACK flww32_vertexCallback(GLvoid * vertex);
static void CALLBACK flww32_beginCallback(GLenum which);
static void CALLBACK flww32_endCallback(void);
static void CALLBACK flww32_combineCallback(GLdouble coords[3], GLvoid * data, GLfloat weight[4], int **dataOut);
static void CALLBACK flww32_errorCallback(GLenum error_code);
static void flww32_addTessVertex(double * vertex);

static void flww32_buildVertexList(struct cc_flw_vector_glyph * newglyph);
static void flww32_buildFaceIndexList(struct cc_flw_vector_glyph * newglyph);
static void flww32_buildEdgeIndexList(struct cc_flw_vector_glyph * newglyph);

static int flww32_calcfontsize(float complexity);


/* ************************************************************************* */

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>

#include <Inventor/C/tidbits.h>
#include <Inventor/C/base/hash.h>
#include <Inventor/C/errors/debugerror.h>
#include <Inventor/C/base/string.h>
#include <Inventor/C/glue/win32api.h>
#include "fontlib_wrapper.h"

/* ************************************************************************* */

typedef struct flww32_tessellator_t {  
  coin_GLUtessellator * tessellator_object;
  SbBool contour_open;

  GLenum triangle_mode;
  int triangle_fan_root_index;
  int triangle_indices[3];
  int triangle_index_counter;
  SbBool triangle_strip_flipflop;

  int vertex_counter;
  float vertex_scale;
  int edge_start_vertex;

  cc_list * vertexlist;
  cc_list * faceindexlist;
  cc_list * edgeindexlist;
} flww32_tessellator_t;

static flww32_tessellator_t flww32_tessellator;
static int flww32_font3dsize = 200;

struct cc_flww32_globals_s {
  /* Offscreen device context for connecting to fonts. */
  HDC devctx;

  /* This is a hash of hashes. The unique keys are HFONT instances,
     which each maps to a new hash. This hash then contains a set of
     glyph ids (i.e. which are the hash keys) which maps to struct
     cc_flw_bitmap instances. */
  cc_hash * font2glyphhash;

  /* This is a hash of hashes. Unique keys are HFONT instances. This again
     contains hashes for each glyph which contain a hash for its 
     pairing glyphs. This again contains the kerning value for that pair. */
  cc_hash * font2kerninghash;

};

static struct cc_flww32_globals_s cc_flww32_globals = {
  NULL, /* devctx */
  NULL /* font2glyphhash */
};

struct cc_flww32_glyph {
  struct cc_flw_bitmap * bitmap;
};

/* Callback functions for cleaning up kerninghash table */
void cc_flww32_kerninghash_deleteCB1(unsigned long key, void * val, void * closure);
void cc_flww32_kerninghash_deleteCB2(unsigned long key, void * val, void * closure);
void cc_flww32_kerninghash_deleteCB3(unsigned long key, void * val, void * closure);

/* ************************************************************************* */

/* dumps debug information about one of the system fonts */
static int CALLBACK
font_enum_proc(ENUMLOGFONTEX * logicalfont, NEWTEXTMETRICEX * physicalfont,
               int fonttype, LPARAM userdata)
{
  cc_string str;
  cc_string_construct(&str);

  cc_string_sprintf(&str,
                    "fontenum: fullname=='%s', style=='%s', script=='%s' "
                    "<h, w>==<%d, %d>",
                    logicalfont->elfFullName, logicalfont->elfStyle,
                    logicalfont->elfScript,
                    logicalfont->elfLogFont.lfHeight,
                    logicalfont->elfLogFont.lfWidth);

  cc_string_append_text(&str, " -- fonttype: ");
  switch (fonttype) {
  case DEVICE_FONTTYPE: cc_string_append_text(&str, "DEVICE"); break;
  case RASTER_FONTTYPE: cc_string_append_text(&str, "RASTER"); break;
  case TRUETYPE_FONTTYPE: cc_string_append_text(&str, "TRUETYPE"); break;
  default: cc_string_append_text(&str, "<unknown>"); break;
  }

  cc_debugerror_postinfo("font_enum_proc", "%s", cc_string_get_text(&str));

  cc_string_clean(&str);
  return 1; /* non-0 to continue enumeration */
}

static cc_hash *
get_glyph_hash(void * font)
{
  void * val;
  SbBool found;

  found = cc_hash_get(cc_flww32_globals.font2glyphhash,
                      (unsigned long)font, &val);
  return found ? ((cc_hash *)val) : NULL;
}

static struct cc_flww32_glyph *
get_glyph_struct(void * font, int glyph)
{
  void * val;
  SbBool found;

  cc_hash * ghash = get_glyph_hash(font);
  if (ghash == NULL) { return NULL; }

  found = cc_hash_get(ghash, (unsigned long)glyph, &val);
  return found ? ((struct cc_flww32_glyph *)val) : NULL;
}

/* ************************************************************************* */

SbBool
cc_flww32_initialize(void)
{
  if (cc_flw_debug()) { /* list all fonts on system */
    LOGFONT logfont; /* logical font information */

    /* Only these are inspected by the EnumFontFamiliesEx()
       function. Set up to list as many fonts as possible. */
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfFaceName[0] = '\0';
    logfont.lfPitchAndFamily = 0;

    /* FIXME: for some peculiar reason, this has stopped working --
       font_enum_proc() is never called. It used to work..?
       Investigate what is going on. 20030610 mortene. */
    (void)EnumFontFamiliesEx(cc_flww32_globals.devctx,
                             (LPLOGFONT)&logfont,
                             (FONTENUMPROC)font_enum_proc,
                             0, /* user data for callback */
                             0); /* reserved, must be zero */
  }

  cc_flww32_globals.devctx = CreateDC("DISPLAY", NULL, NULL, NULL);
  if (cc_flww32_globals.devctx == NULL) {
    cc_win32_print_error("cc_flww32_initialize", "CreateDC()", GetLastError());
    return FALSE;
  }

  cc_flww32_globals.font2glyphhash = cc_hash_construct(17, 0.75);
  cc_flww32_globals.font2kerninghash = cc_hash_construct(17,0.75);

  /* Setup temporary glyph-struct used during for tessellation */
  flww32_tessellator.vertexlist = NULL;
  flww32_tessellator.faceindexlist = NULL;
  flww32_tessellator.edgeindexlist = NULL;

  return TRUE;
}

void
cc_flww32_exit(void)
{
  BOOL ok;

  /* FIXME: this hash should be empty at this point, or it means that
     one or more calls to cc_flww32_done_font() are missing. Should
     insert a check (plus dump) for that here. 20030610 mortene. */
  /* UPDATE: Ditto for the kerning hash. (20030930 handegar) */
  cc_hash_destruct(cc_flww32_globals.font2glyphhash);
  cc_hash_destruct(cc_flww32_globals.font2kerninghash);	

  ok = DeleteDC(cc_flww32_globals.devctx);
  if (!ok) {
    cc_win32_print_error("cc_flww32_exit", "DeleteDC()", GetLastError());
  }
}

/* Callbacks for kerninghash delete */
void 
cc_flww32_kerninghash_deleteCB1(unsigned long key, void * val, void * closure)
{
  cc_hash * khash;   
  khash = (cc_hash *) val;
  cc_hash_apply(khash, cc_flww32_kerninghash_deleteCB2, NULL);
  cc_hash_destruct(khash);
}
void 
cc_flww32_kerninghash_deleteCB2(unsigned long key, void * val, void * closure)
{
  cc_hash * khash;   
  khash = (cc_hash *) val;
  cc_hash_apply(khash, cc_flww32_kerninghash_deleteCB3, NULL);
  cc_hash_destruct(khash);
}
void 
cc_flww32_kerninghash_deleteCB3(unsigned long key, void * val, void * closure)
{
  float * kerning;   
  kerning = (float *) val;
  free(kerning);
}


/* ************************************************************************* */

/* Allocates and returns a new font id matching the exact fontname.
   Returns NULL on error.
*/
void *
cc_flww32_get_font(const char * fontname, int sizex, int sizey)
{
  
  int i;
  DWORD nrkpairs, ret;
  KERNINGPAIR * kpairs;
  cc_hash * khash;
  cc_hash * fontkerninghash;
  float * kerningvalue;
  HFONT previousfont;


  /* FIXME: an idea about sizex / width specification for fonts: let
     sizex==0 indicate "don't care". Should update API and API doc
     upstream to that effect. 20030911 mortene.
  */
  cc_hash * glyphhash;

  HFONT wfont = CreateFont(-sizey, /* Using a negative
                                      'sizey'. Otherwise leads to less
                                      details as it seems like the Win32
                                      systems tries to 'quantize' the
                                      glyph to match the pixels of the
                                      choosen resolution. */
                           0, /* really sizex, but let Win32 choose to
                                 get correct aspect ratio */
                           0, /* escapement */
                           0, /* orientation */
                           FW_DONTCARE, /* weight */
                           FALSE, FALSE, FALSE, /* italic, underline, strikeout */
                           /* FIXME: using DEFAULT_CHARSET is probably
                              not what we should do, AFAICT from a
                              quick read-over of the CreateFont() API
                              doc. 20030530 mortene. */
                           DEFAULT_CHARSET,
                           /* FIXME: to also make it possible to use
                              Window's raster fonts, this should
                              rather be OUT_DEFAULT_PRECIS. Then when
                              GetGlyphOutline() fails on a font, we
                              should grab it's bitmap by using
                              TextOut() and GetDIBits(). 20030610
                              mortene.
                           */
                           OUT_TT_ONLY_PRECIS, /* output precision */
                           CLIP_DEFAULT_PRECIS, /* clipping precision */
                           PROOF_QUALITY, /* output quality */
                           DEFAULT_PITCH, /* pitch and family */
                           fontname); /* typeface name */


  if (!wfont) {
    DWORD lasterr = GetLastError();
    cc_string * str = cc_string_construct_new();
    cc_string_sprintf(str, "CreateFont(%d, %d, ..., '%s')",
                      sizey, sizex, fontname);
    cc_win32_print_error("cc_flww32_get_font", cc_string_get_text(str), lasterr);
    cc_string_destruct(str);
    return NULL;
  }

  glyphhash = cc_hash_construct(127, 0.75);
  cc_hash_put(cc_flww32_globals.font2glyphhash, (unsigned long)wfont, glyphhash);


  /* 
     Constructing a multilevel kerninghash for this font
  */

  previousfont = SelectObject(cc_flww32_globals.devctx, (HFONT)wfont);
  if (previousfont == NULL) {
    cc_win32_print_error("cc_flww32_get_font", "SelectObject()", GetLastError());
    return NULL;
  }

  nrkpairs = GetKerningPairs(cc_flww32_globals.devctx, 0, NULL);
  if (nrkpairs) {
    
    kpairs = (KERNINGPAIR *) malloc(nrkpairs * sizeof(KERNINGPAIR));

    ret = GetKerningPairs(cc_flww32_globals.devctx, nrkpairs, kpairs);
    if (ret == 0) {
      cc_win32_print_error("cc_flww32_get_font", "GetKerningPairs()", GetLastError());
      return NULL;
    }

    if (!cc_hash_get(cc_flww32_globals.font2kerninghash, (unsigned long) wfont, &fontkerninghash)) {
      fontkerninghash = cc_hash_construct(5, 0.75);
      cc_hash_put(cc_flww32_globals.font2kerninghash, (unsigned long) wfont, fontkerninghash);
    }

    for (i=0;i<(int) nrkpairs;++i) {
      if (cc_hash_get(fontkerninghash, kpairs[i].wFirst, &khash)) {
        
        if (!cc_hash_get(khash, kpairs[i].wSecond, &khash)) {
          kerningvalue = (float *) malloc(sizeof(float)); /* Ugly... (handegar)*/
          kerningvalue[0] = (float) kpairs[i].iKernAmount;
          cc_hash_put(khash, kpairs[i].wSecond, kerningvalue);
        }

      } else {
        
        khash = cc_hash_construct(127, 0.75);
        kerningvalue = (float *) malloc(sizeof(float)); /* Ugly... (handegar)*/
        kerningvalue[0] = (float) kpairs[i].iKernAmount;

        /* FIXME: A standalone cc_floathash should have been made so that we dont have to 
           allocate memory to store a single float. We could have used the pointer-value to 
           store the float, but that might cause problems later when we go from 32 to 64 bits.
           (20030929 handegar) */

        cc_hash_put(khash, (unsigned long) kpairs[i].wSecond, kerningvalue);
        cc_hash_put(fontkerninghash, (unsigned long) kpairs[i].wFirst, khash);
        
      }
    }

    free(kpairs);

  } 
  else {
    cc_win32_print_error("cc_flww32_get_font", "GetKerningPairs()", GetLastError());
    return NULL;
  }

  return (void *)wfont;
}

/* Returns with name of given font id in the string. cc_string needs
   to have been constructed properly by caller. */
void
cc_flww32_get_font_name(void * font, cc_string * str)
{
  int size;
  char * s;

  /* Connect device context to font. */
  HFONT previousfont = SelectObject(cc_flww32_globals.devctx, (HFONT)font);
  if (previousfont == NULL) {
    cc_string_set_text(str, "<unknown>");
    cc_win32_print_error("cc_flww32_get_font_name", "SelectObject()", GetLastError());
    return;
  }

  size = cc_win32()->GetTextFace(cc_flww32_globals.devctx, 0, NULL);
  s = (char *)malloc(size);
  assert(s); /* FIXME: handle alloc problem better. 20030530 mortene. */

  (void)cc_win32()->GetTextFace(cc_flww32_globals.devctx, size, s);
  cc_string_set_text(str, s);

  free(s);

  /* Reconnect device context to default font. */
  if (SelectObject(cc_flww32_globals.devctx, previousfont) != (HFONT)font) {
    cc_win32_print_error("cc_flww32_get_font_name", "SelectObject()", GetLastError());
  }
}

/* Deallocates the resources connected with the given font id. Assumes
   that the font will no longer be used. */
void
cc_flww32_done_font(void * font)
{
  BOOL ok;
  SbBool found;
  cc_hash * khash;	

  cc_hash * glyphs = get_glyph_hash(font);
  assert(glyphs && "called with non-existent font");

  found = cc_hash_remove(cc_flww32_globals.font2glyphhash,
                         (unsigned long)font);
  assert(found && "huh?");

  /* FIXME: the hash should really be checked to see if it's empty or
     not, but the cc_flww32_done_glyph() method hasn't been
     implemented yet. 20030610 mortene. */
  cc_hash_destruct(glyphs);
    
  /* Delete kerninghash for this font using apply-callbacks */
  if (cc_hash_get(cc_flww32_globals.font2kerninghash, (unsigned long) font, &khash))
    cc_hash_apply(khash, cc_flww32_kerninghash_deleteCB2, NULL);

  ok = DeleteObject((HFONT)font);
  assert(ok && "DeleteObject() failed, investigate");
}

/* Returns the number of character mappings available for the given
   font. A character mapping can e.g. be "unicode" or "latin_1". */
int
cc_flww32_get_num_charmaps(void * font)
{
  /* FIXME: unimplemented. 20030515 mortene. */
  return 0;
}

/* Returns the name of the character mapping given by the index
   number. */
const char *
cc_flww32_get_charmap_name(void * font, int charmapidx)
{
  /* FIXME: unimplemented. 20030515 mortene. */
  return "unknown";
}

/* Set the current character translation map. */
void
cc_flww32_set_charmap(void * font, int charmap)
{
  /* FIXME: unimplemented. 20030515 mortene. */
}

/* Set the character dimensions of the given font. */
void
cc_flww32_set_char_size(void * font, int width, int height)
{
  /* FIXME: unimplemented. 20030515 mortene. */
}

/* Set a transformation on the font characters that rotates them the
   given angle. Angle specified in radians. */
void
cc_flww32_set_font_rotation(void * font, float angle)
{
  /* FIXME: unimplemented. 20030515 mortene. */
}

/* Returns the glyph index for the given character code. If the
   character code is undefined for this font, returns 0. */
int
cc_flww32_get_glyph(void * font, unsigned int charidx)
{
  /* FIXME: unimplemented. 20030515 mortene. */
  return charidx;
}


/* Returns, in x and y input arguments, how much to advance cursor
   after rendering glyph. */
void
cc_flww32_get_bitmap_advance(void * font, int glyph, int * x, int * y)
{
  struct cc_flww32_glyph * glyphstruct = get_glyph_struct(font, glyph);
#if 0
  /* FIXME: is this too strict? Could make it on demand. Fix if we
     ever run into this assert. 20030610 mortene. */
  assert(glyphstruct && "glyph was not made yet");
  /* UPDATE: assert hits if glyph does not exist. Re-enable assert
     after we have set up the code for making an empty rectangle on
     non-existent glyph. 20030610 mortene. */
#else /* tmp enabled */
  if (glyphstruct == NULL) {
    *x = 10;
    *y = 0;
    return;
  }
#endif

  *x = glyphstruct->bitmap->advanceX;
  *y = glyphstruct->bitmap->advanceY;
}


/* Returns, in x and y input arguments, how much to advance cursor
   after rendering glyph. */
void
cc_flww32_get_vector_advance(void * font, int glyph, float * x, float * y)
{

  LOGFONT lfont;
  GLYPHMETRICS gm;
  static const MAT2 identitymatrix = { { 0, 1 }, { 0, 0 },
                                       { 0, 0 }, { 0, 1 } };
  DWORD ret;
  DWORD size = 0;
  HFONT previousfont;

  /* Connect device context to font. */
  previousfont = SelectObject(cc_flww32_globals.devctx, (HFONT)font);
  if (previousfont == NULL) {
    cc_win32_print_error("cc_flww32_vector_advance", "SelectObject()", GetLastError());
    return;
  }

  /* The GetGlyphOutline function retrieves the outline or bitmap for
     a character in the TrueType font that is selected into the
     specified device context. */
  ret = GetGlyphOutline(cc_flww32_globals.devctx,
                        glyph, /* character to query */
                        GGO_METRICS, /* format of data to return */
                        &gm, /* metrics */
                        0, /* size of buffer for data */
                        NULL, /* buffer for data */
                        &identitymatrix /* transformation matrix */
                        );

  /* As of now, GetGlyphOutline() should have no known reason to
     fail.
     FIXME: We should eventually allow non-TT fonts to be loaded
     aswell, by changing the "precision" setting in the call to
     CreateFont() to also allow raster fonts (see FIXME comment where
     CreateFont() is called). Then, when GetGlyphOutline() fails, use
     TextOut() and GetDIBits() to grab a font glyph's bitmap.
     20030610 mortene.
  */
  if (ret == GDI_ERROR) {
    cc_string str;
    cc_string_construct(&str);
    cc_string_sprintf(&str,
                      "GetGlyphOutline(HDC=%p, 0x%x '%c', GGO_BITMAP, "
                      "<metricsstruct>, 0, NULL, <idmatrix>)",
                      cc_flww32_globals.devctx, glyph, (unsigned char)glyph);
    cc_win32_print_error("cc_flww32_get_bitmap", cc_string_get_text(&str), GetLastError());
    cc_string_clean(&str);
    return;
  }
 
  ret = GetObject((HFONT) font,sizeof(lfont), (LPVOID) &lfont);
  size = -lfont.lfHeight;
  if (ret == 0) {
    cc_win32_print_error("cc_flww32_get_advance", "GetObject()", GetLastError());
    size = 1;
  }
  
  *x = (float) gm.gmCellIncX / ((float) size);
  *y = (float) gm.gmCellIncY / ((float) size);

}

/* Returns kerning, in x and y input arguments, for a pair of
   glyphs. */
void
cc_flww32_get_bitmap_kerning(void * font, int glyph1, int glyph2, int * x, int * y)
{

  float * kerning = NULL;
  cc_hash * khash;	

  if (cc_hash_get(cc_flww32_globals.font2kerninghash, (unsigned long) font, &khash)) {	 
    if (cc_hash_get(khash, (unsigned long) glyph1, &khash)) {
      if (cc_hash_get((cc_hash *) khash, (unsigned long) glyph2, &kerning)) {
        *x = (int) kerning[0];
        *y = 0;
        return;
      }
    }
  }
 
  *x = 0;
  *y = 0;
}

/* Returns kerning, in x and y input arguments, for a pair of
   glyphs. */
void
cc_flww32_get_vector_kerning(void * font, int glyph1, int glyph2, float * x, float * y)
{

  float * kerning = NULL;
  cc_hash * khash;	
  DWORD ret;
  DWORD size;
  LOGFONT lfont;

  ret = GetObject((HFONT) font,sizeof(lfont), (LPVOID) &lfont);
  size = -lfont.lfHeight;
  if (ret == 0) {
    cc_win32_print_error("cc_flww32_get_advance", "GetObject()", GetLastError());
    size = 1;
  }

  if (cc_hash_get(cc_flww32_globals.font2kerninghash, (unsigned long) font, &khash)) {	 
    if (cc_hash_get(khash, (unsigned long) glyph1, &khash)) {
      if (cc_hash_get((cc_hash *) khash, (unsigned long) glyph2, &kerning)) {
        *x = kerning[0] / size;
        *y = 0;
        return;
      }
    }
  }

  *x = 0.0f;
  *y = 0.0f;
}


/* Client should use this to indicate it's done with a glyph, so the
   resources associated with it can be deallocated. */
void
cc_flww32_done_glyph(void * font, int glyph)
{
  /* FIXME: unimplemented. 20030515 mortene. */
}

/* Draws a bitmap for the given glyph. */
struct cc_flw_bitmap *
cc_flww32_get_bitmap(void * font, int glyph)
{
  struct cc_flw_bitmap * bm = NULL;
  GLYPHMETRICS gm;
  static const MAT2 identitymatrix = { { 0, 1 }, { 0, 0 },
                                       { 0, 0 }, { 0, 1 } };
  DWORD ret;
  DWORD size = 0;
  uint8_t * w32bitmap = NULL;
  HFONT previousfont;
  SbBool unused;
  cc_hash * glyphhash;
  struct cc_flww32_glyph * glyphstruct;

  /* See if we can just return the bitmap from cached glyph. */
  glyphstruct = get_glyph_struct(font, glyph);
  if (NULL != glyphstruct) { return glyphstruct->bitmap; }

  /* Connect device context to font. */
  previousfont = SelectObject(cc_flww32_globals.devctx, (HFONT)font);
  if (previousfont == NULL) {
    cc_win32_print_error("cc_flww32_get_font", "SelectObject()", GetLastError());
    return NULL;
  }

  /* The GetGlyphOutline function retrieves the outline or bitmap for
     a character in the TrueType font that is selected into the
     specified device context. */

  ret = GetGlyphOutline(cc_flww32_globals.devctx,
                        glyph, /* character to query */
                        GGO_BITMAP, /* format of data to return */
                        &gm, /* metrics */
                        0, /* size of buffer for data */
                        NULL, /* buffer for data */
                        &identitymatrix /* transformation matrix */
                        );

  /* As of now, GetGlyphOutline() should have no known reason to
     fail.

     FIXME: We should eventually allow non-TT fonts to be loaded
     aswell, by changing the "precision" setting in the call to
     CreateFont() to also allow raster fonts (see FIXME comment where
     CreateFont() is called). Then, when GetGlyphOutline() fails, use
     TextOut() and GetDIBits() to grab a font glyph's bitmap.
     20030610 mortene.
  */
  if (ret == GDI_ERROR) {
    cc_string str;
    cc_string_construct(&str);
    cc_string_sprintf(&str,
                      "GetGlyphOutline(HDC=%p, 0x%x '%c', GGO_BITMAP, "
                      "<metricsstruct>, 0, NULL, <idmatrix>)",
                      cc_flww32_globals.devctx, glyph, (unsigned char)glyph);
    cc_win32_print_error("cc_flww32_get_bitmap", cc_string_get_text(&str), GetLastError());
    cc_string_clean(&str);
    goto done;
  }

  assert((ret < 1024*1024) && "bogus buffer size");
  size = ret;

  /* "size" can be equal to zero, it is for instance known to happen
     for at least the space char glyph for some charsets. */
  if (size > 0) {
    w32bitmap = (uint8_t *)malloc(ret);
    assert(w32bitmap != NULL); /* FIXME: be robust. 20030530 mortene. */

    ret = GetGlyphOutline(cc_flww32_globals.devctx,
                          glyph, /* character to query */
                          GGO_BITMAP, /* format of data to return */
                          &gm, /* metrics */
                          size, /* size of buffer for data */
                          w32bitmap, /* buffer for data */
                          &identitymatrix /* transformation matrix */
                          );

    if (ret == GDI_ERROR) {
      cc_string str;
      cc_string_construct(&str);
      cc_string_sprintf(&str,
                        "GetGlyphOutline(HDC=%p, 0x%x '%c', GGO_BITMAP, "
                        "<metricsstruct>, %d, <buffer>, <idmatrix>)",
                        cc_flww32_globals.devctx, glyph, (unsigned char)glyph, size);
      cc_win32_print_error("cc_flww32_get_bitmap", cc_string_get_text(&str), GetLastError());
      cc_string_clean(&str);
      goto done;
    }
  }

  bm = (struct cc_flw_bitmap *)malloc(sizeof(struct cc_flw_bitmap));
  assert(bm);
  bm->bearingX = gm.gmptGlyphOrigin.x;
  bm->bearingY = gm.gmptGlyphOrigin.y;
  bm->advanceX = gm.gmCellIncX;
  bm->advanceY = gm.gmCellIncY;
  bm->rows = gm.gmBlackBoxY;
  bm->width = gm.gmBlackBoxX;
  bm->pitch = (bm->width + 7) / 8;
  bm->buffer = NULL;
  if (w32bitmap != NULL) { /* Could be NULL for at least space char glyph. */
    unsigned int i;
    bm->buffer = (unsigned char *)malloc(bm->rows * bm->pitch);
    for (i = 0; i < bm->rows; i++) {
      (void)memcpy(&(bm->buffer[i * bm->pitch]),
                   /* the win32 bitmap is doubleword aligned pr row */
                   w32bitmap + i * (((bm->pitch + 3) / 4) * 4),
                   bm->pitch);
    }
  }

  glyphhash = get_glyph_hash(font);
  glyphstruct = (struct cc_flww32_glyph *)malloc(sizeof(struct cc_flww32_glyph));
  glyphstruct->bitmap = bm;
  unused = cc_hash_put(glyphhash, (unsigned long)glyph, glyphstruct);
  assert(unused);

 done:  
  free(w32bitmap);

  /* Reconnect device context to default font. */
  if (SelectObject(cc_flww32_globals.devctx, previousfont) != (HFONT)font) {
    cc_win32_print_error("cc_flww32_get_font", "SelectObject()", GetLastError());
  }
  
  return bm;
}


static void
flww32_getVerticesFromPath(HDC hdc)
{

  double * vertex;
  LPPOINT p_points = NULL;
  LPBYTE p_types = NULL;
  int numpoints, i, lastmoveto;

  if (FlattenPath(hdc) == 0) {
    cc_win32_print_error("getVerticesFromPath", "Failed when handeling TrueType font; "
                         "FlattenPath()", GetLastError());
    /* The system cannot convert splines to vectors. Aborting. */
    return;
  }

  /* determine the number of endpoints in the path*/
  numpoints = GetPath(hdc, NULL, NULL, 0);
  if (numpoints < 0) {
    cc_win32_print_error("getVerticesFromPath", "Failed when handeling TrueType font; "
                         "GetPath()", GetLastError());
    return;    
  }

  if (numpoints > 0) {
    /* allocate memory for the point data and for the vertex types  */
    p_points = (POINT *)malloc(numpoints * sizeof(POINT));
    p_types = (BYTE *)malloc(numpoints * sizeof(BYTE));
        
    /* get the path's description */
    GetPath(hdc, p_points, p_types, numpoints);
                       
    /* go through the endpoints */
    for (i = 0; i < numpoints; i++) {
            	
      /* if this endpoint starts a new contour */
      if (p_types[i] == PT_MOVETO) {

        lastmoveto = i;	
        if (flww32_tessellator.contour_open) {
          GLUWrapper()->gluTessEndContour(flww32_tessellator.tessellator_object);
          cc_list_truncate(flww32_tessellator.edgeindexlist, 
                           cc_list_get_length(flww32_tessellator.edgeindexlist)-1);
          cc_list_append(flww32_tessellator.edgeindexlist, 
                         (void *) (flww32_tessellator.edge_start_vertex));
        }

        GLUWrapper()->gluTessBeginContour(flww32_tessellator.tessellator_object);
        flww32_tessellator.edge_start_vertex = flww32_tessellator.vertex_counter;
        flww32_tessellator.contour_open = TRUE;		
      }                
					
      /* Close the conture? */
      if (p_types[i] & PT_CLOSEFIGURE) {				
        vertex = (double *) malloc(3*sizeof(double));
        vertex [0] = p_points[lastmoveto].x;
        vertex [1] = p_points[lastmoveto].y;
        vertex [2] = 0;
        flww32_addTessVertex(vertex);
        free(vertex);
      } else {
        vertex = (double *) malloc(3*sizeof(double));
        vertex [0] = p_points[i].x;
        vertex [1] = p_points[i].y;
        vertex [2] = 0;
        flww32_addTessVertex(vertex);		
        free(vertex);
      }
    }       
    if (p_points != NULL) free(p_points);
    if (p_types != NULL) free(p_types);	    
  }
    
}

cc_flw_vector_glyph *
cc_flww32_get_vector_glyph(void * font, unsigned int glyph, float complexity)
{

  HDC memdc;
  HBITMAP membmp;
  HDC screendc;
  TCHAR string[1];
  struct cc_flw_vector_glyph * new_vector_glyph;
  cc_string * fontname = NULL;


  if (!GLUWrapper()->available) {
    cc_debugerror_post("cc_flww32_get_vector_glyph",
                       "GLU library could not be loaded.");
    return NULL;
  }

  if ((GLUWrapper()->gluNewTess == NULL) ||
      (GLUWrapper()->gluTessCallback == NULL) ||
      (GLUWrapper()->gluTessBeginPolygon == NULL) ||
      (GLUWrapper()->gluTessEndContour == NULL) ||
      (GLUWrapper()->gluTessEndPolygon == NULL) ||
      (GLUWrapper()->gluDeleteTess == NULL) ||
      (GLUWrapper()->gluTessVertex == NULL) ||
      (GLUWrapper()->gluTessBeginContour == NULL)) {
    cc_debugerror_post("cc_flww32_get_vector_glyph",
                       "Unable to bind required GLU tessellation "
                       "functions for 3D Win32 TrueType font support.");
    return NULL;
  }

 
  /* Due to the way W32 handles the fonts, a new font object must be 
     created if size is to be changed. */
  /* FIXME: the cc_flww32_done_glyph() should have been implemented 
     and called here to prevent possible accumulation of glyphs. 
     (20030912 handegar) */
  fontname = cc_string_construct_new();
  cc_flww32_get_font_name(font, fontname);
  flww32_font3dsize = flww32_calcfontsize(complexity);
  font = cc_flww32_get_font(cc_string_get_text(fontname), flww32_font3dsize, flww32_font3dsize);
  cc_string_destruct(fontname);



  /* 
     If NULL is returned due to an error, glyph3d.c will load the default font instead. 
  */

  memdc = CreateCompatibleDC(NULL);
  if (memdc == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling CreateCompatibleDC(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  screendc = GetDC(NULL);
  if (screendc == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling GetDC(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  membmp = CreateCompatibleBitmap(screendc, 300, 300);
  if (membmp == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling CreateCompatibleBitmap(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (SelectObject(memdc, membmp) == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SelectObject(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (SelectObject(memdc, font) == NULL) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SelectObject(). "
                         "Cannot vectorize font.", GetLastError());
    return NULL;
  }
  
  if (SetBkMode(memdc, TRANSPARENT) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling SetBkMode()", GetLastError());
    /* Not a critical error, continuing. Glyphs might look abit wierd though. */
  }
  if (BeginPath(memdc) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling BeginPath(). Cannot vectorize font.", GetLastError());
    return NULL;
  }
  string[0] = glyph;
  if (TextOut(memdc, 0, 0, string, 1) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling TextOut(). Cannot vectorize font.", GetLastError());
    return NULL;
  }
  if (EndPath(memdc) == 0) {
    cc_win32_print_error("cc_flww32_get_vector_glyph","Error calling EndPath(). Cannot vectorize font.", GetLastError());
    return NULL;
  }

  if (flww32_tessellator.vertexlist == NULL)
    flww32_tessellator.vertexlist = cc_list_construct();
  if (flww32_tessellator.faceindexlist == NULL)
    flww32_tessellator.faceindexlist = cc_list_construct();
  if (flww32_tessellator.edgeindexlist == NULL)
    flww32_tessellator.edgeindexlist = cc_list_construct();
  
  flww32_tessellator.tessellator_object = GLUWrapper()->gluNewTess();
  flww32_tessellator.contour_open = FALSE;
  flww32_tessellator.vertex_scale = 1.0f;
  flww32_tessellator.triangle_mode = 0;
  flww32_tessellator.triangle_index_counter = 0;
  flww32_tessellator.triangle_strip_flipflop = FALSE;
  flww32_tessellator.vertex_counter = 0;


  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_VERTEX, (gluTessCallback_cb_t)flww32_vertexCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_BEGIN, (gluTessCallback_cb_t)flww32_beginCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_END, (gluTessCallback_cb_t)flww32_endCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_COMBINE, (gluTessCallback_cb_t)flww32_combineCallback);
  GLUWrapper()->gluTessCallback(flww32_tessellator.tessellator_object, GLU_TESS_ERROR, (gluTessCallback_cb_t)flww32_errorCallback);

  GLUWrapper()->gluTessBeginPolygon(flww32_tessellator.tessellator_object, NULL);
  flww32_getVerticesFromPath(memdc);
  if (flww32_tessellator.contour_open) {
    GLUWrapper()->gluTessEndContour(flww32_tessellator.tessellator_object);        
    cc_list_truncate(flww32_tessellator.edgeindexlist, 
                     cc_list_get_length(flww32_tessellator.edgeindexlist)-1);
    cc_list_append(flww32_tessellator.edgeindexlist, (void *) (flww32_tessellator.edge_start_vertex));
  }
  GLUWrapper()->gluTessEndPolygon(flww32_tessellator.tessellator_object);  
  GLUWrapper()->gluDeleteTess(flww32_tessellator.tessellator_object);
  
  cc_list_append(flww32_tessellator.faceindexlist, (void *) -1);  
  cc_list_append(flww32_tessellator.edgeindexlist, (void *) -1);

  /* Copy the static vector_glyph struct to a newly allocated struct
     returned to the user. This is done due to the fact that the
     tessellation callback solution needs a static working struct. */
  new_vector_glyph = (struct cc_flw_vector_glyph *) malloc(sizeof(struct cc_flw_vector_glyph));

  flww32_buildVertexList(new_vector_glyph);
  flww32_buildFaceIndexList(new_vector_glyph);
  flww32_buildEdgeIndexList(new_vector_glyph);
  
  return new_vector_glyph; 

}


static void
flww32_addTessVertex(double * vertex)
{

  int * counter;
  float * point;
  point = malloc(sizeof(float)*2);
  point[0] = flww32_tessellator.vertex_scale * ((float) vertex[0]);
  point[1] = flww32_tessellator.vertex_scale * ((float) vertex[1]);
  cc_list_append(flww32_tessellator.vertexlist, point);
  
  cc_list_append(flww32_tessellator.edgeindexlist, (void *) (flww32_tessellator.vertex_counter));

  counter = malloc(sizeof(int));
  counter[0] = flww32_tessellator.vertex_counter++;
  GLUWrapper()->gluTessVertex(flww32_tessellator.tessellator_object, vertex, counter);

  cc_list_append(flww32_tessellator.edgeindexlist, (void *) (flww32_tessellator.vertex_counter));

}



static void CALLBACK
flww32_vertexCallback(GLvoid * data)
{

  int index;
  index = ((int *) data)[0];


  if ((flww32_tessellator.triangle_fan_root_index == -1) &&
      (flww32_tessellator.triangle_index_counter == 0)) {
    flww32_tessellator.triangle_fan_root_index = index;
  }

  if (flww32_tessellator.triangle_mode == GL_TRIANGLE_FAN) {      
    if (flww32_tessellator.triangle_index_counter == 0) {
      flww32_tessellator.triangle_indices[0] = flww32_tessellator.triangle_fan_root_index; 
      flww32_tessellator.triangle_indices[1] = index;
      ++flww32_tessellator.triangle_index_counter;
    } 
    else flww32_tessellator.triangle_indices[flww32_tessellator.triangle_index_counter++] = index;
  }
  else {
    flww32_tessellator.triangle_indices[flww32_tessellator.triangle_index_counter++] = index; 
  }
  
  assert(flww32_tessellator.triangle_index_counter < 4);

  if (flww32_tessellator.triangle_index_counter == 3) {
    
    
    if (flww32_tessellator.triangle_mode == GL_TRIANGLE_STRIP) { 
      if (flww32_tessellator.triangle_strip_flipflop) {        
        index = flww32_tessellator.triangle_indices[1];
        flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
        flww32_tessellator.triangle_indices[2] = index;
      }
    }
    

    cc_list_append(flww32_tessellator.faceindexlist, (void *) flww32_tessellator.triangle_indices[0]);  
    cc_list_append(flww32_tessellator.faceindexlist, (void *) flww32_tessellator.triangle_indices[1]);  
    cc_list_append(flww32_tessellator.faceindexlist, (void *) flww32_tessellator.triangle_indices[2]);  

    if (flww32_tessellator.triangle_mode == GL_TRIANGLE_FAN) {
      flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
      flww32_tessellator.triangle_index_counter = 2;
    }

    else if (flww32_tessellator.triangle_mode == GL_TRIANGLE_STRIP) {

      if (flww32_tessellator.triangle_strip_flipflop) {        
        index = flww32_tessellator.triangle_indices[1];
        flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
        flww32_tessellator.triangle_indices[2] = index;
      }

      flww32_tessellator.triangle_indices[0] = flww32_tessellator.triangle_indices[1];
      flww32_tessellator.triangle_indices[1] = flww32_tessellator.triangle_indices[2];
      flww32_tessellator.triangle_index_counter = 2;    
      flww32_tessellator.triangle_strip_flipflop = !flww32_tessellator.triangle_strip_flipflop;

    } else flww32_tessellator.triangle_index_counter = 0;

  } 

}

static void CALLBACK
flww32_beginCallback(GLenum which)
{
  flww32_tessellator.triangle_mode = which;
  if (which == GL_TRIANGLE_FAN)
    flww32_tessellator.triangle_fan_root_index = -1;
  else
    flww32_tessellator.triangle_fan_root_index = 0;
  flww32_tessellator.triangle_index_counter = 0;
  flww32_tessellator.triangle_strip_flipflop = FALSE;
  
}
    
static void CALLBACK 
flww32_endCallback(void)
{
}

static void CALLBACK 
flww32_combineCallback(GLdouble coords[3], GLvoid * vertex_data, GLfloat weight[4], int **dataOut)
{

  int * ret;  
  float * point;
  point = malloc(sizeof(float)*2);
  point[0] = flww32_tessellator.vertex_scale * ((float) coords[0]);
  point[1] = flww32_tessellator.vertex_scale * ((float) coords[1]);

  cc_list_append(flww32_tessellator.vertexlist, point);

  ret = malloc(sizeof(int));
  ret[0] = flww32_tessellator.vertex_counter++;
  
  *dataOut = ret;

}

static void CALLBACK 
flww32_errorCallback(GLenum error_code)
{
  cc_debugerror_post("flww32_errorCallback","Error when tesselating glyph (GLU errorcode: %d):, investigate.", error_code); 
}

static void
flww32_buildVertexList(struct cc_flw_vector_glyph * newglyph)
{

  int numcoords,i;
  float * coord;

  assert(flww32_tessellator.vertexlist && "Error fetching vector glyph coordinates\n");
  numcoords = cc_list_get_length(flww32_tessellator.vertexlist);

  newglyph->vertices = (float *) malloc(sizeof(float)*numcoords*2);

  for (i=0;i<numcoords;++i) {
    coord = (float *) cc_list_get(flww32_tessellator.vertexlist,i);

    /* Must flip and translate glyph due to the W32 coord system 
       which has a y-axis pointing downwards */
    newglyph->vertices[i*2 + 0] = coord[0] / flww32_font3dsize;
    newglyph->vertices[i*2 + 1] = (-coord[1] / flww32_font3dsize) + 1; 
    free(coord);
  }

  cc_list_destruct(flww32_tessellator.vertexlist);
  flww32_tessellator.vertexlist = NULL;

}

const float *
cc_flww32_get_vector_glyph_coords(struct cc_flw_vector_glyph * vecglyph)
{   
  assert(vecglyph->vertices && "Vertices not initialized properly");
  return vecglyph->vertices;
} 


static void
flww32_buildEdgeIndexList(struct cc_flw_vector_glyph * newglyph)
{

  int i,len;

  assert(flww32_tessellator.edgeindexlist);
 
  len = cc_list_get_length(flww32_tessellator.edgeindexlist);
  newglyph->edgeindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i) 
    newglyph->edgeindices[i] = (int) cc_list_get(flww32_tessellator.edgeindexlist, i);

  cc_list_destruct(flww32_tessellator.edgeindexlist);
  flww32_tessellator.edgeindexlist = NULL;

}

const int *
cc_flww32_get_vector_glyph_edgeidx(struct cc_flw_vector_glyph * vecglyph)
{
  assert(vecglyph->edgeindices && "Edge indices not initialized properly");
  return vecglyph->edgeindices;
} 

static void
flww32_buildFaceIndexList(struct cc_flw_vector_glyph * newglyph)
{
  int len,i;

  assert(flww32_tessellator.faceindexlist);
  
  len = cc_list_get_length(flww32_tessellator.faceindexlist);
  newglyph->faceindices = (int *) malloc(sizeof(int)*len);

  for (i=0;i<len;++i)
    newglyph->faceindices[i] = (int) cc_list_get(flww32_tessellator.faceindexlist, i);
 
  cc_list_destruct(flww32_tessellator.faceindexlist);
  flww32_tessellator.faceindexlist = NULL;

}

const int *
cc_flww32_get_vector_glyph_faceidx(struct cc_flw_vector_glyph * vecglyph)
{  
  assert(vecglyph->faceindices && "Face indices not initialized properly");
  return vecglyph->faceindices;
} 


static int
flww32_calcfontsize(float complexity)
{
  int sizes[] = {10, 25, 50, 100, 500, 1000, 2500, 4000, 6000, 8000, 10000};
  unsigned int index;
  index = (unsigned int) (10*complexity);
  if (index > 10) index = 10;	
  return sizes[index];
}

#endif /* HAVE_WIN32_API */

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

/*!
  For the library/API doc, here's the environment variables
  influencing the OpenGL binding:

  - COIN_DEBUG_GLGLUE: set equal to "1" to make the wrapper
    initalization spit out lots of info about the underlying OpenGL
    implementation.

  - COIN_PREFER_GLPOLYGONOFFSET_EXT: when set to "1" and both
    glPolygonOffset() and glPolygonOffsetEXT() is available, the
    latter will be used. This can be useful to work around a
    problematic glPolygonOffset() implementation for certain SGI
    platforms.

  - COIN_FULL_INDIRECT_RENDERING: set to "1" to let Coin take
    advantage of OpenGL1.1+ and extensions even when doing
    remote/indirect rendering.

    We don't allow this by default now, for mainly two reasons: 1)
    we've seen NVidia GLX bugs when attempting this. 2) We generally
    prefer a "better safe than sorry" strategy.

    We might consider changing this strategy to allow it by default,
    and provide an envvar to turn it off instead -- if we can get
    confirmation that the assumed NVidia driver bug is indeed NVidia's
    problem.

  - COIN_FORCE_GL1_0_ONLY: set to "1" to disallow use of OpenGL1.1+
    and extensions under all circumstances.
*/


/*
  Useful resources:

   - About OpenGL 1.2, 1.3, 1.4:
     <URL:http://www.opengl.org/developers/documentation/OpenGL12.html>
     <URL:http://www.opengl.org/developers/documentation/OpenGL13.html>
     <URL:http://www.opengl.org/developers/documentation/OpenGL14.html>
     (explains all new features in depth)

   - The OpenGL Extension Registry:
     <URL:http://oss.sgi.com/projects/ogl-sample/registry/>

   - A great overview of what OpenGL driver capabilities are available
     for different cards, check out "3D Hardware Info" on
     <URL:http://www.delphi3d.net/>.

   - Brian Paul presentation "Using OpenGL Extensions" from SIGGRAPH '97:
     <URL:http://www.mesa3d.org/brianp/sig97/exten.htm>

   - Sun's man pages:
     <URL:http://wwws.sun.com/software/graphics/OpenGL/manpages>

   - IBM AIX GL man pages (try to find a "more official looking" link):
     <URL:http://molt.zdv.uni-mainz.de/doc_link/en_US/a_doc_lib/libs/openglrf/OpenGLXEnv.htm>

   - HP GL man pages:
     <URL:http://www.hp.com/workstations/support/documentation/manuals/user_guides/graphics/opengl/RefTOC.html>

   - An Apple Technical Q&A on how to do dynamic binding to OpenGL symbols:
     <URL:http://developer.apple.com/qa/qa2001/qa1188.html>

     Full documentation on all "Object File Image" functions, see:
     <URL:http://developer.apple.com/techpubs/macosx/DeveloperTools/MachORuntime/5rt_api_reference/_Object_Fil_e_Functions.html>
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_AGL
#include <AGL/AGL.h>
#endif /* HAVE_AGL */

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/glue/glp.h>

#include <Inventor/C/glue/dl.h>
#include <Inventor/C/base/hash.h>
#include <Inventor/C/errors/debugerror.h>

/*
  FIXME: should make it possible to fetch a single common define from
  include/Inventor/C/glue/dl.h (or dlp.h). 20020919 mortene.
*/
#if defined(HAVE_DL_LIB) || defined(HAVE_WINDLL_RUNTIME_BINDING) || defined(HAVE_DLD_LIB) || defined(HAVE_DYLD_RUNTIME_BINDING)
#define COIN_OPENGL_DYNAMIC_BINDING
#endif /* dynamic binding */

#include <Inventor/C/base/hash.h>
#include <Inventor/C/threads/threadsutilp.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/C/tidbitsp.h>
#include <Inventor/C/glue/gl_wgl.h>
#include <Inventor/C/glue/gl_glx.h>
#include <Inventor/C/glue/gl_agl.h>

static cc_libhandle glglue_self_handle = NULL;
static SbBool glglue_tried_open_self = FALSE;

/* Resolve and return the integer value of an environment variable. */
static int
glglue_resolve_envvar(const char * txt)
{
  const char * val = coin_getenv(txt);
  return val ? atoi(val) : 0;
}

/* Return value of COIN_GLGLUE_NO_RADEON_WARNING environment variable. */
static int
coin_glglue_radeon_warning(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_GLGLUE_NO_RADEON_WARNING"); }
  /* Note the inversion of the envvar value versus the return value. */
  return (d > 0) ? 0 : 1;
}

/* Return value of COIN_GLGLUE_NO_G400_WARNING environment variable. */
static int
coin_glglue_old_matrox_warning(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_GLGLUE_NO_G400_WARNING"); }
  /* Note the inversion of the envvar value versus the return value. */
  return (d > 0) ? 0 : 1;
}

/* Return value of COIN_GLGLUE_NO_ELSA_WARNING environment variable. */
static int
coin_glglue_old_elsa_warning(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_GLGLUE_NO_ELSA_WARNING"); }
  /* Note the inversion of the envvar value versus the return value. */
  return (d > 0) ? 0 : 1;
}

/* Return value of COIN_GLGLUE_NO_SUN_EXPERT3D_WARNING environment variable. */
static int
coin_glglue_sun_expert3d_warning(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_GLGLUE_NO_SUN_EXPERT3D_WARNING"); }
  /* Note the inversion of the envvar value versus the return value. */
  return (d > 0) ? 0 : 1;
}

/* Return value of COIN_DEBUG_GLGLUE environment variable. */
int
coin_glglue_debug(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_DEBUG_GLGLUE"); }
  return (d > 0) ? 1 : 0;
}

/* Return value of COIN_PREFER_GLPOLYGONOFFSET_EXT environment variable. */
static int
glglue_prefer_glPolygonOffsetEXT(void)
{
  static int d = -1;
  if (d == -1) { d = glglue_resolve_envvar("COIN_PREFER_GLPOLYGONOFFSET_EXT"); }
  return (d > 0) ? 1 : 0;
}

/* doc in header file */
void *
cc_glglue_getprocaddress(const char * symname)
{
  void * ptr = NULL;

  ptr = coin_wgl_getprocaddress(symname);
  if (ptr) goto returnpoint;

  ptr = glxglue_getprocaddress(symname);
  if (ptr) goto returnpoint;

  ptr = coin_agl_getprocaddress(symname);
  if (ptr) goto returnpoint;

  if (glglue_self_handle) {
    ptr = cc_dl_sym(glglue_self_handle, symname);
    if (ptr) goto returnpoint;
  }

returnpoint:
  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("cc_glglue_getprocaddress", "%s==%p", symname, ptr);
  }
  return ptr;
}

/* Global dictionary which stores the mappings from the context IDs to
   actual cc_glglue instances. */
static cc_hash * gldict = NULL;

static void
free_glglue_instance(unsigned long key, void * value, void * closure)
{
  free(value);
}

/* Cleans up at exit. */
static void
glglue_cleanup(void)
{
  cc_hash_apply(gldict, free_glglue_instance, NULL);
  cc_hash_destruct(gldict);
  if (glglue_self_handle) cc_dl_close(glglue_self_handle);
}

/*
  Set the OpenGL version variables in the given cc_glglue struct
  instance.

  Note: this code has been copied from GLUWrapper.c, so if any changes
  are made, make sure they are propagated over if necessary.
*/
static void
glglue_set_glVersion(cc_glglue * w)
{
  char buffer[256];
  char * dotptr;

  /* NB: if you are getting a crash here, it's because an attempt at
   * setting up a cc_glglue instance was made when there is no current
   * OpenGL context. */
  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glglue_set_glVersion",
                           "glGetString(GL_VERSION)=='%s'", w->versionstr);
  }

  w->version.major = 0;
  w->version.minor = 0;
  w->version.release = 0;

  (void)strncpy(buffer, (const char *)w->versionstr, 255);
  buffer[255] = '\0'; /* strncpy() will not null-terminate if strlen > 255 */
  dotptr = strchr(buffer, '.');
  if (dotptr) {
    char * spaceptr;
    char * start = buffer;
    *dotptr = '\0';
    w->version.major = atoi(start);
    start = ++dotptr;

    dotptr = strchr(start, '.');
    spaceptr = strchr(start, ' ');
    if (!dotptr && spaceptr) dotptr = spaceptr;
    if (dotptr && spaceptr && spaceptr < dotptr) dotptr = spaceptr;
    if (dotptr) {
      int terminate = *dotptr == ' ';
      *dotptr = '\0';
      w->version.minor = atoi(start);
      if (!terminate) {
        start = ++dotptr;
        dotptr = strchr(start, ' ');
        if (dotptr) *dotptr = '\0';
        w->version.release = atoi(start);
      }
    }
    else {
      w->version.minor = atoi(start);
    }
  }

  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("glglue_set_glVersion",
                           "parsed to major=='%d', minor=='%d', micro=='%d'",
                           w->version.major,
                           w->version.minor,
                           w->version.release);
  }
}

void
cc_glglue_glversion(const cc_glglue * w,
                    unsigned int * major,
                    unsigned int * minor,
                    unsigned int * release)
{
  *major = w->version.major;
  *minor = w->version.minor;
  *release = w->version.release;
}


SbBool
cc_glglue_glversion_matches_at_least(const cc_glglue * w,
                                     unsigned int major,
                                     unsigned int minor,
                                     unsigned int revision)
{
  if (w->version.major < major) return FALSE;
  else if (w->version.major > major) return TRUE;
  if (w->version.minor < minor) return FALSE;
  else if (w->version.minor > minor) return TRUE;
  if (w->version.release < revision) return FALSE;
  return TRUE;
}

SbBool
cc_glglue_glxversion_matches_at_least(const cc_glglue * w,
                                      int major,
                                      int minor)
{
  if (w->glx.version.major < major) return FALSE;
  else if (w->glx.version.major > major) return TRUE;
  if (w->glx.version.minor < minor) return FALSE;
  return TRUE;
}

/* Sanity checks for enum _EXT value assumed to be equal to the final
 * / "proper" / standard OpenGL enum values. (If not, we could end up
 * with hard-to-find bugs because of mismatches with the compiled
 * values versus the run-time values.)
 *
 * This doesn't really _fix_ anything, it is just meant as an aid to
 * smoke out platforms where we're getting unexpected enum values. */
static void
glglue_sanity_check_enums(void)
{
  SbBool enumsok = TRUE;

#ifdef GL_CLAMP_TO_EDGE_EXT
  enumsok = enumsok && (GL_CLAMP_TO_EDGE == GL_CLAMP_TO_EDGE_EXT);
#endif /* GL_CLAMP_TO_EDGE_EXT */
#ifdef GL_CLAMP_TO_EDGE_SGIS /* Sanity check */
  enumsok = enumsok && (GL_CLAMP_TO_EDGE == GL_CLAMP_TO_EDGE_SGIS);
#endif /* GL_CLAMP_TO_EDGE_SGIS */
#ifdef GL_MAX_3D_TEXTURE_SIZE_EXT
  enumsok = enumsok && (GL_MAX_3D_TEXTURE_SIZE == GL_MAX_3D_TEXTURE_SIZE_EXT);
#endif /* GL_MAX_3D_TEXTURE_SIZE_EXT */
#ifdef GL_PACK_IMAGE_HEIGHT_EXT
  enumsok = enumsok && (GL_PACK_IMAGE_HEIGHT == GL_PACK_IMAGE_HEIGHT_EXT);
#endif /* GL_PACK_IMAGE_HEIGHT_EXT */
#ifdef GL_PACK_SKIP_IMAGES_EXT
  enumsok = enumsok && (GL_PACK_SKIP_IMAGES == GL_PACK_SKIP_IMAGES_EXT);
#endif /* GL_PACK_SKIP_IMAGES_EXT */
#ifdef GL_PROXY_TEXTURE_2D_EXT
  enumsok = enumsok && (GL_PROXY_TEXTURE_2D == GL_PROXY_TEXTURE_2D_EXT);
#endif /* GL_PROXY_TEXTURE_2D_EXT */
#ifdef GL_PROXY_TEXTURE_3D_EXT
  enumsok = enumsok && (GL_PROXY_TEXTURE_3D == GL_PROXY_TEXTURE_3D_EXT);
#endif /* GL_PROXY_TEXTURE_3D_EXT */
#ifdef GL_TEXTURE_3D_EXT
  enumsok = enumsok && (GL_TEXTURE_3D == GL_TEXTURE_3D_EXT);
#endif /* GL_TEXTURE_3D_EXT */
#ifdef GL_TEXTURE_DEPTH_EXT
  enumsok = enumsok && (GL_TEXTURE_DEPTH == GL_TEXTURE_DEPTH_EXT);
#endif /* GL_TEXTURE_DEPTH_EXT */
#ifdef GL_TEXTURE_WRAP_R_EXT
  enumsok = enumsok && (GL_TEXTURE_WRAP_R == GL_TEXTURE_WRAP_R_EXT);
#endif /* GL_TEXTURE_WRAP_R_EXT */
#ifdef GL_UNPACK_IMAGE_HEIGHT_EXT
  enumsok = enumsok && (GL_UNPACK_IMAGE_HEIGHT == GL_UNPACK_IMAGE_HEIGHT_EXT);
#endif /* GL_UNPACK_IMAGE_HEIGHT_EXT */
#ifdef GL_UNPACK_SKIP_IMAGES_EXT
  enumsok = enumsok && (GL_UNPACK_SKIP_IMAGES == GL_UNPACK_SKIP_IMAGES_EXT);
#endif /* GL_UNPACK_SKIP_IMAGES_EXT */
#ifdef GL_FUNC_ADD_EXT
  enumsok = enumsok && (GL_FUNC_ADD == GL_FUNC_ADD_EXT);
#endif /* GL_FUNC_ADD_EXT */
#ifdef GL_MIN_EXT
  enumsok = enumsok && (GL_MIN == GL_MIN_EXT);
#endif /* GL_MIN_EXT */
#ifdef GL_MAX_EXT
  enumsok = enumsok && (GL_MAX == GL_MAX_EXT);
#endif /* GL_MAX_EXT */
#ifdef GL_COLOR_TABLE_WIDTH_EXT
  enumsok = enumsok && (GL_COLOR_TABLE_WIDTH == GL_COLOR_TABLE_WIDTH_EXT);
#endif /* GL_COLOR_TABLE_WIDTH_EXT */

  assert(enumsok && "OpenGL enum value assumption(s) failed!");
}

int
coin_glglue_extension_available(const char * extensions, const char * ext)
{
  const char * start;
  int extlen;
  SbBool found = FALSE;

  assert(ext && "NULL string");
  assert((ext[0] != '\0') && "empty string");
  assert((strchr(ext, ' ') == NULL) && "extension name can't have spaces");

  start = extensions;
  extlen = strlen(ext);

  while (1) {
    const char * where = strstr(start, ext);
    if (!where) goto done;

    if (where == start || *(where - 1) == ' ') {
      const char * terminator = where + extlen;
      if (*terminator == ' ' || *terminator == '\0') {
        found = TRUE;
        goto done;
      }
    }

    start = where + extlen;
  }

done:
  if (coin_glglue_debug()) {
    cc_debugerror_postinfo("coin_glglue_extension_available",
                           "extension '%s' is%s present",
                           ext, found ? "" : " NOT");
  }

  return found ? 1 : 0;
}

int
cc_glglue_glext_supported(const cc_glglue * wrapper, const char * extension)
{
  /* FIXME: should do caching of results here for guaranteed quick
     look-ups. (And then get rid of caching in
     SoGLCacheContextElement::extSupported(). 20020926 mortene. */
  return coin_glglue_extension_available(wrapper->extensionsstr, extension);
}

#ifdef COIN_OPENGL_DYNAMIC_BINDING

#define PROC(_func_) cc_glglue_getprocaddress(SO__QUOTE(_func_))

/* The OpenGL library which we dynamically pick up symbols from
   /could/ have all these defined. For the code below which tries to
   dynamically resolve the methods, we will assume that they are all
   defined. By doing this little "trick", can we use the same code
   below for resolving stuff dynamically as we need anyway to resolve
   in a static manner. */
#define GL_VERSION_1_1 1
#define GL_VERSION_1_2 1
#define GL_VERSION_1_3 1
#define GL_VERSION_1_4 1
#define GL_EXT_polygon_offset 1
#define GL_EXT_texture_object 1
#define GL_EXT_subtexture 1
#define GL_EXT_texture3D 1
#define GL_ARB_multitexture 1
#define GL_ARB_texture_compression 1
#define GL_EXT_paletted_texture 1
#define GL_ARB_imaging 1
#define GL_EXT_blend_minmax 1
#define GL_EXT_color_table 1
#define GL_EXT_color_subtable 1
#define GL_SGI_color_table 1
#define GL_SGI_texture_color_table 1

#define GLX_VERSION_1_1 1
#define GLX_VERSION_1_2 1
#define GLX_VERSION_1_3 1
#define GLX_EXT_import_context 1

#else /* static binding */

#define PROC(_func_) (&_func_)

#endif /* static binding */


static void
glglue_resolve_symbols(cc_glglue * w)
{
  /* Note that there's a good reason why we use version checking
     *along* with dynamic resolving (if the platform allows it): the
     *OpenGL library could include */

  /* Appeared in OpenGL v1.1. We store both the "real" function
     pointer and the extension pointer, in case we need to work around
     an SGI bug (see comments in cc_glglue_glPolygonOffset(). */
  w->glPolygonOffset = NULL;
  w->glPolygonOffsetEXT = NULL;
#ifdef GL_VERSION_1_1
  if (cc_glglue_glversion_matches_at_least(w, 1, 1, 0)) {
    w->glPolygonOffset = (COIN_PFNGLPOLYGONOFFSETPROC)PROC(glPolygonOffset);
  }
#endif /* GL_VERSION_1_1 */
#ifdef GL_EXT_polygon_offset
  if (cc_glglue_glext_supported(w, "GL_EXT_polygon_offset")) {
    w->glPolygonOffsetEXT = (COIN_PFNGLPOLYGONOFFSETPROC)PROC(glPolygonOffsetEXT);
  }
#endif /* GL_EXT_polygon_offset */



  /* Appeared in OpenGL v1.1. */
  w->glGenTextures = NULL;
  w->glBindTexture = NULL;
  w->glDeleteTextures = NULL;
#ifdef GL_VERSION_1_1
  if (cc_glglue_glversion_matches_at_least(w, 1, 1, 0)) {
    w->glGenTextures = (COIN_PFNGLGENTEXTURESPROC)PROC(glGenTextures);
    w->glBindTexture = (COIN_PFNGLBINDTEXTUREPROC)PROC(glBindTexture);
    w->glDeleteTextures = (COIN_PFNGLDELETETEXTURESPROC)PROC(glDeleteTextures);
  }
#endif /* GL_VERSION_1_1 */
#ifdef GL_EXT_texture_object
  if (!w->glGenTextures && cc_glglue_glext_supported(w, "GL_EXT_texture_object")) {
    w->glGenTextures = (COIN_PFNGLGENTEXTURESPROC)PROC(glGenTexturesEXT);
    w->glBindTexture = (COIN_PFNGLBINDTEXTUREPROC)PROC(glBindTextureEXT);
    w->glDeleteTextures = (COIN_PFNGLDELETETEXTURESPROC)PROC(glDeleteTexturesEXT);
  }
#endif /* GL_EXT_texture_object */

  /* Appeared in OpenGL v1.1. */
  w->glTexSubImage2D = NULL;
#ifdef GL_VERSION_1_1
  if (cc_glglue_glversion_matches_at_least(w, 1, 1, 0)) {
    w->glTexSubImage2D = (COIN_PFNGLTEXSUBIMAGE2DPROC)PROC(glTexSubImage2D);
  }
#endif /* GL_VERSION_1_1 */
#ifdef GL_EXT_subtexture
  if (!w->glTexSubImage2D && cc_glglue_glext_supported(w, "GL_EXT_subtexture")) {
    w->glTexSubImage2D = (COIN_PFNGLTEXSUBIMAGE2DPROC)PROC(glTexSubImage2DEXT);
  }
#endif /* GL_EXT_subtexture */

  /* These were introduced with OpenGL v1.2. */
  w->glTexImage3D = NULL;
  w->glCopyTexSubImage3D = NULL;
  w->glTexSubImage3D = NULL;
#ifdef GL_VERSION_1_2
  if (cc_glglue_glversion_matches_at_least(w, 1, 2, 0)) {
    w->glTexImage3D = (COIN_PFNGLTEXIMAGE3DPROC)PROC(glTexImage3D);
    w->glCopyTexSubImage3D = (COIN_PFNGLCOPYTEXSUBIMAGE3DPROC)PROC(glCopyTexSubImage3D);
    w->glTexSubImage3D = (COIN_PFNGLTEXSUBIMAGE3DPROC)PROC(glTexSubImage3D);
  }
#endif /* GL_VERSION_1_2 */
#ifdef GL_EXT_texture3D
  if (!w->glTexImage3D && cc_glglue_glext_supported(w, "GL_EXT_texture3D")) {
    w->glTexImage3D = (COIN_PFNGLTEXIMAGE3DPROC)PROC(glTexImage3DEXT);
    /* These are implicitly given if GL_EXT_texture3D is defined. */
    w->glCopyTexSubImage3D = (COIN_PFNGLCOPYTEXSUBIMAGE3DPROC)PROC(glCopyTexSubImage3DEXT);
    w->glTexSubImage3D = (COIN_PFNGLTEXSUBIMAGE3DPROC)PROC(glTexSubImage3DEXT);
  }
#endif /* GL_EXT_texture3D */


  /* Appeared in OpenGL v1.3. */
  w->glActiveTexture = NULL;
  w->glMultiTexCoord2f = NULL;
#ifdef GL_VERSION_1_3
  if (cc_glglue_glversion_matches_at_least(w, 1, 3, 0)) {
    w->glActiveTexture = (COIN_PFNGLACTIVETEXTUREPROC)PROC(glActiveTexture);
    w->glMultiTexCoord2f = (COIN_PFNGLMULTITEXCOORD2FPROC)PROC(glMultiTexCoord2f);
  }
#endif /* GL_VERSION_1_3 */
#ifdef GL_ARB_multitexture
  if (!w->glActiveTexture && cc_glglue_glext_supported(w, "GL_ARB_multitexture")) {
    w->glActiveTexture = (COIN_PFNGLACTIVETEXTUREPROC)PROC(glActiveTextureARB);
    w->glMultiTexCoord2f = (COIN_PFNGLMULTITEXCOORD2FPROC)PROC(glMultiTexCoord2fARB);
  }
#endif /* GL_ARB_multitexture */

  /* SGI's glx.h header file shipped with the NVidia Linux drivers
     identifies glXGetCurrentDisplay() as a GLX 1.3 method, but Sun's
     GL man pages lists it as a GLX 1.2 function, ditto for HP's GL
     man pages, and ditto for AIX's man pages. (See top of this file
     for URL). So we will assume the man pages are correct.
  */
  w->glXGetCurrentDisplay = NULL;
#ifdef GLX_VERSION_1_2
  if (cc_glglue_glxversion_matches_at_least(w, 1, 2)) {
    w->glXGetCurrentDisplay = (COIN_PFNGLXGETCURRENTDISPLAYPROC)PROC(glXGetCurrentDisplay);
  }
#endif /* GLX_VERSION_1_2 */
#ifdef GLX_EXT_import_context
  if (!w->glXGetCurrentDisplay && glxglue_ext_supported(w, "GLX_EXT_import_context")) {
    w->glXGetCurrentDisplay = (COIN_PFNGLXGETCURRENTDISPLAYPROC)PROC(glXGetCurrentDisplayEXT);
  }
#endif /* GLX_EXT_import_context */

  w->glCompressedTexImage1D = NULL;
  w->glCompressedTexImage2D = NULL;
  w->glCompressedTexImage3D = NULL;
  w->glCompressedTexSubImage1D = NULL;
  w->glCompressedTexSubImage2D = NULL;
  w->glCompressedTexSubImage3D = NULL;
  w->glGetCompressedTexImage = NULL;

#ifdef GL_VERSION_1_3
  if (cc_glglue_glversion_matches_at_least(w, 1, 3, 0)) {
    w->glCompressedTexImage1D = (COIN_PFNGLCOMPRESSEDTEXIMAGE1DPROC)PROC(glCompressedTexImage1D);
    w->glCompressedTexImage2D = (COIN_PFNGLCOMPRESSEDTEXIMAGE2DPROC)PROC(glCompressedTexImage2D);
    w->glCompressedTexImage3D = (COIN_PFNGLCOMPRESSEDTEXIMAGE3DPROC)PROC(glCompressedTexImage3D);
    w->glCompressedTexSubImage1D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)PROC(glCompressedTexSubImage1D);
    w->glCompressedTexSubImage2D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)PROC(glCompressedTexSubImage2D);
    w->glCompressedTexSubImage3D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)PROC(glCompressedTexSubImage3D);
    w->glGetCompressedTexImage = (COIN_PFNGLGETCOMPRESSEDTEXIMAGEPROC)PROC(glGetCompressedTexImage);
  }
#endif /* GL_VERSION_1_3 */

#ifdef GL_ARB_texture_compression
  if ((w->glCompressedTexImage1D == NULL) && 
      cc_glglue_glext_supported(w, "GL_ARB_texture_compression")) {
    w->glCompressedTexImage1D = (COIN_PFNGLCOMPRESSEDTEXIMAGE1DPROC)PROC(glCompressedTexImage1DARB);
    w->glCompressedTexImage2D = (COIN_PFNGLCOMPRESSEDTEXIMAGE2DPROC)PROC(glCompressedTexImage2DARB);
    w->glCompressedTexImage3D = (COIN_PFNGLCOMPRESSEDTEXIMAGE3DPROC)PROC(glCompressedTexImage3DARB);
    w->glCompressedTexSubImage1D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)PROC(glCompressedTexSubImage1DARB);
    w->glCompressedTexSubImage2D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)PROC(glCompressedTexSubImage2DARB);
    w->glCompressedTexSubImage3D = (COIN_PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)PROC(glCompressedTexSubImage3DARB);
    w->glGetCompressedTexImage = (COIN_PFNGLGETCOMPRESSEDTEXIMAGEPROC)PROC(glGetCompressedTexImageARB);
  }
#endif /* GL_ARB_texture_compression */

  w->glColorTable = NULL;
  w->glColorSubTable = NULL;
  w->glGetColorTable = NULL;
  w->glGetColorTableParameteriv = NULL;
  w->glGetColorTableParameterfv = NULL;

#if defined(GL_VERSION_1_2) && defined(GL_ARB_imaging)
  if (cc_glglue_glversion_matches_at_least(w, 1, 2, 0) &&
      cc_glglue_glext_supported(w, "GL_ARB_imaging")) {
    w->glColorTable = (COIN_PFNGLCOLORTABLEPROC)PROC(glColorTable);
    w->glColorSubTable = (COIN_PFNGLCOLORSUBTABLEPROC)PROC(glColorSubTable);
    w->glGetColorTable = (COIN_PFNGLGETCOLORTABLEPROC)PROC(glGetColorTable);
    w->glGetColorTableParameteriv = (COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)PROC(glGetColorTableParameteriv);
    w->glGetColorTableParameterfv = (COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)PROC(glGetColorTableParameterfv);
  }
#endif /* GL_VERSION_1_2 && GL_ARB_imaging */

#if defined(GL_EXT_color_table)
  if ((w->glColorTable == NULL) &&
      cc_glglue_glext_supported(w, "GL_EXT_color_table")) {
    w->glColorTable = (COIN_PFNGLCOLORTABLEPROC)PROC(glColorTableEXT);
    w->glGetColorTable = (COIN_PFNGLGETCOLORTABLEPROC)PROC(glGetColorTableEXT);
    w->glGetColorTableParameteriv = (COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)PROC(glGetColorTableParameterivEXT);
    w->glGetColorTableParameterfv = (COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)PROC(glGetColorTableParameterfvEXT);
  }
#endif /* GL_EXT_color_table */

#if defined(GL_SGI_color_table)
  if ((w->glColorTable == NULL) &&
      cc_glglue_glext_supported(w, "GL_SGI_color_table")) {
    w->glColorTable = (COIN_PFNGLCOLORTABLEPROC)PROC(glColorTableSGI);
    w->glGetColorTable = (COIN_PFNGLGETCOLORTABLEPROC)PROC(glGetColorTableSGI);
    w->glGetColorTableParameteriv = (COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)PROC(glGetColorTableParameterivSGI);
    w->glGetColorTableParameterfv = (COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)PROC(glGetColorTableParameterfvSGI);
  }
#endif /* GL_SGI_color_table */

#if defined(GL_EXT_color_subtable)
  if ((w->glColorSubTable == NULL) &&
      cc_glglue_glext_supported(w, "GL_EXT_color_subtable")) {
    w->glColorSubTable = (COIN_PFNGLCOLORSUBTABLEPROC)PROC(glColorSubTableEXT);
  }
#endif /* GL_EXT_color_subtable */

  w->supportsPalettedTextures =
    cc_glglue_glext_supported(w, "GL_EXT_paletted_texture");

#ifdef GL_EXT_paletted_texture
  /* Note that EXT_paletted_texture defines glColorTableEXT et al
     "on it's own", i.e. it doesn't need the presence of
     EXT_color_table / SGI_color_table / OGL1.2+ + ARB_imaging. It
     only defines a *subset* of what EXT_color_table etc defines,
     though. */
  if ((w->glColorTable == NULL) && 
      cc_glglue_glext_supported(w, "GL_EXT_paletted_texture")) {
    w->glColorTable = (COIN_PFNGLCOLORTABLEPROC)PROC(glColorTableEXT);
    w->glColorSubTable = (COIN_PFNGLCOLORSUBTABLEPROC)PROC(glColorSubTableEXT);
    w->glGetColorTable = (COIN_PFNGLGETCOLORTABLEPROC)PROC(glGetColorTableEXT);
    w->glGetColorTableParameteriv = (COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)PROC(glGetColorTableParameterivEXT);
    w->glGetColorTableParameterfv = (COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)PROC(glGetColorTableParameterfvEXT);
  }
#endif /* GL_EXT_paletted_texture */

  /*
    Using the SGI_texture_color_table extension has been temporarily
    disabled, as it uses a different enum value for
    glColorTable(<target>,...), and seems to only support 2D
    textures. Quoting from the extension spec document:

        Accepted by the <cap> parameter of Enable, Disable, and
        IsEnabled, [...] and by the <target> parameter of
        ColorTableSGI, CopyColorTableSGI, GetColorTableSGI,
        ColorTableParameterfvSGI, ColorTableParameterivSGI,
        GetColorTableParameterfvSGI, GetColorTableParameterivSGI:

	TEXTURE_COLOR_TABLE_SGI		0x80BC

        Accepted by the <target> parameter of ColorTableSGI,
        GetColorTableParameterivSGI, and GetColorTableParameterfvSGI:

	PROXY_TEXTURE_COLOR_TABLE_SGI	0x80BD

    As paletted textures can only be supported through extensions, we
    should probably implement support for using this one in addition
    to EXT_paletted_texture.

    Note: our O2 supports this extension, but not
    EXT_paletted_texture, so it can be used for development and
    testing of support for this extension.

    20030129 mortene.
   */
#if 0
  w->supportsPalettedTextures = w->supportsPalettedTextures ||
    cc_glglue_glext_supported(w, "GL_SGI_texture_color_table");

#ifdef GL_SGI_texture_color_table
  /* Note that SGI_texture_color_table defines glColorTableEXT et al
     "on it's own", i.e. it doesn't need the presence of
     EXT_color_table / SGI_color_table / OGL1.2+ + ARB_imaging. It
     only defines a *subset* of what EXT_color_table etc defines,
     though. */
  if ((w->glColorTable == NULL) &&
      cc_glglue_glext_supported(w, "SGI_texture_color_table")) {
    w->glColorTable = (COIN_PFNGLCOLORTABLEPROC)PROC(glColorTableSGI);
    w->glGetColorTable = (COIN_PFNGLGETCOLORTABLEPROC)PROC(glGetColorTableSGI);
    w->glGetColorTableParameteriv = (COIN_PFNGLGETCOLORTABLEPARAMETERIVPROC)PROC(glGetColorTableParameterivSGI);
    w->glGetColorTableParameterfv = (COIN_PFNGLGETCOLORTABLEPARAMETERFVPROC)PROC(glGetColorTableParameterfvSGI);
  }
#endif /* GL_SGI_texture_color_table */
#endif /* disabled */


  w->glBlendEquation = NULL;
  w->glBlendEquationEXT = NULL;

#if defined(GL_VERSION_1_4)
  if (cc_glglue_glversion_matches_at_least(w, 1, 4, 0)) {
    w->glBlendEquation = (COIN_PFNGLBLENDEQUATIONPROC)PROC(glBlendEquation);
  }
#endif /* GL_VERSION_1_4 */

  if (w->glBlendEquation == NULL) {
#if defined(GL_VERSION_1_2) && defined(GL_ARB_imaging)
    if (cc_glglue_glversion_matches_at_least(w, 1, 2, 0) &&
        cc_glglue_glext_supported(w, "GL_ARB_imaging")) {
      w->glBlendEquation = (COIN_PFNGLBLENDEQUATIONPROC)PROC(glBlendEquation);
    }
#endif /* GL_VERSION_1_2 && GL_ARB_imaging */
  }

#ifdef GL_EXT_blend_minmax
  if (cc_glglue_glext_supported(w, "GL_EXT_blend_minmax")) {
    w->glBlendEquationEXT = (COIN_PFNGLBLENDEQUATIONPROC)PROC(glBlendEquationEXT);
  }
#endif /* GL_EXT_blend_minmax */
}

#undef PROC


/* Give warnings on known faulty drivers. */
static void
glglue_check_driver(const char * vendor, const char * renderer,
                    const char * version)
{
#ifdef COIN_DEBUG
  /* Only spit out this in debug builds, as the bug was never properly
     confirmed. */
  if (coin_glglue_radeon_warning()) {
    if (strcmp(renderer, "Radeon 7500 DDR x86/SSE2") == 0) {
      cc_debugerror_postwarning("glglue_check_driver",
                                "We've had an unconfirmed bugreport that "
                                "this OpenGL driver ('%s') may crash upon "
                                "attempts to use 3D texturing. "
                                "We would like to get assistance to help "
                                "us debug the cause of this problem, so "
                                "please get in touch with us at "
                                "<coin-support@coin3d.org>. "
                                "This debug message can be turned off "
                                "permanently by setting the environment "
                                "variable COIN_GLGLUE_NO_RADEON_WARNING=1.",
                                renderer);

      /*
        Some additional information:

        The full driver information for the driver where this was
        reported is as follows:

        GL_VENDOR == 'ATI Technologies Inc.'
        GL_RENDERER == 'Radeon 7500 DDR x86/SSE2'
        GL_VERSION == '1.3.3302 Win2000 Release'

        The driver was reported to crash on MSWin with the
        SoGuiExamples/nodes/texture3 example. The reporter couldn't
        help us debug it, as he could a) not get a call-stack
        backtrace, and b) swapped his card for an NVidia card.

        Perhaps we should get hold of a Radeon card ourselves, to test
        and debug the problem.

        <mortene@sim.no>
      */
    }
  }
#endif /* COIN_DEBUG */

  if (coin_glglue_old_matrox_warning() &&
      (strcmp(renderer, "Matrox G400") == 0) &&
      (strcmp(version, "1.1.3 Aug 30 2001") == 0)) {
    cc_debugerror_postwarning("glglue_check_driver",
                              "This old OpenGL driver (\"%s\" \"%s\") has "
                              "known bugs, please upgrade.  "
                              "(This debug message can be turned off "
                              "permanently by setting the environment "
                              "variable COIN_GLGLUE_NO_G400_WARNING=1).",
                              renderer, version);
  }

  if (coin_glglue_old_elsa_warning() &&
      (strcmp(renderer, "ELSA TNT2 Vanta/PCI/SSE") == 0) &&
      (strcmp(version, "1.1.4 (4.06.00.266)") == 0)) {
    cc_debugerror_postwarning("glglue_check_driver",
                              "This old OpenGL driver (\"%s\" \"%s\") has "
                              "known bugs, please upgrade.  "
                              "(This debug message can be turned off "
                              "permanently by setting the environment "
                              "variable COIN_GLGLUE_NO_ELSA_WARNING=1).",
                              renderer, version);
  }

  /*
    The full driver information for the driver where this was reported
    is as follows:

    GL_VENDOR == 'Matrox Graphics Inc.'
    GL_RENDERER == 'Matrox G400'
    GL_VERSION == '1.1.3 Aug 30 2001'

    GL_VENDOR == 'ELSA AG (Aachen, Germany).'
    GL_RENDERER == 'ELSA TNT2 Vanta/PCI/SSE'
    GL_VERSION == '1.1.4 (4.06.00.266)'

    The driver was reported to crash on MSWin under following
    conditions, quoted verbatim from the problem report:

    ------8<---- [snip] -----------8<---- [snip] -----

    I observe a bit of strange behaviour on my NT4 systems. I have an
    appliction which uses the the following bit of code:

    // Define line width
    SoDrawStyle *drawStyle = new SoDrawStyle;
    drawStyle->lineWidth.setValue(3);
    drawStyle->linePattern.setValue(0x0F0F);
    root->addChild(drawStyle);
           
    // Define line connection
    SoCoordinate3 *coords = new SoCoordinate3;
    coords->point.setValues(0, 2, vert);
    root->addChild(coords);
           
    SoLineSet *lineSet = new SoLineSet ;
    lineSet->numVertices.set1Value(0, 2) ;
    root->addChild(lineSet);
           
    It defines a line with a dashed pattern. When the line is in a
    direction and the viewing direction is not parrallel to this line
    all works fine. In case the viewing direction is the same as the
    line direction one of my systems crashes [...]

    ------8<---- [snip] -----------8<---- [snip] -----

    <mortene@sim.no>

    UPDATE 20030116 mortene: as of this date, the most recent Matrox
    driver (version 5.86.032, from 2002-11-21) still exhibits the same
    problem, while the ELSA driver can be upgraded to a version that
    does not have the bug any more.
  */

  if (coin_glglue_sun_expert3d_warning() &&
      (strcmp(renderer, "Sun Expert3D, VIS") == 0) &&
      (strcmp(version, "1.2 Sun OpenGL 1.2.1 patch 109544-19 for Solaris") == 0)) {
    cc_debugerror_postwarning("glglue_check_driver",
                              "This OpenGL driver (\"%s\" \"%s\") has known "
                              "problems with dual screen configurations, "
                              "please upgrade.  "
                              "(This debug message can be turned off "
                              "permanently by setting the environment variable"
                              " COIN_GLGLUE_NO_SUN_EXPERT3D_WARNING=1).",
                              renderer, version);
  /*
    The full driver information for the driver where this was reported
    is as follows:

    GL_VENDOR == 'Sun Microsystems, Inc.'
    GL_RENDERER == 'Sun Expert3D, VIS'
    GL_VERSION == '1.2 Sun OpenGL 1.2.1 patch 109544-19 for Solaris'

    The driver was reported to fail when running on a Sun Solaris
    system with the XVR1000 graphics card. Quoted verbatim from the
    problem report:

    ------8<---- [snip] -----------8<---- [snip] -----

    [The client] works with two screens. One of the screen works as it
    should, while the otherone has erronious apperance (see uploaded
    image). The errors are the stripes on the texture (It should be
    one continious texture). The texture is wrapped on a rectangle
    (i.e. two large triangles). It is not only the OpenGl part of the
    window that is weired.  Some buttons are missing and other buttons
    have wrong colors++.

    ------8<---- [snip] -----------8<---- [snip] -----

    The error disappeared after a driver upgrade.

    <mortene@sim.no>
  */
  }
}

/* We're basically using the Singleton pattern to instantiate and
   return OpenGL-glue "object structs". We're constructing one
   instance for each OpenGL context, though.  */
const cc_glglue *
cc_glglue_instance(int contextid)
{
  SbBool found;
  void * ptr;
  cc_glglue * gi = NULL;

  CC_SYNC_BEGIN(cc_glglue_instance);

  if (!gldict) {  /* First invocation, do initializations. */
    gldict = cc_hash_construct(16, 0.75f);
    coin_atexit((coin_atexit_f *)glglue_cleanup, 0);
  }

  found = cc_hash_get(gldict, (unsigned long) contextid, &ptr);

  if (!found) {
    /* Internal consistency checking. */
#ifdef HAVE_GLX
    /* Disabled this assert because GLX in Mesa version 3.4.2
       (GL_VENDOR "VA Linux Systems, Inc", GL_RENDERER "Mesa GLX
       Indirect", GL_VERSION "1.2 Mesa 3.4.2") returns NULL even
       though there really is a current context set up. (Reported by
       kintel.) */
/*     assert((glXGetCurrentContext() != NULL) && "must have a current GL context when instantiating cc_glglue"); */
#endif /* HAVE_GLX */
#ifdef HAVE_WGL
    assert((wglGetCurrentContext() != NULL) && "must have a current GL context when instantiating cc_glglue");
#endif /* HAVE_WGL */
#ifdef HAVE_AGL
    /* NB! It would seem logical to assert on aglGetCurrentContext()
       != NULL here, but that does not work, since
       aglGetCurrentContext() only returns a value != NULL if the
       context has been set using aglSetCurrentContext(). Therefore,
       GLUT or Cocoa application using Coin would not work anymore!

       FIXME: Find out how to check for valid OpenGL context on a
       lower level (i.e. CGL) kyrah 20030122 */
#endif /* HAVE_AGL */

    glglue_sanity_check_enums();

    gi = (cc_glglue*)malloc(sizeof(cc_glglue));
    /* FIXME: handle out-of-memory on malloc(). 20000928 mortene. */

    ptr = gi;
    cc_hash_put(gldict, (unsigned long) contextid, ptr);

    if (!glglue_self_handle && !glglue_tried_open_self) {
      glglue_self_handle = cc_dl_open(NULL);
      glglue_tried_open_self = TRUE;
    }

    /* NB: if you are getting a crash here, it's because an attempt at
     * setting up a cc_glglue instance was made when there is no
     * current OpenGL context. */
    gi->versionstr = (const char *)glGetString(GL_VERSION);
    assert(gi->versionstr && "could not call glGetString() -- no current GL context?");
    assert(glGetError() == GL_NO_ERROR && "GL error when calling glGetString() -- no current GL context?");

    glglue_set_glVersion(gi);
    glxglue_init(gi);

    gi->vendorstr = (const char *)glGetString(GL_VENDOR);
    gi->vendor_is_SGI = strcmp((const char *)gi->vendorstr, "SGI") == 0;
    gi->rendererstr = (const char *)glGetString(GL_RENDERER);
    gi->extensionsstr = (const char *)glGetString(GL_EXTENSIONS);

    if (coin_glglue_debug()) {
      cc_debugerror_postinfo("cc_glglue_instance",
                             "glGetString(GL_VENDOR)=='%s' (=> vendor_is_SGI==%s)",
                             gi->vendorstr,
                             gi->vendor_is_SGI ? "TRUE" : "FALSE");
      cc_debugerror_postinfo("cc_glglue_instance",
                             "glGetString(GL_RENDERER)=='%s'",
                             gi->rendererstr);
      cc_debugerror_postinfo("cc_glglue_instance",
                             "glGetString(GL_EXTENSIONS)=='%s'",
                             gi->extensionsstr);

      cc_debugerror_postinfo("cc_glglue_instance",
                             "Rendering is %sdirect.",
                             gi->glx.isdirect ? "" : "in");
    }

    glglue_check_driver(gi->vendorstr, gi->rendererstr, gi->versionstr);

    /* Resolve our function pointers. */
    glglue_resolve_symbols(gi);
  }
  else {
    gi = (cc_glglue *)ptr;
  }

  CC_SYNC_END(cc_glglue_instance);
  return gi;
}

const cc_glglue *
cc_glglue_instance_from_context_ptr(void * ctx)
{
  /* The id can really be anything unique for the current context, but
     we should avoid a crash with the possible ids defined by
     SoGLCacheContextElement. It's a bit of a hack, this. */
  const int id = (int)((long)ctx);
  return cc_glglue_instance(id);
}

SbBool
cc_glglue_isdirect(const cc_glglue * w)
{
  return w->glx.isdirect;
}

/* Returns a flag which indicates whether or not to allow the use of
   OpenGL 1.1+ features and extensions.

   We default to *not* allowing this if rendering is indirect, as
   we've seen major problems with at least NVidia GLX when using
   OpenGL 1.1+ features. It can be forced on by an environment
   variable, though.

   (A better strategy *might* be to default to allow it, but to smoke
   out and warn if we detect NVidia GLX, and in addition to provide an
   environment variable that disables it.)
*/
static SbBool
glglue_allow_newer_opengl(const cc_glglue * w)
{
  static SbBool fullindirect = -1;
  static SbBool force1_0 = -1;

  if (fullindirect == -1) {
    fullindirect = (glglue_resolve_envvar("COIN_FULL_INDIRECT_RENDERING") > 0);
  }

  if (force1_0 == -1) {
    force1_0 = (glglue_resolve_envvar("COIN_FORCE_GL1_0_ONLY") > 0);
  }

  if (force1_0) return FALSE;
  if (!w->glx.isdirect && !fullindirect) return FALSE;
  return TRUE;
}

/*!
  Whether glPolygonOffset() is availble or not: either we're on OpenGL
  1.1 or the GL_EXT_polygon_offset extension is available.

  Method then available for use:
  \li cc_glglue_glPolygonOffset
*/
SbBool
cc_glglue_has_polygon_offset(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return (w->glPolygonOffset || w->glPolygonOffsetEXT) ? TRUE : FALSE;
}

/* Returns the glPolygonOffset() we're actually going to use. */
static COIN_PFNGLPOLYGONOFFSETPROC
glglue_glPolygonOffset(const cc_glglue * w)
{
  COIN_PFNGLPOLYGONOFFSETPROC poff = NULL;

  assert(w->glPolygonOffset ||  w->glPolygonOffsetEXT);

  poff = w->glPolygonOffset;

  /* Some SGI OpenGL 1.1 driver(s) seems to have a buggy
     implementation of glPolygonOffset(), according to pederb after
     some debugging he did for Fedem. These drivers'
     glPolygonOffsetEXT() actually seems to work better, so we prefer
     that if available. */
  if (w->vendor_is_SGI && w->glPolygonOffsetEXT &&
      cc_glglue_glversion_matches_at_least(w, 1, 1, 0) &&
      !cc_glglue_glversion_matches_at_least(w, 1, 2, 0)) {
    poff = w->glPolygonOffsetEXT;
  }

  /* Since we know glPolygonOffset() can be problematic, we also
     provide a way to prefer the EXT function instead through an
     environment variable "COIN_PREFER_GLPOLYGONOFFSET_EXT" (which
     could be handy for help debugging remote systems, at least). */
  if (w->glPolygonOffsetEXT && glglue_prefer_glPolygonOffsetEXT()) {
    poff = w->glPolygonOffsetEXT;
  }

  /* If glPolygonOffset() is not available (and the function pointer
     was not set by any of the bug workaround if-checks above), fall
     back on extension. */
  if (poff == NULL) { poff = w->glPolygonOffsetEXT; }

  return poff;
}

/*!
  Enable or disable z-buffer offsetting for the given primitive types.
*/
void
cc_glglue_glPolygonOffsetEnable(const cc_glglue * w,
                                SbBool enable, int m)
{
  COIN_PFNGLPOLYGONOFFSETPROC poff = glglue_glPolygonOffset(w);

  if (enable) {
    if (poff == w->glPolygonOffset) {
      if (m & cc_glglue_FILLED) glEnable(GL_POLYGON_OFFSET_FILL);
      else glDisable(GL_POLYGON_OFFSET_FILL);
      if (m & cc_glglue_LINES) glEnable(GL_POLYGON_OFFSET_LINE);
      else glDisable(GL_POLYGON_OFFSET_LINE);
      if (m & cc_glglue_POINTS) glEnable(GL_POLYGON_OFFSET_POINT);
      else glDisable(GL_POLYGON_OFFSET_POINT);
    }
    else { /* using glPolygonOffsetEXT() */
      /* The old pre-1.1 extension only supports filled polygon
         offsetting. */
      if (m & cc_glglue_FILLED) glEnable(GL_POLYGON_OFFSET_EXT);
      else glDisable(GL_POLYGON_OFFSET_EXT);

      if (coin_glglue_debug() && (m != cc_glglue_FILLED)) {
        static SbBool first = TRUE;
        if (first) {
          cc_debugerror_postwarning("cc_glglue_glPolygonOffsetEnable",
                                    "using EXT_polygon_offset, which only "
                                    "supports filled-polygon offsetting");
          first = FALSE;
        }
      }
    }
  }
  else { /* disable */
    if (poff == w->glPolygonOffset) {
      if (m & cc_glglue_FILLED) glDisable(GL_POLYGON_OFFSET_FILL);
      if (m & cc_glglue_LINES) glDisable(GL_POLYGON_OFFSET_LINE);
      if (m & cc_glglue_POINTS) glDisable(GL_POLYGON_OFFSET_POINT);
    }
    else { /* using glPolygonOffsetEXT() */
      if (m & cc_glglue_FILLED) glDisable(GL_POLYGON_OFFSET_EXT);
      /* Pre-1.1 glPolygonOffset extension only supported filled primitives.*/
    }
  }
}

void
cc_glglue_glPolygonOffset(const cc_glglue * w,
                          GLfloat factor,
                          GLfloat units)
{
  COIN_PFNGLPOLYGONOFFSETPROC poff = glglue_glPolygonOffset(w);

  if (poff == w->glPolygonOffsetEXT) {
    /* Try to detect if user actually attempted to specify a valid
       bias value, like the old glPolygonOffsetEXT() extension
       needs. If not, assume that the "units" argument was set up for
       the "real" glPolygonOffset() function, and use a default value
       that should work fairly ok under most circumstances. */
    SbBool isbias = (units > 0.0f) && (units < 0.01f);
    if (!isbias) units = 0.000001f;

    /* FIXME: shouldn't there be an attempt to convert the other way
       around too? Ie, if it *is* a "bias" value and we're using the
       "real" 1.1 glPolygonOffset() function, try to convert it into a
       valid "units" value? 20020919 mortene. */
  }

  poff(factor, units);
}

/*!
  Whether 3D texture objects are available or not: either we're on OpenGL
  1.1, or the GL_EXT_texture_object extension is available.

  Methods then available for use:

  \li cc_glglue_glGenTextures
  \li cc_glglue_glBindTexture
  \li cc_glglue_glDeleteTextures
*/
SbBool
cc_glglue_has_texture_objects(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return w->glGenTextures && w->glBindTexture && w->glDeleteTextures;
}

void
cc_glglue_glGenTextures(const cc_glglue * w, GLsizei n, GLuint * textures)
{
  assert(w->glGenTextures);
  w->glGenTextures(n, textures);
}

void
cc_glglue_glBindTexture(const cc_glglue * w, GLenum target, GLuint texture)
{
  assert(w->glBindTexture);
  w->glBindTexture(target, texture);
}

void
cc_glglue_glDeleteTextures(const cc_glglue * w, GLsizei n, const GLuint * textures)
{
  assert(w->glDeleteTextures);
  w->glDeleteTextures(n, textures);
}

/*!
  Whether sub-textures are supported: either we're on OpenGL 1.2, or
  the GL_EXT_texture3D extension is available.

  Methods then available for use:

  \li cc_glglue_glTexImage3D
  \li cc_glglue_glTexSubImage3D
  \li cc_glglue_glCopyTexSubImage3D
*/
SbBool
cc_glglue_has_texsubimage(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return w->glTexSubImage2D ? TRUE : FALSE;
}

void
cc_glglue_glTexSubImage2D(const cc_glglue * w,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLint yoffset,
                          GLsizei width,
                          GLsizei height,
                          GLenum format,
                          GLenum type,
                          const GLvoid * pixels)
{
  assert(w->glTexSubImage2D);
  w->glTexSubImage2D(target, level, xoffset, yoffset,
                     width, height, format, type, pixels);
}

/*!
  Whether 3D textures are available or not: either we're on OpenGL
  1.2, or the GL_EXT_texture3D extension is available.

  Methods then available for use:

  \li cc_glglue_glTexImage3D
  \li cc_glglue_glTexSubImage3D
  \li cc_glglue_glCopyTexSubImage3D
*/
SbBool
cc_glglue_has_3d_textures(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return
    w->glTexImage3D &&
    w->glCopyTexSubImage3D &&
    w->glTexSubImage3D;
}

SbBool
cc_glglue_has_2d_proxy_textures(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  /* FIXME: there are differences between the 1.1 proxy mechanisms and
     the GL_EXT_texture proxy extension; the 1.1 support considers
     mipmaps. I think. Check documentation in the GL spec. If that is
     correct, we can't really use them interchangeable versus each
     other like we now do in Coin code. 20030121 mortene. */
  return
    cc_glglue_glversion_matches_at_least(w, 1, 1, 0) ||
    cc_glglue_glext_supported(w, "GL_EXT_texture");
}

SbBool
cc_glglue_has_texture_edge_clamp(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return
    cc_glglue_glversion_matches_at_least(w, 1, 2, 0) ||
    cc_glglue_glext_supported(w, "GL_EXT_texture_edge_clamp") ||
    cc_glglue_glext_supported(w, "GL_SGIS_texture_edge_clamp");
}

SbBool
cc_glglue_has_multitexture(const cc_glglue * w)
{
  if (!glglue_allow_newer_opengl(w)) return FALSE;

  return w->glMultiTexCoord2f && w->glActiveTexture;
}

void
cc_glglue_glTexImage3D(const cc_glglue * w,
                       GLenum target,
                       GLint level,
                       GLenum internalformat,
                       GLsizei width,
                       GLsizei height,
                       GLsizei depth,
                       GLint border,
                       GLenum format,
                       GLenum type,
                       const GLvoid *pixels)
{
  assert(w->glTexImage3D);
  w->glTexImage3D(target, level, internalformat,
                  width, height, depth, border,
                  format, type, pixels);
}

void
cc_glglue_glTexSubImage3D(const cc_glglue * w,
                          GLenum target,
                          GLint level,
                          GLint xoffset,
                          GLint yoffset,
                          GLint zoffset,
                          GLsizei width,
                          GLsizei height,
                          GLsizei depth,
                          GLenum format,
                          GLenum type,
                          const GLvoid * pixels)
{
  assert(w->glTexSubImage3D);
  w->glTexSubImage3D(target, level, xoffset, yoffset,
                     zoffset, width, height, depth, format,
                     type, pixels);
}

void
cc_glglue_glCopyTexSubImage3D(const cc_glglue * w,
                              GLenum target,
                              GLint level,
                              GLint xoffset,
                              GLint yoffset,
                              GLint zoffset,
                              GLint x,
                              GLint y,
                              GLsizei width,
                              GLsizei height)
{
  assert(w->glCopyTexSubImage3D);
  w->glCopyTexSubImage3D(target,
                         level,
                         xoffset,
                         yoffset,
                         zoffset,
                         x,
                         y,
                         width,
                         height);
}

void
cc_glglue_glActiveTexture(const cc_glglue * w,
                          GLenum texture)
{
  assert(w->glActiveTexture);
  w->glActiveTexture(texture);
}

void
cc_glglue_glMultiTexCoord2f(const cc_glglue * w,
                            GLenum target,
                            GLfloat s,
                            GLfloat t)
{
  assert(w->glMultiTexCoord2f);
  w->glMultiTexCoord2f(target, s, t);
}


SbBool
cc_glue_has_texture_compression(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;

  return 
    glue->glCompressedTexImage1D && 
    glue->glCompressedTexImage2D && 
    glue->glCompressedTexImage3D && 
    glue->glGetCompressedTexImage;
}

SbBool
cc_glue_has_texture_compression_2d(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;
  return glue->glCompressedTexImage2D && glue->glGetCompressedTexImage;
}

SbBool
cc_glue_has_texture_compression_3d(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;
  return glue->glCompressedTexImage3D && glue->glGetCompressedTexImage;
}

void
cc_glglue_glCompressedTexImage3D(const cc_glglue * glue,
                                 GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLsizei width,
                                 GLsizei height,
                                 GLsizei depth,
                                 GLint border,
                                 GLsizei imageSize,
                                 const GLvoid * data)
{
  assert(glue->glCompressedTexImage3D);
  glue->glCompressedTexImage3D(target,
                               level,
                               internalformat,
                               width,
                               height,
                               depth,
                               border,
                               imageSize,
                               data);
}

void
cc_glglue_glCompressedTexImage2D(const cc_glglue * glue,
                                 GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLsizei width,
                                 GLsizei height,
                                 GLint border,
                                 GLsizei imageSize,
                                 const GLvoid *data)
{
  assert(glue->glCompressedTexImage2D);
  glue->glCompressedTexImage2D(target,
                               level,
                               internalformat,
                               width,
                               height,
                               border,
                               imageSize,
                               data);
}

void
cc_glglue_glCompressedTexImage1D(const cc_glglue * glue,
                                 GLenum target,
                                 GLint level,
                                 GLenum internalformat,
                                 GLsizei width,
                                 GLint border,
                                 GLsizei imageSize,
                                 const GLvoid *data)
{
  assert(glue->glCompressedTexImage1D);
  glue->glCompressedTexImage1D(target,
                               level,
                               internalformat,
                               width,
                               border,
                               imageSize,
                               data);
}

void
cc_glglue_glCompressedTexSubImage3D(const cc_glglue * glue,
                                    GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLint yoffset,
                                    GLint zoffset,
                                    GLsizei width,
                                    GLsizei height,
                                    GLsizei depth,
                                    GLenum format,
                                    GLsizei imageSize,
                                    const GLvoid *data)
{
  assert(glue->glCompressedTexSubImage3D);
  glue->glCompressedTexSubImage3D(target,
                                  level,
                                  xoffset,
                                  yoffset,
                                  zoffset,
                                  width,
                                  height,
                                  depth,
                                  format,
                                  imageSize,
                                  data);
}

void
cc_glglue_glCompressedTexSubImage2D(const cc_glglue * glue,
                                    GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLint yoffset,
                                    GLsizei width,
                                    GLsizei height,
                                    GLenum format,
                                    GLsizei imageSize,
                                    const GLvoid *data)
{
  assert(glue->glCompressedTexSubImage2D);
  glue->glCompressedTexSubImage2D(target,
                                  level,
                                  xoffset,
                                  yoffset,
                                  width,
                                  height,
                                  format,
                                  imageSize,
                                  data);
}

void
cc_glglue_glCompressedTexSubImage1D(const cc_glglue * glue,
                                    GLenum target,
                                    GLint level,
                                    GLint xoffset,
                                    GLsizei width,
                                    GLenum format,
                                    GLsizei imageSize,
                                    const GLvoid *data)
{
  assert(glue->glCompressedTexSubImage1D);
  glue->glCompressedTexSubImage1D(target,
                                  level,
                                  xoffset,
                                  width,
                                  format,
                                  imageSize,
                                  data);
}

void
cc_glglue_glGetCompressedTexImage(const cc_glglue * glue,
                                  GLenum target,
                                  GLint level,
                                  void * img)
{
  assert(glue->glGetCompressedTexImage);
  glue->glGetCompressedTexImage(target,
                                level,
                                img);
}

SbBool
cc_glglue_has_paletted_textures(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;

  return glue->supportsPalettedTextures;
}

SbBool
cc_glglue_has_color_tables(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;
  return glue->glColorTable != NULL;
}

SbBool
cc_glglue_has_color_subtables(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;
  return glue->glColorSubTable != NULL;
}

void
cc_glglue_glColorTable(const cc_glglue * glue,
                       GLenum target,
                       GLenum internalFormat,
                       GLsizei width,
                       GLenum format,
                       GLenum type,
                       const GLvoid *table)
{
  assert(glue->glColorTable);
  glue->glColorTable(target,
                     internalFormat,
                     width,
                     format,
                     type,
                     table);
}

void
cc_glglue_glColorSubTable(const cc_glglue * glue,
                          GLenum target,
                          GLsizei start,
                          GLsizei count,
                          GLenum format,
                          GLenum type,
                          const GLvoid * data)
{
  assert(glue->glColorSubTable);
  glue->glColorSubTable(target,
                        start,
                        count,
                        format,
                        type,
                        data);
}

void
cc_glglue_glGetColorTable(const cc_glglue * glue,
                          GLenum target,
                          GLenum format,
                          GLenum type,
                          GLvoid *data)
{
  assert(glue->glGetColorTable);
  glue->glGetColorTable(target,
                        format,
                        type,
                        data);
}

void
cc_glglue_glGetColorTableParameteriv(const cc_glglue * glue,
                                     GLenum target,
                                     GLenum pname,
                                     GLint *params)
{
  assert(glue->glGetColorTableParameteriv);
  glue->glGetColorTableParameteriv(target,
                                   pname,
                                   params);
}

void
cc_glglue_glGetColorTableParameterfv(const cc_glglue * glue,
                                     GLenum target,
                                     GLenum pname,
                                     GLfloat *params)
{
  assert(glue->glGetColorTableParameterfv);
  glue->glGetColorTableParameterfv(target,
                                   pname,
                                   params);
}

SbBool
cc_glglue_has_blendequation(const cc_glglue * glue)
{
  if (!glglue_allow_newer_opengl(glue)) return FALSE;

  return glue->glBlendEquation || glue->glBlendEquationEXT;
}

void
cc_glglue_glBlendEquation(const cc_glglue * glue, GLenum mode)
{
  assert(glue->glBlendEquation || glue->glBlendEquationEXT);

  if (glue->glBlendEquation) glue->glBlendEquation(mode);
  else glue->glBlendEquationEXT(mode);
}

/*!
  Returns current X11 display the OpenGL context is in. If none, or if
  the glXGetCurrentDisplay() method is not available (it was
  introduced with GLX 1.3), returns \c NULL.
*/
void *
cc_glglue_glXGetCurrentDisplay(const cc_glglue * w)
{
  return w->glXGetCurrentDisplay ? w->glXGetCurrentDisplay() : NULL;
}

/*** Offscreen buffer handling. *********************************************/

/*
  Below is a stand-alone example that can be compiled and linked with
  the Coin library for testing that the context handling interface
  works:
 */
/*
  #include <Inventor/C/glue/gl.h>
  #include <assert.h>
  #include <stdio.h>
  
  int
  main(void)
  {
    void * ctx = cc_glglue_context_create_offscreen(128, 128);
    assert(ctx);
    SbBool ok = cc_glglue_context_make_current(ctx);
    assert(ok);
  
    const GLubyte * str = glGetString(GL_VERSION);
    assert(str && "could not call glGetString() -- no current GL context?");
    assert(glGetError() == GL_NO_ERROR && "GL error when calling glGetString() -- no current GL context?");
  
    (void)fprintf(stdout, "glGetString(GL_VERSION)=='%s'\n", str);
    (void)fprintf(stdout, "glGetString(GL_VENDOR)=='%s'\n", glGetString(GL_VENDOR));
    (void)fprintf(stdout, "glGetString(GL_RENDERER)=='%s'\n", glGetString(GL_RENDERER));
 
    (void)fprintf(stdout, "glGenTextures=='%p'\n",
                  cc_glglue_getprocaddress("glGenTextures"));

    (void)fprintf(stdout, "glGenTexturesEXT=='%p'\n",
                  cc_glglue_getprocaddress("glGenTexturesEXT"));
 
    cc_glglue_context_reinstate_previous(ctx);
    cc_glglue_context_destruct(ctx);
    return 0;
  }
*/

void *
cc_glglue_context_create_offscreen(unsigned int width, unsigned int height)
{
#ifdef HAVE_GLX
  return glxglue_context_create_offscreen(width, height);
#elif defined(HAVE_AGL)
  return aglglue_context_create_offscreen(width, height);
#elif defined(HAVE_WGL)
  return wglglue_context_create_offscreen(width, height);
#else
  assert(FALSE && "unimplemented");
  return NULL;
#endif
}

SbBool
cc_glglue_context_make_current(void * ctx)
{
#ifdef HAVE_GLX
  return glxglue_context_make_current(ctx);
#elif defined(HAVE_AGL)
  return aglglue_context_make_current(ctx);
#elif defined(HAVE_WGL)
  return wglglue_context_make_current(ctx);
#else
  assert(FALSE && "unimplemented");
  return FALSE;
#endif
}

void
cc_glglue_context_reinstate_previous(void * ctx)
{
#ifdef HAVE_GLX
  glxglue_context_reinstate_previous(ctx);
#elif defined(HAVE_AGL)
  aglglue_context_reinstate_previous(ctx);
#elif defined(HAVE_WGL)
  wglglue_context_reinstate_previous(ctx);
#else
  assert(FALSE && "unimplemented");
#endif
}

void
cc_glglue_context_destruct(void * ctx)
{
#ifdef HAVE_GLX
  glxglue_context_destruct(ctx);
#elif defined(HAVE_AGL)
  aglglue_context_destruct(ctx);
#elif defined(HAVE_WGL)
  wglglue_context_destruct(ctx);
#else 
  assert(FALSE && "unimplemented");
#endif
}

/*** </Offscreen buffer handling.> ******************************************/


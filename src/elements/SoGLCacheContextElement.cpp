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
  \class SoGLCacheElement Inventor/elements/SoGLCacheElement.h
  \brief The SoGLCacheElement class handles OpenGL caches for multiple contexts.
  \ingroup elements
*/

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbName.h>
#include <Inventor/misc/SoState.h>
#include <../tidbits.h> // coin_atexit()
#include <Inventor/misc/SoGL.h>
#include <string.h>
#include <stdlib.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>
#include <Inventor/C/threads/threadsutilp.h>

static int biggest_cache_context_id = 0;

SO_ELEMENT_SOURCE(SoGLCacheContextElement);

typedef struct {
  SbName extname;
  SbList <int> context;
  SbList <SbBool> supported;
} so_glext_info;

typedef struct {
  int context;
  int handle;
} so_gltexhandle_info;

static SbList <so_glext_info *> *extsupportlist;
static SbList <SoGLDisplayList*> *scheduledeletelist;
static void * glcache_mutex;

static void soglcachecontext_cleanup(void)
{
  int i,n = extsupportlist->getLength();
  for (i = 0; i < n; i++) {
    delete (*extsupportlist)[i];
  }
  delete extsupportlist;
  delete scheduledeletelist;
  CC_MUTEX_DESTRUCT(glcache_mutex);
}

//
// check if OpenGL extension is supported.
//
static SbBool
extension_supported(const char * extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return FALSE;

  if (!extensions)
    extensions = glGetString(GL_EXTENSIONS);
  start = extensions;
  for (;;) {
    where = (GLubyte *) strstr((const char *)start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return TRUE;
      }
    }
    start = terminator;
  }
  return FALSE;
}


// doc from parent
void
SoGLCacheContextElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLCacheContextElement, inherited);

  extsupportlist = new SbList <so_glext_info *>;
  scheduledeletelist = new SbList <SoGLDisplayList*>;
  CC_MUTEX_CONSTRUCT(glcache_mutex);
  coin_atexit((coin_atexit_f *)soglcachecontext_cleanup);
}

/*!
  Destructor.
*/
SoGLCacheContextElement::~SoGLCacheContextElement()
{
}

// doc from parent
void
SoGLCacheContextElement::init(SoState * state)
{
  // these values will be set int set(), but initialize them anyway
  this->context = 0;
  this->twopass = FALSE;
  this->remote = FALSE;
  this->autocachebits = 0;
}

// doc from parent
SbBool
SoGLCacheContextElement::matches(const SoElement * elt) const
{
  const SoGLCacheContextElement * elem = (SoGLCacheContextElement*) elt;

  return
    this->context == elem->context &&
    this->twopass == elem->twopass &&
    this->remote == elem->remote;
}

// doc from parent
SoElement *
SoGLCacheContextElement::copyMatchInfo(void) const
{
  SoGLCacheContextElement * elem = (SoGLCacheContextElement*)
    this->getTypeId().createInstance();
  elem->context = this->context;
  elem->twopass = this->twopass;
  elem->remote = this->remote;
  return elem;
}

/*!
  Sets data for context.
*/
void
SoGLCacheContextElement::set(SoState * state, int context,
                             SbBool twopasstransparency,
                             SbBool remoterendering)
{
  SoGLCacheContextElement * elem = (SoGLCacheContextElement *)
    state->getElementNoPush(classStackIndex);
  elem->twopass = twopasstransparency;
  elem->remote = remoterendering;
  elem->autocachebits = 0;
  elem->context = context;
  if (context > biggest_cache_context_id) {
    biggest_cache_context_id = context;
  }

  if (remoterendering) elem->autocachebits = DO_AUTO_CACHE;

  CC_MUTEX_LOCK(glcache_mutex);

  int i = 0;
  int n = scheduledeletelist->getLength();

  while (i < n) {
    SoGLDisplayList * dl = (*scheduledeletelist)[i];
    if (dl->getContext() == context) {
      scheduledeletelist->removeFast(i);
      n--;
      delete dl;
    }
    else i++;
  }
  CC_MUTEX_UNLOCK(glcache_mutex);
}

/*!
  Returns context id.
*/
int
SoGLCacheContextElement::get(SoState * state)
{
  const SoGLCacheContextElement * elem = (const SoGLCacheContextElement *)
    SoElement::getConstElement(state, classStackIndex);
  return elem->context;
}

/*!
  Returns an extension id based on the GL extension string.
  The extension id can be used to quickly test for the availability
  of an extension later, using extSupported().
*/
int
SoGLCacheContextElement::getExtID(const char * str)
{
  CC_MUTEX_LOCK(glcache_mutex);

  SbName extname(str);
  int i, n = extsupportlist->getLength();
  for (i = 0; i < n; i++) {
    if ((*extsupportlist)[i]->extname == extname) break;
  }
  if (i == n) { // not found
    so_glext_info * info = new so_glext_info;
    info->extname = extname;
    extsupportlist->append(info);
  }

  CC_MUTEX_UNLOCK(glcache_mutex);

  return i;
}

/*!
  Returns TRUE if the extension is supported for the current context.
  \a extid must be an id returned from getExtId(). The test result
  is cached so this method is pretty fast and can be used run-time.
*/
SbBool
SoGLCacheContextElement::extSupported(SoState * state, int extid)
{
  CC_MUTEX_LOCK(glcache_mutex);

  assert(extid >= 0 && extid < extsupportlist->getLength());

  so_glext_info * info = (*extsupportlist)[extid];

  int currcontext = SoGLCacheContextElement::get(state);
  int n = info->context.getLength();
  for (int i = 0; i < n; i++) {
    if (info->context[i] == currcontext) return info->supported[i];
  }
  SbBool supported = extension_supported(info->extname.getString());
  info->context.append(currcontext);
  info->supported.append(supported);

  CC_MUTEX_UNLOCK(glcache_mutex);

  return supported;
}

/*!
  Returns the OpenGL version for the current context. This method is
  an extension versus the Open Inventor API.  
*/
void
SoGLCacheContextElement::getOpenGLVersion(SoState * state,
                                          int & major, int & minor)
{
  int currcontext = SoGLCacheContextElement::get(state);
  const cc_glglue * w = cc_glglue_instance(currcontext);
  unsigned int majoru, minoru, dummy;
  cc_glglue_glversion(w, &majoru, &minoru, &dummy);
  major = (int)majoru;
  minor = (int)minoru;
}

/*!
  Returns if mipmapped textures are fast for the current context.
  In Coin, we just return TRUE for the moment.
*/
SbBool
SoGLCacheContextElement::areMipMapsFast(SoState * state)
{
  return TRUE; // FIXME: how do we test this? pederb 20001003
}

/*!
  Not properly supported yet.
*/
void
SoGLCacheContextElement::shouldAutoCache(SoState * state, int bits)
{
}

/*!
  Not properly supported yet.
*/
void
SoGLCacheContextElement::setAutoCacheBits(SoState * state, int bits)
{
  SoGLCacheContextElement * elem = (SoGLCacheContextElement*)
    state->getElementNoPush(classStackIndex);

  elem->autocachebits = bits;
}

/*!
  Not properly supported yet.
*/
int
SoGLCacheContextElement::resetAutoCacheBits(SoState * state)
{
  SoGLCacheContextElement *elem = (SoGLCacheContextElement *)
    state->getElementNoPush(classStackIndex);
  int ret = elem->autocachebits;
  elem->autocachebits = elem->remote ? DO_AUTO_CACHE : 0;
  return ret;
}

/*!
  Returns TRUE if rendering is remote.
*/
SbBool
SoGLCacheContextElement::getIsRemoteRendering(SoState * state)
{
  const SoGLCacheContextElement *elem = (const SoGLCacheContextElement *)
    state->getConstElement(classStackIndex);
  return elem->remote;
}

//
// internal method used by SoGLDisplayList to delete list as soon as
// the display list context is current again.
//
void
SoGLCacheContextElement::scheduleDelete(SoState * state, class SoGLDisplayList * dl)
{
  if (state && dl->getContext() == SoGLCacheContextElement::get(state)) {
    delete dl;
  }
  else {
    CC_MUTEX_LOCK(glcache_mutex);
    scheduledeletelist->append(dl);
    CC_MUTEX_UNLOCK(glcache_mutex);
  }
}

/*!
  Returns an unique cache context id. If you render the same scene graph
  into two or different cache contexts, and you've not using display
  list and texture object sharing among contexts, the cache context
  id need to be unique for rendering to work.

  This method is an extension versus the Open Inventor API.

  \sa SoGLRenderAction::setCacheContext()
*/
uint32_t
SoGLCacheContextElement::getUniqueCacheContext(void)
{
  CC_MUTEX_LOCK(glcache_mutex);
  uint32_t id = ++biggest_cache_context_id;
  CC_MUTEX_UNLOCK(glcache_mutex);
  return id;
}




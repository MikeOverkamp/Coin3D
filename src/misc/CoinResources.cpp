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

#include "misc/CoinResources.h"

/*!
  \class CoinResources "misc/CoinResources.h"
  \brief Static utility functions for managing data resources at the basic buffer level.

  This class is just a static scope for some resource managing
  functions.  With this, Coin can register built-in default resources
  that can later be retrieved through the same resource locator,
  either from disk (if present) and as a fallback the built-in
  (compiled in) buffer

  The resource locators take the form "coin:path/to/resource.ext".
  The "coin:" prefix is for Coin to differentiate a resource locator from
  a filename (for multipurpose function usage).  The path/to/resource.ext
  is the path under the environment variable $COINDIR where the file
  should be present. This file can be an updated version, compared to the
  compiled-in buffer, which is why the externalized files are prioritized
  over the builtin buffers.

  A resource does not need to have a corresponding external file.  This is
  configured in the flags parameter when the resource is set.  You can in
  other words also register built-in-only resources.

  \ingroup internal
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <map>

#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/C/tidbits.h>

// internal data
class ResourceHandle {
public:
  ResourceHandle(void)
    : resloc(NULL), canbefile(FALSE), filenotfound(FALSE),
      loadedbuf(NULL), loadedbufsize(0),
      internalbuf(NULL), internalbufsize(0)
  { }

  char * resloc;
  SbBool canbefile;
  SbBool filenotfound;

  char * loadedbuf;
  size_t loadedbufsize;
  char * internalbuf;
  size_t internalbufsize;
};

// internal static class
class CoinResourcesP {
public:
  typedef std::map<const char *, ResourceHandle *> ResourceMap;

  static ResourceMap resourcemap;

  static ResourceHandle * getResourceHandle(const char * resloc);
  static ResourceHandle * createResourceHandle(const char * resloc);
};

CoinResourcesP::ResourceMap CoinResourcesP::resourcemap;

/*!
  Returns a resource if one exists. If the Coin installation permits,
  the resource will be loaded from file, but if the file can not be
  located or loaded, builtin versions will be returned instead.

  \return TRUE on success, and FALSE if there is no such resource.
*/
SbBool
CoinResources::get(const char * resloc, const char *& buffer, size_t & bufsize)
{
  if (strncmp(resloc, "coin:", 5) != 0) return FALSE;

  ResourceHandle * handle = CoinResourcesP::getResourceHandle(resloc);
  if (!handle) return FALSE;

  if (handle->loadedbuf == NULL && handle->canbefile && !handle->filenotfound) {
    // try loading file from COINDIR/...
    do {
      static const char * coindirenv = coin_getenv("COINDIR");
      if (coindirenv == NULL) {
        coindirenv = coin_getenv("OIV_HOME");
      }
      if (coindirenv != NULL) {
        SbString filename;
        filename.sprintf("%s/share/Coin/%s", coindirenv, resloc + 5);

        FILE * fp = fopen(filename.getString(), "rb");
        if (!fp) {
          handle->filenotfound = TRUE;
          break;
        }

        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        if (size < 0) {
          fclose(fp);
          handle->filenotfound = TRUE;
          break;
        }

        fseek(fp, 0, SEEK_SET);

        char * buffer = new char [ size + 1 ];
        buffer[size] = '\0';

        size_t num = fread(buffer, size, 1, fp);
        fclose(fp);
        fp = NULL;

        if (num == 1) {
          // FIXME: at this point we can check if this is the first
          // load, and if so hook up freeLoadedExternals() to atexit()
          // to clean up those buffers automatically.  Or we can maybe
          // hook up something that clears out everything instead.
          handle->loadedbuf = buffer;
          handle->loadedbufsize = size;
        } else {
          handle->filenotfound = TRUE;
          delete [] buffer;
          break;
        }
      }
    } while ( FALSE );
  }

  if (handle->loadedbuf != NULL) {
    buffer = handle->loadedbuf;
    bufsize = handle->loadedbufsize;
    return TRUE;
  }

  buffer = handle->internalbuf;
  bufsize = handle->internalbufsize;
  return TRUE;
}

/*!
  This function registers a new resource.  The resource locator (\a resloc)
  should take the form "coin:" followed by a relative file path that should
  lead to the file representation of the resource from where the COINDIR
  environment variable points.  The relative path should use / for directory
  separation, and not \ if on MS Windows.

  If you put COIN_RESOURCE_NOT_A_FILE in the \a flags argument, then the
  automatic file searching will not be performed.

  \returns TRUE if the resource was set, and FALSE if something went wrong.
  FALSE would most likely be returned because the resource already exists.
*/

SbBool
CoinResources::set(const char * resloc, const char * buffer, size_t bufsize, unsigned int flags)
{
  ResourceHandle * handle = CoinResourcesP::getResourceHandle(resloc);
  if (handle) { // already set
    return FALSE;
  }
  handle = CoinResourcesP::createResourceHandle(resloc);
  assert(handle);
  handle->internalbuf = const_cast<char *>(buffer);
  handle->internalbufsize = bufsize;
  if (flags & COIN_RESOURCE_NOT_A_FILE) {
    handle->canbefile = FALSE;
  } else {
    handle->canbefile = TRUE;
  }
  return TRUE;
}

/*!
  This function deallocates all the buffers that have been loaded
  from disk.  Internal buffers will not be freed, as they are expected
  to be compiled into the Coin library data section.
*/
void
CoinResources::freeLoadedExternals(void)
{
  CoinResourcesP::ResourceMap::iterator it =
    CoinResourcesP::resourcemap.begin();
  while (it != CoinResourcesP::resourcemap.end()) {
    ResourceHandle * handle = it->second;
    if (handle->loadedbuf != NULL) {
      delete [] handle->loadedbuf;
      handle->loadedbuf = NULL;
      handle->loadedbufsize = 0;
    }
    ++it;
  }
}

// internal
ResourceHandle *
CoinResourcesP::getResourceHandle(const char * resloc)
{
  assert(resloc);
  SbName reslochash(resloc);
  CoinResourcesP::ResourceMap::iterator it =
    CoinResourcesP::resourcemap.find(reslochash.getString());
  if (it == CoinResourcesP::resourcemap.end()) return NULL;
  return it->second;
}

// internal
ResourceHandle *
CoinResourcesP::createResourceHandle(const char * resloc)
{
  assert(resloc);
  SbName reslochash(resloc);
  ResourceHandle * handle = new ResourceHandle;
  handle->resloc = const_cast<char *>(reslochash.getString());
  std::pair<const char *, ResourceHandle *> mapentry(reslochash.getString(), handle);
  CoinResourcesP::resourcemap.insert(mapentry);
  return handle;
}

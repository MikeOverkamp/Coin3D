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
  \class SbImage SbImage.h Inventor/SbImage.h
  \brief The SbImage class is an abstract datatype for 2D and 3D images.
  \ingroup base

  This class is a Coin extension to the original Open Inventor API.
*/

// FIXME: this class could be used to handle image reusage, since it's
// quite common that the same image is used several times in a scene
// and for different contexts. The API should stay the same though.
// 20001026 mortene (original comment by pederb).

#include <Inventor/SbImage.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3s.h>
#include <Inventor/SbString.h>
#include <string.h>
#include <stdlib.h>
#include <../misc/simage_wrapper.h>
#include <Inventor/SoInput.h> // for SoInput::searchForFile()
#include <Inventor/lists/SbStringList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_THREADS
#include <Inventor/threads/SbRWMutex.h>
#endif // HAVE_THREADS

#ifndef DOXYGEN_SKIP_THIS

class SbImageP {
public:
  enum DataType {
    INTERNAL_DATA,
    SIMAGE_DATA,
    SETVALUEPTR_DATA
  };

  SbImageP(void)
    : bytes(NULL),
      datatype(SETVALUEPTR_DATA),
      size(0,0,0),
      bpp(0),
      schedulecb(NULL)
#ifdef HAVE_THREADS
    , rwmutex(SbRWMutex::READ_PRECEDENCE)
#endif // HAVE_THREADS
  { }
  void freeData(void) {
    if (this->bytes) {
      switch (this->datatype) {
      default:
        assert(0 && "unknown data type");
        break;
      case INTERNAL_DATA:
        delete[] this->bytes;
        this->bytes = NULL;
        break;
      case SIMAGE_DATA:
        simage_wrapper()->simage_free_image(this->bytes);
        this->bytes = NULL;
        break;
      case SETVALUEPTR_DATA:
        this->bytes = NULL;
        break;
      }
    }
    this->datatype = SETVALUEPTR_DATA;
  }

  unsigned char * bytes;
  DataType datatype;
  SbVec3s size;
  int bpp;
  SbString schedulename;
  SbImageScheduleReadCB * schedulecb;
  void * scheduleclosure;

#ifdef HAVE_THREADS
  SbRWMutex rwmutex;
  void readLock(void) {
    //    fprintf(stderr,"readlock: %p\n", this);
    this->rwmutex.readLock();
    //fprintf(stderr,"readlock achieved: %p\n", this);
  }
  void readUnlock(void) {
    //fprintf(stderr,"readUnlock: %p\n", this);
    this->rwmutex.readUnlock();
  }
  void writeLock(void) {
    //fprintf(stderr,"writelock: %p\n", this);
    this->rwmutex.writeLock();
    //fprintf(stderr,"writelock achived: %p\n", this);
  }
  void writeUnlock(void) {
    //fprintf(stderr,"writeUnlock: %p\n", this);
    this->rwmutex.writeUnlock();
  }
#else // HAVE_THREADS
  void readLock(void) { }
  void readUnlock(void) { }
  void writeLock(void) { }
  void writeUnlock(void) { }
#endif // ! HAVE_THREADS
};

#endif // DOXYGEN_SKIP_THIS

//////////////////////////////////////////////////////////////////////////

#undef THIS
#define THIS (this->pimpl)
#define PRIVATE(image) ((image)->pimpl)

/*!
  Default constructor.
*/
SbImage::SbImage(void)
{
  THIS = new SbImageP;
}

/*!
  Constructor which sets 2D data using setValue().
  \sa setValue()
*/
SbImage::SbImage(const unsigned char * bytes,
                 const SbVec2s & size, const int bytesperpixel)
{
  THIS = new SbImageP;
  this->setValue(size, bytesperpixel, bytes);
}

/*!
  Constructor which sets 3D data using setValue().
  \sa setValue()

  \since 2001-11-19
*/
SbImage::SbImage(const unsigned char * bytes,
                 const SbVec3s & size, const int bytesperpixel)
{
  THIS = new SbImageP;
  this->setValue(size, bytesperpixel, bytes);
}

/*!
  Destructor.
*/
SbImage::~SbImage(void)
{
  THIS->freeData();
  delete THIS;
}

/*!
  Apply a read lock on this image. This will make it impossible for
  other threads to change the image while this lock is active. Other
  threads can do read-only operations on this image, of course.

  For the single thread version of Coin, this method does nothing.

  \sa readUnlock()

  \since 2001-11-06
*/
void
SbImage::readLock(void) const
{
  THIS->readLock();
}

/*!
  Release a read lock on this image.

  For the single thread version of Coin, this method does nothing.

  \ sa readLock()

  \since 2001-11-06
*/
void
SbImage::readUnlock(void) const
{
  THIS->readUnlock();
}

/*!
  Convenience 2D version of setValuePtr.

  \sa setValue()

  \since 2001-11-06
*/
void
SbImage::setValuePtr(const SbVec2s & size, const int bytesperpixel,
                     const unsigned char * bytes)
{
  SbVec3s tmpsize(size[0], size[1], 0);
  this->setValuePtr(tmpsize, bytesperpixel, bytes);
}

/*!
  Sets the image data without copying the data. \a bytes will be used
  directly, and the data will not be freed when the image instance is
  destructed.

  If the depth of the image (size[2]) is zero, the image is considered
  a 2D image.

  \sa setValue()

  \since 2001-11-19
*/
void
SbImage::setValuePtr(const SbVec3s & size, const int bytesperpixel,
                     const unsigned char * bytes)
{
  THIS->writeLock();
  THIS->schedulename = "";
  THIS->schedulecb = NULL;
  THIS->freeData();
  THIS->bytes = (unsigned char *) bytes;
  THIS->datatype = SbImageP::SETVALUEPTR_DATA;
  THIS->size = size;
  THIS->bpp = bytesperpixel;
  THIS->writeUnlock();
}

/*!
  Convenience 2D version of setValue.
*/
void
SbImage::setValue(const SbVec2s & size, const int bytesperpixel,
                  const unsigned char * bytes)
{
  SbVec3s tmpsize(size[0], size[1], 0);
  this->setValue(tmpsize, bytesperpixel, bytes);
}

/*!
  Sets the image to \a size and \a bytesperpixel. If \a bytes !=
  NULL, data is copied from \a bytes into this class' image data. If
  \a bytes == NULL, the image data is left uninitialized.

  The image data will always be allocated in multiples of four. This
  means that if you set an image with size == (1,1,1) and bytesperpixel
  == 1, four bytes will be allocated to hold the data. This is mainly
  done to simplify the export code in SoSFImage and normally you'll
  not have to worry about this feature.

  If the depth of the image (size[2]) is zero, the image is considered
  a 2D image.

  \since 2001-11-19
*/
void
SbImage::setValue(const SbVec3s & size, const int bytesperpixel,
                  const unsigned char * bytes)
{
  THIS->writeLock();
  THIS->schedulename = "";
  THIS->schedulecb = NULL;
  if (THIS->bytes && THIS->datatype == SbImageP::INTERNAL_DATA) {
    // check for special case where we don't have to reallocate
    if (bytes && (size == THIS->size) && (bytesperpixel == THIS->bpp)) {
      memcpy(THIS->bytes, bytes, 
             int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) *
             bytesperpixel);
      THIS->writeUnlock();
      return;
    }
  }
  THIS->freeData();
  THIS->size = size;
  THIS->bpp = bytesperpixel;
  int buffersize = int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) * 
    bytesperpixel;
  if (buffersize) {
    // Align buffers because the binary file format has the data aligned
    // (simplifies export code in SoSFImage).
    buffersize = ((buffersize + 3) / 4) * 4;
    THIS->bytes = new unsigned char[buffersize];
    THIS->datatype = SbImageP::INTERNAL_DATA;

    if (bytes) {
      // Important: don't copy buffersize num bytes here!
      (void)memcpy(THIS->bytes, bytes,
                   int(size[0]) * int(size[1]) * int(size[2]==0?1:size[2]) * 
                   bytesperpixel);
    }
  }
  THIS->writeUnlock();
}

/*!
  Returns the 2D image data.
*/
unsigned char *
SbImage::getValue(SbVec2s & size, int & bytesperpixel) const
{
  SbVec3s tmpsize;
  unsigned char *bytes = this->getValue(tmpsize, bytesperpixel);
  size.setValue(tmpsize[0], tmpsize[1]);
  return bytes;
}

/*!
  Returns the 3D image data.

  \since 2001-11-19
*/
unsigned char *
SbImage::getValue(SbVec3s & size, int & bytesperpixel) const
{
  THIS->readLock();
  if (THIS->schedulecb) {
    SbImage * thisp = (SbImage*) this;
    // start a thread to read the image.
    SbBool scheduled = THIS->schedulecb(THIS->schedulename, thisp, 
                                        THIS->scheduleclosure);
    if (scheduled) {
      THIS->schedulecb = NULL;
    }
  }
  size = THIS->size;
  bytesperpixel = THIS->bpp;
  unsigned char * bytes = THIS->bytes;
  THIS->readUnlock();
  return bytes;

}

/*!
  Given a \a basename for a file and and array of directories to
  search (in \a dirlist, of length \a numdirs), returns the full name
  of the file found.

  In addition to looking at the root of each directory in \a dirlist,
  we also look into the subdirectories \e texture/, \e textures/, \e
  images/, \e pics/ and \e pictures/ of each \a dirlist directory.

  If no file matching \a basename could be found, returns an empty
  string.
*/
SbString
SbImage::searchForFile(const SbString & basename,
                       const SbString * const * dirlist, const int numdirs)
{
  int i;
  SbStringList directories;
  SbStringList subdirectories;

  for (i = 0; i < numdirs; i++) {
    directories.append((SbString*) dirlist[i]);
  }
  subdirectories.append(new SbString("texture"));
  subdirectories.append(new SbString("textures"));
  subdirectories.append(new SbString("images"));
  subdirectories.append(new SbString("pics"));
  subdirectories.append(new SbString("pictures"));

  SbString ret = SoInput::searchForFile(basename, directories, subdirectories);
  for (i = 0; i < subdirectories.getLength(); i++) {
    delete subdirectories[i];
  }
  return ret;
}

/*!
  Reads image data from \a filename. In Coin, simage is used to
  load image files, and several common file formats are supported.
  simage can be downloaded from our webpages.  If loading
  fails for some reason this method returns FALSE, and the instance
  is set to an empty image. If the file is successfully loaded, the
  file image data is copied into this class.

  If \a numdirectories > 0, this method will search for \a filename
  in all directories in \a searchdirectories.
*/
SbBool
SbImage::readFile(const SbString & filename,
                  const SbString * const * searchdirectories,
                  const int numdirectories)
{
  // FIXME: Add 3D image support when that is added to simage (kintel 20011118)
  SbString finalname = SbImage::searchForFile(filename, searchdirectories,
                                              numdirectories);
  if (finalname.getLength()) {
    int w, h, nc;
    unsigned char * simagedata = NULL;

    if (simage_wrapper()->available && simage_wrapper()->simage_read_image) {
      simagedata = simage_wrapper()->simage_read_image(finalname.getString(), 
                                                       &w, &h, &nc);
#if COIN_DEBUG
      if (!simagedata) {
        SoDebugError::post("SbImage::readFile", "(%s) %s",
                           filename.getString(),
                           simage_wrapper()->simage_get_last_error ?
                           simage_wrapper()->simage_get_last_error() :
                           "Unknown error");
      }
#endif // COIN_DEBUG
    }
    
    if (simagedata) {
      //FIXME: Add 3'rd dimension (kintel 20011110)
      this->setValuePtr(SbVec3s((short)w, (short)h, (short)0), nc, simagedata);
      // NB, this is a trick. We use setValuePtr() to set the size
      // and data pointer, and then we change the data type to simage
      // peder, 2002-03-22
      THIS->datatype = SbImageP::SIMAGE_DATA;
      return TRUE;
    }
  }

  this->setValue(SbVec3s(0,0,0), 0, NULL);
  return FALSE;
}

/*!
  \fn int SbImage::operator!=(const SbImage & image) const
  Compare image of \a image with the image in this class and
  return \c FALSE if they are equal.
*/


/*!
  Compare image of \a image with the image in this class and
  return \c TRUE if they are equal.
*/
int
SbImage::operator==(const SbImage & image) const
{
  this->readLock();
  int ret = 0;
  if (!THIS->schedulecb && !PRIVATE(&image)->schedulecb) {
    if (THIS->size != PRIVATE(&image)->size) return FALSE;
    if (THIS->bpp != PRIVATE(&image)->bpp) return FALSE;
    if (THIS->bytes == NULL || PRIVATE(&image)->bytes == NULL) {
      return (THIS->bytes == PRIVATE(&image)->bytes);
    }
    SbBool ret = memcmp(THIS->bytes, PRIVATE(&image)->bytes,
                        int(THIS->size[0]) *
                        int(THIS->size[1]) *
                        int(THIS->size[2]==0?1:THIS->size[2]) * 
                        THIS->bpp) == 0;
  }
  this->readUnlock();
  return ret;
}

/*!
  Schedule a file for reading. \a cb will be called the first time
  getValue() is called for this image, and the callback should then
  start a thread to read the image. Do not read the image in the
  callback, as this will lock up the application.

  \sa readFile()
  \since 2001-11-07
*/
SbBool
SbImage::scheduleReadFile(SbImageScheduleReadCB * cb,
                          void * closure,
                          const SbString & filename,
                          const SbString * const * searchdirectories,
                          const int numdirectories)
{
  this->setValue(SbVec3s(0,0,0), 0, NULL);
  THIS->writeLock();
  THIS->schedulecb = NULL;
  THIS->schedulename =
    this->searchForFile(filename, searchdirectories, numdirectories);
  int len = THIS->schedulename.getLength();
  if (len > 0) {
    THIS->schedulecb = cb;
    THIS->scheduleclosure = closure;
  }
  THIS->writeUnlock();
  return len > 0;
}

/*!
  Returns \a TRUE if the image is not empty. This can be useful, since
  getValue() will start loading the image if scheduleReadFile() has
  been used to set the image data.

  \since 2001-11-08
*/
SbBool 
SbImage::hasData(void) const
{
  SbBool ret;
  this->readLock();
  ret = THIS->bytes != NULL;
  this->readUnlock();
  return ret;
}

/*!
  Returns the size of the image. If this is a 2D image, the
  z component is zero. If this is a 3D image, the z component is
  >= 1.

  \since 2001-11-19
 */
SbVec3s
SbImage::getSize(void) const
{
  return THIS->size;
}

#undef THIS
#undef PRIVATE

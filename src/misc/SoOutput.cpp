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
  \class SoOutput SoOutput.h Inventor/SoOutput.h
  \brief The SoOutput class is an abstraction of an output stream.
  \ingroup general

  SoOutput offers the ability to write basic types to a file or a
  memory buffer in either ASCII or binary format.

  \sa SoInput
*/

#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbDict.h>
#include <Inventor/SbName.h>
#include <assert.h>
#include <string.h>
#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "../tidbits.h"

#if HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H

/*! \enum SoOutput::Stage
  Enumerates the possible stages of a write operation (writing needs to be
  done in mutiple passes).

  \sa setStage(), getStage()
*/
/*! \enum SoOutput::Stage SoOutput::COUNT_REFS
  Not writing, just counting the internal references in the scene graph.
*/
/*! \enum SoOutput::Stage SoOutput::WRITE
  Signifies that actual data export should take place during this pass.
*/

/*! \enum SoOutput::Annotations
  Values from this enum is used for debugging purposes to annotate the
  output from a write operation.
*/
/*! \enum SoOutput::Annotations SoOutput::ADDRESSES
  Annotate output with pointer address information.
*/
/*! \enum SoOutput::Annotations SoOutput::REF_COUNTS
  Annotate output with reference counts of the objects written.
*/

/*! \var SbBool SoOutput::wroteHeader
  Indicates whether or not the file format header has been written out.
  As long as this is \a FALSE, the header will be written once upon the
  first invocation of any write method in the class.
*/


// FIXME: need to fix EOL on other platforms? 19990621 mortene.
static const char EOLSTR[] = "\n";

// FIXME: I guess this should be modified on non-32 bit platforms? Or?
// Wouldn't that puck up cross-platform compatibility of binary files?
// 19990627 mortene.
static const int HOSTWORDSIZE = 4;


/*!
  The default constructor makes an SoOutput instance which will write
  to the standard output.

  \sa setFilePointer(), openFile(), setBuffer()
*/
SoOutput::SoOutput(void)
{
  this->constructorCommon();
  this->sobase2id = NULL;
  this->defnames = NULL;
}

/*!
  Constructs an SoOutput which has a copy of the reference SbDict instance
  from \a dictOut.
*/
SoOutput::SoOutput(SoOutput * dictOut)
{
  assert(dictOut != NULL);
  this->constructorCommon();
  this->sobase2id = new SbDict(*(dictOut->sobase2id));
  this->defnames = new SbDict(*(dictOut->defnames));
}

/*!
  \internal
  Common constructor actions.
 */
void
SoOutput::constructorCommon(void)
{
  this->usersetfp = FALSE;
  this->binarystream = FALSE;
  this->disabledwriting = FALSE;
  this->wroteHeader = FALSE;
  this->memorybuffer = FALSE;
  this->writecompact = FALSE;
  this->filep = stdout;
  this->buffer = NULL;
  this->headerstring = NULL;
  this->precision = 3;
  this->indentlevel = 0;
  this->nextreferenceid = 0;
  this->annotationbits = 0x00;
}

/*!
  Destructor.
*/
SoOutput::~SoOutput(void)
{
  this->reset();
  delete this->headerstring;
}

/*!
  Set up a new file pointer which we will write to.

  Important note: do \e not use this method when the Coin library has
  been compiled as an MSWindows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for MSWindows DLLs.

  \sa openFile(), setBuffer(), getFilePointer()
 */
void
SoOutput::setFilePointer(FILE * newFP)
{
  this->reset();
  this->filep = newFP;
}

/*!
  Returns the current filepointer. If we're writing to a memory
  buffer, \c NULL is returned.

  Important note: do \e not use this method when the Coin library has
  been compiled as an MSWindows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for MSWindows DLLs.

  \sa setFilePointer()
 */
FILE *
SoOutput::getFilePointer(void) const
{
  if (this->isToBuffer()) return NULL;
  else return this->filep;
}

/*!
  Opens a file for writing. If the file can not be opened or is not
  writeable, \a FALSE will be returned.

  Files opened by this method will automatically be closed if the
  user supplies another filepointer, another filename for writing,
  or if the SoOutput instance is deleted.

  \sa setFilePointer(), setBuffer(), closeFile()
 */
SbBool
SoOutput::openFile(const char * const fileName)
{
  FILE * newfile = fopen(fileName, "wb");
  if (newfile) {
    this->setFilePointer(newfile);
    this->usersetfp = TRUE;
  }
  else {
    SoDebugError::postWarning("SoOutput::openFile",
                              "Couldn't open file '%s' for writing.",
                              fileName);
  }

  return this->usersetfp;
}

/*!
  Closes the currently opened file, but only if the file was passed to
  SoOutput through the openFile() method.

  \sa openFile()
 */
void
SoOutput::closeFile(void)
{
  if (this->usersetfp) fclose(this->filep);
  this->filep = NULL;
  this->usersetfp = FALSE;
}

/*!
  Sets up a memory buffer of size \a initSize for writing.
  Writing will start at \a bufPointer + \a offset.

  If the buffer is filled up, \a reallocFunc is called to get more
  memory. If \a reallocFunc returns \a NULL, further writing is
  disabled.

  Important note: remember that the resultant memory buffer after
  write operations have completed may reside somewhere else in memory
  than on \a bufPointer if \a reallocFunc is set. It is a good idea to
  make it a habit to always use getBuffer() to retrieve the memory
  buffer pointer after write operations.

  \sa getBuffer(), getBufferSize(), resetBuffer()
*/
void
SoOutput::setBuffer(void * bufPointer, size_t initSize,
                    SoOutputReallocCB * reallocFunc, int32_t offset)
{
  this->reset();

  this->memorybuffer = TRUE;
  this->buffer = bufPointer;
  this->buffersize = initSize;
  this->reallocfunc = reallocFunc;
  this->bufferoffset = offset;
}

/*!
  Returns the current buffer in \a bufPointer and the current
  write position of the buffer in \a nBytes. If we're writing into a
  file and not a memory buffer, \a FALSE is returned and the other return
  values will be undefined.

  \sa getBufferSize()
 */
SbBool
SoOutput::getBuffer(void *& bufPointer, size_t & nBytes) const
{
  if (this->isToBuffer()) {
    bufPointer = this->buffer;
    nBytes = this->bufferoffset;
    return TRUE;
  }

  return FALSE;
}

/*!
  Returns total size of memory buffer.

  \sa getBuffer()
 */
size_t
SoOutput::getBufferSize(void) const
{
  return this->buffersize;
}

/*!
  Reset the memory buffer write pointer back to the beginning of the
  buffer.
 */
void
SoOutput::resetBuffer(void)
{
  assert(this->isToBuffer());
  this->bufferoffset = 0;
}

/*!
  Set whether or not to write the output as a binary stream.

  \sa isBinary()
 */
// FIXME: write doc on endianness, netformat etc -- best thing would
// be to document the format completely in BNF. 19990627 mortene.
void
SoOutput::setBinary(const SbBool flag)
{
  this->binarystream = flag;
}

/*!
  Returns a flag which indicates whether or not we're writing the output
  as a binary stream.

  \sa setBinary()
 */
SbBool
SoOutput::isBinary(void) const
{
  return this->binarystream;
}

/*!
  Set the output file header string.

  \sa resetHeaderString(), getDefaultASCIIHeader(), getDefaultBinaryHeader()
 */
void
SoOutput::setHeaderString(const SbString & str)
{
  if (this->headerstring) *(this->headerstring) = str;
  else this->headerstring = new SbString(str);
}

/*!
  Reset the header string to the default one.

  \sa setHeaderString(), getDefaultASCIIHeader(), getDefaultBinaryHeader()
 */
void
SoOutput::resetHeaderString(void)
{
  delete this->headerstring;
  this->headerstring = NULL;
}

/*!
  Return the default header string written to ASCII files.

  \sa setHeaderString(), resetHeaderString(), getDefaultBinaryHeader()
 */
SbString
SoOutput::getDefaultASCIIHeader(void)
{
  return SbString("#Inventor V2.1 ascii");
}

/*!
  Return the default header string written to binary files.

  \sa setHeaderString(), resetHeaderString(), getDefaultASCIIHeader()
 */
SbString
SoOutput::getDefaultBinaryHeader(void)
{
  return SbString("#Inventor V2.1 binary");
}

/*!
  Set the precision used when writing floating point numbers to
  ASCII files.
*/
void
SoOutput::setFloatPrecision(const int precision)
{
  this->precision = precision;
}

/*!
  Sets an indicator on the current stage. This is necessary to do as writing
  has to be done in multiple stages to account for the export of
  references/connections within the scene graphs.

  This method is basically just used from within SoWriteAction.

  \sa getStage()
*/
void
SoOutput::setStage(Stage stage)
{
  this->stage = stage;
}

/*!
  Returns an indicator on the current write stage. Writing is done in two
  passes, one to count and check connections, one to do the actual ascii or
  binary export of data.

  You should not need to use this method, as it is meant for internal
  purposes in Coin.

  \sa setStage()
*/
SoOutput::Stage
SoOutput::getStage(void) const
{
  return this->stage;
}


/*!
  Write the character in \a c.

  For binary write, the character plus 3 padding zero characters will be
  written.
 */
void
SoOutput::write(const char c)
{
  this->writeBytesWithPadding(&c, 1);
}

/*!
  Write the character string pointed to by \a s.

  For binary write, a 4-byte MSB-ordered integer with the string length,
  plus the string plus padding zero characters to get on a 4-byte boundary
  (if necessary) will be written.
 */
void
SoOutput::write(const char * s)
{
  int writelen = strlen(s);
  if (this->isBinary()) this->write(writelen);
  this->writeBytesWithPadding(s, writelen);
}

/*!
  Write the character string in \a s. The string will be written with
  apostrophes. Cast SbString to char * to write without apostrophes.

  If we are supposed to write in binary format, no apostrophes will be
  added, and writing will be done in the exact same manner as with
  SoOutput::write(const char * s).
 */
void
SoOutput::write(const SbString & s)
{
  // FIXME: share code with SoOutput::write(const SbName &). 19991113 mortene.

  if (this->isBinary()) {
    this->write(s.getString());
  }
  else {
    SbString ws("\"");
    ws += s;
    ws += "\"";
    this->write(ws.getString());
  }
}

/*!
  Write the character string in \a n. The name will be enclosed by
  apostrophes. If you want to write an SbName instance without the
  apostrophes, cast the argument to a char *.

  If we are supposed to write in binary format, no apostrophes will be
  added, and writing will be done in the exact same manner as with
  SoOutput::write(const char * s).
 */
void
SoOutput::write(const SbName & n)
{
  // FIXME: share code with SoOutput::write(const SbString &). 19991113 mortene.

  if (this->isBinary()) {
    this->write(n.getString());
  }
  else {
    SbString wn("\"");
    wn += n;
    wn += "\"";
    this->write(wn.getString());
  }
}

/*!
  Write \a i as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const int i)
{
  if (!this->isBinary()) {
    SbString s;
    s.sprintf("%d", i);
    this->writeBytesWithPadding(s.getString(), s.getLength());
  }
  else {
    // FIXME: breaks on 64-bit architectures, which is pretty
    // lame... 19990621 mortene.
    assert(sizeof(int) == sizeof(int32_t));
    int32_t val = i;
    this->writeBinaryArray(&val, 1);
  }
}

/*!
  Write \a i as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const unsigned int i)
{
  if (!this->isBinary()) {
    SbString s;
    s.sprintf("0x%x", i);
    this->writeBytesWithPadding(s.getString(), s.getLength());
  }
  else {
    assert(sizeof(i) == sizeof(int32_t));
    char buff[sizeof(i)];
    this->convertInt32((int32_t)i, buff);
    this->writeBytesWithPadding(buff, sizeof(i));
  }
}

/*!
  Write \a s as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const short s)
{
  if (!this->isBinary()) {
    SbString str;
    str.sprintf("%hd", s);
    this->writeBytesWithPadding(str.getString(), str.getLength());
  }
  else {
    this->write((int)s);
  }
}

/*!
  Write \a s as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated. If we're writing in ASCII
  format, the value will be written in base 16 (hexadecimal).
 */
void
SoOutput::write(const unsigned short s)
{
  if (!this->isBinary()) {
    SbString str;
    str.sprintf("0x%hx", s);
    this->writeBytesWithPadding(str.getString(), str.getLength());
  }
  else {
    this->write((unsigned int)s);
  }
}

/*!
  Write \a f as a character string.
 */
void
SoOutput::write(const float f)
{
  if (!this->isBinary()) {
    // FIXME: precision stuff doesn't work. 19980910 mortene.
    SbString s;
    s.sprintf("%g", f);
    this->writeBytesWithPadding(s.getString(), s.getLength());
  }
  else {
    char buff[sizeof(f)];
    this->convertFloat(f, buff);
    this->writeBytesWithPadding(buff, sizeof(f));
  }
}

/*!
  Write \a d as a character string.
 */
void
SoOutput::write(const double d)
{
  if (!this->isBinary()) {
    // FIXME: precision stuff not implemented. 19980910 mortene.
    SbString s;
    s.sprintf("%f", d);
    this->writeBytesWithPadding(s.getString(), s.getLength());
  }
  else {
    char buff[sizeof(d)];
    this->convertDouble(d, buff);
    this->writeBytesWithPadding(buff, sizeof(d));
  }
}

/*!
  Write the given number of bytes to either a file or a memory buffer in
  binary format.
 */
void
SoOutput::writeBinaryArray(const unsigned char * constc, const int length)
{
  if (this->disabledwriting) return;

  this->checkHeader();

  if (this->isToBuffer()) {
    // Needs a \0 at the end if we're writing in ASCII.
    int writelen = this->isBinary() ? length : length + 1;

    if (this->makeRoomInBuf(writelen)) {
      char * writeptr = &(((char *)(this->buffer))[this->bufferoffset]);
      (void)memcpy(writeptr, constc, length);
      writeptr += length;
      this->bufferoffset += length;
      if (!this->isBinary()) *writeptr = '\0'; // Terminate.
    }
    else {
      SoDebugError::postWarning("SoOutput::writeBinaryArray",
                                "Couldn't write any more bytes to the memory "
                                "buffer.");
      this->disabledwriting = TRUE;
    }
  }
  else {
    size_t wrote = fwrite(constc, 1, length, this->filep);
    if (wrote != (size_t)length) {
      SoDebugError::postWarning("SoOutput::writeBinaryArray",
                                "Couldn't write to file.");
      this->disabledwriting = TRUE;
    }
  }
}

/*!
  Write an \a length array of int32_t values in binary format.
 */
void
SoOutput::writeBinaryArray(const int32_t * const l, const int length)
{
  // Slooooow. We can do much better by using convertInt32Array().

  char val[sizeof(int32_t)];
  for (int i=0; i < length; i++) {
    this->convertInt32(l[i], val);
    this->writeBytesWithPadding(val, sizeof(int32_t));
  }
}

/*!
  Write an array of float values in binary format.
 */
void
SoOutput::writeBinaryArray(const float * const f, const int length)
{
  // Slooooow. We can do much better by using convertFloatArray().

  char val[sizeof(float)];
  for (int i=0; i < length; i++) {
    this->convertFloat(f[i], val);
    this->writeBytesWithPadding(val, sizeof(float));
  }
}

/*!
  Write an array of double values in binary format.
 */
void
SoOutput::writeBinaryArray(const double * const d, const int length)
{
  // Slooooow. We can do much better by using convertDoubleArray().

  char val[sizeof(double)];
  for (int i=0; i < length; i++) {
    this->convertDouble(d[i], val);
    this->writeBytesWithPadding(val, sizeof(double));
  }
}

/*!
  Increase indentation level in the file.

  \sa decrementIndent(), indent()
 */
void
SoOutput::incrementIndent(const int levels)
{
  this->indentlevel += levels;
}

/*!
  Decrease indentation level in the file.

  \sa incrementIndent(), indent()
 */
void
SoOutput::decrementIndent(const int levels)
{
  this->indentlevel -= levels;
#if COIN_DEBUG
  if (this->indentlevel < 0) {
    SoDebugError::postInfo("SoOutput::decrementIndent",
                           "indentation level < 0!");
    this->indentlevel = 0;
  }
#endif // COIN_DEBUG
}

/*!
  Call this method after writing a newline to a file to indent the next
  line to the correct position.

  \sa incrementIndent(), decrementIndent()
 */
void
SoOutput::indent(void)
{
#if COIN_DEBUG
  if (this->isBinary()) {
    SoDebugError::postWarning("SoOutput::indent",
                              "Don't try to indent when you're doing binary "
                              "format output.");
    return;
  }
#endif // COIN_DEBUG

  int i = indentlevel;
  while (i > 1) {
    this->write('\t');
    i -= 2;
  }

  if (i == 1) this->write("  ");
}

/*!
  Reset all value and make ready for using another filepointer or buffer.
*/
void
SoOutput::reset(void)
{
  this->closeFile();
  delete this->sobase2id; this->sobase2id = NULL;
  delete this->defnames; this->defnames = NULL;

  this->usersetfp = FALSE;
  this->disabledwriting = FALSE;
  this->wroteHeader = FALSE;
  this->memorybuffer = FALSE;
  this->filep = stdout;
  this->buffer = NULL;
  this->indentlevel = 0;
}

/*!
  Set up the output to be more compact than with the default write routines.
*/
void
SoOutput::setCompact(SbBool flag)
{
  // FIXME: go through output code and make the output more
  // compact. 19990623 morten.
#if COIN_DEBUG
  if (!this->writecompact && flag) {
    SoDebugError::postWarning("SoOutput::setCompact",
                              "compact export is not implemented in Coin yet");
  }
#endif // COIN_DEBUG

  this->writecompact = flag;
}

/*!
  Returns whether or not the write routines tries to compact the data when
  writing it (i.e. using less whitespace, etc).

  Note that "compact" in this sense does \e not mean "bitwise compression",
  as it could easily be mistaken for.
*/
SbBool
SoOutput::isCompact(void) const
{
  return this->writecompact;
}

/*!
  Set up annotation of different aspects of the output data. This is not
  useful for much else than debugging purposes, I s'pose.
*/
void
SoOutput::setAnnotation(uint32_t bits)
{
  // FIXME: go through output code and insert annotations where applicable.
  // 19990623 morten.
#if COIN_DEBUG
  if (this->annotationbits != bits) {
    SoDebugError::postWarning("SoOutput::setAnnotation",
                              "annotated export is not implemented in Coin "
                              "yet");
  }
#endif // COIN_DEBUG

  this->annotationbits = bits;
}

/*!
  Returns the current annotation debug bitflag settings.
*/
uint32_t
SoOutput::getAnnotation(void)
{
  return this->annotationbits;
}

/*!
  Check that the current memory buffer has enough space to contain the
  given number of bytes needed for the next write operation.

  Returns \a FALSE if there's not enough space left, otherwise \a TRUE.

  Note that there will automatically be made an attempt at allocating
  more memory if the realloction callback function argument of
  setBuffer() was not \a NULL.
*/
SbBool
SoOutput::makeRoomInBuf(size_t bytes)
{
  if ((this->bufferoffset + bytes) > this->buffersize) {
    if (this->reallocfunc) {
      this->buffersize =  this->bufferoffset + bytes;
      this->buffer = reallocfunc(this->buffer, this->buffersize);
      if (this->buffer) return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

/*!
  \internal

  Write the given number of bytes from the array, pad with zeroes to get
  on a 4-byte boundary if file format is binary.
*/
void
SoOutput::writeBytesWithPadding(const char * const p, const size_t nr)
{
  this->writeBinaryArray((const unsigned char *)p, nr);

  // Pad binary writes to a 4-byte boundary if necessary.
  if (this->isBinary()) {
    // Static buffer filled with enough bytes of all-zero bits.
    static unsigned char padbytes[HOSTWORDSIZE] = "X";
    if (padbytes[0] == 'X')
      for (int i=0; i < HOSTWORDSIZE; i++) padbytes[i] = '\0';

    int writeposition = this->bytesInBuf();
    int padsize = HOSTWORDSIZE - (writeposition % HOSTWORDSIZE);
    if (padsize == HOSTWORDSIZE) padsize = 0;
    this->writeBinaryArray(padbytes, padsize);
  }
}

/*!
  \internal

  If the file header hasn't been written yet, write it out now.
*/
void
SoOutput::checkHeader(void)
{
  if (!this->wroteHeader) {
    // NB: this flag _must_ be set before we do any writing, or we'll
    // end up in an eternal double-recursive loop.
    this->wroteHeader = TRUE;

    SbString h;
    if (this->headerstring) h = *(this->headerstring);
    else if (this->isBinary()) h = SoOutput::getDefaultBinaryHeader();
    else h = SoOutput::getDefaultASCIIHeader();

    if (this->isBinary()) h = this->padHeader(h);
    h += EOLSTR;
    if (!this->isBinary()) h += EOLSTR;
    // Note: SoField::get() and SoFieldContainer::get() depends on the
    // fact that the header identification line ends in "\n\n".

    // Write as char * to avoid the addition of any "s.
    this->writeBinaryArray((const unsigned char *)h.getString(),
                           strlen(h.getString()));
  }
}

/*!
  Returns \a TRUE of we're set up to write to a memory buffer.
*/
SbBool
SoOutput::isToBuffer(void) const
{
  return this->memorybuffer;
}

/*!
  Returns number of bytes written to file or pushed into memory buffer.
*/
size_t
SoOutput::bytesInBuf(void) const
{
  if (this->isToBuffer()) return this->bufferoffset;
  else return ftell(this->filep);
}

/*!
  Makes a unique id for \a base and adds a mapping into our dictionary.
*/
int
SoOutput::addReference(const SoBase * base)
{
  if (!this->sobase2id) this->sobase2id = new SbDict;
  int id = this->nextreferenceid++;
  // Ugly! Should be solved by making a decent templetized
  // SbDict-alike class.
  this->sobase2id->enter((unsigned long)base, (void *)id);
  return id;
}

/*!
  Returns the unique identifier for \a base or -1 if not found.
*/
int
SoOutput::findReference(const SoBase * base) const
{
  // Ugly! Should be solved by making a decent templetized
  // SbDict-alike class.
  void * id;
  SbBool ok = this->sobase2id && this->sobase2id->find((unsigned long)base, id);
  return ok ? (int)id : -1;
}

/*!
  Sets the reference for \a base manually.
*/
void
SoOutput::setReference(const SoBase * base, int refid)
{
  if (!this->sobase2id) this->sobase2id = new SbDict;
  this->sobase2id->enter((unsigned long)base, (void *)refid);
}

/*!
  Adds \a name to the set of currently DEF'ed node names so far in the output 
  process.
*/
void 
SoOutput::addDEFNode(SbName name)
{
  void * value = NULL;
  if (!this->defnames) this->defnames = new SbDict;
  this->defnames->enter((unsigned long)name.getString(), value);
}

/*!
  Checks whether \a name is already DEF'ed at this point in the output process.
  Returns TRUE if \a name is DEF'ed.
*/
SbBool 
SoOutput::lookupDEFNode(SbName name)
{
  void * value;
  if (!this->defnames) this->defnames = new SbDict;
  return this->defnames->find((unsigned long)name.getString(), value);
}

/*!
  Removes \a name from the set of DEF'ed node names. Used after the last
  reference to a DEF'ed node if we want to reuse the DEF at a later point
  in the file.
*/
void 
SoOutput::removeDEFNode(SbName name)
{
  assert(this->defnames);
  if (this->defnames) this->defnames->remove((unsigned long)name.getString());
}


/*!
  Convert the short integer in \a s to most-significant-byte first format
  and put the resulting bytes sequentially at \a to.

  \sa SoInput::convertShort()
*/
void
SoOutput::convertShort(short s, char * to)
{
  // Convert LSB -> MSB order, if necessary.
  // FIXME: ugly hack, can we do better? 19990627 mortene.
  assert(sizeof(s) == sizeof(uint16_t));
  *((uint16_t *)to) = coin_hton_uint16((uint16_t) s);
}

/*!
  Convert the 32-bit integer in \a l to most-significant-byte first format
  and put the resulting bytes sequentially at \a to.

  \sa SoInput::convertInt32()
*/
void
SoOutput::convertInt32(int32_t l, char * to)
{
  // FIXME: ugly hack, probably breaks on 64-bit architectures --
  // lame. 19990627 mortene.
  assert(sizeof(l) == sizeof(uint32_t));
  *((uint32_t *)to) = coin_hton_uint32(l);
}

/*!
  Convert the single-precision floating point number in \a f to
  most-significant-byte first format and put the resulting bytes
  sequentially at \a to.

  \sa SoInput::convertFloat()
*/
void
SoOutput::convertFloat(float f, char * to)
{
  // Jesus H. Christ -- this unbelievably ugly hack actually kinda
  // works. Probably because the bitpatterns of the parts of float
  // numbers are standardized according to IEEE <something>, so we
  // just need to flip the bytes to make them be in MSB
  // format. 19990627 mortene.
  assert(sizeof(f) == sizeof(uint32_t));
  *((uint32_t *)to) = coin_hton_uint32(*((uint32_t *)&f));
}

/*!
  Convert the double-precision floating point number in \a d to
  most-significant-byte first format and put the resulting bytes
  sequentially at \a to.

  \sa SoInput::convertDouble()
*/
void
SoOutput::convertDouble(double d, char * to)
{
  // This code is so ugly it makes Sylvia Brustad a beauty queen, but
  // hey -- it works for me (at least at the current phase of the
  // moon).  See SoOutput::convertFloat() for further comments.
  assert(sizeof(int32_t) * 2 == sizeof(double));
  int32_t * dbitvals = (int32_t *)&d;
  this->convertInt32(dbitvals[1], to);
  this->convertInt32(dbitvals[0], to + sizeof(double)/2);
}

/*!
  Convert \a len short integer values from the array at \a from into
  the array at \a to from native host format to network independent
  format (i.e. most significant byte first).
*/
void
SoOutput::convertShortArray(short * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertShort(*from++, to);
    to += sizeof(short);
  }
}

/*!
  Convert \a len 32-bit integer values from the array at \a from into
  the array at \a to from native host format to network independent
  format (i.e. most significant byte first).
*/
void
SoOutput::convertInt32Array(int32_t * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertInt32(*from++, to);
    to += sizeof(int32_t);
  }
}

/*!
  Convert \a len single-precision floating point values from the array at
  \a from into the array at \a to from native host format to network
  independent format (i.e. most significant byte first).
*/
void
SoOutput::convertFloatArray(float * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertFloat(*from++, to);
    to += sizeof(float);
  }
}

/*!
  Convert \a len double-precision floating point values from the array at
  \a from into the array at \a to from native host format to network
  independent format (i.e. most significant byte first).
*/
void
SoOutput::convertDoubleArray(double * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertDouble(*from++, to);
    to += sizeof(double);
  }
}

/*!
  Pads the header we're writing so it contains the correct amount of bytes
  for the alignment of the following binary writes.
*/
SbString
SoOutput::padHeader(const SbString & inString)
{
  SbString h = inString;
  const int EOLLEN = strlen(EOLSTR);
  int hlen = h.getLength();
  int pad = HOSTWORDSIZE - ((hlen + EOLLEN) % HOSTWORDSIZE);
  pad = pad == HOSTWORDSIZE ? 0 : pad;
  for (int i=0; i < pad; i++) h += ' ';

  return h;
}

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2005 by Systems in Motion.  All rights reserved.
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
 *  See <URL:http://www.coin3d.org/> for more information.
 *
 *  Systems in Motion, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  <URL:http://www.sim.no/>.
 *
\**************************************************************************/

/*!
  \class SoFragmentProgram SoFragmentProgram.h Inventor/nodes/SoFragmentProgram.h
  \brief The SoFragmentProgram class is used for loading fragment shader programs.
  \ingroup nodes
*/

#include <Inventor/nodes/SoFragmentShader.h>

#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/C/glue/glp.h>

// *************************************************************************

SO_NODE_SOURCE(SoFragmentShader);

// *************************************************************************

void
SoFragmentShader::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFragmentShader,
                              SO_FROM_COIN_2_4|SO_FROM_INVENTOR_5_0);
}

SoFragmentShader::SoFragmentShader(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoFragmentShader);
}

SoFragmentShader::~SoFragmentShader()
{  
}

// *************************************************************************

SbBool
SoFragmentShader::isVertexShader(void) const
{
  return FALSE;
}

/*!
  Returns a boolean indicating whether the requested source type is
  supported by the OpenGL driver or not. 

  <i>Beware:</i> To get a correct answer, a valid OpenGL context must
  be available.
*/
SbBool
SoFragmentShader::isSupported(SourceType sourceType)
{
  // The function signature is not very well designed, as we really
  // need a guaranteed GL context for this. (We've chosen to be
  // compatible with TGS Inventor, so don't change the signature.)
  
  void * ptr = coin_gl_current_context();
  assert(ptr && "No active OpenGL context found!");
  if (!ptr) return FALSE; // Always bail out. Even when compiled in 'release' mode.
  
  const cc_glglue * glue = cc_glglue_instance_from_context_ptr(ptr);
  
  if (sourceType == ARB_PROGRAM) {
    return cc_glglue_has_arb_fragment_program(glue);
  }
  else if (sourceType == GLSL_PROGRAM) {    
    // FIXME: Maybe we should check for OpenGL 2.0 aswell? (20050428
    // handegar)
    return cc_glglue_has_arb_shader_objects(glue);
  } 
  // FIXME: Add support for detecting missing Cg support (20050427
  // handegar)
  else if (sourceType == CG_PROGRAM) return TRUE;

  return FALSE;
}

// *************************************************************************

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation. See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin for applications not compatible with the
 *  LGPL, please contact SIM to acquire a Professional Edition license.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGL.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoProfileElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoProfile.h>
#include <Inventor/lists/SbList.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#if HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
#include <GL/gl.h>
#include <GLUWrapper.h>

#if GL_VERSION_1_1
#elif GL_EXT_polygon_offset
static int polygonOffsetEXT;
#endif
#if GL_VERSION_1_1
#elif GL_EXT_texture_object
static int textureObjectEXT;
#endif
#if GL_VERSION_1_1
#elif GL_EXT_vertex_array
static int vertexArrayEXT;
#endif

// generate a 3d circle in the x-z plane
static void
sogl_generate_3d_circle(SbVec3f *coords, const int num, const float radius, const float y)
{
  float delta = 2.0f*float(M_PI)/float(num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = -float(sin(angle)) * radius;
    coords[i][1] = y;
    coords[i][2] = -float(cos(angle)) * radius;
    angle += delta;
  }
}

// generate a 2d circle
static void
sogl_generate_2d_circle(SbVec2f *coords, const int num, const float radius)
{
  float delta = 2.0f*float(M_PI)/float(num);
  float angle = 0.0f;
  for (int i = 0; i < num; i++) {
    coords[i][0] = -float(sin(angle)) * radius;
    coords[i][1] = -float(cos(angle)) * radius;
    angle += delta;
  }
}

void
sogl_render_cone(const float radius,
                 const float height,
                 const int numslices,
                 SoMaterialBundle * const material,
                 const unsigned int flags)
{
  int i;
  // use a limit of 128 to avoid allocating memory each time
  // a cone is drawn
  int slices = numslices;
  if (slices > 128) slices = 128;
  if (slices < 4) slices = 4;

  float h2 = height * 0.5f;

  // put coordinates on the stack
  SbVec3f coords[129];
  SbVec3f normals[130];
  SbVec2f texcoords[129];

  sogl_generate_3d_circle(coords, slices, radius, -h2);
  coords[slices] = coords[0];

  if (flags & SOGL_NEED_NORMALS) {
    double a = atan(height/radius);
    sogl_generate_3d_circle(normals, slices, float(sin(a)), float(cos(a)));
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];
  }

  int matnr = 0;

  if (flags & SOGL_RENDER_SIDE) {
    glBegin(GL_TRIANGLES);
    i = 0;

    float t = 1.0;
    float delta = 1.0f / slices;

    while (i < slices) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t - delta*0.5f, 1.0f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        SbVec3f n = (normals[i] + normals[i+1])*0.5f;
        glNormal3f(n[0], n[1], n[2]);
      }
      glVertex3f(0.0f, h2, 0.0f);
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 0.0f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i]);
      }
      glVertex3fv((const GLfloat*)&coords[i]);

      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t - delta, 0.0f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i+1]);
      }
      glVertex3fv((const GLfloat*)&coords[i+1]);

      i++;
      t -= delta;
    }

    matnr++;
    glEnd();
  }

  if (flags & SOGL_RENDER_BOTTOM) {
    if (flags & SOGL_NEED_TEXCOORDS) {
      sogl_generate_2d_circle(texcoords, slices, 0.5f);
      texcoords[slices] = texcoords[0];
    }

    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    for (i = slices-1; i >= 0; i--) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
      }
      glVertex3fv((const GLfloat*)&coords[i]);
    }
    glEnd();
  }
}

void
sogl_render_cylinder(const float radius,
                     const float height,
                     const int numslices,
                     SoMaterialBundle * const material,
                     const unsigned int flags)
{
  int i;
  int slices = numslices;
  if (slices > 128) slices = 128;
  if (slices < 4) slices = 4;

  float h2 = height * 0.5f;

  SbVec3f coords[129];
  SbVec3f normals[130];
  SbVec2f texcoords[129];

  sogl_generate_3d_circle(coords, slices, radius, -h2);
  coords[slices] = coords[0];

  if (flags & SOGL_NEED_NORMALS) {
    sogl_generate_3d_circle(normals, slices, 1.0f, 0.0f);
    normals[slices] = normals[0];
    normals[slices+1] = normals[1];
  }

  int matnr = 0;

  if (flags & SOGL_RENDER_SIDE) {
    glBegin(GL_QUAD_STRIP);
    i = 0;

    float t = 0.0;
    float inc = 1.0f / slices;

    while (i <= slices) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 1.0f);
      }
      if (flags & SOGL_NEED_NORMALS) {
        glNormal3fv((const GLfloat*)&normals[i]);
      }
      SbVec3f c = coords[i];
      glVertex3f(c[0], h2, c[2]);
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(t, 0.0f);
      }
      glVertex3f(c[0], c[1], c[2]);
      i++;
      t += inc;
    }

    matnr++;
    glEnd();
  }

  if ((flags & SOGL_NEED_TEXCOORDS) &&
      (flags & (SOGL_RENDER_BOTTOM | SOGL_RENDER_TOP))) {
    sogl_generate_2d_circle(texcoords, slices, 0.5f);
    texcoords[slices] = texcoords[0];
  }

  if (flags & SOGL_RENDER_TOP) {
    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);

    for (i = 0; i < slices; i++) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, 1.0f - texcoords[i][1] - 0.5f);
      }
      const SbVec3f &c = coords[i];
      glVertex3f(c[0], h2, c[2]);
    }
    glEnd();
    matnr++;
  }
  if (flags & SOGL_RENDER_BOTTOM) {
    if (flags & SOGL_MATERIAL_PER_PART) {
      material->send(matnr, TRUE);
    }
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);

    for (i = slices-1; i >= 0; i--) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2f(texcoords[i][0]+0.5f, texcoords[i][1]+0.5f);
      }
      glVertex3fv((const GLfloat*)&coords[i]);
    }
    glEnd();
  }
}

void
sogl_render_sphere(const float radius,
                   const int numstacks,
                   const int numslices,
                   SoMaterialBundle * const /* material */,
                   const unsigned int /* flags */)
{
  int stacks = numstacks;
  int slices = numslices;

  if (stacks < 3) stacks = 3;
  if (slices < 4) slices = 4;

  if (slices > 128) slices = 128;

  // used to cache last stack's data
  SbVec3f coords[129];
  SbVec3f normals[129];
  float S[129];

  int i, j;
  float rho;
  float drho;
  float theta;
  float dtheta;
  float tc, ts;
  SbVec3f tmp;

  drho = float(M_PI) / (float) (stacks-1);
  dtheta = 2.0f * float(M_PI) / (float) slices;

  float currs = 0.0f;
  float incs = 1.0f / (float)slices;
  rho = drho;
  theta = 0.0f;
  tc = (float) cos(rho);
  ts = - (float) sin(rho);
  tmp.setValue(0.0f,
               tc,
               ts);
  normals[0] = tmp;
  tmp *= radius;
  coords[0] = tmp;
  S[0] = currs;
  float dT = 1.0f / (float) (stacks-1);
  float T = 1.0f - dT;

  glBegin(GL_TRIANGLES);

  for (j = 1; j <= slices; j++) {
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(currs + 0.5f * incs, 1.0f);
    glVertex3f(0.0f, radius, 0.0f);

    glNormal3fv((const GLfloat*) &normals[j-1]);
    glTexCoord2f(currs, T);
    glVertex3fv((const GLfloat*) &coords[j-1]);

    currs += incs;
    theta += dtheta;
    S[j] = currs;
    tmp.setValue(float(sin(theta))*ts,
                 tc,
                 float(cos(theta))*ts);

    normals[j] = tmp;
    tmp *= radius;
    coords[j] = tmp;

    glNormal3fv((const GLfloat*)&normals[j]);
    glTexCoord2f(currs, T);
    glVertex3fv((const GLfloat*)&coords[j]);
  }
  glEnd(); // GL_TRIANGLES

  rho += drho;

  for (i = 2; i < stacks-1; i++) {
    tc = (float)cos(rho);
    ts = - (float) sin(rho);
    glBegin(GL_QUAD_STRIP);
    theta = 0.0f;
    for (j = 0; j <= slices; j++) {
      glTexCoord2f(S[j], T);
      glNormal3fv((const GLfloat*)&normals[j]);
      glVertex3fv((const GLfloat*)&coords[j]);

      glTexCoord2f(S[j], T - dT);
      tmp.setValue(float(sin(theta))*ts,
                   tc,
                   float(cos(theta))*ts);
      normals[j] = tmp;
      glNormal3f(tmp[0], tmp[1], tmp[2]);
      tmp *= radius;
      glVertex3f(tmp[0], tmp[1], tmp[2]);
      coords[j] = tmp;
      theta += dtheta;
    }
    glEnd(); // GL_QUAD_STRIP
    rho += drho;
    T -= dT;
  }

  glBegin(GL_TRIANGLES);
  for (j = 0; j < slices; j++) {
    glTexCoord2f(S[j], T);
    glNormal3fv((const GLfloat*)&normals[j]);
    glVertex3fv((const GLfloat*)&coords[j]);

    glTexCoord2f(S[j]+incs*0.5f, 0.0f);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, -radius, 0.0f);

    glTexCoord2f(S[j+1], T);
    glNormal3fv((const GLfloat*)&normals[j+1]);
    glVertex3fv((const GLfloat*)&coords[j+1]);
  }
  glEnd(); // GL_TRIANGLES
}

//
// the 12 triangles in the cube
//
static int sogl_cube_vindices[] =
{
  0, 1, 3, 2,
  5, 4, 6, 7,
  1, 5, 7, 3,
  4, 0, 2, 6,
  4, 5, 1, 0,
  2, 3, 7, 6
};

static float sogl_cube_texcoords[] =
{
  1.0f, 1.0f,
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 0.0f
};

static float sogl_cube_normals[] =
{
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, -1.0f,
  -1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, -1.0f, 0.0f
};

static void
sogl_generate_cube_vertices(SbVec3f *varray,
                       const float w,
                       const float h,
                       const float d)
{
  for (int i = 0; i < 8; i++) {
    varray[i].setValue((i&1) ? -w : w,
                       (i&2) ? -h : h,
                       (i&4) ? -d : d);
  }
}


void
sogl_render_cube(const float width,
                 const float height,
                 const float depth,
                 SoMaterialBundle * const material,
                 const unsigned int flags)
{
  SbVec3f varray[8];
  sogl_generate_cube_vertices(varray,
                         width * 0.5f,
                         height * 0.5f,
                         depth * 0.5f);
  glBegin(GL_QUADS);
  int *iptr = sogl_cube_vindices;

  for (int i = 0; i < 6; i++) { // 6 quads
    if (flags & SOGL_NEED_NORMALS)
      glNormal3fv((const GLfloat*)&sogl_cube_normals[i*3]);
    if (flags & SOGL_MATERIAL_PER_PART)
      material->send(i, TRUE);
    for (int j = 0; j < 4; j++) {
      if (flags & SOGL_NEED_TEXCOORDS) {
        glTexCoord2fv(&sogl_cube_texcoords[j<<1]);
      }
      glVertex3fv((const GLfloat*)&varray[*iptr++]);
    }
  }
  glEnd();
}

// **************************************************************************

#if !defined(NO_LINESET_RENDER)

typedef void sogl_render_ils_func ( const SoGLCoordinateElement * /*const*/ coords,
                                    const int32_t *indices,
                                    int num_vertexindices,
                                    const SbVec3f *normals,
                                    const int32_t *normindices,
                                    SoMaterialBundle *const materials,
                                    const int32_t *matindices,
                                    const SoTextureCoordinateBundle * const texcoords,
                                    const int32_t *texindices,
                                    const int drawAsPoints);


#define OVERALL             0
#define PER_SEGMENT         1
#define PER_SEGMENT_INDEXED 2
#define PER_LINE            3
#define PER_LINE_INDEXED    4
#define PER_VERTEX          5
#define PER_VERTEX_INDEXED  6

#define OVERALL             0
#define PER_SEGMENT         1
#define PER_SEGMENT_INDEXED 2
#define PER_LINE            3
#define PER_LINE_INDEXED    4
#define PER_VERTEX          5
#define PER_VERTEX_INDEXED  6

#define MBINDING OVERALL
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m0_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m1_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m2_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m3_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m4_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m5_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ils_m6_n0_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m0_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m1_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m2_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m3_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m4_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m5_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES FALSE
static void sogl_ils_m6_n1_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m0_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m1_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m2_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m3_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m4_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m5_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m6_n2_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m0_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m1_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m2_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m3_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m4_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m5_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_LINE
#define TEXTURES FALSE
static void sogl_ils_m6_n3_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m0_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m1_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m2_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m3_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m4_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m5_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m6_n4_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m0_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m1_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m2_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m3_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m4_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m5_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ils_m6_n5_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m0_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m1_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m2_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m3_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m4_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m5_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ils_m6_n6_t0
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m0_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m1_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m2_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m3_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m4_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m5_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_ils_m6_n0_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m0_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m1_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m2_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m3_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m4_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m5_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_SEGMENT
#define TEXTURES TRUE
static void sogl_ils_m6_n1_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m0_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m1_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m2_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m3_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m4_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m5_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_SEGMENT_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m6_n2_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m0_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m1_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m2_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m3_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m4_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m5_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_LINE
#define TEXTURES TRUE
static void sogl_ils_m6_n3_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m0_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m1_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m2_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m3_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m4_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m5_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_LINE_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m6_n4_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m0_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m1_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m2_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m3_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m4_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m5_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_ils_m6_n5_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m0_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m1_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_SEGMENT_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m2_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m3_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_LINE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m4_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m5_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_ils_m6_n6_t1
#include "SoGLindexedLineSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES


void
sogl_render_lineset(const SoGLCoordinateElement * const coords,
                    const int32_t *cindices,
                    int numindices,
                    const SbVec3f *normals,
                    const int32_t *nindices,
                    SoMaterialBundle *const mb,
                    const int32_t *mindices,
                    const SoTextureCoordinateBundle * const tb,
                    const int32_t *tindices,
                    int nbind,
                    int mbind,
                    const int texture,
                    const int drawAsPoints)
{
  static sogl_render_ils_func *ils_render_funcs[ 127 ];
  static int first = 1;
  if (first) {
    first = 0;
    ils_render_funcs[  0] = sogl_ils_m0_n0_t0;
    ils_render_funcs[  1] = sogl_ils_m0_n0_t1;
    ils_render_funcs[  2] = sogl_ils_m0_n1_t0;
    ils_render_funcs[  3] = sogl_ils_m0_n1_t1;
    ils_render_funcs[  4] = sogl_ils_m0_n2_t0;
    ils_render_funcs[  5] = sogl_ils_m0_n2_t1;
    ils_render_funcs[  6] = sogl_ils_m0_n3_t0;
    ils_render_funcs[  7] = sogl_ils_m0_n3_t1;
    ils_render_funcs[  8] = sogl_ils_m0_n4_t0;
    ils_render_funcs[  9] = sogl_ils_m0_n4_t1;
    ils_render_funcs[ 10] = sogl_ils_m0_n5_t0;
    ils_render_funcs[ 11] = sogl_ils_m0_n5_t1;
    ils_render_funcs[ 12] = sogl_ils_m0_n6_t0;
    ils_render_funcs[ 13] = sogl_ils_m0_n6_t1;

    ils_render_funcs[ 16] = sogl_ils_m1_n0_t0;
    ils_render_funcs[ 17] = sogl_ils_m1_n0_t1;
    ils_render_funcs[ 18] = sogl_ils_m1_n1_t0;
    ils_render_funcs[ 19] = sogl_ils_m1_n1_t1;
    ils_render_funcs[ 20] = sogl_ils_m1_n2_t0;
    ils_render_funcs[ 21] = sogl_ils_m1_n2_t1;
    ils_render_funcs[ 22] = sogl_ils_m1_n3_t0;
    ils_render_funcs[ 23] = sogl_ils_m1_n3_t1;
    ils_render_funcs[ 24] = sogl_ils_m1_n4_t0;
    ils_render_funcs[ 25] = sogl_ils_m1_n4_t1;
    ils_render_funcs[ 26] = sogl_ils_m1_n5_t0;
    ils_render_funcs[ 27] = sogl_ils_m1_n5_t1;
    ils_render_funcs[ 28] = sogl_ils_m1_n6_t0;
    ils_render_funcs[ 29] = sogl_ils_m1_n6_t1;

    ils_render_funcs[ 32] = sogl_ils_m2_n0_t0;
    ils_render_funcs[ 33] = sogl_ils_m2_n0_t1;
    ils_render_funcs[ 34] = sogl_ils_m2_n1_t0;
    ils_render_funcs[ 35] = sogl_ils_m2_n1_t1;
    ils_render_funcs[ 36] = sogl_ils_m2_n2_t0;
    ils_render_funcs[ 37] = sogl_ils_m2_n2_t1;
    ils_render_funcs[ 38] = sogl_ils_m2_n3_t0;
    ils_render_funcs[ 39] = sogl_ils_m2_n3_t1;
    ils_render_funcs[ 40] = sogl_ils_m2_n4_t0;
    ils_render_funcs[ 41] = sogl_ils_m2_n4_t1;
    ils_render_funcs[ 42] = sogl_ils_m2_n5_t0;
    ils_render_funcs[ 43] = sogl_ils_m2_n5_t1;
    ils_render_funcs[ 44] = sogl_ils_m2_n6_t0;
    ils_render_funcs[ 45] = sogl_ils_m2_n6_t1;

    ils_render_funcs[ 48] = sogl_ils_m3_n0_t0;
    ils_render_funcs[ 49] = sogl_ils_m3_n0_t1;
    ils_render_funcs[ 50] = sogl_ils_m3_n1_t0;
    ils_render_funcs[ 51] = sogl_ils_m3_n1_t1;
    ils_render_funcs[ 52] = sogl_ils_m3_n2_t0;
    ils_render_funcs[ 53] = sogl_ils_m3_n2_t1;
    ils_render_funcs[ 54] = sogl_ils_m3_n3_t0;
    ils_render_funcs[ 55] = sogl_ils_m3_n3_t1;
    ils_render_funcs[ 56] = sogl_ils_m3_n4_t0;
    ils_render_funcs[ 57] = sogl_ils_m3_n4_t1;
    ils_render_funcs[ 58] = sogl_ils_m3_n5_t0;
    ils_render_funcs[ 59] = sogl_ils_m3_n5_t1;
    ils_render_funcs[ 60] = sogl_ils_m3_n6_t0;
    ils_render_funcs[ 61] = sogl_ils_m3_n6_t1;

    ils_render_funcs[ 64] = sogl_ils_m4_n0_t0;
    ils_render_funcs[ 65] = sogl_ils_m4_n0_t1;
    ils_render_funcs[ 66] = sogl_ils_m4_n1_t0;
    ils_render_funcs[ 67] = sogl_ils_m4_n1_t1;
    ils_render_funcs[ 68] = sogl_ils_m4_n2_t0;
    ils_render_funcs[ 69] = sogl_ils_m4_n2_t1;
    ils_render_funcs[ 70] = sogl_ils_m4_n3_t0;
    ils_render_funcs[ 71] = sogl_ils_m4_n3_t1;
    ils_render_funcs[ 72] = sogl_ils_m4_n4_t0;
    ils_render_funcs[ 73] = sogl_ils_m4_n4_t1;
    ils_render_funcs[ 74] = sogl_ils_m4_n5_t0;
    ils_render_funcs[ 75] = sogl_ils_m4_n5_t1;
    ils_render_funcs[ 76] = sogl_ils_m4_n6_t0;
    ils_render_funcs[ 77] = sogl_ils_m4_n6_t1;

    ils_render_funcs[ 80] = sogl_ils_m5_n0_t0;
    ils_render_funcs[ 81] = sogl_ils_m5_n0_t1;
    ils_render_funcs[ 82] = sogl_ils_m5_n1_t0;
    ils_render_funcs[ 83] = sogl_ils_m5_n1_t1;
    ils_render_funcs[ 84] = sogl_ils_m5_n2_t0;
    ils_render_funcs[ 85] = sogl_ils_m5_n2_t1;
    ils_render_funcs[ 86] = sogl_ils_m5_n3_t0;
    ils_render_funcs[ 87] = sogl_ils_m5_n3_t1;
    ils_render_funcs[ 88] = sogl_ils_m5_n4_t0;
    ils_render_funcs[ 89] = sogl_ils_m5_n4_t1;
    ils_render_funcs[ 90] = sogl_ils_m5_n5_t0;
    ils_render_funcs[ 91] = sogl_ils_m5_n5_t1;
    ils_render_funcs[ 92] = sogl_ils_m5_n6_t0;
    ils_render_funcs[ 93] = sogl_ils_m5_n6_t1;

    ils_render_funcs[ 96] = sogl_ils_m6_n0_t0;
    ils_render_funcs[ 97] = sogl_ils_m6_n0_t1;
    ils_render_funcs[ 98] = sogl_ils_m6_n1_t0;
    ils_render_funcs[ 99] = sogl_ils_m6_n1_t1;
    ils_render_funcs[100] = sogl_ils_m6_n2_t0;
    ils_render_funcs[101] = sogl_ils_m6_n2_t1;
    ils_render_funcs[102] = sogl_ils_m6_n3_t0;
    ils_render_funcs[103] = sogl_ils_m6_n3_t1;
    ils_render_funcs[104] = sogl_ils_m6_n4_t0;
    ils_render_funcs[105] = sogl_ils_m6_n4_t1;
    ils_render_funcs[106] = sogl_ils_m6_n5_t0;
    ils_render_funcs[107] = sogl_ils_m6_n5_t1;
    ils_render_funcs[108] = sogl_ils_m6_n6_t0;
    ils_render_funcs[109] = sogl_ils_m6_n6_t1;
  }

  ils_render_funcs[ (mbind << 4) | (nbind << 1) | texture ](
    coords,
    cindices,
    numindices,
    normals,
    nindices,
    mb,
    mindices,
    tb,
    tindices,
    drawAsPoints);
}


#undef OVERALL
#undef PER_SEGMENT
#undef PER_SEGMENT_INDEXED
#undef PER_LINE
#undef PER_LINE_INDEXED
#undef PER_VERTEX
#undef PER_VERTEX_INDEXED

#endif // !NO_LINESET_RENDER


static SbList <float> * tmpcoordlist = NULL;
static SbList <float> * tmptexcoordlist = NULL;

static void nurbs_coord_cleanup(void)
{
  delete tmpcoordlist;
}

static void nurbs_texcoord_cleanup(void)
{
  delete tmptexcoordlist;
}

void
sogl_render_nurbs_surface(SoAction * action, SoShape * shape,
                          void * nurbsrenderer,
                          const int numuctrlpts, const int numvctrlpts,
                          const float * uknotvec, const float * vknotvec,
                          const int numuknot, const int numvknot,
                          const int numsctrlpts, const int numtctrlpts,
                          const float * sknotvec, const float * tknotvec,
                          const int numsknot, const int numtknot,
                          const SbBool glrender,
                          const int numcoordindex, const int32_t * coordindex,
                          const int numtexcoordindex, const int32_t * texcoordindex)
{
  if (GLUWrapper()->available == 0) {
    // Should never get here if the NURBS functionality is missing.
    assert(FALSE && "Function sogl_render_nurbs_surface() is de-funct.");
  }

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0) == 0 &&
      // We use GLU_NURBS_TESSELLATOR further down in the function if
      // glrender==FALSE (i.e. on callback actions were we want to get
      // the polygons), and this is not supported before GLU v1.3.
      !glrender) {
#if COIN_DEBUG
    static int first = 1;
    if (first) {
      SoDebugError::postInfo("sogl_render_nurbs_surface",
                             "NURBS tessellator requires GLU 1.3.");
      first = 0;
    }
#endif // COIN_DEBUG
    return;
  }

  SoState * state = action->getState();

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // Should not set mode if GLU version is < 1.3, as NURBS_RENDERER
    // was the only game in town back then in the old days.
    GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_NURBS_MODE,
                                   glrender ? GLU_W_NURBS_RENDERER : GLU_W_NURBS_TESSELLATOR);
  }
  // Need to load sampling matrices if glrender==FALSE.
  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_AUTO_LOAD_MATRIX, glrender);

  if (!glrender) { // supply the sampling matrices
    SbMatrix glmodelmatrix = SoViewingMatrixElement::get(state);
    glmodelmatrix.multLeft(SoModelMatrixElement::get(state));
    SbVec2s origin = SoViewportRegionElement::get(state).getViewportOriginPixels();
    SbVec2s size = SoViewportRegionElement::get(state).getViewportSizePixels();
    GLint viewport[4];
    viewport[0] = origin[0];
    viewport[1] = origin[1];
    viewport[2] = size[0];
    viewport[3] = size[1];
    GLUWrapper()->gluLoadSamplingMatrices(nurbsrenderer,
                                          (float*)glmodelmatrix,
                                          SoProjectionMatrixElement::get(state)[0],
                                          viewport);
  }

  int dim = coords->is3D() ? 3 : 4;

  const SoCoordinateElement * coordelem =
    SoCoordinateElement::getInstance(state);

  if (!coords->getNum()) return;

  GLfloat * ptr = coords->is3D() ?
    (GLfloat *)coordelem->getArrayPtr3() :
    (GLfloat *)coordelem->getArrayPtr4();

  // just copy indexed control points into a linear array
  if (numcoordindex && coordindex) {
    if (tmpcoordlist == NULL) {
      tmpcoordlist = new SbList <float>;
      atexit(nurbs_coord_cleanup);
    }
    tmpcoordlist->truncate(0);
    for (int i = 0; i < numcoordindex; i++) {
      for (int j = 0; j < dim; j++) {
        tmpcoordlist->append(ptr[coordindex[i]*dim+j]);
      }
    }
    ptr = (float*) tmpcoordlist->getArrayPtr();
  }

  switch (SoComplexityTypeElement::get(state)) {
  case SoComplexityTypeElement::SCREEN_SPACE:
    {
      SbBox3f box;
      SbVec3f center;
      shape->computeBBox(action, box, center);
      float diag;
      { 
        float dx, dy, dz;
        box.getSize(dx, dy, dz);
        diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
        if (diag == 0.0f) diag = 1.0f;
      }
      SbVec2s size;
      SoShape::getScreenSize(state, box, size);
      float maxpix = (float) SbMax(size[0], size[1]);
      if (maxpix < 1.0f) maxpix = 1.0f;
      float complexity = SoComplexityElement::get(state);
      if (complexity < 0.0001f) complexity = 0.0001f;
      complexity *= maxpix;
      complexity = diag * 0.5f / complexity;

      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_SAMPLING_METHOD,
                                     GLU_W_OBJECT_PARAMETRIC_ERROR);
      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_PARAMETRIC_TOLERANCE, 
                                     complexity);
      break;
    }
  case SoComplexityTypeElement::OBJECT_SPACE:
    {
      float diag;
      {
        SbBox3f box;
        SbVec3f center;
        shape->computeBBox(action, box, center);
        float dx, dy, dz;
        box.getSize(dx, dy, dz);
        diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
        if (diag == 0.0f) diag = 1.0f;
      }
      float complexity = SoComplexityElement::get(state);
      complexity *= complexity;
      if (complexity < 0.0001f) complexity = 0.0001f;
      complexity = diag * 0.01f / complexity;

      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_SAMPLING_METHOD,
                                     GLU_W_OBJECT_PARAMETRIC_ERROR);
      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_PARAMETRIC_TOLERANCE, 
                                     complexity);
      break;
    }
  case SoComplexityTypeElement::BOUNDING_BOX:
    assert(0 && "should never get here");
    break;
  default:
    assert(0 && "unknown complexity type");
    break;
  }

  GLUWrapper()->gluBeginSurface(nurbsrenderer);
  GLUWrapper()->gluNurbsSurface(nurbsrenderer,
                                numuknot, (GLfloat*) uknotvec,
                                numvknot, (GLfloat*) vknotvec,
                                dim, dim * numuctrlpts, ptr,
                                numuknot - numuctrlpts, numvknot - numvctrlpts,
                                (dim == 3) ? GL_MAP2_VERTEX_3 : GL_MAP2_VERTEX_4);

  if (!glrender || SoGLTextureEnabledElement::get(state)) {
    const SoTextureCoordinateElement * tc =
      SoTextureCoordinateElement::getInstance(state);
    if (numsctrlpts && numtctrlpts && numsknot && numtknot &&
        (tc->getType() == SoTextureCoordinateElement::EXPLICIT) && tc->getNum()) {
      int texdim = tc->is2D() ? 2 : 4;
      GLfloat * texptr = tc->is2D() ?
        (GLfloat*) tc->getArrayPtr2() :
        (GLfloat*) tc->getArrayPtr4();

      // copy indexed texcoords into temporary array
      if (numtexcoordindex && texcoordindex) {
        if (tmptexcoordlist == NULL) {
          tmptexcoordlist = new SbList <float>;
          atexit(nurbs_texcoord_cleanup);
        }
        tmptexcoordlist->truncate(0);
        for (int i = 0; i < numtexcoordindex; i++) {
          for (int j = 0; j < texdim; j++) {
            tmptexcoordlist->append(texptr[texcoordindex[i]*texdim+j]);
          }
        }
        texptr = (float*) tmptexcoordlist->getArrayPtr();
      }

      GLUWrapper()->gluNurbsSurface(nurbsrenderer,
                                    numsknot, (GLfloat*) sknotvec,
                                    numtknot, (GLfloat*) tknotvec,
                                    texdim, texdim * numsctrlpts,
                                    texptr, numsknot - numsctrlpts, numtknot - numtctrlpts,
                                    (texdim == 2) ? GL_MAP2_TEXTURE_COORD_2 : GL_MAP2_TEXTURE_COORD_4);

    }
    else if ((tc->getType() == SoTextureCoordinateElement::DEFAULT) ||
             (tc->getType() == SoTextureCoordinateElement::EXPLICIT)) {
      static float defaulttex[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
      };
      static GLfloat defaulttexknot[] = {0.0f, 0.0f, 1.0f, 1.0f};
      GLUWrapper()->gluNurbsSurface(nurbsrenderer, 4, defaulttexknot, 4, defaulttexknot,
                                    2, 2*2, defaulttex, 4-2, 4-2,
                                    GL_MAP2_TEXTURE_COORD_2);
    }
  }
  const SoNodeList & profilelist = SoProfileElement::get(state);
  int i, n = profilelist.getLength();
  SbBool istrimming = FALSE;

  if (n) {
    for (i = 0; i < n; i++) {
      float * points;
      int numpoints;
      int floatspervec;
      int numknots;
      float * knotvector;

      SoProfile * profile = (SoProfile*) profilelist[i];

      if (istrimming && (profile->linkage.getValue() != SoProfileElement::ADD_TO_CURRENT)) {
        istrimming = FALSE;
        GLUWrapper()->gluEndTrim(nurbsrenderer);
      }
      if (!istrimming) {
        GLUWrapper()->gluBeginTrim(nurbsrenderer);
        istrimming = TRUE;
      }
      profile->getTrimCurve(state, numpoints,
                            points, floatspervec,
                            numknots, knotvector);

      if (numknots) {
        GLUWrapper()->gluNurbsCurve(nurbsrenderer, numknots, knotvector, floatspervec,
                                    points, numknots-numpoints, floatspervec == 2 ?
                                    (GLenum) GLU_W_MAP1_TRIM_2 : (GLenum) GLU_W_MAP1_TRIM_3);

      }

      else {
        GLUWrapper()->gluPwlCurve(nurbsrenderer, numpoints, points, floatspervec,
                                  floatspervec == 2 ?
                                  (GLenum) GLU_W_MAP1_TRIM_2 : (GLenum) GLU_W_MAP1_TRIM_3 );
      }
    }
    if (istrimming) GLUWrapper()->gluEndTrim(nurbsrenderer);
  }
  GLUWrapper()->gluEndSurface(nurbsrenderer);
  
  // clear GL error(s) if parametric error value is out of range.
  while (glGetError() == GL_INVALID_VALUE);
}

void
sogl_render_nurbs_curve(SoAction * action, SoShape * shape,
                        void * nurbsrenderer,
                        const int numctrlpts,
                        const float * knotvec,
                        const int numknots,
                        const SbBool glrender,
                        const SbBool drawaspoints,
                        const int numcoordindex, const int32_t * coordindex)
{
  if (GLUWrapper()->available == 0) {
    // Should never get here if the NURBS functionality is missing.
    assert(FALSE && "Function sogl_render_nurbs_curve() is de-funct.");
  }

  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0) == 0 &&
      // We use GLU_NURBS_TESSELLATOR further down in the function if
      // glrender==FALSE (i.e. on callback actions were we want to get
      // the polygons), and this is not supported before GLU v1.3.
      !glrender) {
#if COIN_DEBUG
    static int first = 1;
    if (first) {
      SoDebugError::postInfo("sogl_render_nurbs_curve",
                             "NURBS tessellator requires GLU 1.3.");
      first = 0;
    }
#endif // COIN_DEBUG
    return;
  }

  SoState * state = action->getState();

  const SoCoordinateElement * coords =
    SoCoordinateElement::getInstance(state);

  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_DISPLAY_MODE, drawaspoints ? GLU_W_POINT : GLU_W_LINE);
  if (GLUWrapper()->versionMatchesAtLeast(1, 3, 0)) {
    // Should not set mode if GLU version is < 1.3, as NURBS_RENDERER
    // was the only game in town back then in the old days.
    GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_NURBS_MODE,
                                   glrender ? GLU_W_NURBS_RENDERER : GLU_W_NURBS_TESSELLATOR);
  }
  // Need to load sampling matrices if glrender==FALSE.
  GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_AUTO_LOAD_MATRIX, glrender);

  if (!glrender) { // supply the sampling matrices
    SbMatrix glmodelmatrix = SoViewingMatrixElement::get(state);
    glmodelmatrix.multLeft(SoModelMatrixElement::get(state));
    SbVec2s origin = SoViewportRegionElement::get(state).getViewportOriginPixels();
    SbVec2s size = SoViewportRegionElement::get(state).getViewportSizePixels();
    GLint viewport[4];
    viewport[0] = origin[0];
    viewport[1] = origin[1];
    viewport[2] = size[0];
    viewport[3] = size[1];
    GLUWrapper()->gluLoadSamplingMatrices(nurbsrenderer,
                                          (float*)glmodelmatrix,
                                          SoProjectionMatrixElement::get(state)[0],
                                          viewport);
  }

  int dim = coords->is3D() ? 3 : 4;

  GLfloat * ptr = coords->is3D() ?
    (GLfloat *)coords->getArrayPtr3() :
    (GLfloat *)coords->getArrayPtr4();

  // just copy indexed control points into a linear array
  if (numcoordindex && coordindex) {
    if (tmpcoordlist == NULL) {
      tmpcoordlist = new SbList <float>;
      atexit(nurbs_coord_cleanup);
    }
    tmpcoordlist->truncate(0);
    for (int i = 0; i < numcoordindex; i++) {
      for (int j = 0; j < dim; j++) {
        tmpcoordlist->append(ptr[coordindex[i]*dim+j]);
      }
    }
    ptr = (float*) tmpcoordlist->getArrayPtr();
  }

  switch (SoComplexityTypeElement::get(state)) {
  case SoComplexityTypeElement::SCREEN_SPACE:
    {
      SbBox3f box;
      SbVec3f center;
      shape->computeBBox(action, box, center);
      float diag;
      { 
        float dx, dy, dz;
        box.getSize(dx, dy, dz);
        diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
        if (diag == 0.0f) diag = 1.0f;
      }
      SbVec2s size;
      SoShape::getScreenSize(state, box, size);
      float maxpix = (float) SbMax(size[0], size[1]);
      if (maxpix < 1.0f) maxpix = 1.0f;
      float complexity = SoComplexityElement::get(state);
      if (complexity < 0.0001f) complexity = 0.0001f;
      complexity *= maxpix;
      complexity = diag * 0.5f / complexity;

      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_SAMPLING_METHOD,
                                     GLU_W_OBJECT_PARAMETRIC_ERROR);
      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_PARAMETRIC_TOLERANCE, 
                                     complexity);
      break;
    }
  case SoComplexityTypeElement::OBJECT_SPACE:
    {
      float diag;
      {
        SbBox3f box;
        SbVec3f center;
        shape->computeBBox(action, box, center);
        float dx, dy, dz;
        box.getSize(dx, dy, dz);
        diag = (float) sqrt(dx*dx+dy*dy+dz*dz);
        if (diag == 0.0f) diag = 1.0f;
      }
      float complexity = SoComplexityElement::get(state);
      complexity *= complexity;
      if (complexity < 0.0001f) complexity = 0.0001f;
      complexity = diag * 0.01f / complexity;

      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_SAMPLING_METHOD,
                                     GLU_W_OBJECT_PARAMETRIC_ERROR);
      GLUWrapper()->gluNurbsProperty(nurbsrenderer, (GLenum) GLU_W_PARAMETRIC_TOLERANCE, 
                                     complexity);
      break;
    }
  case SoComplexityTypeElement::BOUNDING_BOX:
    assert(0 && "should never get here");
    break;
  default:
    assert(0 && "unknown complexity type");
    break;
  }

  GLUWrapper()->gluBeginCurve(nurbsrenderer);
  GLUWrapper()->gluNurbsCurve(nurbsrenderer,
                              numknots,
                              (float*)knotvec,
                              dim,
                              ptr,
                              numknots - numctrlpts,
                              (GLenum)(dim == 3 ? GL_MAP1_VERTEX_3 : GL_MAP1_VERTEX_4));

  GLUWrapper()->gluEndCurve(nurbsrenderer);
  // clear GL error(s) if parametric error value is out of range.
  while (glGetError() == GL_INVALID_VALUE);
}



// **************************************************************************

#if !defined(NO_FACESET_RENDER)


typedef void sogl_render_faceset_func(const SoGLCoordinateElement * const coords,
                                      const int32_t *vertexindices,
                                      int num_vertexindices,
                                      const SbVec3f *normals,
                                      const int32_t *normindices,
                                      SoMaterialBundle *materials,
                                      const int32_t *matindices,
                                      const SoTextureCoordinateBundle * const texcoords,
                                      const int32_t *texindices);

static sogl_render_faceset_func *render_funcs[74];

//
// optimized faceset rendering functions.
// the functions are generated by including a template file once for
// each function, with different defines.
//

#define OVERALL             0
#define PER_FACE            1
#define PER_FACE_INDEXED    2
#define PER_VERTEX          3
#define PER_VERTEX_INDEXED  4

#define MBINDING OVERALL
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_fs_n0_m0
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_fs_n0_m1
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_fs_n0_m2
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_fs_n0_m3
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_fs_n0_m4
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES





#define MBINDING OVERALL
#define NBINDING PER_FACE
#define TEXTURES FALSE
static void sogl_fs_n1_m0
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_FACE
#define TEXTURES FALSE
static void sogl_fs_n1_m1
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_FACE
#define TEXTURES FALSE
static void sogl_fs_n1_m2
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_FACE
#define TEXTURES FALSE
static void sogl_fs_n1_m3
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_FACE
#define TEXTURES FALSE
static void sogl_fs_n1_m4
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES




#define MBINDING OVERALL
#define NBINDING PER_FACE_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n2_m0
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_FACE_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n2_m1
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_FACE_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n2_m2
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_FACE_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n2_m3
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_FACE_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n2_m4
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES





#define MBINDING OVERALL
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_fs_n3_m0
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_fs_n3_m1
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_fs_n3_m2
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_fs_n3_m3
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_fs_n3_m4
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES






#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n4_m0
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n4_m1
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n4_m2
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n4_m3
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_fs_n4_m4
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES



/* Textures ************************************************/


#define MBINDING OVERALL
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_fs_n0_m0_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_fs_n0_m1_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_fs_n0_m2_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_fs_n0_m3_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
#define TEXTURES TRUE
static void sogl_fs_n0_m4_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES





#define MBINDING OVERALL
#define NBINDING PER_FACE
#define TEXTURES TRUE
static void sogl_fs_n1_m0_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_FACE
#define TEXTURES TRUE
static void sogl_fs_n1_m1_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_FACE
#define TEXTURES TRUE
static void sogl_fs_n1_m2_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_FACE
#define TEXTURES TRUE
static void sogl_fs_n1_m3_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_FACE
#define TEXTURES TRUE
static void sogl_fs_n1_m4_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES




#define MBINDING OVERALL
#define NBINDING PER_FACE_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n2_m0_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_FACE_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n2_m1_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_FACE_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n2_m2_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_FACE_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n2_m3_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_FACE_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n2_m4_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES





#define MBINDING OVERALL
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_fs_n3_m0_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_fs_n3_m1_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_fs_n3_m2_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_fs_n3_m3_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES TRUE
static void sogl_fs_n3_m4_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES






#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n4_m0_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n4_m1_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_FACE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n4_m2_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n4_m3_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES TRUE
static void sogl_fs_n4_m4_tex
#include "SoGLFaceSetTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

void
sogl_render_faceset(const SoGLCoordinateElement * const vertexlist,
                    const int32_t *vertexindices,
                    int num_vertexindices,
                    const SbVec3f *normals,
                    const int32_t *normindices,
                    SoMaterialBundle *const materials,
                    const int32_t *matindices,
                    SoTextureCoordinateBundle * const texcoords,
                    const int32_t *texindices,
                    const int nbind,
                    const int mbind,
                    const int texture)
{
  static int first = 1;
  if (first) {
    first = 0;
    render_funcs[0] = sogl_fs_n0_m0;
    render_funcs[1] = sogl_fs_n0_m0_tex;
    render_funcs[2] = sogl_fs_n0_m1;
    render_funcs[3] = sogl_fs_n0_m1_tex;
    render_funcs[4] = sogl_fs_n0_m2;
    render_funcs[5] = sogl_fs_n0_m2_tex;
    render_funcs[6] = sogl_fs_n0_m3;
    render_funcs[7] = sogl_fs_n0_m3_tex;
    render_funcs[8] = sogl_fs_n0_m4;
    render_funcs[9] = sogl_fs_n0_m4_tex;

    render_funcs[16] = sogl_fs_n1_m0;
    render_funcs[17] = sogl_fs_n1_m0_tex;
    render_funcs[18] = sogl_fs_n1_m1;
    render_funcs[19] = sogl_fs_n1_m1_tex;
    render_funcs[20] = sogl_fs_n1_m2;
    render_funcs[21] = sogl_fs_n1_m2_tex;
    render_funcs[22] = sogl_fs_n1_m3;
    render_funcs[23] = sogl_fs_n1_m3_tex;
    render_funcs[24] = sogl_fs_n1_m4;
    render_funcs[25] = sogl_fs_n1_m4_tex;

    render_funcs[32] = sogl_fs_n2_m0;
    render_funcs[33] = sogl_fs_n2_m0_tex;
    render_funcs[34] = sogl_fs_n2_m1;
    render_funcs[35] = sogl_fs_n2_m1_tex;
    render_funcs[36] = sogl_fs_n2_m2;
    render_funcs[37] = sogl_fs_n2_m2_tex;
    render_funcs[38] = sogl_fs_n2_m3;
    render_funcs[39] = sogl_fs_n2_m3_tex;
    render_funcs[40] = sogl_fs_n2_m4;
    render_funcs[41] = sogl_fs_n2_m4_tex;

    render_funcs[48] = sogl_fs_n3_m0;
    render_funcs[49] = sogl_fs_n3_m0_tex;
    render_funcs[50] = sogl_fs_n3_m1;
    render_funcs[51] = sogl_fs_n3_m1_tex;
    render_funcs[52] = sogl_fs_n3_m2;
    render_funcs[53] = sogl_fs_n3_m2_tex;
    render_funcs[54] = sogl_fs_n3_m3;
    render_funcs[55] = sogl_fs_n3_m3_tex;
    render_funcs[56] = sogl_fs_n3_m4;
    render_funcs[57] = sogl_fs_n3_m4_tex;

    render_funcs[64] = sogl_fs_n4_m0;
    render_funcs[65] = sogl_fs_n4_m0_tex;
    render_funcs[66] = sogl_fs_n4_m1;
    render_funcs[67] = sogl_fs_n4_m1_tex;
    render_funcs[68] = sogl_fs_n4_m2;
    render_funcs[69] = sogl_fs_n4_m2_tex;
    render_funcs[70] = sogl_fs_n4_m3;
    render_funcs[71] = sogl_fs_n4_m3_tex;
    render_funcs[72] = sogl_fs_n4_m4;
    render_funcs[73] = sogl_fs_n4_m4_tex;
  }

  int idx = (nbind << 4) | (mbind << 1) | texture;

  // just in case someone forgot
  if (matindices == NULL) matindices = vertexindices;
  if (normindices == NULL) normindices = vertexindices;

  render_funcs[idx](vertexlist,
                    vertexindices,
                    num_vertexindices,
                    normals,
                    normindices,
                    materials,
                    matindices,
                    texcoords,
                    texindices);
}

#undef OVERALL
#undef PER_FACE
#undef PER_FACE_INDEXED
#undef PER_VERTEX
#undef PER_VERTEX_INDEXED

#endif // !NO_FACESET_RENDER


// **************************************************************************

#if !defined(NO_TRISTRIPSET_RENDER)


#define OVERALL              0
#define PER_STRIP            1
#define PER_STRIP_INDEXED    2
#define PER_TRIANGLE         3
#define PER_TRIANGLE_INDEXED 4
#define PER_VERTEX           5
#define PER_VERTEX_INDEXED   6

#define MBINDING OVERALL
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
#define TEXTURES FALSE
static void sogl_ts_n0_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES





#define MBINDING OVERALL
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_STRIP
#define TEXTURES FALSE
static void sogl_ts_n1_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES







#define MBINDING OVERALL
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_STRIP_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n2_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES








#define MBINDING OVERALL
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_TRIANGLE
#define TEXTURES FALSE
static void sogl_ts_n3_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES






#define MBINDING OVERALL
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n4_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES







#define MBINDING OVERALL
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
#define TEXTURES FALSE
static void sogl_ts_n5_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES








#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m0
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m1
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m2
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m3
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m4
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m5
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
#define TEXTURES FALSE
static void sogl_ts_n6_m6
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING
#undef TEXTURES


/* textures */
#undef TEXTURES
#define TEXTURES TRUE

#define MBINDING OVERALL
#define NBINDING OVERALL
static void sogl_ts_n0_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING OVERALL
static void sogl_ts_n0_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING OVERALL
static void sogl_ts_n0_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING OVERALL
static void sogl_ts_n0_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING OVERALL
static void sogl_ts_n0_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING OVERALL
static void sogl_ts_n0_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING OVERALL
static void sogl_ts_n0_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING





#define MBINDING OVERALL
#define NBINDING PER_STRIP
static void sogl_ts_n1_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_STRIP
static void sogl_ts_n1_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_STRIP
static void sogl_ts_n1_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_STRIP
static void sogl_ts_n1_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_STRIP
static void sogl_ts_n1_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_STRIP
static void sogl_ts_n1_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_STRIP
static void sogl_ts_n1_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING





#define MBINDING OVERALL
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_STRIP_INDEXED
static void sogl_ts_n2_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING





#define MBINDING OVERALL
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_TRIANGLE
static void sogl_ts_n3_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING




#define MBINDING OVERALL
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_TRIANGLE_INDEXED
static void sogl_ts_n4_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING


#define MBINDING OVERALL
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX
static void sogl_ts_n5_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING






#define MBINDING OVERALL
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m0_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m1_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_STRIP_INDEXED
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m2_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m3_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_TRIANGLE_INDEXED
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m4_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m5_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING

#define MBINDING PER_VERTEX_INDEXED
#define NBINDING PER_VERTEX_INDEXED
static void sogl_ts_n6_m6_tex
#include "SoGLTristripTemplate.cpp"
#undef MBINDING
#undef NBINDING


#undef TEXTURES


static sogl_render_faceset_func *render_ts_funcs[110];


void
sogl_render_tristrip(const SoGLCoordinateElement * const vertexlist,
                     const int32_t *vertexindices,
                     int num_vertexindices,
                     const SbVec3f *normals,
                     const int32_t *normindices,
                     SoMaterialBundle *const materials,
                     const int32_t *matindices,
                     const SoTextureCoordinateBundle * const texcoords,
                     const int32_t *texindices,
                     const int nbind,
                     const int mbind,
                     const int texture)
{
  static int first = 1;
  if (first) {
    first = 0;
    render_ts_funcs[0] = sogl_ts_n0_m0;
    render_ts_funcs[1] = sogl_ts_n0_m0_tex;
    render_ts_funcs[2] = sogl_ts_n0_m1;
    render_ts_funcs[3] = sogl_ts_n0_m1_tex;
    render_ts_funcs[4] = sogl_ts_n0_m2;
    render_ts_funcs[5] = sogl_ts_n0_m2_tex;
    render_ts_funcs[6] = sogl_ts_n0_m3;
    render_ts_funcs[7] = sogl_ts_n0_m3_tex;
    render_ts_funcs[8] = sogl_ts_n0_m4;
    render_ts_funcs[9] = sogl_ts_n0_m4_tex;
    render_ts_funcs[10] = sogl_ts_n0_m5;
    render_ts_funcs[11] = sogl_ts_n0_m5_tex;
    render_ts_funcs[12] = sogl_ts_n0_m6;
    render_ts_funcs[13] = sogl_ts_n0_m6_tex;

    render_ts_funcs[16] = sogl_ts_n1_m0;
    render_ts_funcs[17] = sogl_ts_n1_m0_tex;
    render_ts_funcs[18] = sogl_ts_n1_m1;
    render_ts_funcs[19] = sogl_ts_n1_m1_tex;
    render_ts_funcs[20] = sogl_ts_n1_m2;
    render_ts_funcs[21] = sogl_ts_n1_m2_tex;
    render_ts_funcs[22] = sogl_ts_n1_m3;
    render_ts_funcs[23] = sogl_ts_n1_m3_tex;
    render_ts_funcs[24] = sogl_ts_n1_m4;
    render_ts_funcs[25] = sogl_ts_n1_m4_tex;
    render_ts_funcs[26] = sogl_ts_n1_m5;
    render_ts_funcs[27] = sogl_ts_n1_m5_tex;
    render_ts_funcs[28] = sogl_ts_n1_m6;
    render_ts_funcs[29] = sogl_ts_n1_m6_tex;

    render_ts_funcs[32] = sogl_ts_n2_m0;
    render_ts_funcs[33] = sogl_ts_n2_m0_tex;
    render_ts_funcs[34] = sogl_ts_n2_m1;
    render_ts_funcs[35] = sogl_ts_n2_m1_tex;
    render_ts_funcs[36] = sogl_ts_n2_m2;
    render_ts_funcs[37] = sogl_ts_n2_m2_tex;
    render_ts_funcs[38] = sogl_ts_n2_m3;
    render_ts_funcs[39] = sogl_ts_n2_m3_tex;
    render_ts_funcs[40] = sogl_ts_n2_m4;
    render_ts_funcs[41] = sogl_ts_n2_m4_tex;
    render_ts_funcs[42] = sogl_ts_n2_m5;
    render_ts_funcs[43] = sogl_ts_n2_m5_tex;
    render_ts_funcs[44] = sogl_ts_n2_m6;
    render_ts_funcs[45] = sogl_ts_n2_m6_tex;

    render_ts_funcs[48] = sogl_ts_n3_m0;
    render_ts_funcs[49] = sogl_ts_n3_m0_tex;
    render_ts_funcs[50] = sogl_ts_n3_m1;
    render_ts_funcs[51] = sogl_ts_n3_m1_tex;
    render_ts_funcs[52] = sogl_ts_n3_m2;
    render_ts_funcs[53] = sogl_ts_n3_m2_tex;
    render_ts_funcs[54] = sogl_ts_n3_m3;
    render_ts_funcs[55] = sogl_ts_n3_m3_tex;
    render_ts_funcs[56] = sogl_ts_n3_m4;
    render_ts_funcs[57] = sogl_ts_n3_m4_tex;
    render_ts_funcs[57] = sogl_ts_n3_m5;
    render_ts_funcs[58] = sogl_ts_n3_m5_tex;
    render_ts_funcs[59] = sogl_ts_n3_m6;
    render_ts_funcs[60] = sogl_ts_n3_m6_tex;

    render_ts_funcs[64] = sogl_ts_n4_m0;
    render_ts_funcs[65] = sogl_ts_n4_m0_tex;
    render_ts_funcs[66] = sogl_ts_n4_m1;
    render_ts_funcs[67] = sogl_ts_n4_m1_tex;
    render_ts_funcs[68] = sogl_ts_n4_m2;
    render_ts_funcs[69] = sogl_ts_n4_m2_tex;
    render_ts_funcs[70] = sogl_ts_n4_m3;
    render_ts_funcs[71] = sogl_ts_n4_m3_tex;
    render_ts_funcs[72] = sogl_ts_n4_m4;
    render_ts_funcs[73] = sogl_ts_n4_m4_tex;
    render_ts_funcs[74] = sogl_ts_n4_m5;
    render_ts_funcs[75] = sogl_ts_n4_m5_tex;
    render_ts_funcs[76] = sogl_ts_n4_m6;
    render_ts_funcs[77] = sogl_ts_n4_m6_tex;

    render_ts_funcs[80] = sogl_ts_n5_m0;
    render_ts_funcs[81] = sogl_ts_n5_m0_tex;
    render_ts_funcs[82] = sogl_ts_n5_m1;
    render_ts_funcs[83] = sogl_ts_n5_m1_tex;
    render_ts_funcs[84] = sogl_ts_n5_m2;
    render_ts_funcs[85] = sogl_ts_n5_m2_tex;
    render_ts_funcs[86] = sogl_ts_n5_m3;
    render_ts_funcs[87] = sogl_ts_n5_m3_tex;
    render_ts_funcs[88] = sogl_ts_n5_m4;
    render_ts_funcs[89] = sogl_ts_n5_m4_tex;
    render_ts_funcs[90] = sogl_ts_n5_m5;
    render_ts_funcs[91] = sogl_ts_n5_m5_tex;
    render_ts_funcs[92] = sogl_ts_n5_m6;
    render_ts_funcs[93] = sogl_ts_n5_m6_tex;

    render_ts_funcs[96] = sogl_ts_n6_m0;
    render_ts_funcs[97] = sogl_ts_n6_m0_tex;
    render_ts_funcs[98] = sogl_ts_n6_m1;
    render_ts_funcs[99] = sogl_ts_n6_m1_tex;
    render_ts_funcs[100] = sogl_ts_n6_m2;
    render_ts_funcs[101] = sogl_ts_n6_m2_tex;
    render_ts_funcs[102] = sogl_ts_n6_m3;
    render_ts_funcs[103] = sogl_ts_n6_m3_tex;
    render_ts_funcs[104] = sogl_ts_n6_m4;
    render_ts_funcs[105] = sogl_ts_n6_m4_tex;
    render_ts_funcs[106] = sogl_ts_n6_m5;
    render_ts_funcs[107] = sogl_ts_n6_m5_tex;
    render_ts_funcs[108] = sogl_ts_n6_m6;
    render_ts_funcs[109] = sogl_ts_n6_m6_tex;
  }

  int idx = (nbind << 4) | (mbind << 1) | texture;

  // just in case someone forgot...
  if (matindices == NULL) matindices = vertexindices;
  if (normindices == NULL) normindices = vertexindices;

  render_ts_funcs[idx](vertexlist,
                       vertexindices,
                       num_vertexindices,
                       normals,
                       normindices,
                       materials,
                       matindices,
                       texcoords,
                       texindices);
}

#undef OVERALL
#undef PER_STRIP
#undef PER_STRIP_INDEXED
#undef PER_TRIANGLE
#undef PER_TRIANGLE_INDEXED
#undef PER_VERTEX
#undef PER_VERTEX_INDEXED


#endif // !NO_TRISTRIPSET_RENDER

// **************************************************************************

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
  \class SbMatrix SbLinear.h Inventor/SbLinear.h
  \brief The SbMatrix class is a 4x4 dimensional representation of a matrix.
  \ingroup base

  SbMatrix is used by many other classes in Coin.  It provides storage
  for a 4x4 matrix in row-major mode. Many common geometrical
  operations which involves matrix calculations are implemented as
  methods on this class.
*/

// FIXME:
//
//  * The SbMatrix::factor() function has not been implemented yet.
//
//  * The element access methods should be inlined.
//
//  * Optimizations are not done yet, so there's a lot of room for
//    improvements.


#include <Inventor/SbMatrix.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbLine.h>
#include <coindefs.h> // COIN_STUB()
#include <assert.h>
#include <string.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

// FIXME: should merge all the PD code we're using from GGIV into
// SbMatrix, SbRotation and SbVec3f proper (for two reasons: 1)
// there's a lot of duplicated code here (like for instance the
// matrix->quaternion decomposition, which also exists in
// SbRotation::setValue(SbMatrix&)), and 2) the remaining code
// snippets look generally useful outside the purpose of breaking down
// a matrix into it's transformation components). 20010114 mortene.

/*
 * declarations for polar_decomp algorithm from Graphics Gems IV,
 * by Ken Shoemake <shoemake@graphics.cis.upenn.edu>
 */
enum QuatPart {X, Y, Z, W};
typedef float HMatrix[4][4]; /* Right-handed, for column vectors */
typedef struct {
  SbVec4f t;    /* Translation components */
  SbRotation  q;        /* Essential rotation     */
  SbRotation  u;        /* Stretch rotation       */
  SbVec4f k;    /* Stretch factors        */
  float f;      /* Sign of determinant    */
} AffineParts;
static float polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
static SbVec4f spect_decomp(HMatrix S, HMatrix U);
static SbRotation snuggle(SbRotation q, SbVec4f & k);
static void decomp_affine(HMatrix A, AffineParts * parts);


/*!
  The default constructor does nothing. The matrix will be uninitialized.
 */
SbMatrix::SbMatrix(void)
{
}


/*!
  Constructs a matrix instance with the given initial elements.
 */
SbMatrix::SbMatrix(const float a11, const float a12,
                   const float a13, const float a14,
                   const float a21, const float a22,
                   const float a23, const float a24,
                   const float a31, const float a32,
                   const float a33, const float a34,
                   const float a41, const float a42,
                   const float a43, const float a44)
{
  const SbMat m = { { a11, a12, a13, a14 },
                    { a21, a22, a23, a24 },
                    { a31, a32, a33, a34 },
                    { a41, a42, a43, a44 } };
  this->setValue(m);
}

/*!
  Constructs a matrix instance with the initial elements from the
  \a matrix argument.
 */
SbMatrix::SbMatrix(const SbMat & matrix)
{
  this->setValue(matrix);
}

/*!
  This constructor is courtesy of the Microsoft Visual C++ compiler.
*/
SbMatrix::SbMatrix(const SbMat * matrix)
{
  this->setValue(*matrix);
}

/*!
  Default destructor does nothing.
 */
SbMatrix::~SbMatrix(void)
{
}

/*!
  Returns a pointer to the 2 dimensional float array with the matrix
  elements.

  \sa setValue().
 */
const SbMat &
SbMatrix::getValue(void) const
{
  return this->matrix;
}

/*!
  Copies the elements from \a m into the matrix.

  \sa getValue().
 */
void
SbMatrix::setValue(const SbMat & m)
{
  (void)memmove(this->matrix, m, sizeof(float)*4*4);
}

/*!
  Assignment operator. Copies the elements from \a m to the matrix.
 */
SbMatrix &
SbMatrix::operator=(const SbMat & m)
{
  this->setValue(m);
  return *this;
}

/*!
  Assignment operator. Copies the elements from \a m to the matrix.
 */
SbMatrix &
SbMatrix::operator=(const SbMatrix & m)
{
  this->setValue(m.matrix);
  return *this;
}

/*!
  Set the matrix to be the identity matrix.

  \sa identity().
 */
void
SbMatrix::makeIdentity(void)
{
  this->matrix[0][0]=this->matrix[1][1]=
    this->matrix[2][2]=this->matrix[3][3] = 1.0f;

  this->matrix[0][1]=this->matrix[0][2]=this->matrix[0][3]=
    this->matrix[1][0]=this->matrix[1][2]=this->matrix[1][3]=
    this->matrix[2][0]=this->matrix[2][1]=this->matrix[2][3]=
    this->matrix[3][0]=this->matrix[3][1]=this->matrix[3][2] = 0.0f;
}

/*!
  Set matrix to be a rotation matrix with the given rotation.

  \sa setTranslate(), setScale().
*/
void
SbMatrix::setRotate(const SbRotation & q)
{
  q.getValue(*this);
}

/*!
  Multiply all element values in the matrix with \a v.
 */
void
SbMatrix::operator*=(const float v)
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      this->matrix[i][j] *= v;
    }
  }
}

/*!
  Divide all element values in the matrix on \a v.
 */
void
SbMatrix::operator/=(const float v)
{
#if COIN_DEBUG
  if (v==0.0f)
    SoDebugError::postWarning("SbMatrix::operator/=",
                              "Division by zero.");
#endif // COIN_DEBUG

  this->operator*=(1.0f/v);
}

/*!
  Returns the determinant of the 3x3 submatrix specified by the row and
  column indices.
 */
float
SbMatrix::det3(int r1, int r2, int r3,
               int c1, int c2, int c3) const
{
#if COIN_EXTRA_DEBUG
  // Check indices.
  if (r1<0 || r1>3 || r2<0 || r2>3 || r3<0 || r3>3 ||
      c1<0 || c1>3 || c2<0 || c2>3 || c3<0 || c3>3) {
    SoDebugError::post("SbMatrix::det3",
                       "At least one idx out of bounds [0, 3]. ");
  }
  if (r1==r2 || r1==r3 || r2==r3 ||
      c1==c2 || c1==c3 || c2==c3)
    SoDebugError::post("SbMatrix::det3", "Indices should be distinct.");
#endif // COIN_EXTRA_DEBUG

  // More or less directly from "Advanced Engineering Mathematics"
  // (E. Kreyszig), 6th edition.

  float a11 = this->matrix[r1][c1];
  float a12 = this->matrix[r1][c2];
  float a13 = this->matrix[r1][c3];
  float a21 = this->matrix[r2][c1];
  float a22 = this->matrix[r2][c2];
  float a23 = this->matrix[r2][c3];
  float a31 = this->matrix[r3][c1];
  float a32 = this->matrix[r3][c2];
  float a33 = this->matrix[r3][c3];

  float M11 = a22 * a33 - a32 * a23;
  float M21 = -(a12 * a33 - a32 * a13);
  float M31 = a12 * a23 - a22 * a13;

  return (a11 * M11 + a21 * M21 + a31 * M31);
}

/*!
  Returns the determinant of the upper left 3x3 submatrix.
 */
float
SbMatrix::det3(void) const
{
  return this->det3(0, 1, 2, 0, 1, 2);
}

/*!
  Returns the determinant of the matrix.
 */
float
SbMatrix::det4(void) const
{
  float det = 0.0f;
  det += this->matrix[0][0] * det3(1, 2, 3, 1, 2, 3);
  det -= this->matrix[1][0] * det3(0, 2, 3, 1, 2, 3);
  det += this->matrix[2][0] * det3(0, 1, 3, 1, 2, 3);
  det -= this->matrix[3][0] * det3(0, 1, 2, 1, 2, 3);
  return det;
}

/*!
  Return a new matrix which is the inverse matrix of this.

  The user is responsible for checking that this is a valid operation
  to execute, by first making sure that the result of SbMatrix::det4()
  is not equal to zero.
 */
SbMatrix
SbMatrix::inverse(void) const
{
  float det = this->det4();
#if COIN_DEBUG
  if (det == 0.0f) {
    SoDebugError::postWarning("SbMatrix::inverse",
                              "Determinant of matrix is zero.");
    return *this;
  }
#endif // COIN_DEBUG

  SbMatrix result;

  // FIXME: we should be using an optimized way of calculating the
  // inverse matrix. 20010114 mortene.
  result.matrix[0][0] = this->det3(1, 2, 3, 1, 2, 3);
  result.matrix[1][0] = -this->det3(1, 2, 3, 0, 2, 3);
  result.matrix[2][0] = this->det3(1, 2, 3, 0, 1, 3);
  result.matrix[3][0] = -this->det3(1, 2, 3, 0, 1, 2);
  result.matrix[0][1] = -this->det3(0, 2, 3, 1, 2, 3);
  result.matrix[1][1] = this->det3(0, 2, 3, 0, 2, 3);
  result.matrix[2][1] = -this->det3(0, 2, 3, 0, 1, 3);
  result.matrix[3][1] = this->det3(0, 2, 3, 0, 1, 2);
  result.matrix[0][2] = this->det3(0, 1, 3, 1, 2, 3);
  result.matrix[1][2] = -this->det3(0, 1, 3, 0, 2, 3);
  result.matrix[2][2] = this->det3(0, 1, 3, 0, 1, 3);
  result.matrix[3][2] = -this->det3(0, 1, 3, 0, 1, 2);
  result.matrix[0][3] = -this->det3(0, 1, 2, 1, 2, 3);
  result.matrix[1][3] = this->det3(0, 1, 2, 0, 2, 3);
  result.matrix[2][3] = -this->det3(0, 1, 2, 0, 1, 3);
  result.matrix[3][3] = this->det3(0, 1, 2, 0, 1, 2);

  result /= det;
  return result;
}

/*!
  Check if the \a m matrix is equal to this one, within the given tolerance
  value. The tolerance value is applied in the comparison on a component by
  component basis.
 */
SbBool
SbMatrix::equals(const SbMatrix & m, float tolerance) const
{
#if COIN_DEBUG
  if (tolerance<0.0f)
    SoDebugError::postWarning("SbMatrix::equals",
                              "tolerance should be >=0.0f.");
#endif // COIN_DEBUG

  for (int i=0; i < 4; i++) {
    for (int j=0;  j< 4; j++) {
      if (fabs(this->matrix[i][j] - m.matrix[i][j]) > tolerance) return FALSE;
    }
  }

  return TRUE;
}


/*!
  Return pointer to the matrix' 4x4 float array.
 */
SbMatrix::operator float*(void)
{
  return &(this->matrix[0][0]);
}


/*!
  Return pointer to the matrix' 4x4 float array.
 */
SbMatrix::operator SbMat&(void)
{
  return this->matrix;
}

/*!
  Returns pointer to the 4 element array representing a matrix row.
  \a i should be within [0, 3].

  \sa getValue(), setValue().
 */
float *
SbMatrix::operator [](int i)
{
#if COIN_EXTRA_DEBUG
  if (i<0 || i>3) {
    SoDebugError::post("SbMatrix::operator[]", "Index out of bounds. ");
  }
#endif // COIN_EXTRA_DEBUG

   return this->matrix[i];
}

/*!
  Returns pointer to the 4 element array representing a matrix row.
  \a i should be within [0, 3].

  \sa getValue(), setValue().
 */
const float *
SbMatrix::operator [](int i) const
{
#if COIN_EXTRA_DEBUG
  if (i<0 || i>3) {
    SoDebugError::postWarning("SbMatrix::operator[]", "Index out of bounds. ");
  }
#endif // COIN_EXTRA_DEBUG

   return this->matrix[i];
}

/*!
  Set matrix to be a rotation matrix with the given rotation.

  \sa setRotate().
*/
SbMatrix&
SbMatrix::operator =(const SbRotation & q)
{
  this->setRotate(q);
  return *this;
}

/*!
  Right-multiply with the \a m matrix.

  \sa multRight().
 */
SbMatrix&
SbMatrix::operator *=(const SbMatrix & m)
{
  return this->multRight(m);
}

/*!
  \relates SbMatrix

  Multiplies matrix \a m1 with matrix \a m2 and returns the resultant
  matrix.
*/
SbMatrix
operator *(const SbMatrix & m1, const SbMatrix & m2)
{
  SbMatrix result = m1;
  result *= m2;
  return result;
}

/*!
  \relates SbMatrix

  Compare matrices to see if they are equal. For two matrices to be equal,
  all their individual elements must be equal.

  \sa equals().
*/
int
operator ==(const SbMatrix & m1, const SbMatrix & m2)
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      if (m1.matrix[i][j] != m2.matrix[i][j]) return FALSE;
    }
  }

  return TRUE;
}

/*!
  \relates SbMatrix

  Compare matrices to see if they are not equal. For two matrices to not be
  equal, it is enough that at least one of their elements are unequal.

  \sa equals().
*/
int
operator !=(const SbMatrix & m1, const SbMatrix & m2)
{
  return !(m1 == m2);
}

/*!
  Return matrix components in the SbMat structure.

  \sa setValue().
 */
void
SbMatrix::getValue(SbMat & m) const
{
  (void)memmove(&m[0][0], &(this->matrix[0][0]), sizeof(float)*4*4);
}

/*!
  Return the identity matrix.

  \sa makeIdentity().
 */
SbMatrix
SbMatrix::identity(void)
{
  return SbMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 0.0f, 0.0f,
                  0.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f);
}

/*!
  Set matrix to be a pure scaling matrix. Scale factors are specified
  by \a s.

  \sa setRotate(), setTranslate().
 */
void
SbMatrix::setScale(const float s)
{
  this->makeIdentity();
  this->matrix[0][0] = s;
  this->matrix[1][1] = s;
  this->matrix[2][2] = s;
}

/*!
  Set matrix to be a pure scaling matrix. Scale factors in x, y and z
  is specified by the \a s vector.

  \sa setRotate(), setTranslate().
 */
void
SbMatrix::setScale(const SbVec3f & s)
{
  this->makeIdentity();
  this->matrix[0][0] = s[0];
  this->matrix[1][1] = s[1];
  this->matrix[2][2] = s[2];
}

/*!
  Make this matrix into a pure translation matrix (no scale or rotation
  components) with the given vector \t as the translation.

  \sa setRotate(), setScale().
 */
void
SbMatrix::setTranslate(const SbVec3f & t)
{
  this->makeIdentity();
  this->matrix[3][0] = t[0];
  this->matrix[3][1] = t[1];
  this->matrix[3][2] = t[2];
}

/*!
  Set translation, rotation and scaling all at once. The resulting
  matrix gets calculated like this:

  \code
  M = S * R * T
  \endcode

  where \a S, \a R and \a T is scaling, rotation and translation
  matrices.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbMatrix::setTransform(const SbVec3f & t, const SbRotation & r, const SbVec3f & s)
{
  SbMatrix tmp;

  this->setScale(s);

  tmp.setRotate(r);
  this->multRight(tmp);

  tmp.setTranslate(t);
  this->multRight(tmp);
}

/*!
  Set translation, rotation and scaling all at once with a specified
  scale orientation. The resulting matrix gets calculated like this:

  \code
  M = Ro-� * S * Ro * R * T
  \endcode

  where \a Ro is the scale orientation, and \a S, \a R
  and \a T is scaling, rotation and translation.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbMatrix::setTransform(const SbVec3f & t, const SbRotation & r,
                       const SbVec3f & s, const SbRotation & so)
{
  SbMatrix tmp;

  this->setRotate(so.inverse());

  tmp.setScale(s);
  this->multRight(tmp);

  tmp.setRotate(so);
  this->multRight(tmp);

  tmp.setRotate(r);
  this->multRight(tmp);

  tmp.setTranslate(t);
  this->multRight(tmp);
}

/*!
  Set translation, rotation and scaling all at once with a specified
  scale orientation and center point. The resulting matrix gets
  calculated like this:

  \code
  M = -Tc * Ro-� * S * Ro * R * T * Tc
  \endcode

  where \a Tc is the center point, \a Ro the scale orientation, \a S,
  \a R and \a T is scaling, rotation and translation.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbMatrix::setTransform(const SbVec3f & translation,
                       const SbRotation & rotation,
                       const SbVec3f & scaleFactor,
                       const SbRotation & scaleOrientation,
                       const SbVec3f & center)
{
  SbMatrix tmp;

  this->setTranslate(-center);

  tmp.setRotate(scaleOrientation.inverse());
  this->multRight(tmp);

  tmp.setScale(scaleFactor);
  this->multRight(tmp);

  tmp.setRotate(scaleOrientation);
  this->multRight(tmp);

  tmp.setRotate(rotation);
  this->multRight(tmp);

  tmp.setTranslate(translation);
  this->multRight(tmp);

  tmp.setTranslate(center);
  this->multRight(tmp);
}

/*!
  Factor the matrix back into its translation, rotation, scale and
  scaleorientation components.

  \sa factor()
 */
void
SbMatrix::getTransform(SbVec3f & t, SbRotation & r, SbVec3f & s,
                       SbRotation & so) const
{
  // FIXME: test if this code works with non-affine matrices.
  // pederb, 2000-01-17

  AffineParts parts;
  HMatrix hmatrix;

  // transpose-copy
  hmatrix[0][0] = this->matrix[0][0];
  hmatrix[0][1] = this->matrix[1][0];
  hmatrix[0][2] = this->matrix[2][0];
  hmatrix[0][3] = this->matrix[3][0];

  hmatrix[1][0] = this->matrix[0][1];
  hmatrix[1][1] = this->matrix[1][1];
  hmatrix[1][2] = this->matrix[2][1];
  hmatrix[1][3] = this->matrix[3][1];

  hmatrix[2][0] = this->matrix[0][2];
  hmatrix[2][1] = this->matrix[1][2];
  hmatrix[2][2] = this->matrix[2][2];
  hmatrix[2][3] = this->matrix[3][2];

  hmatrix[3][0] = this->matrix[0][3];
  hmatrix[3][1] = this->matrix[1][3];
  hmatrix[3][2] = this->matrix[2][3];
  hmatrix[3][3] = this->matrix[3][3];

  decomp_affine(hmatrix, &parts);

  float mul = 1.0f;
  if (parts.t[W] != 0.0f) mul = 1.0f / parts.t[W];
  t[0] = parts.t[X] * mul;
  t[1] = parts.t[Y] * mul;
  t[2] = parts.t[Z] * mul;

  r = parts.q;
  mul = 1.0f;
  if (parts.k[W] != 0.0f) mul = 1.0f / parts.k[W];
  // mul be sign of determinant to support negative scales.
  mul *= parts.f;
  s[0] = parts.k[X] * mul;
  s[1] = parts.k[Y] * mul;
  s[2] = parts.k[Z] * mul;

  so = parts.u;
}

/*!
  Factor the matrix back into its \a translation, \a rotation,
  \a scaleFactor and \a scaleorientation components. Will eliminate
  the \a center variable from the matrix.

  \sa factor()
 */
void
SbMatrix::getTransform(SbVec3f & translation,
                       SbRotation & rotation,
                       SbVec3f & scaleFactor,
                       SbRotation & scaleOrientation,
                       const SbVec3f & center) const
{
  SbMatrix m2 = *this;
  SbMatrix trans;
  trans.setTranslate(center);
  m2.multLeft(trans);
  trans.setTranslate(-center);
  m2.multRight(trans);

  m2.getTransform(translation, rotation, scaleFactor, scaleOrientation);
}

/*!
  This function is not implemented in Coin.

  \sa getTransform()
 */
SbBool
SbMatrix::factor(SbMatrix & r, SbVec3f & s, SbMatrix & u, SbVec3f & t,
                 SbMatrix & proj)
{
  // FIXME: not implemented, not documented. 1998MMDD mortene.
  COIN_STUB();
  return FALSE;
}

/*!
  This function produces a permuted LU decomposition of the matrix.  It
  uses the common single-row-pivoting strategy.

  \a FALSE is returned if the matrix is singular, which it never is, because
  of small adjustment values inserted if a singularity is found (as Open
  Inventor does too).

  The parity argument is always set to 1.0 or -1.0.  Don't really know what
  it's for, so it's not checked for correctness.

  The index[] argument returns the permutation that was done on the matrix
  to LU-decompose it.  index[i] is the row that row i was swapped with at
  step i in the decomposition, so index[] is not the actual permutation of
  the row indexes!

  BUGS:  The function does not produce results that are numerically identical
  with those produced by Open Inventor for the same matrices, because the
  pivoting strategy in OI was never fully understood.

  \sa SbMatrix::LUBackSubstitution
*/


SbBool
SbMatrix::LUDecomposition(int index[4], float & d)
{
    int i;
    for (i = 0; i < 4; i++) index[i] = i;
    d = 1.0f;

    const float MINIMUM_PIVOT = 1e-6f; // Inventor fix...

    for (int row = 1; row < 4; row++) {
        int swap_row = row;
        float max_pivot = 0.0f;
        for (int test_row = row; test_row < 4; test_row++) {
            const float test_pivot = SbAbs(matrix[test_row][row]);
            if (test_pivot > max_pivot) {
                swap_row = test_row;
                max_pivot = test_pivot;
            }
        }

        // swap rows
        if (swap_row != row) {
            d = -d;
            index[row] = swap_row;
            for (i = 0; i < 4; i++)
                SbSwap(matrix[row][i], matrix[swap_row][i]);
        }

        float pivot = matrix[row][row];
        if (matrix[row][row] == 0.0f) {
//            return FALSE;
            // instead of returning FALSE on singulars...
            matrix[row][row] = pivot = MINIMUM_PIVOT;
        }

        // the factorization
        for (i = row + 1; i < 4; i++) {
            const float factor = (matrix[i][row] /= pivot);
            for (int j = row + 1; j < 4; j++)
                matrix[i][j] -= factor * matrix[row][j];
        }
    }
    return TRUE;
}

/*!
  This function does a solve on the "Ax = b" system, given that the matrix
  is LU-decomposed in advance.  First, a forward substitution is done on the
  lower system (Ly = b), and then a backwards substitution is done on the
  upper triangular system (Ux = y).

  The index[] argument is the one returned from
  SbMatrix::LUDecomposition(), so see that function for an explanation.

  The b[] argument must contain the b vector in "Ax = b" when calling the
  function.  After the function has solved the system, the b[] vector contains
  the x vector.

  BUGS:  As is done by Open Inventor, unsolvable x values will not return
  NaN but 0.
*/

void
SbMatrix::LUBackSubstitution(int index[4], float b[4]) const
{
    int i;

    // permute b[] the way matrix[][] is permuted
    for (i = 0; i < 4; i++)
        if (i != index[i])
            SbSwap(b[i], b[index[i]]);

    // forward substitution on L (Ly = b)
    float y[4];
    for (i = 0; i < 4; i++) {
        float sum = 0.0f;
        for (int j = 0; j < i; j++)
            sum += matrix[i][j] * y[j];
        y[i] = b[i] - sum;
    }

    // backwards substitution on U (Ux = y)
    float x[4];
    for (i = 3; i >= 0; i--) {
        float sum = 0.0f;
        for (int j = i + 1; j < 4; j++)
             sum += matrix[i][j] * x[j];
        if (matrix[i][i] != 0.0f)
            x[i] = (y[i] - sum) / matrix[i][i];
        else
            x[i] = 0.0f;
    }

    // de-permute x[]?  doesn't look like it
//    for (i = 3; i >= 0; i--)
//        if (i != index[i])
//            SbSwap(x[i], x[index[i]]);

    // copy x[] into b[] for "return to sender"
    for (i = 0; i < 4; i++) b[i] = x[i];
}

/*!
  Returns the transpose of this matrix.
*/

SbMatrix
SbMatrix::transpose(void) const
{
  SbMatrix trans = (*this);

  for (int i=0; i < 3; i++) {
    for (int j=i+1; j < 4; j++) {
      SbSwap(trans[i][j], trans[j][i]);
    }
  }

  return trans;
}

/*!
  Let this matrix be right-multiplied by \a m. Returns reference to
  self.

  \sa multLeft()
*/
SbMatrix &
SbMatrix::multRight(const SbMatrix & m)
{
  // FIXME: should check if one or the other matrix is the
  // identity-matrix first. (Because it's major optimization if one of
  // them _is_, and the check should be very quick in the common case
  // where none of them are.)  20010919 mortene.

  SbMatrix tmp(*this);
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      this->matrix[i][j] =
        tmp.matrix[i][0] * m.matrix[0][j] +
        tmp.matrix[i][1] * m.matrix[1][j] +
        tmp.matrix[i][2] * m.matrix[2][j] +
        tmp.matrix[i][3] * m.matrix[3][j];
    }
  }
  return *this;
}

/*!
  Let this matrix be left-multiplied by \a m. Returns reference to
  self.

  \sa multRight()
*/
SbMatrix&
SbMatrix::multLeft(const SbMatrix & m)
{
  // FIXME: should check if one or the other matrix is the
  // identity-matrix first. (Because it's major optimization if one of
  // them _is_, and the check should be very quick in the common case
  // where none of them are.)  20010919 mortene.

  SbMatrix tmp(*this);
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      this->matrix[i][j] =
        tmp.matrix[0][j] * m.matrix[i][0] +
        tmp.matrix[1][j] * m.matrix[i][1] +
        tmp.matrix[2][j] * m.matrix[i][2] +
        tmp.matrix[3][j] * m.matrix[i][3];
    }
  }
  return *this;
}

/*!
  Multiply \a src vector with this matrix and return the result in \a dst.
  Multiplication is done with the vector on the right side of the
  expression, i.e. dst = M * src.

  \sa multVecMatrix(), multDirMatrix() and multLineMatrix().
*/
void
SbMatrix::multMatrixVec(const SbVec3f & src, SbVec3f & dst) const
{
  // FIXME: should check if we're the identity matrix? 20010919 mortene.

  const float * t0 = (*this)[0];
  const float * t1 = (*this)[1];
  const float * t2 = (*this)[2];
  const float * t3 = (*this)[3];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3f s = src;

  float W = s[0]*t3[0] + s[1]*t3[1] + s[2]*t3[2] + t3[3];

  dst[0] = (s[0]*t0[0] + s[1]*t0[1] + s[2]*t0[2] + t0[3])/W;
  dst[1] = (s[0]*t1[0] + s[1]*t1[1] + s[2]*t1[2] + t1[3])/W;
  dst[2] = (s[0]*t2[0] + s[1]*t2[1] + s[2]*t2[2] + t2[3])/W;
}

/*!
  Multiply \a src vector with this matrix and return the result in \a dst.
  Multiplication is done with the vector on the left side of the
  expression, i.e. dst = src * M.

  It is safe to let \a src and \a dst be the same SbVec3f instance.

  \sa multMatrixVec(), multDirMatrix() and multLineMatrix().
*/
void
SbMatrix::multVecMatrix(const SbVec3f & src, SbVec3f & dst) const
{
  // FIXME: should check if we're the identity matrix? 20010919 mortene.

  const float * t0 = this->matrix[0];
  const float * t1 = this->matrix[1];
  const float * t2 = this->matrix[2];
  const float * t3 = this->matrix[3];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3f s = src;

  float W = s[0]*t0[3] + s[1]*t1[3] + s[2]*t2[3] + t3[3];

  dst[0] = (s[0]*t0[0] + s[1]*t1[0] + s[2]*t2[0] + t3[0])/W;
  dst[1] = (s[0]*t0[1] + s[1]*t1[1] + s[2]*t2[1] + t3[1])/W;
  dst[2] = (s[0]*t0[2] + s[1]*t1[2] + s[2]*t2[2] + t3[2])/W;
}

/*!
  \overload
*/
void
SbMatrix::multVecMatrix(const SbVec4f & src, SbVec4f & dst) const
{
  // FIXME: should check if we're the identity matrix? 20010919 mortene.

  const float * t0 = (*this)[0];
  const float * t1 = (*this)[1];
  const float * t2 = (*this)[2];
  const float * t3 = (*this)[3];

  SbVec4f s = src;

  dst[0] = (s[0]*t0[0] + s[1]*t0[1] + s[2]*t0[2] + t0[3]);
  dst[1] = (s[0]*t1[0] + s[1]*t1[1] + s[2]*t1[2] + t1[3]);
  dst[2] = (s[0]*t2[0] + s[1]*t2[1] + s[2]*t2[2] + t2[3]);
  dst[3] = (s[0]*t3[0] + s[1]*t3[1] + s[2]*t3[2] + t3[3]);
}

/*!
  Multiplies \a src by the matrix. \a src is assumed to be a direction
  vector, and the translation components of the matrix are therefore
  ignored.

  Multiplication is done with the vector on the left side of the
  expression, i.e. dst = src * M.

  \sa multVecMatrix(), multMatrixVec() and multLineMatrix().
 */
void
SbMatrix::multDirMatrix(const SbVec3f & src, SbVec3f & dst) const
{
  // FIXME: should check if we're the identity matrix? 20010919 mortene.

  const float * t0 = (*this)[0];
  const float * t1 = (*this)[1];
  const float * t2 = (*this)[2];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3f s = src;

  dst[0] = s[0]*t0[0] + s[1]*t1[0] + s[2]*t2[0];
  dst[1] = s[0]*t0[1] + s[1]*t1[1] + s[2]*t2[1];
  dst[2] = s[0]*t0[2] + s[1]*t1[2] + s[2]*t2[2];
}

/*!
  Multiplies line point with the full matrix and multiplies the
  line direction with the matrix without the translation components.

  \sa multVecMatrix(), multMatrixVec() and multDirMatrix().
 */
void
SbMatrix::multLineMatrix(const SbLine & src, SbLine & dst) const
{
  // FIXME: should check if we're the identity matrix? 20010919 mortene.

  SbVec3f newpt, newdir;
  this->multVecMatrix(src.getPosition(), newpt);
  this->multDirMatrix(src.getDirection(), newdir);

  dst.setValue(newpt, newpt + newdir);
}

/*!
  Write out the matrix contents to the given file.
 */
void
SbMatrix::print(FILE * fp) const
{
  for (int i=0; i < 4; i++) {
    fprintf(fp, "%10.5g\t%10.5g\t%10.5g\t%10.5g\n",
            this->matrix[i][0], this->matrix[i][1],
            this->matrix[i][2], this->matrix[i][3]);
  }
}

/***********************************************************************
   below is the polar_decomp implementation by Ken Shoemake
   <shoemake@graphics.cis.upenn.edu>. It was part of the
   Graphics Gems IV archive.
************************************************************************/

// FIXME: should merge all the PD code we're using from GGIV into
// SbMatrix, SbRotation and SbVec3f proper (for two reasons: 1)
// there's a lot of duplicated code here (like for instance the
// matrix->quaternion decomposition, which also exists in
// SbRotation::setValue(SbMatrix&)), and 2) the remaining code
// snippets look generally useful outside the purpose of breaking down
// a matrix into it's transformation components). 20010114 mortene.


/**** Decompose.c ****/
/* Ken Shoemake, 1993 */

/******* Matrix Preliminaries *******/

/** Fill out 3x3 matrix to 4x4 **/
#define mat_pad(A) (A[W][X]=A[X][W]=A[W][Y]=A[Y][W]=A[W][Z]=A[Z][W]=0, A[W][W]=1)

/** Copy nxn matrix A to C using "gets" for assignment **/
#define mat_copy(C, gets, A, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    C[i][j] gets (A[i][j]);}

/** Copy transpose of nxn matrix A to C using "gets" for assignment **/
#define mat_tpose(AT, gets, A, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    AT[i][j] gets (A[j][i]);}

/** Assign nxn matrix C the element-wise combination of A and B using "op" **/
#define mat_binop(C, gets, A, op, B, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    C[i][j] gets (A[i][j]) op (B[i][j]);}

/** Multiply the upper left 3x3 parts of A and B to get AB **/
static void
mat_mult(HMatrix A, HMatrix B, HMatrix AB)
{
  int i, j;
  for (i=0; i<3; i++) for (j=0; j<3; j++)
    AB[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + A[i][2]*B[2][j];
}

/** Return dot product of length 3 vectors va and vb **/
static float
vdot(float * va, float * vb)
{
  return (va[0]*vb[0] + va[1]*vb[1] + va[2]*vb[2]);
}

/** Set v to cross product of length 3 vectors va and vb **/
static void
vcross(float * va, float * vb, float * v)
{
  v[0] = va[1]*vb[2] - va[2]*vb[1];
  v[1] = va[2]*vb[0] - va[0]*vb[2];
  v[2] = va[0]*vb[1] - va[1]*vb[0];
}

/** Set MadjT to transpose of inverse of M times determinant of M **/
static void
adjoint_transpose(HMatrix M, HMatrix MadjT)
{
  vcross(M[1], M[2], MadjT[0]);
  vcross(M[2], M[0], MadjT[1]);
  vcross(M[0], M[1], MadjT[2]);
}

/******* Decomp Auxiliaries *******/

static HMatrix mat_id = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

/** Compute either the 1 or infinity norm of M, depending on tpose **/
static float
mat_norm(HMatrix M, int tpose)
{
  int i;
  float sum, max;
  max = 0.0;
  for (i=0; i<3; i++) {
    if (tpose) sum = (float)(fabs(M[0][i])+fabs(M[1][i])+fabs(M[2][i]));
    else sum = (float)(fabs(M[i][0])+fabs(M[i][1])+fabs(M[i][2]));
    if (max<sum) max = sum;
  }
  return max;
}

static float norm_inf(HMatrix M) {return mat_norm(M, 0);}
static float norm_one(HMatrix M) {return mat_norm(M, 1);}

/** Return index of column of M containing maximum abs entry, or -1 if M=0 **/
static int
find_max_col(HMatrix M)
{
  float abs, max;
  int i, j, col;
  max = 0.0; col = -1;
  for (i=0; i<3; i++) for (j=0; j<3; j++) {
    abs = M[i][j]; if (abs<0.0) abs = -abs;
    if (abs>max) {max = abs; col = j;}
  }
    return col;
}

/** Setup u for Household reflection to zero all v components but first **/
static void
make_reflector(float * v, float * u)
{
  float s = (float)sqrt(vdot(v, v));
  u[0] = v[0]; u[1] = v[1];
  u[2] = v[2] + ((v[2]<0.0) ? -s : s);
  s = (float)sqrt(2.0/vdot(u, u));
  u[0] = u[0]*s; u[1] = u[1]*s; u[2] = u[2]*s;
}

/** Apply Householder reflection represented by u to column vectors of M **/
static void
reflect_cols(HMatrix M, float * u)
{
  int i, j;
  for (i=0; i<3; i++) {
    float s = u[0]*M[0][i] + u[1]*M[1][i] + u[2]*M[2][i];
    for (j=0; j<3; j++) M[j][i] -= u[j]*s;
  }
}
/** Apply Householder reflection represented by u to row vectors of M **/
static void
reflect_rows(HMatrix M, float * u)
{
  int i, j;
  for (i=0; i<3; i++) {
    float s = vdot(u, M[i]);
    for (j=0; j<3; j++) M[i][j] -= u[j]*s;
  }
}

/** Find orthogonal factor Q of rank 1 (or less) M **/
static void
do_rank1(HMatrix M, HMatrix Q)
{
  float v1[3], v2[3], s;
  int col;
  mat_copy(Q, =, mat_id, 4);
  /* If rank(M) is 1, we should find a non-zero column in M */
  col = find_max_col(M);
  if (col<0) return; /* Rank is 0 */
  v1[0] = M[0][col]; v1[1] = M[1][col]; v1[2] = M[2][col];
  make_reflector(v1, v1); reflect_cols(M, v1);
  v2[0] = M[2][0]; v2[1] = M[2][1]; v2[2] = M[2][2];
  make_reflector(v2, v2); reflect_rows(M, v2);
  s = M[2][2];
  if (s<0.0) Q[2][2] = -1.0;
  reflect_cols(Q, v1); reflect_rows(Q, v2);
}

/** Find orthogonal factor Q of rank 2 (or less) M using adjoint transpose **/
static void
do_rank2(HMatrix M, HMatrix MadjT, HMatrix Q)
{
  float v1[3], v2[3];
  float w, x, y, z, c, s, d;
  int col;
  /* If rank(M) is 2, we should find a non-zero column in MadjT */
  col = find_max_col(MadjT);
  if (col<0) {do_rank1(M, Q); return;} /* Rank<2 */
  v1[0] = MadjT[0][col]; v1[1] = MadjT[1][col]; v1[2] = MadjT[2][col];
  make_reflector(v1, v1); reflect_cols(M, v1);
  vcross(M[0], M[1], v2);
  make_reflector(v2, v2); reflect_rows(M, v2);
  w = M[0][0]; x = M[0][1]; y = M[1][0]; z = M[1][1];
  if (w*z>x*y) {
    c = z+w; s = y-x; d = (float)sqrt(c*c+s*s); c = c/d; s = s/d;
    Q[0][0] = Q[1][1] = c; Q[0][1] = -(Q[1][0] = s);
  }
  else {
    c = z-w; s = y+x; d = (float)sqrt(c*c+s*s); c = c/d; s = s/d;
    Q[0][0] = -(Q[1][1] = c); Q[0][1] = Q[1][0] = s;
  }
  Q[0][2] = Q[2][0] = Q[1][2] = Q[2][1] = 0.0; Q[2][2] = 1.0;
  reflect_cols(Q, v1); reflect_rows(Q, v2);
}


/******* Polar Decomposition *******/

/* Polar Decomposition of 3x3 matrix in 4x4,
 * M = QS.  See Nicholas Higham and Robert S. Schreiber,
 * Fast Polar Decomposition of An Arbitrary Matrix,
 * Technical Report 88-942, October 1988,
 * Department of Computer Science, Cornell University.
 */
static float
polar_decomp(HMatrix M, HMatrix Q, HMatrix S)
{
#define TOL 1.0e-6
  HMatrix Mk, MadjTk, Ek;
  float det, M_one, M_inf, MadjT_one, MadjT_inf, E_one, gamma, g1, g2;
  int i, j;
  mat_tpose(Mk, =, M, 3);
  M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
  do {
    adjoint_transpose(Mk, MadjTk);
    det = vdot(Mk[0], MadjTk[0]);
    if (det==0.0) {do_rank2(Mk, MadjTk, Mk); break;}
    MadjT_one = norm_one(MadjTk); MadjT_inf = norm_inf(MadjTk);
    gamma = (float)sqrt(sqrt((MadjT_one*MadjT_inf)/(M_one*M_inf))/fabs(det));
    g1 = gamma*0.5f;
    g2 = 0.5f/(gamma*det);
    mat_copy(Ek, =, Mk, 3);
    mat_binop(Mk, =, g1*Mk, +, g2*MadjTk, 3);
    mat_copy(Ek, -=, Mk, 3);
    E_one = norm_one(Ek);
    M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
  } while (E_one>(M_one*TOL));
  mat_tpose(Q, =, Mk, 3); mat_pad(Q);
  mat_mult(Mk, M, S);    mat_pad(S);
  for (i=0; i<3; i++) for (j=i; j<3; j++)
    S[i][j] = S[j][i] = 0.5f*(S[i][j]+S[j][i]);
  return (det);
}

/******* Spectral Decomposition *******/

/* Compute the spectral decomposition of symmetric positive semi-definite S.
 * Returns rotation in U and scale factors in result, so that if K is a diagonal
 * matrix of the scale factors, then S = U K (U transpose). Uses Jacobi method.
 * See Gene H. Golub and Charles F. Van Loan. Matrix Computations. Hopkins 1983.
 */
static SbVec4f
spect_decomp(HMatrix S, HMatrix U)
{
  SbVec4f kv;
  double Diag[3], OffD[3]; /* OffD is off-diag (by omitted index) */
  double g, h, fabsh, fabsOffDi, t, theta, c, s, tau, ta, OffDq, a, b;
  static char nxt[] = {Y, Z, X};
  int sweep, i, j;
  mat_copy(U, =, mat_id, 4);
  Diag[X] = S[X][X]; Diag[Y] = S[Y][Y]; Diag[Z] = S[Z][Z];
  OffD[X] = S[Y][Z]; OffD[Y] = S[Z][X]; OffD[Z] = S[X][Y];
  for (sweep=20; sweep>0; sweep--) {
    float sm = (float)(fabs(OffD[X])+fabs(OffD[Y])+fabs(OffD[Z]));
    if (sm==0.0) break;
    for (i=Z; i>=X; i--) {
      int p = nxt[i]; int q = nxt[p];
      fabsOffDi = fabs(OffD[i]);
      g = 100.0*fabsOffDi;
      if (fabsOffDi>0.0) {
        h = Diag[q] - Diag[p];
        fabsh = fabs(h);
        if (fabsh+g==fabsh) {
          t = OffD[i]/h;
        }
        else {
          theta = 0.5*h/OffD[i];
          t = 1.0/(fabs(theta)+sqrt(theta*theta+1.0));
          if (theta<0.0) t = -t;
        }
        c = 1.0/sqrt(t*t+1.0); s = t*c;
        tau = s/(c+1.0);
        ta = t*OffD[i]; OffD[i] = 0.0;
        Diag[p] -= ta; Diag[q] += ta;
        OffDq = OffD[q];
        OffD[q] -= s*(OffD[p] + tau*OffD[q]);
        OffD[p] += s*(OffDq   - tau*OffD[p]);
        for (j=Z; j>=X; j--) {
          a = U[j][p]; b = U[j][q];
          U[j][p] -= (float)(s*(b + tau*a));
          U[j][q] += (float)(s*(a - tau*b));
        }
      }
    }
  }
  kv[X] = (float)Diag[X]; kv[Y] = (float)Diag[Y]; kv[Z] = (float)Diag[Z];
  kv[W] = 1.0f;
  return (kv);
}

/******* Spectral Axis Adjustment *******/

/* Given a unit quaternion, q, and a scale vector, k, find a unit quaternion, p,
 * which permutes the axes and turns freely in the plane of duplicate scale
 * factors, such that q p has the largest possible w component, i.e. the
 * smallest possible angle. Permutes k's components to go with q p instead of q.
 * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
 * Proceedings of Graphics Interface 1992. Details on p. 262-263.
 */
static SbRotation
snuggle(SbRotation q, SbVec4f & k)
{
#define SQRTHALF (0.7071067811865475244f)
#define sgn(n, v)    ((n)?-(v):(v))
#define swap(a, i, j) {a[3]=a[i]; a[i]=a[j]; a[j]=a[3];}
#define cycle(a, p)  if (p) {a[3]=a[0]; a[0]=a[1]; a[1]=a[2]; a[2]=a[3];}\
                    else   {a[3]=a[2]; a[2]=a[1]; a[1]=a[0]; a[0]=a[3];}
  SbRotation p;
  float ka[4];
  int i, turn = -1;
  ka[X] = k[X]; ka[Y] = k[Y]; ka[Z] = k[Z];
  if (ka[X]==ka[Y]) {if (ka[X]==ka[Z]) turn = W; else turn = Z;}
  else {if (ka[X]==ka[Z]) turn = Y; else if (ka[Y]==ka[Z]) turn = X;}
  if (turn>=0) {
    SbRotation qtoz, qp;
    unsigned neg[3], win;
    double mag[3], t;
    static SbRotation qxtoz(0.0f, SQRTHALF, 0.0f, SQRTHALF);
    static SbRotation qytoz(SQRTHALF, 0.0f, 0.0f, SQRTHALF);
    static SbRotation qppmm(0.5f, 0.5f, -0.5f, -0.5f);
    static SbRotation qpppp(0.5f, 0.5f, 0.5f, 0.5f);
    static SbRotation qmpmm(-0.5f, 0.5f, -0.5f, -0.5f);
    static SbRotation qpppm(0.5f, 0.5f, 0.5f, -0.5f);
    static SbRotation q0001(0.0f, 0.0f, 0.0f, 1.0f);
    static SbRotation q1000(1.0f, 0.0f, 0.0f, 0.0f);
    switch (turn) {
    default: return SbRotation(q).invert();
    case X: q = (qtoz = qxtoz) * q; swap(ka, X, Z) break;
    case Y: q = (qtoz = qytoz) * q; swap(ka, Y, Z) break;
    case Z: qtoz = q0001; break;
    }
    q.invert();
    mag[0] = (double)q.getValue()[Z]*q.getValue()[Z]+(double)q.getValue()[W]*q.getValue()[W]-0.5;
    mag[1] = (double)q.getValue()[X]*q.getValue()[Z]-(double)q.getValue()[Y]*q.getValue()[W];
    mag[2] = (double)q.getValue()[Y]*q.getValue()[Z]+(double)q.getValue()[X]*q.getValue()[W];
    for (i=0; i<3; i++) if ((neg[i] = (mag[i] < 0.0))) mag[i] = -mag[i];
    if (mag[0]>mag[1]) {if (mag[0]>mag[2]) win = 0; else win = 2;}
    else {if (mag[1]>mag[2]) win = 1; else win = 2;}
    switch (win) {
    case 0: if (neg[0]) p = q1000; else p = q0001; break;
    case 1: if (neg[1]) p = qppmm; else p = qpppp; cycle(ka, 0) break;
    case 2: if (neg[2]) p = qmpmm; else p = qpppm; cycle(ka, 1) break;
    }
    qp = p * q;
    t = sqrt(mag[win]+0.5);
    p = SbRotation(0.0, 0.0, -qp.getValue()[Z]/(float)t, qp.getValue()[W]/(float)t) * p;
    p = SbRotation(p).invert() * qtoz;
  }
  else {
    float qa[4], pa[4];
    unsigned lo, hi, neg[4], par = 0;
    double all, big, two;
    qa[0] = q.getValue()[X]; qa[1] = q.getValue()[Y]; qa[2] = q.getValue()[Z]; qa[3] = q.getValue()[W];
    for (i=0; i<4; i++) {
      pa[i] = 0.0;
      if ((neg[i] = (qa[i]<0.0))) qa[i] = -qa[i];
      par ^= neg[i];
    }
    /* Find two largest components, indices in hi and lo */
    if (qa[0]>qa[1]) lo = 0; else lo = 1;
    if (qa[2]>qa[3]) hi = 2; else hi = 3;
    if (qa[lo]>qa[hi]) {
      if (qa[lo^1]>qa[hi]) {hi = lo; lo ^= 1;}
      else {hi ^= lo; lo ^= hi; hi ^= lo;}
    } else {if (qa[hi^1]>qa[lo]) lo = hi^1;}
    all = (qa[0]+qa[1]+qa[2]+qa[3])*0.5;
    two = (qa[hi]+qa[lo])*SQRTHALF;
    big = qa[hi];
    if (all>two) {
      if (all>big) {/*all*/
        {int i; for (i=0; i<4; i++) pa[i] = sgn(neg[i], 0.5f);}
        cycle(ka, par)
          } else {/*big*/ pa[hi] = sgn(neg[hi], 1.0f);}
    }
    else {
      if (two>big) {/*two*/
        pa[hi] = sgn(neg[hi], SQRTHALF); pa[lo] = sgn(neg[lo], SQRTHALF);
        if (lo>hi) {hi ^= lo; lo ^= hi; hi ^= lo;}
        if (hi==W) {hi = "\001\002\000"[lo]; lo = 3-hi-lo;}
        swap(ka, hi, lo)
          } else {/*big*/ pa[hi] = sgn(neg[hi], 1.0f);}
    }
    // FIXME: p = conjugate(pa)? 20010114 mortene.
    p.setValue(-pa[0], -pa[1], -pa[2], pa[3]);
  }
  k[X] = ka[X]; k[Y] = ka[Y]; k[Z] = ka[Z];
  return (p);
}

/******* Decompose Affine Matrix *******/

/* Decompose 4x4 affine matrix A as TFRUK(U transpose), where t contains the
 * translation components, q contains the rotation R, u contains U, k contains
 * scale factors, and f contains the sign of the determinant.
 * Assumes A transforms column vectors in right-handed coordinates.
 * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
 * Proceedings of Graphics Interface 1992.
 */
static void
decomp_affine(HMatrix A, AffineParts * parts)
{
  HMatrix Q, S, U;
  SbRotation p;
  parts->t = SbVec4f(A[X][W], A[Y][W], A[Z][W], 0);
  float det = polar_decomp(A, Q, S);
  if (det<0.0) {
    mat_copy(Q, =, -Q, 3);
    parts->f = -1;
  }
  else parts->f = 1;

  // Transpose for our code (we use OpenGL's convention for numbering
  // rows and columns).
  SbMatrix TQ((const SbMat *) &Q);
  parts->q = SbRotation(TQ.transpose());
  parts->k = spect_decomp(S, U);
  // Transpose for our code (we use OpenGL's convention for numbering
  // rows and columns).
  SbMatrix TU((const SbMat *) &U);
  parts->u = SbRotation(TU.transpose());
  p = snuggle(parts->u, parts->k);
  parts->u = p * parts->u;
}

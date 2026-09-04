#include "rw/rwengine.h"
#include "rw/rwerror.h"
#include "rw/rwfreelist.h"
#include "rw/rwmatrix.h"
#include "rw/rwvector.h"
#include "runtime/cmath.h"

typedef void (*rwMatrixMultFn)(RwMatrix *, const RwMatrix *, const RwMatrix *);

typedef struct rwMatrixGlobals {
  RwFreeList *matrixFreeList;
  int matrixOptimizations;
  rwMatrixMultFn multMatrix;
  RwMatrixTolerance tolerance;
} rwMatrixGlobals;

typedef char rwMatrixGlobalsSizeCheck[
    sizeof(rwMatrixGlobals) == 0x18 ? 1 : -1];

static RwModuleInfo matrixModule;

static void MatrixMultiply(RwMatrix *dst, const RwMatrix *left,
                           const RwMatrix *right) {
  dst->right.x = left->right.x * right->right.x + left->right.y * right->up.x +
                 left->right.z * right->at.x;
  dst->right.y = left->right.x * right->right.y + left->right.y * right->up.y +
                 left->right.z * right->at.y;
  dst->right.z = left->right.x * right->right.z + left->right.y * right->up.z +
                 left->right.z * right->at.z;
  dst->up.x = left->up.x * right->right.x + left->up.y * right->up.x +
              left->up.z * right->at.x;
  dst->up.y = left->up.x * right->right.y + left->up.y * right->up.y +
              left->up.z * right->at.y;
  dst->up.z = left->up.x * right->right.z + left->up.y * right->up.z +
              left->up.z * right->at.z;
  dst->at.x = left->at.x * right->right.x + left->at.y * right->up.x +
              left->at.z * right->at.x;
  dst->at.y = left->at.x * right->right.y + left->at.y * right->up.y +
              left->at.z * right->at.y;
  dst->at.z = left->at.x * right->right.z + left->at.y * right->up.z +
              left->at.z * right->at.z;
  dst->pos.x = right->pos.x + left->pos.x * right->right.x +
               left->pos.y * right->up.x + left->pos.z * right->at.x;
  dst->pos.y = right->pos.y + left->pos.x * right->right.y +
               left->pos.y * right->up.y + left->pos.z * right->at.y;
  dst->pos.z = right->pos.z + left->pos.x * right->right.z +
               left->pos.y * right->up.z + left->pos.z * right->at.z;
}

static RwMatrix *MatrixOrthoNormalize(RwMatrix *dst, const RwMatrix *src) {
  RwV3d right = src->right;
  RwV3d up = src->up;
  RwV3d at = src->at;
  RwV3d pos = src->pos;
  RwV3d inverseLengths;
  RwV3d alignment;
  RwV3d *primary;
  RwV3d *secondary;
  RwV3d *rebuilt;
  float inverseLength;
  float lengthSquared;
  float crossX;
  float crossY;
  float crossZ;

  lengthSquared = right.x * right.x + right.y * right.y + right.z * right.z;
  inverseLengths.x = _rwInvSqrt(lengthSquared);
  right.x *= inverseLengths.x;
  right.y *= inverseLengths.x;
  right.z *= inverseLengths.x;
  lengthSquared = up.x * up.x + up.y * up.y + up.z * up.z;
  inverseLengths.y = _rwInvSqrt(lengthSquared);
  up.x *= inverseLengths.y;
  up.y *= inverseLengths.y;
  up.z *= inverseLengths.y;
  lengthSquared = at.x * at.x + at.y * at.y + at.z * at.z;
  inverseLengths.z = _rwInvSqrt(lengthSquared);
  at.x *= inverseLengths.z;
  at.y *= inverseLengths.z;
  at.z *= inverseLengths.z;
  if (inverseLengths.x > 0.0f) {
    if (inverseLengths.y > 0.0f) {
      if (inverseLengths.z > 0.0f) {
        alignment.x = up.x * at.x + up.y * at.y + up.z * at.z;
        alignment.y = at.x * right.x + at.y * right.y + at.z * right.z;
        alignment.z = right.x * up.x + right.y * up.y + right.z * up.z;
        if (alignment.x < 0.0f)
          alignment.x = -alignment.x;
        if (alignment.y < 0.0f)
          alignment.y = -alignment.y;
        if (alignment.z < 0.0f)
          alignment.z = -alignment.z;
        if (alignment.x < alignment.y) {
          if (alignment.x < alignment.z) {
            primary = &up;
            secondary = &at;
            rebuilt = &right;
          } else {
            primary = &right;
            secondary = &up;
            rebuilt = &at;
          }
        } else if (alignment.y < alignment.z) {
          primary = &at;
          secondary = &right;
          rebuilt = &up;
        } else {
          primary = &right;
          secondary = &up;
          rebuilt = &at;
        }
      } else {
        primary = &right;
        secondary = &up;
        rebuilt = &at;
      }
    } else {
      primary = &at;
      secondary = &right;
      rebuilt = &up;
    }
  } else {
    primary = &up;
    secondary = &at;
    rebuilt = &right;
  }
  crossX = primary->y * secondary->z - primary->z * secondary->y;
  crossY = primary->z * secondary->x - primary->x * secondary->z;
  crossZ = primary->x * secondary->y - primary->y * secondary->x;
  rebuilt->x = crossX;
  rebuilt->y = crossY;
  rebuilt->z = crossZ;
  lengthSquared = crossX * crossX + crossY * crossY + crossZ * crossZ;
  inverseLength = _rwInvSqrt(lengthSquared);
  rebuilt->x *= inverseLength;
  rebuilt->y *= inverseLength;
  rebuilt->z *= inverseLength;

  crossX = rebuilt->y * primary->z - rebuilt->z * primary->y;
  crossY = rebuilt->z * primary->x - rebuilt->x * primary->z;
  crossZ = rebuilt->x * primary->y - rebuilt->y * primary->x;
  secondary->x = crossX;
  secondary->y = crossY;
  secondary->z = crossZ;
  lengthSquared = crossX * crossX + crossY * crossY + crossZ * crossZ;
  inverseLength = _rwInvSqrt(lengthSquared);
  secondary->x *= inverseLength;
  secondary->y *= inverseLength;
  secondary->z *= inverseLength;
  dst->right = right;
  dst->up = up;
  dst->at = at;
  dst->pos = pos;
  dst->flags = (dst->flags | 3) & ~0x20000;
  return dst;
}

static RwMatrix *MatrixInvertOrthoNormalized(RwMatrix *dst,
                                             const RwMatrix *src) {
  dst->right.x = src->right.x;
  dst->right.y = src->up.x;
  dst->right.z = src->at.x;
  dst->up.x = src->right.y;
  dst->up.y = src->up.y;
  dst->up.z = src->at.y;
  dst->at.x = src->right.z;
  dst->at.y = src->up.z;
  dst->at.z = src->at.z;
  dst->pos.x = -(src->pos.x * dst->right.x + src->pos.y * dst->up.x +
                 src->pos.z * dst->at.x);
  dst->pos.y = -(src->pos.x * dst->right.y + src->pos.y * dst->up.y +
                 src->pos.z * dst->at.y);
  dst->pos.z = -(src->pos.x * dst->right.z + src->pos.y * dst->up.z +
                 src->pos.z * dst->at.z);
  dst->flags = 3;
  return dst;
}

static RwMatrix *MatrixInvertGeneric(RwMatrix *dst, const RwMatrix *src) {
  RwSplitBits determinant;
  float inverseDeterminant;

  dst->right.x = src->up.y * src->at.z - src->up.z * src->at.y;
  dst->right.y = -(src->right.y * src->at.z - src->right.z * src->at.y);
  dst->right.z = src->right.y * src->up.z - src->right.z * src->up.y;
  determinant.nReal = dst->right.x * src->right.x + dst->right.y * src->up.x +
                      dst->right.z * src->at.x;
  inverseDeterminant = determinant.nInt != 0 ? 1.0f / determinant.nReal : 1.0f;
  dst->right.x *= inverseDeterminant;
  dst->right.y *= inverseDeterminant;
  dst->right.z *= inverseDeterminant;
  dst->up.x =
      -(src->up.x * src->at.z - src->up.z * src->at.x) * inverseDeterminant;
  dst->up.y = (src->right.x * src->at.z - src->right.z * src->at.x) *
              inverseDeterminant;
  dst->up.z = -(src->right.x * src->up.z - src->right.z * src->up.x) *
              inverseDeterminant;
  dst->at.x =
      (src->up.x * src->at.y - src->up.y * src->at.x) * inverseDeterminant;
  dst->at.y = -(src->right.x * src->at.y - src->right.y * src->at.x) *
              inverseDeterminant;
  dst->at.z = (src->right.x * src->up.y - src->right.y * src->up.x) *
              inverseDeterminant;
  dst->pos.x = -(src->pos.x * dst->right.x + src->pos.y * dst->up.x +
                 src->pos.z * dst->at.x);
  dst->pos.y = -(src->pos.x * dst->right.y + src->pos.y * dst->up.y +
                 src->pos.z * dst->at.y);
  dst->pos.z = -(src->pos.x * dst->right.z + src->pos.y * dst->up.z +
                 src->pos.z * dst->at.z);
  dst->flags = 0;
  return dst;
}

int _rwMatrixSetMultFn(rwMatrixMultFn multiply) {
  if (multiply == 0) {
    multiply = MatrixMultiply;
  }
  *(rwMatrixMultFn *)((unsigned char *)RwEngineInstance +
                      matrixModule.globalsOffset + 8) = multiply;
  return 1;
}

int _rwMatrixSetOptimizations(int optimizeFlags) {
  rwMatrixGlobals *globals = (rwMatrixGlobals *)((unsigned char *)RwEngineInstance +
                                                 matrixModule.globalsOffset);
  globals->matrixOptimizations = optimizeFlags;
  return 1;
}

#pragma optimization_level 0
float _rwMatrixDeterminant(const RwMatrix *matrix) {
  const RwV3d *right = &matrix->right;
  const RwV3d *up = &matrix->up;
  const RwV3d *at = &matrix->at;
  RwV3d cross;
  float determinant;
  cross.x = up->y * at->z - up->z * at->y;
  cross.y = up->z * at->x - up->x * at->z;
  cross.z = up->x * at->y - up->y * at->x;
  determinant =
      cross.x * right->x + cross.y * right->y + cross.z * right->z;
  return determinant;
}
#pragma optimization_level 4

#pragma optimization_level 0
float _rwMatrixOrthogonalError(const RwMatrix *matrix) {
  const RwV3d *right = &matrix->right;
  const RwV3d *up = &matrix->up;
  const RwV3d *at = &matrix->at;
  RwV3d dot;
  float error;
  dot.x = up->x * at->x + up->y * at->y + up->z * at->z;
  dot.y = at->x * right->x + at->y * right->y + at->z * right->z;
  dot.z = right->x * up->x + right->y * up->y + right->z * up->z;
  error = dot.x * dot.x + dot.y * dot.y + dot.z * dot.z;
  return error;
}
#pragma optimization_level 4

#define rwMatrixVectorDot(left, right)                                      \
  ((left)->z * (right)->z +                                                 \
   ((left)->x * (right)->x + (left)->y * (right)->y))

#pragma optimization_level 0
float _rwMatrixNormalError(const RwMatrix *matrix) {
  const RwV3d *right = &matrix->right;
  const RwV3d *up = &matrix->up;
  const RwV3d *at = &matrix->at;
  RwV3d error;
  float totalError;
  error.x = rwMatrixVectorDot(right, right) - 1.0f;
  error.y = rwMatrixVectorDot(up, up) - 1.0f;
  error.z = rwMatrixVectorDot(at, at) - 1.0f;
  totalError = rwMatrixVectorDot(&error, &error);
  return totalError;
}
#pragma optimization_level 4

#pragma optimization_level 0
float _rwMatrixIdentityError(const RwMatrix *matrix) {
  const RwV3d *right = &matrix->right;
  const RwV3d *up = &matrix->up;
  const RwV3d *at = &matrix->at;
  const RwV3d *pos = &matrix->pos;
  float rightX = right->x - 1.0f;
  float upY = up->y - 1.0f;
  float atZ = at->z - 1.0f;
  float totalError =
      pos->z * pos->z + (pos->x * pos->x + pos->y * pos->y) +
      (atZ * atZ + (at->x * at->x + at->y * at->y)) +
      ((right->z * right->z +
        (rightX * rightX + right->y * right->y)) +
       (up->z * up->z + (up->x * up->x + upY * upY)));
  return totalError;
}

/* Releases the matrix free list and closes the matrix module instance. */
void *_rwMatrixClose(void *instance, int offset, int size) {
  if (*(RwFreeList **)((unsigned char *)RwEngineInstance +
                       matrixModule.globalsOffset) != 0) {
    RwFreeListDestroy(*(RwFreeList **)((unsigned char *)RwEngineInstance +
                                       matrixModule.globalsOffset));
    *(RwFreeList **)((unsigned char *)RwEngineInstance +
                     matrixModule.globalsOffset) = 0;
  }
  matrixModule.numInstances--;
  return instance;
}
#pragma optimization_level 4

static int _rwMatrixFreeListBlockSize = 256,
               _rwMatrixFreeListPreallocBlocks = 1;
static RwFreeList _rwMatrixFreeList;

void *_rwMatrixOpen(void *instance, int offset, int size) {
  rwMatrixGlobals *globals;

  matrixModule.globalsOffset = offset;
  globals = (rwMatrixGlobals *)((unsigned char *)RwEngineInstance + offset);
  globals->matrixFreeList = RwFreeListCreateAndPreallocateSpace(
      sizeof(RwMatrix), _rwMatrixFreeListBlockSize, 4,
      _rwMatrixFreeListPreallocBlocks, &_rwMatrixFreeList, 0x40000 | 0x0D);
  if (globals->matrixFreeList == 0) {
    instance = 0;
  } else {
    const RwMatrixTolerance tolerance = {((float)0.01), ((float)0.01),
                                         ((float)0.01)};
    _rwMatrixSetOptimizations(0x20000);
    _rwMatrixSetMultFn(0);
    RwEngineSetMatrixTolerances(&tolerance);
    matrixModule.numInstances++;
  }
  return instance;
}

#pragma optimization_level 0
int RwEngineSetMatrixTolerances(const RwMatrixTolerance *const tolerance) {
  RwMatrixTolerance *globalTolerance = (RwMatrixTolerance *)(
      (unsigned char *)RwEngineInstance + matrixModule.globalsOffset + 0xC);
  *globalTolerance = *tolerance;
  return 1;
}
#pragma optimization_level 4

RwMatrix *RwMatrixOptimize(RwMatrix *matrix,
                           const RwMatrixTolerance *tolerance) {
  rwMatrixGlobals *globals = (rwMatrixGlobals *)((unsigned char *)RwEngineInstance +
                                                 matrixModule.globalsOffset);
  unsigned int flags;
  int isNormal;
  int isOrthogonal;
  int isIdentity;

  if (tolerance == 0) {
    tolerance = &globals->tolerance;
  }
  isNormal = tolerance->Normal >= _rwMatrixNormalError(matrix);
  isOrthogonal = tolerance->Orthogonal >= _rwMatrixOrthogonalError(matrix);
  isIdentity = isNormal && isOrthogonal &&
               tolerance->Identity >= _rwMatrixIdentityError(matrix);
  flags = matrix->flags;
  if (isNormal)
    flags |= 1;
  else
    flags &= ~1;
  if (isOrthogonal)
    flags |= 2;
  else
    flags &= ~2;
  if (isIdentity)
    flags |= 0x20000;
  else
    flags &= ~0x20000;
  matrix->flags = flags;
  return matrix;
}

/* Clears cached matrix classifications after callers modify its elements. */
void RwMatrixUpdate(RwMatrix *matrix) {
  matrix->flags &= ~(3 | 0x20000);
}

RwMatrix *RwMatrixMultiply(RwMatrix *dst, const RwMatrix *src1,
                           const RwMatrix *src2) {
  MatrixMultiply(dst, src1, src2);
  dst->flags = src1->flags & src2->flags;
  return dst;
}

#pragma optimization_level 0
RwMatrix *RwMatrixOrthoNormalize(RwMatrix *dst, const RwMatrix *src) {
  return MatrixOrthoNormalize(dst, src);
}
#pragma optimization_level 4

RwMatrix *RwMatrixRotateOneMinusCosineSine(RwMatrix *matrix,
                                           const RwV3d *unitAxis,
                                           float oneMinusCosine, float sine,
                                           RwOpCombineType combineOp) {
  RwMatrix rotation;
  RwMatrix result;
  float xx = 1.0f - unitAxis->x * unitAxis->x;
  float yy = 1.0f - unitAxis->y * unitAxis->y;
  float zz = 1.0f - unitAxis->z * unitAxis->z;
  float xy;
  float yz;
  float zx;
  float xSine;
  float ySine;
  float zSine;

  xx *= oneMinusCosine;
  yy *= oneMinusCosine;
  zz *= oneMinusCosine;
  xy = unitAxis->x * unitAxis->y;
  yz = unitAxis->y * unitAxis->z;
  zx = unitAxis->z * unitAxis->x;
  xy *= oneMinusCosine;
  yz *= oneMinusCosine;
  zx *= oneMinusCosine;
  xSine = unitAxis->x * sine;
  ySine = unitAxis->y * sine;
  zSine = unitAxis->z * sine;

  rotation.right.x = 1.0f - xx;
  rotation.right.y = xy + zSine;
  rotation.right.z = zx - ySine;
  rotation.up.x = xy - zSine;
  rotation.up.y = 1.0f - yy;
  rotation.up.z = yz + xSine;
  rotation.at.x = zx + ySine;
  rotation.at.y = yz - xSine;
  rotation.at.z = 1.0f - zz;
  rotation.pos.x = 0.0f;
  rotation.pos.y = 0.0f;
  rotation.pos.z = 0.0f;
  rotation.flags = 3;

  switch (combineOp) {
  case 0:
    *matrix = rotation;
    break;
  case 1:
    RwMatrixMultiply(&result, &rotation, matrix);
    *matrix = result;
    break;
  case 2:
    RwMatrixMultiply(&result, matrix, &rotation);
    *matrix = result;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000003, "Invalid combination type");
    RwErrorSet(&error);
    matrix = 0;
    break;
  }
  }
  return matrix;
}

RwMatrix *RwMatrixRotate(RwMatrix *matrix, const RwV3d *axis, float angle,
                         RwOpCombineType combineOp) {
  RwV3d unitAxis;
  float radians = angle * (float)(3.14159265358979323846 / 180.0);
  float inverseLength =
      _rwInvSqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
  float sinVal;
  float oneMinusCosVal;

  unitAxis.x = axis->x * inverseLength;
  unitAxis.y = axis->y * inverseLength;
  unitAxis.z = axis->z * inverseLength;
  sinVal = sinf(radians);
  oneMinusCosVal = 1.0f - cosf(radians);
  RwMatrixRotateOneMinusCosineSine(matrix, &unitAxis, oneMinusCosVal, sinVal,
                                   combineOp);
  return matrix;
}

RwMatrix *RwMatrixInvert(RwMatrix *dst, const RwMatrix *src) {
  rwMatrixGlobals *globals = (rwMatrixGlobals *)((unsigned char *)RwEngineInstance +
                                                 matrixModule.globalsOffset);
  if (src->flags & (globals->matrixOptimizations & 0x20000)) {
    *dst = *src;
  } else if ((src->flags & 3) == 3) {
    MatrixInvertOrthoNormalized(dst, src);
  } else {
    MatrixInvertGeneric(dst, src);
  }
  return dst;
}

RwMatrix *RwMatrixScale(RwMatrix *matrix, const RwV3d *scale,
                        RwOpCombineType combineOp) {
  switch (combineOp) {
  case 0:
    matrix->right.x = 1.0f;
    matrix->right.y = 0.0f;
    matrix->right.z = 0.0f;
    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->up.z = 0.0f;
    matrix->at.x = 0.0f;
    matrix->at.y = 0.0f;
    matrix->at.z = 1.0f;
    matrix->pos.x = 0.0f;
    matrix->pos.y = 0.0f;
    matrix->pos.z = 0.0f;
    matrix->flags |= 0x20003;
    matrix->right.x = scale->x;
    matrix->up.y = scale->y;
    matrix->at.z = scale->z;
    break;
  case 1:
    matrix->right.x *= scale->x;
    matrix->right.y *= scale->x;
    matrix->right.z *= scale->x;
    matrix->up.x *= scale->y;
    matrix->up.y *= scale->y;
    matrix->up.z *= scale->y;
    matrix->at.x *= scale->z;
    matrix->at.y *= scale->z;
    matrix->at.z *= scale->z;
    break;
  case 2:
    matrix->right.x *= scale->x;
    matrix->right.y *= scale->y;
    matrix->right.z *= scale->z;
    matrix->up.x *= scale->x;
    matrix->up.y *= scale->y;
    matrix->up.z *= scale->z;
    matrix->at.x *= scale->x;
    matrix->at.y *= scale->y;
    matrix->at.z *= scale->z;
    matrix->pos.x *= scale->x;
    matrix->pos.y *= scale->y;
    matrix->pos.z *= scale->z;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000003, "Invalid combination type");
    RwErrorSet(&error);
    matrix = 0;
    break;
  }
  }
  matrix->flags &= ~(0x20000 | 3);
  return matrix;
}

RwMatrix *RwMatrixTranslate(RwMatrix *matrix, const RwV3d *translation,
                            RwOpCombineType combineOp) {
  switch (combineOp) {
  case 0:
    matrix->right.x = 1.0f;
    matrix->right.y = 0.0f;
    matrix->right.z = 0.0f;
    matrix->up.x = 0.0f;
    matrix->up.y = 1.0f;
    matrix->up.z = 0.0f;
    matrix->at.x = 0.0f;
    matrix->at.y = 0.0f;
    matrix->at.z = 1.0f;
    matrix->pos.x = 0.0f;
    matrix->pos.y = 0.0f;
    matrix->pos.z = 0.0f;
    matrix->flags |= 0x20003;
    matrix->pos.x = translation->x;
    matrix->pos.y = translation->y;
    matrix->pos.z = translation->z;
    break;
  case 1:
    matrix->pos.x += translation->z * matrix->at.x +
                     translation->x * matrix->right.x +
                     translation->y * matrix->up.x;
    matrix->pos.y += translation->z * matrix->at.y +
                     translation->x * matrix->right.y +
                     translation->y * matrix->up.y;
    matrix->pos.z += translation->z * matrix->at.z +
                     translation->x * matrix->right.z +
                     translation->y * matrix->up.z;
    break;
  case 2:
    matrix->pos.x += translation->x;
    matrix->pos.y += translation->y;
    matrix->pos.z += translation->z;
    break;
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000003, "Invalid combination type");
    RwErrorSet(&error);
    matrix = 0;
    break;
  }
  }
  matrix->flags &= ~0x20000;
  return matrix;
}

RwMatrix *RwMatrixTransform(RwMatrix *matrix, const RwMatrix *transform,
                            RwOpCombineType combineOp) {
  switch (combineOp) {
  case 0:
    *matrix = *transform;
    break;
  case 1: {
    RwMatrix result;
    RwMatrixMultiply(&result, transform, matrix);
    *matrix = result;
    break;
  }
  case 2: {
    RwMatrix result;
    RwMatrixMultiply(&result, matrix, transform);
    *matrix = result;
    break;
  }
  default: {
    RwError error;
    error.pluginID = 1;
    error.errorCode = _rwerror(0x80000003, "Invalid combination type");
    RwErrorSet(&error);
    matrix = 0;
    break;
  }
  }
  return matrix;
}

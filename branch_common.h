// Common definitions to be used between the runtime and the asset processor

#include <stdint.h>
#include <math.h>

#define PI 3.14159265358979323846f

#define clamp(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define abs(x) (((x) >= 0) ? (x) : -(x))

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) < (b)) ? (b) : (a))

#define KILOBYTES(count) (count * 1024ull)
#define MEGABYTES(count) (count * KILOBYTES(1024ull))
#define GIGABYTES(count) (count * MEGABYTES(1024ull))

#if !defined(B_INLINE)
#define B_INLINE inline
#endif

#define SWAP(type, a, b) type temp = a;		\
   a = b;					\
   b = temp					\

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t b8;
typedef int32_t b32;

static u32 SCREEN_WIDTH = 0;
static u32 SCREEN_HEIGHT = 0;

// For us, floats with a difference
// of < 0.00001 are the same numbers
#define F_EPSILON 0.00001f

// typedef size_t size;
union m2
{
   float e[4];
   float e2[2][2];

   struct
   {
      float a, b,
	 c, d;
   };

   m2 operator*(m2 other)
   {
      m2 result;
      result.a = a * other.a + c * other.b;
      result.c = a * other.c + c * other.d;
      result.b = b * other.a + d * other.b;
      result.d = b * other.c + d * other.d;
      return result;
   }
};

m2 Rotation(float rads)
{
   m2 result;
   result.a = cosf(rads);
   result.b = -sinf(rads);
   result.c = sinf(rads);
   result.d = cosf(rads);
   return result;
}

m2 Scale(float x, float y)
{
   m2 result;
   result.a = x;
   result.b = 0.0f;
   result.c = 0.0f;
   result.d = y;
   return result;
}

union v3
{
   float e[3];

   struct
   {
      float x, y, z;
   };

   inline v3 operator-(v3 b);
   inline v3 operator*(float b);
   inline v3 operator/(float b);
   inline v3 operator+(v3 b);
};

inline
v3 v3::operator/(float b)
{
   return {x / b, y / b, z / b};
}

inline
v3 v3::operator+(v3 b)
{
   return {x + b.x, y + b.y, z + b.z};
}

inline
v3 v3::operator*(float b)
{
   return {b * x, b * y, b * z};
}

static inline
v3 V3(float x, float y, float z)
{
   return {x, y, z};
}

inline
v3 v3::operator-(v3 b)
{
   return {x - b.x, y - b.y, z - b.z};
}

static inline
float dot(v3 a, v3 b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline
v3 operator*(float c, v3 v)
{
   return V3(c * v.x, c * v.y, c * v.z);
}

static inline bool
Approx(v3 a, v3 b)
{
   float x = abs(a.x - b.x);
   float y = abs(a.y - b.y);
   float z = abs(a.z - b.z);

   return x < F_EPSILON && y < F_EPSILON && z < F_EPSILON;
}


union v3i
{
   i32 e[3];

   struct
   {
      i32 x, y, z;
   };
};

union m3
{
   float e[9];
   float e2[3][3];

   m3 operator*(m3 other)
   {
      m3 result;

      result.e2[0][0] = e2[0][0] * other.e2[0][0] + e2[1][0] * other.e2[0][1] + e2[2][0] * other.e2[0][2];
      result.e2[1][0] = e2[0][0] * other.e2[1][0] + e2[1][0] * other.e2[1][1] + e2[2][0] * other.e2[1][2];
      result.e2[2][0] = e2[0][0] * other.e2[2][0] + e2[1][0] * other.e2[2][1] + e2[2][0] * other.e2[2][2];

      result.e2[0][1] = e2[0][1] * other.e2[0][0] + e2[1][1] * other.e2[0][1] + e2[2][1] * other.e2[0][2];
      result.e2[1][1] = e2[0][1] * other.e2[1][0] + e2[1][1] * other.e2[1][1] + e2[2][1] * other.e2[1][2];
      result.e2[2][1] = e2[0][1] * other.e2[2][0] + e2[1][1] * other.e2[2][1] + e2[2][1] * other.e2[2][2];

      result.e2[0][2] = e2[0][2] * other.e2[0][0] + e2[1][2] * other.e2[0][1] + e2[2][2] * other.e2[0][2];
      result.e2[1][2] = e2[0][2] * other.e2[1][0] + e2[1][2] * other.e2[1][1] + e2[2][2] * other.e2[1][2];
      result.e2[2][2] = e2[0][2] * other.e2[2][0] + e2[1][2] * other.e2[2][1] + e2[2][2] * other.e2[2][2];

      return result;
   }

   v3 operator*(v3 other)
   {
      v3 result;

      result.e[0] = e2[0][0] * other.e[0] + e2[1][0] * other.e[1] + e2[2][0] * other.e[2];
      result.e[1] = e2[0][1] * other.e[0] + e2[1][1] * other.e[1] + e2[2][1] * other.e[2];
      result.e[2] = e2[0][2] * other.e[0] + e2[1][2] * other.e[1] + e2[2][2] * other.e[2];

      return result;
   }
};

static inline
m3 Scale(float x, float y, float z)
{
   return {x, 0.0f, 0.0f,
	 0.0f, y, 0.0f,
	 0.0f, 0.0f, z};
}

static inline
m3 Translate(float x, float y)
{
   return {1.0f, 0.0f, 0.0f,
	 0.0f, 1.0f, 0.0f,
	 x, y, 1.0f};
}

union v2
{
   struct
   {
      float x, y;
   };

   float e[2];

   inline
   v2 operator-(v2 other)
   {
      v2 result;
      result.x = x - other.x;
      result.y = y - other.y;
      return result;
   }

   inline
   v2 operator+(v2 other)
   {
      v2 result;
      result.x = x + other.x;
      result.y = y + other.y;
      return result;
   }

   inline
   v2 operator*(float c)
   {
      return {c * x, c * y};
   }
};

static inline
v2 V2(float x, float y)
{
   v2 result;
   result.x = x;
   result.y = y;
   return result;
}

static inline
v2 operator*(float c, v2 v)
{
   return V2(c * v.x, c * v.y);
}

union v2i
{
   struct
   {
      i32 x, y;
   };

   i32 e[2];
};

union tri
{
   struct
   {
      v3 a, b, c;
   };

   v3 e[3];
};

static inline
tri Tri(v3 a, v3 b, v3 c)
{
   tri result;
   result.a = a;
   result.b = b;
   result.c = c;
   return result;
}

static inline
float dot(v2 a, v2 b)
{
   return a.x * b.x + a.y * b.y;
}

static inline
float length(v2 a, v2 b)
{
   v2 c = a - b;
   return sqrtf(c.x * c.x + c.y * c.y);
}

static inline
float magnitude(v2 a)
{
   return sqrtf(a.x * a.x + a.y * a.y);
}

static inline
float magnitude(v3 a)
{
   return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

static inline
v2 unit(v2 a)
{
   float mag = magnitude(a);

   return V2(a.x / mag, a.y / mag);
}

static inline
v3 unit(v3 a)
{
   float mag = magnitude(a);

   return V3(a.x / mag, a.y / mag, a.z / mag);
}

static inline
v3 cross(v3 a, v3 b)
{
   v3 result;

   result.x = a.y * b.z - a.z * b.y;
   result.y = a.z * b.x - a.x * b.z;
   result.z = a.x * b.y - a.y * b.x;

   return result;
}

union v4
{
   struct
   {
      float x, y, z, w;
   };
   float e[4];

   v3 xyz;

   inline v4 operator+(float c);
   inline v4 operator+(v4 b);
   inline v4 operator-(v4 b);
};

inline
v4 v4::operator-(v4 b)
{
   v4 result;

   result.x = x - b.x;
   result.y = y - b.y;
   result.z = z - b.z;
   result.w = w - b.w;

   return result;
}

inline
v4 v4::operator+(float c)
{
   v4 result;

   result.x = c + x;
   result.y = c + y;
   result.z = c + z;
   result.w = c + w;

   return result;
}

inline
v4 v4::operator+(v4 b)
{
   v4 result;

   result.x = x + b.x;
   result.y = y + b.y;
   result.z = z + b.z;
   result.w = w + b.w;

   return result;
}

static inline
v4 V4(float x, float y, float z, float w)
{
   return {x, y, z, w};
}

static inline
float dot(v4 a, v4 b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline
float magnitude(v4 vec)
{
   return dot(vec, vec);
}

static inline
v4 unit(v4 vec)
{
   float mag = magnitude(vec);
   if(mag == 0.0f)
   {
      return V4(0.0f, 0.0f, 0.0f, 0.0f);
   }

   return V4(vec.x / mag, vec.y / mag, vec.z / mag, vec.w / mag);
}

union quat
{
   struct
   {
      float x, y, z, w;
   };

   v4 V4;

   float e[4];

   B_INLINE quat operator*(quat other);
};

inline static
quat Quat(float x, float y, float z, float w)
{
   return {x, y, z, w};
}

B_INLINE
quat quat::operator*(quat other)
{
   return Quat(w * other.x + x * other.w + y * other.z - z * other.y,
	       w * other.y + y * other.w + z * other.x - x * other.z,
	       w * other.z + z * other.w + x * other.y - y * other.x,
	       w * other.w - x * other.x - y * other.y - z * other.z);	       
}

inline static
quat Conjugate(quat q)
{
   quat r;
   r.x = -q.x;
   r.y = -q.y;
   r.z = -q.z;
   r.w = -q.w;

   return r;
}

inline static
float Magnitude(quat q)
{
   return sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

inline static
quat Unit(quat q)
{
   quat result;
   float mag = Magnitude(q);

   result.w = q.w / mag;
   result.x = q.x / mag;
   result.y = q.y / mag;
   result.z = q.z / mag;

   return result;
}

inline static
quat Inverse(quat q)
{
   return Conjugate(Unit(q));
}

inline static
quat Rotation(v3 axis, float angle)
{
   quat result;

   float halfAngle = angle * 0.5f;
   float sinHalfAngle = sinf(halfAngle);

   result.x = axis.x * sinHalfAngle;
   result.y = axis.y * sinHalfAngle;
   result.z = axis.z * sinHalfAngle;
   result.w = cosf(halfAngle);

   return result;
}

inline static
quat CombineRotations(quat apply, quat to)
{
   return (to * apply) * Inverse(apply);
}

struct m4
{
   union
   {
      float e[16];
      float e2[4][4];
   };

   inline m4 operator*(m4 b);
   v4 operator*(v4 a);
};

static inline
m4 transpose(m4 m)
{
   m4 result;

   result.e2[0][0] = m.e2[0][0];
   result.e2[0][1] = m.e2[1][0];
   result.e2[0][2] = m.e2[2][0];
   result.e2[0][3] = m.e2[3][0];

   result.e2[1][0] = m.e2[0][1];
   result.e2[1][1] = m.e2[1][1];
   result.e2[1][2] = m.e2[2][1];
   result.e2[1][3] = m.e2[3][1];

   result.e2[2][0] = m.e2[0][2];
   result.e2[2][1] = m.e2[1][2];
   result.e2[2][2] = m.e2[2][2];
   result.e2[2][3] = m.e2[3][2];

   result.e2[3][0] = m.e2[0][3];
   result.e2[3][1] = m.e2[1][3];
   result.e2[3][2] = m.e2[2][3];
   result.e2[3][3] = m.e2[3][3];

   return result;
}

static
m3 M3(quat q)
{
   float xx = 2.0f * q.x * q.x;
   float yy = 2.0f * q.y * q.y;
   float zz = 2.0f * q.z * q.z;
   float xy = 2.0f * q.x * q.y;
   float wz = 2.0f * q.w * q.z;
   float xz = 2.0f * q.x * q.z;
   float wy = 2.0f * q.w * q.y;
   float yz = 2.0f * q.y * q.z;
   float wx = 2.0f * q.w * q.x;

   m3 result;

   result.e[0] = 1 - yy - zz;
   result.e[1] = xy - wz;
   result.e[2] = xz + wy;

   result.e[3] = xy + wz;
   result.e[4] = 1.0f - xx - zz;
   result.e[5] = yz - wx;

   result.e[6] = xz - wy;
   result.e[7] = yz + wx;
   result.e[8] = 1.0f - xx - yy;

   return result;
}

static
m4 M4(quat q)
{
   float xx = 2.0f * q.x * q.x;
   float yy = 2.0f * q.y * q.y;
   float zz = 2.0f * q.z * q.z;
   float xy = 2.0f * q.x * q.y;
   float wz = 2.0f * q.w * q.z;
   float xz = 2.0f * q.x * q.z;
   float wy = 2.0f * q.w * q.y;
   float yz = 2.0f * q.y * q.z;
   float wx = 2.0f * q.w * q.x;

   m4 result;

   result.e[0] = 1 - yy - zz;
   result.e[1] = xy - wz;
   result.e[2] = xz + wy;
   result.e[3] = 0.0f;

   result.e[4] = xy + wz;
   result.e[5] = 1.0f - xx - zz;
   result.e[6] = yz - wx;
   result.e[7] = 0.0f;

   result.e[8] = xz - wy;
   result.e[9] = yz + wx;
   result.e[10] = 1.0f - xx - yy;
   result.e[11] = 0.0f;

   result.e[12] = 0.0f;
   result.e[13] = 0.0f;
   result.e[14] = 0.0f;
   result.e[15] = 1.0f;

   return result;
}

static inline
m4 InfiniteProjection3D(float width, float height, float nearPlane, float fov)
{
   m4 result = {};

   float AspectRatio = width/height;
   float TanthetaOver2 = width/height;

   result.e2[0][0] = 1.0f / TanthetaOver2;
   result.e2[1][1] = AspectRatio / TanthetaOver2;
   result.e2[2][3] = -1.0f;
   result.e2[2][2] = -1.0f;
   result.e2[3][2] = -2.0f * nearPlane;
   result.e2[3][3] = 0.0f;

   return result;
}

static inline
m4 Projection3D(float width, float height, float nearPlane, float farPlane, float fov)
{
   m4 Result = {};

   float AspectRatio = width/height;
   float TanThetaOver2 = tanf(fov * (PI / 360.0f));
    
   Result.e2[0][0] = 1.0f / TanThetaOver2;
   Result.e2[1][1] = AspectRatio / TanThetaOver2;
   Result.e2[2][3] = -1.0f;
   Result.e2[2][2] = (nearPlane + farPlane) / (nearPlane - farPlane);
   Result.e2[3][2] = (2.0f * nearPlane * farPlane) / (nearPlane - farPlane);
   Result.e2[3][3] = 0.0f;

   return Result;
}

inline
m4 Scale(float x, float y, float z, float w = 1.0f)
{
   m4 result;
   result = {x, 0.0f, 0.0f, 0.0f,
	     0.0f, y, 0.0f, 0.0f,
	     0.0f, 0.0f, z, 0.0f,
	     0.0f, 0.0f, 0.0f, w};

   return result;
}

inline
m4 Scale(v3 s)
{
   return Scale(s.x, s.y, s.z, 1.0f);
}

inline
m4 Translate(float x, float y, float z)
{
   m4 result;
   result = {1.0f, 0.0f, 0.0f, 0.0f,
	     0.0f, 1.0f, 0.0f, 0.0f,
	     0.0f, 0.0f, 1.0f, 0.0f,
	     x, y, z, 1.0f};

   return result;
}

inline
m4 Translate(v3 pos)
{
   m4 result;
   result = {1.0f, 0.0f, 0.0f, 0.0f,
	     0.0f, 1.0f, 0.0f, 0.0f,
	     0.0f, 0.0f, 1.0f, 0.0f,
	     pos.x, pos.y, pos.z, 1.0f};
   return result;
}

inline m4
m4::operator*(m4 b)
{
   m4 result;

   // @simd
   result.e[0] = e[0] * b.e[0] + e[4] * b.e[1] + e[8] * b.e[2] + e[12] * b.e[3];
   result.e[4] = e[0] * b.e[4] + e[4] * b.e[5] + e[8] * b.e[6] + e[12] * b.e[7];
   result.e[8] = e[0] * b.e[8] + e[4] * b.e[9] + e[8] * b.e[10] + e[12] * b.e[11];
   result.e[12] = e[0] * b.e[12] + e[4] * b.e[13] + e[8] * b.e[14] + e[12] * b.e[15];

   result.e[1] = e[1] * b.e[0] + e[5] * b.e[1] + e[9] * b.e[2] + e[13] * b.e[3];
   result.e[5] = e[1] * b.e[4] + e[5] * b.e[5] + e[9] * b.e[6] + e[13] * b.e[7];
   result.e[9] = e[1] * b.e[8] + e[5] * b.e[9] + e[9] * b.e[10] + e[13] * b.e[11];
   result.e[13] = e[1] * b.e[12] + e[5] * b.e[13] + e[9] * b.e[14] + e[13] * b.e[15];

   result.e[2] = e[2] * b.e[0] + e[6] * b.e[1] + e[10] * b.e[2] + e[14] * b.e[3];
   result.e[6] = e[2] * b.e[4] + e[6] * b.e[5] + e[10] * b.e[6] + e[14] * b.e[7];
   result.e[10] = e[2] * b.e[8] + e[6] * b.e[9] + e[10] * b.e[10] + e[14] * b.e[11];
   result.e[14] = e[2] * b.e[12] + e[6] * b.e[13] + e[10] * b.e[14] + e[14] * b.e[15];

   result.e[3] = e[3] * b.e[0] + e[7] * b.e[1] + e[11] * b.e[2] + e[15] * b.e[3];
   result.e[7] = e[3] * b.e[4] + e[7] * b.e[5] + e[11] * b.e[6] + e[15] * b.e[7];
   result.e[11] = e[3] * b.e[8] + e[7] * b.e[9] + e[11] * b.e[10] + e[15] * b.e[11];
   result.e[15] = e[3] * b.e[12] + e[7] * b.e[13] + e[11] * b.e[14] + e[15] * b.e[15];

   return result;
}

v4
m4::operator*(v4 b)
{
   v4 result;

   result.e[0] = e[0] * b.e[0] + e[4] * b.e[1] + e[8] * b.e[2] + e[12] * b.e[3];
   result.e[1] = e[1] * b.e[0] + e[5] * b.e[1] + e[9] * b.e[2] + e[13] * b.e[3];
   result.e[2] = e[2] * b.e[0] + e[6] * b.e[1] + e[10] * b.e[2] + e[14] * b.e[3];
   result.e[3] = e[3] * b.e[0] + e[7] * b.e[1] + e[11] * b.e[2] + e[15] * b.e[3];

   return result;
}

m4 invert(m4 &mat)
{
   m4 result;

   //@simd
   result.e[0] = mat.e[5]  * mat.e[10] * mat.e[15] - 
      mat.e[5]  * mat.e[11] * mat.e[14] - 
      mat.e[9]  * mat.e[6]  * mat.e[15] + 
      mat.e[9]  * mat.e[7]  * mat.e[14] +
      mat.e[13] * mat.e[6]  * mat.e[11] - 
      mat.e[13] * mat.e[7]  * mat.e[10];

   result.e[4] = -mat.e[4]  * mat.e[10] * mat.e[15] + 
      mat.e[4]  * mat.e[11] * mat.e[14] + 
      mat.e[8]  * mat.e[6]  * mat.e[15] - 
      mat.e[8]  * mat.e[7]  * mat.e[14] - 
      mat.e[12] * mat.e[6]  * mat.e[11] + 
      mat.e[12] * mat.e[7]  * mat.e[10];

   result.e[8] = mat.e[4]  * mat.e[9] * mat.e[15] - 
      mat.e[4]  * mat.e[11] * mat.e[13] - 
      mat.e[8]  * mat.e[5] * mat.e[15] + 
      mat.e[8]  * mat.e[7] * mat.e[13] + 
      mat.e[12] * mat.e[5] * mat.e[11] - 
      mat.e[12] * mat.e[7] * mat.e[9];

   result.e[12] = -mat.e[4]  * mat.e[9] * mat.e[14] + 
      mat.e[4]  * mat.e[10] * mat.e[13] +
      mat.e[8]  * mat.e[5] * mat.e[14] - 
      mat.e[8]  * mat.e[6] * mat.e[13] - 
      mat.e[12] * mat.e[5] * mat.e[10] + 
      mat.e[12] * mat.e[6] * mat.e[9];

   result.e[1] = -mat.e[1]  * mat.e[10] * mat.e[15] + 
      mat.e[1]  * mat.e[11] * mat.e[14] + 
      mat.e[9]  * mat.e[2] * mat.e[15] - 
      mat.e[9]  * mat.e[3] * mat.e[14] - 
      mat.e[13] * mat.e[2] * mat.e[11] + 
      mat.e[13] * mat.e[3] * mat.e[10];

   result.e[5] = mat.e[0]  * mat.e[10] * mat.e[15] - 
      mat.e[0]  * mat.e[11] * mat.e[14] - 
      mat.e[8]  * mat.e[2] * mat.e[15] + 
      mat.e[8]  * mat.e[3] * mat.e[14] + 
      mat.e[12] * mat.e[2] * mat.e[11] - 
      mat.e[12] * mat.e[3] * mat.e[10];

   result.e[9] = -mat.e[0]  * mat.e[9] * mat.e[15] + 
      mat.e[0]  * mat.e[11] * mat.e[13] + 
      mat.e[8]  * mat.e[1] * mat.e[15] - 
      mat.e[8]  * mat.e[3] * mat.e[13] - 
      mat.e[12] * mat.e[1] * mat.e[11] + 
      mat.e[12] * mat.e[3] * mat.e[9];

   result.e[13] = mat.e[0]  * mat.e[9] * mat.e[14] - 
      mat.e[0]  * mat.e[10] * mat.e[13] - 
      mat.e[8]  * mat.e[1] * mat.e[14] + 
      mat.e[8]  * mat.e[2] * mat.e[13] + 
      mat.e[12] * mat.e[1] * mat.e[10] - 
      mat.e[12] * mat.e[2] * mat.e[9];

   result.e[2] = mat.e[1]  * mat.e[6] * mat.e[15] - 
      mat.e[1]  * mat.e[7] * mat.e[14] - 
      mat.e[5]  * mat.e[2] * mat.e[15] + 
      mat.e[5]  * mat.e[3] * mat.e[14] + 
      mat.e[13] * mat.e[2] * mat.e[7] - 
      mat.e[13] * mat.e[3] * mat.e[6];

   result.e[6] = -mat.e[0]  * mat.e[6] * mat.e[15] + 
      mat.e[0]  * mat.e[7] * mat.e[14] + 
      mat.e[4]  * mat.e[2] * mat.e[15] - 
      mat.e[4]  * mat.e[3] * mat.e[14] - 
      mat.e[12] * mat.e[2] * mat.e[7] + 
      mat.e[12] * mat.e[3] * mat.e[6];

   result.e[10] = mat.e[0]  * mat.e[5] * mat.e[15] - 
      mat.e[0]  * mat.e[7] * mat.e[13] - 
      mat.e[4]  * mat.e[1] * mat.e[15] + 
      mat.e[4]  * mat.e[3] * mat.e[13] + 
      mat.e[12] * mat.e[1] * mat.e[7] - 
      mat.e[12] * mat.e[3] * mat.e[5];

   result.e[14] = -mat.e[0]  * mat.e[5] * mat.e[14] + 
      mat.e[0]  * mat.e[6] * mat.e[13] + 
      mat.e[4]  * mat.e[1] * mat.e[14] - 
      mat.e[4]  * mat.e[2] * mat.e[13] - 
      mat.e[12] * mat.e[1] * mat.e[6] + 
      mat.e[12] * mat.e[2] * mat.e[5];

   result.e[3] = -mat.e[1] * mat.e[6] * mat.e[11] + 
      mat.e[1] * mat.e[7] * mat.e[10] + 
      mat.e[5] * mat.e[2] * mat.e[11] - 
      mat.e[5] * mat.e[3] * mat.e[10] - 
      mat.e[9] * mat.e[2] * mat.e[7] + 
      mat.e[9] * mat.e[3] * mat.e[6];

   result.e[7] = mat.e[0] * mat.e[6] * mat.e[11] - 
      mat.e[0] * mat.e[7] * mat.e[10] - 
      mat.e[4] * mat.e[2] * mat.e[11] + 
      mat.e[4] * mat.e[3] * mat.e[10] + 
      mat.e[8] * mat.e[2] * mat.e[7] - 
      mat.e[8] * mat.e[3] * mat.e[6];

   result.e[11] = -mat.e[0] * mat.e[5] * mat.e[11] + 
      mat.e[0] * mat.e[7] * mat.e[9] + 
      mat.e[4] * mat.e[1] * mat.e[11] - 
      mat.e[4] * mat.e[3] * mat.e[9] - 
      mat.e[8] * mat.e[1] * mat.e[7] + 
      mat.e[8] * mat.e[3] * mat.e[5];

   result.e[15] = mat.e[0] * mat.e[5] * mat.e[10] - 
      mat.e[0] * mat.e[6] * mat.e[9] - 
      mat.e[4] * mat.e[1] * mat.e[10] + 
      mat.e[4] * mat.e[2] * mat.e[9] + 
      mat.e[8] * mat.e[1] * mat.e[6] - 
      mat.e[8] * mat.e[2] * mat.e[5];

   float det = mat.e[0] * result.e[0] + mat.e[1] * result.e[4] + mat.e[2] * result.e[8] + mat.e[3] * result.e[12];

   det = 1.0f / det;

   for (i32 i = 0; i < 16; i++)
   {
      result.e[i] *= det;
   }
    
   return result;
}

static inline
m4 Identity()
{
   return {1.0f, 0.0f, 0.0f, 0.0f,
	 0.0f, 1.0f, 0.0f, 0.0f,
	 0.0f, 0.0f, 1.0f, 0.0f,
	 0.0f, 0.0f, 0.0f, 1.0f};
} 

static inline
v4 operator*(float c, v4 v)
{
   v4 result;

   result.x = c * v.x;
   result.y = c * v.y;
   result.z = c * v.z;
   result.w = c * v.w;

   return result;
}

static inline
v4 operator+(float c, v4 v)
{
   v4 result;

   result.x = c * v.x;
   result.y = c * v.y;
   result.z = c * v.z;
   result.w = c * v.w;

   return result;
}

static inline
v4 lerp(v4 a, v4 b, float t)
{
   return (1.0f - t) * a + (t * b);
}

static inline
v3 lerp(v3 a, v3 b, float t)
{
   return (1.0f - t) * a + (t * b);
}

static inline
float lerp(float a, float b, float t)
{
   return (1.0f - t) * a + (t * b);
}

static inline
float smoothstep(float a, float b, float t)
{
   t = clamp((t - a) / (b - a), 0.0f, 1.0f);
   return t * t * (3 - 2 * t);
}

static inline
m3 Scale3(float x, float y, float z)
{
   m3 result = {};

   result.e2[0][0] = x;
   result.e2[1][1] = y;
   result.e2[2][2] = z;

   return result;
}

static inline
m3 RotationAboutZ(float rads)
{
   m3 result = {};

   result.e2[0][0] = cosf(rads);
   result.e2[0][1] = -sinf(rads);
   result.e2[1][0] = sinf(rads);
   result.e2[1][1] = cosf(rads);
   result.e2[2][2] = 1.0f;

   return result;
}

//@note: should we force pack these?
struct FontHeader
{
   u32 count;
   u32 mapWidth;
   u32 mapHeight;
};

struct CharInfo
{
   float xoffset;
   float yoffset;
   float xadvance;
   u32 width;
   u32 height;
   u32 x;
   u32 y;
   i8 id;   
};

struct FontData
{
   u32 count;
   u32 mapWidth;
   u32 mapHeight;
   CharInfo *data;
};

struct ImageHeader
{
   i32 x;
   i32 y;
   i32 channels;
};

struct Image
{
   u8 *data;
   i32 x;
   i32 y;
   i32 channels;   
};

struct VirtualCoord
{
   i32 x;
   i32 y;
};

// Higher level Game Object abstraction
struct Object
{
   v3 worldPos;
   v3 scale;
   quat orientation;   
};

#pragma pack(push, 1)
struct Branch_Image_Header
{
   u32 size;
   u32 width, height;
   u32 channels;
   // pixels tagged onto the end
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PackedFont
{
   u32 width;
   u32 height;
   u32 imageOffset;
   // followed by the array of stbtt_packedchar
};
#pragma pack(pop)

struct stbFont
{
   stbtt_packedchar *chars;
   u8 *map;
   u32 width;
   u32 height;
   u32 textureHandle;
};

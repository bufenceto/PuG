#pragma once

#include <math.h>
#include <cstdint>
#include <cassert>

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

#ifndef VECTOR_DEFAULT
#define VECTOR_DEFAULT 0
#endif

#ifndef VMATH_PI
#define VMATH_PI				3.14159265359f
#endif

#ifndef VMATH_E
#define VMATH_E				2.71828182846f
#endif

#ifndef VMATH_EPSILON 
#define VMATH_EPSILON			0.00001f
#endif

#ifndef FLOAT_MAX
#define FLOAT_MAX		0x7fffffff
#endif


#define VMATH_MIN(a,b) (((a)>(b))?(b):(a))
#define VMATH_MAX(a,b) (((a)>(b))?(a):(b))
#define SIGN(x) (x>=0.0f)?(+1.0f):(-1.0f)

#define RADIANS(a) (a * (VMATH_PI/180.0f))
#define DEGREES(a) (a * (180.0f/VMATH_PI))

#define RIGHT	vmath::Vector3(1.0f, 0.0f, 0.0f)
#define UP		vmath::Vector3(0.0f, 1.0f, 0.0f)
#define FORWARD vmath::Vector3(0.0f, 0.0f, 1.0f)

//All matrices are row major (only vector * matrix allowed in row- major! )
//row major uses post order multiplication
//Left handed coordinate system will be used, forward z is positive

namespace vmath
{	
	//forward class declarations
	class Vector2;
	class Vector3;
	class Vector4;

	class Matrix2;
	class Matrix3;
	class Matrix4;

	class Quaternion;

	class Int2;

	//forward functions declarations
	float Dot(const Vector2& a, const Vector2& b);
	float Dot(const Vector3& a, const Vector3& b);
	float Dot(const Vector4& a, const Vector4& b);

	float Length(const Vector2& a);
	float Length(const Vector3& a);
	float Length(const Vector4& a);
	float Length(const Quaternion& a);

	float LengthSqr(const Vector2& a);
	float LengthSqr(const Vector3& a);
	float LengthSqr(const Vector4& a);
	float LengthSqr(const Quaternion& a);

	Vector2 Normalize(const Vector2& a);
	Vector3 Normalize(const Vector3& a);
	Vector4 Normalize(const Vector4& a);
	Quaternion Normalize(const Quaternion& a);

	Vector3 Cross(const Vector3& a, const Vector3& b);

	Matrix2 Transpose(const Matrix2& a);
	Matrix3 Transpose(const Matrix3& a);
	Matrix4 Transpose(const Matrix4& a);

	float Determinant(const Matrix2& a);
	float Determinant(const Matrix3& a);
	float Determinant(const Matrix4& a);

	Matrix3 Adjugate(const Matrix3& a);
	Matrix4 Adjugate(const Matrix4& a);

	Matrix3 Inverse(const Matrix3& a);
	Matrix4 Inverse(const Matrix4& a);
	Quaternion Inverse(const Quaternion& a);

	float Trace(const Matrix3& a);
	float Trace(const Matrix4& a);

	float Norm(Quaternion a);
	Quaternion Conjugate(Quaternion a);
	float Quadrant(const Quaternion& pQ);

	int Clamp(int pValue, int pMin, int pMax);
	float Clamp(float pValue, float pMin, float pMax);
	Vector3 Clamp(const Vector3& pValue, float pMin, float pMax);
	Vector4 Clamp(const Vector4& pValue, float pMin, float pMax);
	
	float Saturate(float pValue);

	Matrix3 QuaternionToRotationMatrix(const Quaternion& pQ);
	Matrix4	QuaternionToMatrix(const Quaternion& pQ);
	Quaternion MatrixToQuaternion(const Matrix3& pM);
	
//#######################################################################################################################################
//															VECTOR2				    												   ##
//#######################################################################################################################################

	class Vector2
	{
	public:
		//defaul constructor
		Vector2()
		{
			x = VECTOR_DEFAULT;
			y = VECTOR_DEFAULT;
		}
		//build vector3 from one float
		Vector2(const float pF)
		{
			x = pF;
			y = pF;
		}
		//build vector2 from different floats
		Vector2(const float pX, const float pY)
		{
			x = pX;
			y = pY;
		}

		//members
		union
		{
			struct { float x, y; };
			float cell[2];
		};

		//operators
		//add two vectors
		friend Vector2 operator+ (const Vector2& a, const Vector2& b)
		{
			return Vector2(a.x + b.x, a.y + b.y);
		}
		//subtract two vectors					  			 
		friend Vector2 operator- (const Vector2& a, const Vector2& b)
		{
			return Vector2(a.x - b.x, a.y - b.y);
		}
		//multiply the component of two vectors
		friend Vector2 operator* (const Vector2& a, const Vector2& b)
		{
			return Vector2(a.x * b.x, a.y * b.y);
		}
		//scale vector	
		friend Vector2 operator* (const Vector2& a, const float b)
		{
			return Vector2(a.x * b, a.y * b);
		}
		//scale vector							  		    
		friend Vector2 operator/ (const Vector2& a, const float b)
		{
			return Vector2(a.x / b, a.y / b);
		}
		//scale vector
		friend Vector2 operator* (const float a, const Vector2& b)
		{
			return Vector2(a * b.x, a * b.y);
		}
		//scale vector									   
		friend Vector2 operator/ (const float a, const Vector2& b)
		{
			return Vector2(a / b.x, a / b.y);
		}
		//invert vector
		friend Vector2 operator- (const Vector2& a)
		{
			return Vector2(-a.x, -a.y);
		}

		//acces by index
		inline float& operator[] (const int& i) { return cell[i]; }
		inline float operator[] (const int& i) const { return cell[i]; }

		//boolean operators
		friend bool operator == (const Vector2& a, const Vector2& b)
		{
			return a.x == b.x && a.y == b.y;
		}
		friend bool operator != (const Vector2& a, const Vector2& b)
		{
			return a.x != b.x || a.y != b.y;
		}

		//equals arithmatic
		Vector2& operator+= (const Vector2& a)
		{
			x = x + a.x;
			y = y + a.y;
			return *this;
		}
		Vector2& operator-= (const Vector2& a)
		{
			x = x - a.x;
			y = y - a.y;
			return *this;
		}
		Vector2& operator*= (const float a)
		{
			x = x * a;
			y = y * a;
			*this;
		}
		Vector2& operator/= (const float a)
		{
			x = x / a;
			y = y / a;
			*this;
		}
	};

//#######################################################################################################################################
//															MATRIX2				    												   ##
//#######################################################################################################################################

	class Matrix2
	{
	public:
		//default constructor
		Matrix2()
		{
			x = Vector2(1.0f, 0.0f);
			y = Vector2(0.0f, 1.0f);
		}
		//build matrix from 2 vectors
		Matrix2(const Vector2& pX, const Vector2& pY)
		{
			x = pX;
			y = pY;
		}
		//build matrix from 4 floats
		Matrix2(const float pXX, const float pXY,
				const float pYX, const float pYY)
		{
			x = Vector2(pXX, pXY);
			y = Vector2(pYX, pYY);
		}
		//build a rotation matrix from an euler angle
		Matrix2(const float pEulerRotation_radians)
		{
			float xr = cosf(pEulerRotation_radians);
			float yr = sinf(pEulerRotation_radians);
			x = Vector2(xr, -yr);
			y = Vector2(yr, xr);
		}

		//members
		union
		{
			struct { Vector2 row[2];};
			struct { Vector2 x, y;  };
		};

		//operators
		//multiply 2 matrices
		friend Matrix2 operator* (const Matrix2& a, const Matrix2& b)//row of a and columns of b
		{
			return Matrix2(Dot(a.x, Vector2(b.x.x, b.y.x)), Dot(a.x, Vector2(b.x.y, b.y.y)),
						   Dot(a.y, Vector2(b.x.x, b.y.x)), Dot(a.y, Vector2(b.x.y, b.y.y)));
		}
		//multiply vector and matrix
		friend Vector2 operator* (const Vector2& b, const Matrix2& a)
		{
			return Vector2(Dot(b, Vector2(a.x.x, a.y.x)),
						   Dot(b, Vector2(a.x.y, a.y.y)));
		}
		//scale matrix
		friend Matrix2 operator* (const float a, const Matrix2& b)
		{
			return Matrix2(a * b.x, a * b.y);
		}
		//add 2 matrices
		friend Matrix2 operator+ (const Matrix2& a, const Matrix2& b)
		{
			return Matrix2(a.x + b.x, a.y + b.y);

		}
		//subtract 2 matrices
		friend Matrix2 operator- (const Matrix2& a, const Matrix2& b)
		{
			return Matrix2(a.x - b.x, a.y - b.y);
		}
		//negate matrix - not the same as inverting or transposing!
		friend Matrix2 operator- (const Matrix2& a)
		{
			return Matrix2(-a.x, -a.y);
		}

		//acces by index
		Vector2& operator[] (int& i) { return row[i]; }
		Vector2 operator[] (const int& i) const { return row[i]; }

		//boolean operators
		friend bool operator== (const Matrix2& a, const Matrix2& b)
		{
			return a.x == b.x && a.y == b.y;
		}
		friend bool operator!= (const Matrix2& a, const Matrix2& b)
		{
			return a.x != b.x || a.y != b.y;
		}

		//equals arithmatic
		Matrix2& operator+= (const Matrix2& a)
		{
			x = x + a.x;
			y = y + a.y;
			return *this;
		}
		Matrix2& operator-= (const Matrix2& a)
		{
			x = x - a.x;
			y = y - a.y;
			return *this;
		}
		Matrix2& operator*= (const Matrix2& a)
		{
			x = x * a.x;
			y = y * a.y;
			return *this;
		}
	};

//#######################################################################################################################################
//															VECTOR3				    												   ##
//#######################################################################################################################################

	class Vector3
	{ 
	public:
		//default constructor
		Vector3()
		{
			x = VECTOR_DEFAULT;
			y = VECTOR_DEFAULT;
			z = VECTOR_DEFAULT;
		}
		//assign same value to all components
		Vector3(const float pF)
		{
			x = pF;
			y = pF;
			z = pF;
		}
		//build vector from 3 floats
		Vector3(const float pX, const float pY, const float pZ)
		{
			x = pX;
			y = pY;
			z = pZ;
		}

		//members
		union
		{
			struct { float x, y, z; };
			float cell[3];
		};

		//operators
		//add 2 vectors
		friend Vector3 operator+ (const Vector3& a, const Vector3& b)
		{
			return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
		}
		//subtract 2 vectors
		friend Vector3 operator- (const Vector3& a, const Vector3& b)
		{
			return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
		}
		//multiply the components of 2 vectors
		friend Vector3 operator* (const Vector3& a, const Vector3& b)
		{
			return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
		}
		//divide the components of 2 vectors
		friend Vector3 operator/ (const Vector3& a, const Vector3& b)
		{
			return Vector3(a.x / b.x, a.y / b.y, a.z / b.z);
		}
		//scale vector
		friend Vector3 operator* (const Vector3& a, const float b)
		{
			return Vector3(a.x * b, a.y * b, a.z * b);
		}
		//scale vector
		friend Vector3 operator* (const float a, const Vector3& b)
		{
			return Vector3(a * b.x, a * b.y, a * b.z);
		}
		//scale vector
		friend Vector3 operator/ (const Vector3& a, const float b)
		{
			return Vector3(a.x / b, a.y / b, a.z / b);
		}
		//negate vector
		friend Vector3 operator- (const Vector3& a)
		{
			return Vector3(-a.x, -a.y, -a.z);
		}

		//acces by index
		float& operator[] (const int& i) { return cell[i]; }
		float operator[] (const int& i) const { return cell[i]; }

		//boolean opeator
		friend bool operator== (const Vector3& a, const Vector3& b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
		friend bool operator!= (const Vector3& a, const Vector3& b) 
		{ 
			return a.x != b.x || a.y != b.y || a.z != b.z; 
		}

		//equals arithmatic
		Vector3& operator+= (const Vector3& a)
		{
			x = x + a.x;
			y = y + a.y;
			z = z + a.z;
			return *this;
		}
		Vector3& operator-= (const Vector3& a)
		{
			x = x - a.x;
			y = y - a.y;
			z = z - a.z;
			return *this;
		}
		Vector3& operator*= (const Vector3& a)
		{
			x = x * a.x;
			y = y * a.y;
			z = z * a.z;
			return *this;
		}
		Vector3& operator*= (const float a)
		{
			x = x * a;
			y = y * a;
			z = z * a;
			return *this;
		}
		Vector3& operator/= (const float a)
		{
			x = x / a;
			y = y / a;
			z = z / a;
			return *this;
		}
	};

//#######################################################################################################################################
//															MATRIX3				    												   ##
//#######################################################################################################################################

	class Matrix3
	{
	public:
		//default constructor
		Matrix3()
		{
			x = Vector3(1.0f, 0.0f, 0.0f);
			y = Vector3(0.0f, 1.0f, 0.0f);
			z = Vector3(0.0f, 0.0f, 1.0f);
		}
		//build matrix from vectors
		Matrix3(const Vector3& pX, const Vector3& pY, const Vector3& pZ)
		{
			x = pX;
			y = pY;
			z = pZ;
		}
		//build matrix from floats
		Matrix3(const float pXX, const float pXY, const float pXZ,
				const float pYX, const float pYY, const float pYZ,
				const float pZX, const float pZY, const float pZZ)
		{
			x = Vector3(pXX, pXY, pXZ);
			y = Vector3(pYX, pYY, pYZ);
			z = Vector3(pZX, pZY, pZZ);
		}
		//build matrix from euler angles and a scale
		Matrix3(const Vector3& pEulerAngles_radians, const Vector3& pScale = Vector3(1.0f))
		{
			float A = cosf(pEulerAngles_radians.x);
			float B = sinf(pEulerAngles_radians.x);
			float C = cosf(pEulerAngles_radians.y);
			float D = sinf(pEulerAngles_radians.y);
			float E = cosf(pEulerAngles_radians.z);
			float F = sinf(pEulerAngles_radians.z);

			float minDminB = -D * -B;
			float minDA = -D * A;

			Matrix3 res = Matrix3(E * C, E * minDminB + A * F, E * minDA + F * B,
				-F * C, -F * minDminB + A * E, -F * minDA + E * B,
				D, -B * C, C * A);

			x = Normalize(res.x) * pScale.x;
			y = Normalize(res.y) * pScale.y;
			z = Normalize(res.z) * pScale.z;
		}

		//members
		union
		{
			struct { Vector3 row[3]; };
			struct { Vector3 x, y, z; };
		};

		//operators
		//multiply matrix a with matrix b
		friend Matrix3 operator* (const Matrix3& a, const Matrix3& b)
		{
			return Matrix3(Dot(a.x, Vector3(b.x.x, b.y.x, b.z.x)), Dot(a.x, Vector3(b.x.y, b.y.y, b.z.y)), Dot(a.x, Vector3(b.x.z, b.y.z, b.z.z)),
						   Dot(a.y, Vector3(b.x.x, b.y.x, b.z.x)), Dot(a.y, Vector3(b.x.y, b.y.y, b.z.y)), Dot(a.y, Vector3(b.x.z, b.y.z, b.z.z)),
						   Dot(a.z, Vector3(b.x.x, b.y.x, b.z.x)), Dot(a.z, Vector3(b.x.y, b.y.y, b.z.y)), Dot(a.z, Vector3(b.x.z, b.y.z, b.z.z)));
		}
		//multiply a vector with a matrix
		friend Vector3 operator* (const Vector3& b, const Matrix3& a)
		{
			return Vector3(Dot(b, Vector3(a.x.x, a.y.x, a.z.x)),
						   Dot(b, Vector3(a.x.y, a.y.y, a.z.y)),
						   Dot(b, Vector3(a.x.z, a.y.z, a.z.z)));
		}
		//scale a matrix
		friend Matrix3 operator* (const float a, const Matrix3& b)
		{
			return Matrix3(a * b.x, a * b.y, a * b.z);
		}
		//scale a matrix
		friend Matrix3 operator* (const Matrix3& a, const float b)
		{
			return Matrix3(a.x * b, a.y * b, a.z * b);
		}
		//scale a matrix
		friend Matrix3 operator/ (const Matrix3& a, const float b)
		{
			float invb = 1.0f / b;
			return Matrix3(a.x * invb, a.y * invb, a.z * invb);
		}
		//add the components of the matrix
		friend Matrix3 operator+ (const Matrix3& a, const Matrix3& b)
		{
			return Matrix3(a.x + b.x, a.y + b.y, a.z + b.z);
		}
		//subtract the components of the matrix
		friend Matrix3 operator- (const Matrix3& a, const Matrix3& b)
		{
			return Matrix3(a.x - b.x, a.y - b.y, a.z - b.z);
		}
		//negate the components of the matrix
		friend Matrix3 operator- (const Matrix3& a)
		{
			return Matrix3(-a.x, -a.y, -a.z);
		}

		//acces by index
		Vector3& operator[] (const int& i) { return row[i]; }
		Vector3 operator[] (const int& i) const { return row[i]; }

		//boolean operators
		friend bool operator== (const Matrix3& a, const Matrix3& b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z;
		}
		friend bool operator!= (const Matrix3& a, const Matrix3& b)
		{
			return a.x != b.x || a.y != b.y || a.z != b.z;
		}

		//equals arithematic
		Matrix3& operator+= (const Matrix3& a)
		{
			x = x + a.x;
			y = y + a.y;
			z = z + a.z;
			return *this;
		}
		Matrix3& operator-= (const Matrix3& a)
		{
			x = x - a.x;
			y = y - a.y;
			z = z - a.z;
			return *this;
		}
		Matrix3& operator*= (const Matrix3& a)
		{
			x = x * a.x;
			y = y * a.y;
			z = z * a.z;
			return *this;
		}
		Matrix3& operator*= (const float a)
		{
			x = x * a;
			y = y * a;
			z = z * a;
			return *this;
		}
		Matrix3& operator/= (const float a)
		{
			x = x / a;
			y = y / a;
			z = z / a;
			return *this;
		}
	};

//#######################################################################################################################################
//															VECTOR4				    												   ##
//#######################################################################################################################################

	class Vector4
	{
	public:
		//default constructor
		Vector4()
		{
			x = VECTOR_DEFAULT;
			y = VECTOR_DEFAULT;
			z = VECTOR_DEFAULT;
			w = VECTOR_DEFAULT;
		}
		//build vector from a single float
		Vector4(const float pF)
		{
			x = pF;
			y = pF;
			z = pF;
			w = pF;
		}
		//build a vector from 4 floats
		Vector4(const float pX, const float pY, const float pZ, const float pW)
		{
			x = pX;
			y = pY;
			z = pZ;
			w = pW;
		}
		//build a vector4 from a Vector3 and a float
		Vector4(const Vector3& pVector, const float pF)
		{
			x = pVector.x;
			y = pVector.y;
			z = pVector.z;
			w = pF;
		}

		//members
		union
		{
			struct { float x, y, z, w; };
			float cell[4];
		};

		//operators
		//add two vectors
		friend Vector4 operator+ (const Vector4& a, const Vector4& b)
		{
			return Vector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
		}
		//subtract two vectors
		friend Vector4 operator- (const Vector4& a, const Vector4& b)
		{
			return Vector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
		}
		//multiply the components of two vectors
		friend Vector4 operator* (const Vector4& a, const Vector4& b)
		{
			return Vector4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
		}
		//scale a vector
		friend Vector4 operator* (const Vector4& a, const float b)
		{
			return Vector4(a.x * b, a.y * b, a.z * b, a.w * b);
		}
		//scale a vector
		friend Vector4 operator/ (const Vector4& a, const float b)
		{
			return Vector4(a.x / b, a.y / b, a.z / b, a.w / b);
		}
		//scale a vector
		friend Vector4 operator* (const float a, const Vector4& b)
		{
			return Vector4(a * b.x, a * b.y, a * b.z, a * b.w);
		}
		//negate a vector
		friend Vector4 operator- (const Vector4& a)
		{
			return Vector4(-a.x, -a.y, -a.z, -a.w);
		}

		//acces by index
		float& operator[] (const int& i) { return cell[i]; }
		float operator[] (const int& i) const { return cell[i]; }

		//boolean opeator
		friend bool operator== (Vector4 a, Vector4 b) 
		{ 
			return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; 
		}
		friend bool operator!= (Vector4 a, Vector4 b)
		{
			return a.x != b.x && a.y || b.y || a.z != b.z || a.w != b.w;
		}

		//equals arithmatic
		Vector4& operator+= (const Vector4& a)
		{
			x = x + a.x;
			y = y + a.y;
			z = z + a.z;
			w = w + a.w;
			return *this;
		}
		Vector4& operator-= (const Vector4& a)
		{
			x = x - a.x;
			y = y - a.y;
			z = z - a.z;
			w = w - a.w;
			return *this;
		}
		Vector4& operator*= (const Vector4& a)
		{
			x = x * a.x;
			y = y * a.y;
			z = z * a.z;
			w = w * a.w;
			return *this;
		}
		Vector4& operator*= (const float a)
		{
			x = x - a;
			y = y - a;
			z = z - a;
			w = w - a;
			return *this;
		}
		Vector4& operator/= (const float  a)
		{
			x = x / a;
			y = y / a;
			z = z / a;
			w = w / a;
			return *this;
		}
	};

//#######################################################################################################################################
//															MATRIX4				    												   ##
//#######################################################################################################################################

	class Matrix4
	{
	public:
		//defaul constructor
		Matrix4()
		{
			x = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
			y = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
			z = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
			w = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		//build a matrix from 4 vectors
		Matrix4(const Vector4& pX, const Vector4& pY, const Vector4& pZ, const Vector4& pW)
		{
			x = pX;
			y = pY;
			z = pZ;
			w = pW;
		}
		//build a matrix from 16 floats
		Matrix4(const float pXX, const float pXY, const float pXZ, const float pXW,
				const float pYX, const float pYY, const float pYZ, const float pYW,
				const float pZX, const float pZY, const float pZZ, const float pZW,
				const float pWX, const float pWY, const float pWZ, const float pWW)
		{
			x = Vector4(pXX, pXY, pXZ, pXW);
			y = Vector4(pYX, pYY, pYZ, pYW);
			z = Vector4(pZX, pZY, pZZ, pZW);
			w = Vector4(pWX, pWY, pWZ, pWW);
		}

		//members
		union
		{
			struct { Vector4 row[4]; };
			struct { Vector4 x, y, z, w; };
		};

		//operators
		//multiply matrix a with matrix b
		friend Matrix4 operator* (const Matrix4& a, const Matrix4& b)
		{
			return Matrix4(Dot(a.x, Vector4(b.x.x, b.y.x, b.z.x, b.w.x)), Dot(a.x, Vector4(b.x.y, b.y.y, b.z.y, b.w.y)), Dot(a.x, Vector4(b.x.z, b.y.z, b.z.z, b.w.z)), Dot(a.x, Vector4(b.x.w, b.y.w, b.z.w, b.w.w)),
						   Dot(a.y, Vector4(b.x.x, b.y.x, b.z.x, b.w.x)), Dot(a.y, Vector4(b.x.y, b.y.y, b.z.y, b.w.y)), Dot(a.y, Vector4(b.x.z, b.y.z, b.z.z, b.w.z)), Dot(a.y, Vector4(b.x.w, b.y.w, b.z.w, b.w.w)),
						   Dot(a.z, Vector4(b.x.x, b.y.x, b.z.x, b.w.x)), Dot(a.z, Vector4(b.x.y, b.y.y, b.z.y, b.w.y)), Dot(a.z, Vector4(b.x.z, b.y.z, b.z.z, b.w.z)), Dot(a.z, Vector4(b.x.w, b.y.w, b.z.w, b.w.w)),
						   Dot(a.w, Vector4(b.x.x, b.y.x, b.z.x, b.w.x)), Dot(a.w, Vector4(b.x.y, b.y.y, b.z.y, b.w.y)), Dot(a.w, Vector4(b.x.z, b.y.z, b.z.z, b.w.z)), Dot(a.w, Vector4(b.x.w, b.y.w, b.z.w, b.w.w)));
		}
		//multiply a vector with a matrix
		friend Vector4 operator* (const Vector4& b, const Matrix4& a)
		{
			return Vector4(Dot(b, Vector4(a.x.x, a.y.x, a.z.x, a.w.x)),
						      Dot(b, Vector4(a.x.y, a.y.y, a.z.y, a.w.y)),
						      Dot(b, Vector4(a.x.z, a.y.z, a.z.z, a.w.z)),
					         Dot(b, Vector4(a.x.w, a.y.w, a.z.w, a.w.w)));
		}
		friend Matrix4 operator* (const float a, const Matrix4& b)
		{
			return Matrix4(b.x * a, b.y * a, b.z * a, b.w * a);
		}
		friend Matrix4 operator* (const Matrix4& b, const float a)
		{
			return Matrix4(b.x * a, b.y * a, b.z * a, b.w * a);
		}
		friend Matrix4 operator/ (const Matrix4& a, const float b)
		{
			float c = 1.0f / b;
			return Matrix4(a.x * c, a.y * c, a.z * c, a.w * c);
		}
		friend Matrix4 operator+ (const Matrix4& a, const Matrix4& b)
		{
			return Matrix4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
		}
		friend Matrix4 operator- (const Matrix4& a, const Matrix4& b)
		{
			return Matrix4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
		}
		friend Matrix4 operator- (const Matrix4& a)
		{
			return Matrix4(-a.x, -a.y, -a.z, -a.w);
		}
		friend Vector3 operator* (const Matrix4& a, const Vector3& b)
		{
			Vector4 t = Vector4(b, 1.0f) * a;
			return Vector3(t.x, t.y, t.z);
		}
		friend Vector3 operator* (const Vector3& b, const Matrix4& a)
		{
			Vector4 t = Vector4(b, 1.0f) * a;
			return Vector3(t.x, t.y, t.z);
		}

		//acces by index
		Vector4& operator[] (const int& i) { return row[i]; }
		Vector4 operator[] (const int& i) const { return row[i]; }

		//boolean operators
		friend bool operator== (Matrix4 a, Matrix4 b)
		{
			return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
		}
		friend bool operator!= (Matrix4 a, Matrix4 b)
		{
			return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
		}

		//equals arithmatic
		Matrix4& operator+= (Matrix4 a)
		{
			x = x + a.x;
			y = y + a.y;
			z = z + a.z;
			w = w + a.w;
			return *this;
		}
		Matrix4& operator-= (Matrix4 a)
		{
			x = x - a.x;
			y = y - a.y;
			z = z - a.z;
			w = w - a.w;
			return *this;
		}
		Matrix4& operator*= (Matrix4 a)
		{
			x = x * a.x;
			y = y * a.y;
			z = z * a.z;
			w = w * a.w;
			return *this;
		}
		Matrix4& operator*= (float a)
		{
			x = x * a;
			y = y * a;
			z = z * a;
			w = w * a;
			return *this;
		}
		Matrix4& operator/= (float a)
		{
			x = x / a;
			y = y / a;
			z = z / a;
			w = w / a;
			return *this;
		}

	};

//#######################################################################################################################################
//															QUATERNION			    												   ##
//#######################################################################################################################################

	class Quaternion
	{
	public:
		//default constructor
		Quaternion()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
			w = 1.0f;
		}
		//build quaternion from 4 floats
		Quaternion(float pX, float pY, float pZ, float pW)
		{
			x = pX;
			y = pY;
			z = pZ;
			w = pW;
		}
		//construct Quaternion from a rotation axis and an euler angle
		Quaternion(Vector3 pAxisOfRotation, float pEulerAngle_radians)
		{
			float tmp = sinf(pEulerAngle_radians / 2.0f);
			x = pAxisOfRotation.x * tmp;
			y = pAxisOfRotation.y * tmp;
			z = pAxisOfRotation.z * tmp;
			w = cosf(pEulerAngle_radians / 2.0f);
		}
		//construct Quaternion from euler angles
		Quaternion(Vector3 pEulerAngles_radians)
		{
			Quaternion xrot = Quaternion(RIGHT, pEulerAngles_radians.x);
			Quaternion yrot = Quaternion(UP, pEulerAngles_radians.y);
			Quaternion zrot = Quaternion(FORWARD, pEulerAngles_radians.z);
			*this = (yrot * xrot) * zrot;
		}

		//members
		union
		{
			struct { float x, y, z; };
			struct { Vector3 axis; };
		};
		float w;

		//operators
		//multiply two quaternions
		friend Quaternion operator* (const Quaternion& a, const Quaternion& b)
		{
				return Quaternion((a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y),
								  (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z),
								  (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x),
								  (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z));
		}
		//scale a quaternion
		friend Quaternion operator* (const Quaternion& a, const float b)
		{
			return Quaternion(a.x * b, a.y * b, a.z * b, a.w * b);
		}
		//scale a quaternion
		friend Quaternion operator* (const float b, const Quaternion& a)
		{
			return Quaternion(b * a.x, b * a.y, b * a.z, b * a.w);
		}
		//scale a quaternion
		friend Quaternion operator/ (const Quaternion& a, const float b)
		{
			return Quaternion(a.x / b, a.y / b, a.z / b, a.w / b);
		}
		//add the component of two quaternions together
		friend Quaternion operator+ (const Quaternion& a, const Quaternion& b)
		{
			return Quaternion(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
		}
		friend Vector3 operator * (const Quaternion& a, const Vector3& b)
		{
			Vector3 t = 2.0f * Cross(a.axis, b);
			return b + a.w * t + Cross(a.axis, t);
		}
		friend Vector3 operator * (const Vector3& b, const Quaternion& a)
		{
			Vector3 t = 2.0f * Cross(a.axis, b);
			return b + a.w * t + Cross(a.axis, t);
		}

		//boolean operators
		friend bool operator== (const Quaternion& a, const Quaternion& b) 
		{ 
			return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; 
		}
		friend bool operator!= (const Quaternion& a, const Quaternion& b)
		{
			return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
		}
	};

	//#######################################################################################################################################
	//															INT2		    												   ##
	//#######################################################################################################################################

	class Int2
	{
	public:
		//defaul constructor
		Int2()
		{
			x = VECTOR_DEFAULT;
			y = VECTOR_DEFAULT;
		}
		//build vector2 from different integers
		Int2(const int32_t& pX, const int32_t& pY)
		{
			x = pX;
			y = pY;
		}

		//members
		union
		{
			struct { int32_t x, y; };
			int32_t cell[2];
		};

		//operators
		//add two vectors
		friend Int2 operator+ (const Int2& a, const Int2& b)
		{
			return Int2(a.x + b.x, a.y + b.y);
		}
		//subtract two vectors					  			 
		friend Int2 operator- (const Int2& a, const Int2& b)
		{
			return Int2(a.x - b.x, a.y - b.y);
		}
		//multiply the component of two vectors
		friend Int2 operator* (const Int2& a, const Int2& b)
		{
			return Int2(a.x * b.x, a.y * b.y);
		}
		//scale vector	
		friend Int2 operator* (const Int2& a, const int32_t& b)
		{
			return Int2(a.x * b, a.y * b);
		}
		//scale vector							  		    
		friend Int2 operator/ (const Int2& a, const int32_t& b)
		{
			return Int2(a.x / b, a.y / b);
		}
		//scale vector
		friend Int2 operator* (const int32_t& a, const Int2& b)
		{
			return Int2(a * b.x, a * b.y);
		}
		//scale vector									   
		friend Int2 operator/ (const int32_t& a, const Int2& b)
		{
			return Int2(a / b.x, a / b.y);
		}
		//invert vector
		friend Int2 operator- (const Int2& a)
		{
			return Int2(-a.x, -a.y);
		}

		//acces by index
		int32_t& operator[] (const int& i) { return cell[i]; }
		int32_t operator[] (const int& i) const { return cell[i]; }

		//boolean operators
		friend bool operator == (const Int2& a, const Int2& b)
		{
			return a.x == b.x && a.y == b.y;
		}
		friend bool operator != (const Int2& a, const Int2& b)
		{
			return a.x != b.x || a.y != b.y;
		}

		//equals arithmatic
		Int2& operator+= (const Int2& a)
		{
			x = x + a.x;
			y = y + a.y;
			return *this;
		}
		Int2& operator-= (const Int2& a)
		{
			x = x - a.x;
			y = y - a.y;
			return *this;
		}
		Int2& operator*= (const int32_t& a)
		{
			x = x * a;
			y = y * a;
			*this;
		}
		Int2& operator/= (const int32_t& a)
		{
			x = x / a;
			y = y / a;
			*this;
		}
	};
}// --> VMATH

static float vmath::Dot(const vmath::Vector2& a, const vmath::Vector2& b)
{
	return a.x * b.x + a.y * b.y;
}
static float vmath::Dot(const vmath::Vector3& a, const vmath::Vector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
static float vmath::Dot(const vmath::Vector4& a, const vmath::Vector4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static float vmath::Length(const vmath::Vector2& a)
{
	return sqrtf(Dot(a, a));
}
static float vmath::Length(const vmath::Vector3& a)
{
	return sqrtf(Dot(a, a));
}
static float vmath::Length(const vmath::Vector4& a)
{
	return sqrtf(Dot(a, a));
}
static float vmath::Length(const vmath::Quaternion& a)
{
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

static float vmath::LengthSqr(const vmath::Vector2& a)
{
	return Dot(a, a);
}
static float vmath::LengthSqr(const vmath::Vector3& a)
{
	return Dot(a, a);
}
static float vmath::LengthSqr(const vmath::Vector4& a)
{
	return Dot(a, a);
}
static float vmath::LengthSqr(const vmath::Quaternion& a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

static vmath::Vector2 vmath::Normalize(const vmath::Vector2& a)
{
	return a / Length(a);
}
static vmath::Vector3 vmath::Normalize(const vmath::Vector3& a)
{
	return a / Length(a);
}
static vmath::Vector4 vmath::Normalize(const vmath::Vector4& a)
{
	return a / Length(a);
}
static vmath::Quaternion vmath::Normalize(const vmath::Quaternion& a)
{
	return a / Length(a);
}

static vmath::Vector3 vmath::Cross(const vmath::Vector3& a, const vmath::Vector3& b)
{
	return Vector3(a.y * b.z, a.z * b.x, a.x * b.y) - Vector3(a.z * b.y, a.x * b.z, a.y * b.x);
}

static vmath::Matrix2 vmath::Transpose(const vmath::Matrix2& a)
{
	return Matrix2(a.x.x, a.y.x,
					a.x.y, a.y.y);
}
static vmath::Matrix3 vmath::Transpose(const vmath::Matrix3& a)
{
	return Matrix3(a.x.x, a.y.x, a.z.x,
					a.x.y, a.y.y, a.z.y,
					a.x.z, a.y.z, a.z.z);
}
static vmath::Matrix4 vmath::Transpose(const vmath::Matrix4& a)
{
	return Matrix4(a.x.x, a.y.x, a.z.x, a.w.x,
					a.x.y, a.y.y, a.z.y, a.w.y,
					a.x.z, a.y.z, a.z.z, a.w.z,
					a.x.w, a.y.w, a.z.w, a.w.w);
}

static float vmath::Determinant(const vmath::Matrix2& a)
{
	return (a.x.x*a.y.y) - (a.x.y*a.y.x);
}
static float vmath::Determinant(const vmath::Matrix3& a)
{
	return	(a.x.x*a.y.y*a.z.z) +
			(a.x.y*a.y.z*a.z.x) +
			(a.x.z*a.y.x*a.z.y) -
			(a.x.z*a.y.y*a.z.x) -
			(a.x.y*a.y.x*a.z.z) -
			(a.x.x*a.y.z*a.z.y);
}
static float vmath::Determinant(const vmath::Matrix4& a)
{
	return	a.x.x*Determinant(Matrix3(a.y.y, a.y.z, a.y.w, a.z.y, a.z.z, a.z.w, a.w.y, a.w.z, a.w.w)) -
			a.x.y*Determinant(Matrix3(a.y.x, a.y.z, a.y.w, a.z.x, a.z.z, a.z.w, a.w.x, a.w.z, a.w.w)) +
			a.x.z*Determinant(Matrix3(a.y.x, a.y.y, a.y.w, a.z.x, a.z.y, a.z.w, a.w.x, a.w.y, a.w.w)) -
			a.x.w*Determinant(Matrix3(a.y.x, a.y.y, a.y.z, a.z.x, a.z.y, a.z.z, a.w.x, a.w.y, a.w.z));
}

static vmath::Matrix3 vmath::Adjugate(const vmath::Matrix3& a)
{
	return Transpose(Matrix3( Determinant(Matrix2(a.y.y, a.y.z, a.z.y, a.z.z)),-Determinant(Matrix2(a.y.x, a.y.z, a.z.x, a.z.z)), Determinant(Matrix2(a.y.x, a.y.y, a.z.x, a.z.y)),
								-Determinant(Matrix2(a.x.y, a.x.z, a.z.y, a.z.z)), Determinant(Matrix2(a.x.x, a.x.z, a.z.x, a.z.z)),-Determinant(Matrix2(a.x.x, a.x.y, a.z.x, a.z.y)),
								Determinant(Matrix2(a.x.y, a.x.z, a.y.y, a.y.z)),-Determinant(Matrix2(a.x.x, a.x.z, a.y.x, a.y.z)), Determinant(Matrix2(a.x.x, a.x.y, a.y.x, a.y.y))));
}
static vmath::Matrix4 vmath::Adjugate(const vmath::Matrix4& a)
{
	Matrix4 m = (Matrix4( Determinant(Matrix3(a.y.y, a.y.z, a.y.w, a.z.y, a.z.z, a.z.w, a.w.y, a.w.z, a.w.w)),-Determinant(Matrix3(a.y.x, a.y.z, a.y.w, a.z.x, a.z.z, a.z.w, a.w.x, a.w.z, a.w.w)), Determinant(Matrix3(a.y.x, a.y.y, a.y.w, a.z.x, a.z.y, a.z.w, a.w.x, a.w.y, a.w.w)), -Determinant(Matrix3(a.y.x, a.y.y, a.y.z, a.z.x, a.z.y, a.z.z, a.w.x, a.w.y, a.w.z)),
						 -Determinant(Matrix3(a.x.y, a.x.z, a.x.w, a.z.y, a.z.z, a.z.w, a.w.y, a.w.z, a.w.w)), Determinant(Matrix3(a.x.x, a.x.z, a.x.w, a.z.x, a.z.z, a.z.w, a.w.x, a.w.z, a.w.w)),-Determinant(Matrix3(a.x.x, a.x.y, a.x.w, a.z.x, a.z.y, a.z.w, a.w.x, a.w.y, a.w.w)),  Determinant(Matrix3(a.x.x, a.x.y, a.x.z, a.z.x, a.z.y, a.z.z, a.w.x, a.w.y, a.w.z)),
						  Determinant(Matrix3(a.x.y, a.x.z, a.x.w, a.y.y, a.y.z, a.y.w, a.w.y, a.w.z, a.w.w)),-Determinant(Matrix3(a.x.x, a.x.z, a.x.w, a.y.x, a.y.z, a.y.w, a.w.x, a.w.z, a.w.w)), Determinant(Matrix3(a.x.x, a.x.y, a.x.w, a.y.x, a.y.y, a.y.w, a.w.x, a.w.y, a.w.w)), -Determinant(Matrix3(a.x.x, a.x.y, a.x.z, a.y.x, a.y.y, a.y.z, a.w.x, a.w.y, a.w.z)),
						 -Determinant(Matrix3(a.x.y, a.x.z, a.x.w, a.y.y, a.y.z, a.y.w, a.z.y, a.z.z, a.z.w)), Determinant(Matrix3(a.x.x, a.x.z, a.x.w, a.y.x, a.y.z, a.y.w, a.z.x, a.z.z, a.z.w)),-Determinant(Matrix3(a.x.x, a.x.y, a.x.w, a.y.x, a.y.y, a.y.w, a.z.x, a.z.y, a.z.w)),  Determinant(Matrix3(a.x.x, a.x.y, a.x.z, a.y.x, a.y.y, a.y.z, a.z.x, a.z.y, a.z.z))));
	return Transpose(m);
}

static vmath::Matrix3 vmath::Inverse(const vmath::Matrix3& a)
{
	return 1.0f / Determinant(a) * Adjugate(a);
}
static vmath::Matrix4 vmath::Inverse(const vmath::Matrix4& a)
{
	float d = Determinant(a);
	if (d != 0) 
	{ 
		return (1 / d) * Adjugate(a); 
	}
	else 
	{ 
		assert(false && "Matrix in not invertible");
		return Matrix4();
	}
}
static vmath::Quaternion vmath::Inverse(const vmath::Quaternion& a)
{
	return Conjugate(a) / Norm(a);
}

static float vmath::Trace(const vmath::Matrix3& a)
{
	return a.x.x + a.y.y + a.z.z;
}
static float vmath::Trace(const vmath::Matrix4& a)
{
	return a.x.x + a.y.y + a.z.z + a.w.w;
}

static float vmath::Norm(vmath::Quaternion a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}
static vmath::Quaternion vmath::Conjugate(vmath::Quaternion a)
{
	return Quaternion(-a.x, -a.y, -a.z, a.w);
}
static float vmath::Quadrant(const vmath::Quaternion& pQ)
{
	return (pQ.w*pQ.w + pQ.x*pQ.x + pQ.y*pQ.y + pQ.z*pQ.z);
}

static int vmath::Clamp(int pValue, int pMin, int pMax)
{
	return VMATH_MIN(VMATH_MAX(pValue, pMin), pMax);
}
static float vmath::Clamp(float pValue, float pMin, float pMax)
{
	return VMATH_MIN(VMATH_MAX(pValue, pMin), pMax);
}
static vmath::Vector3 vmath::Clamp(const vmath::Vector3& pValue, float pMin, float pMax)
{
	float clampedX = Clamp(pValue.x, pMin, pMax);
	float clampedY = Clamp(pValue.y, pMin, pMax);
	float clampedZ = Clamp(pValue.z, pMin, pMax);

	return Vector3(clampedX, clampedY, clampedZ);
}
static vmath::Vector4 vmath::Clamp(const vmath::Vector4& pValue, float pMin, float pMax)
{
	float clampedX = Clamp(pValue.x, pMin, pMax);
	float clampedY = Clamp(pValue.y, pMin, pMax);
	float clampedZ = Clamp(pValue.z, pMin, pMax);
	float clampedW = Clamp(pValue.w, pMin, pMax);

	return Vector4(clampedX, clampedY, clampedZ, clampedW);
}
static float vmath::Saturate(float pValue)
{
	return Clamp(pValue, 1.0f, 0.0f);
}

static vmath::Matrix3 vmath::QuaternionToRotationMatrix(const vmath::Quaternion& pQ)
{
	float w = pQ.w;
	float x = pQ.x;
	float y = pQ.y;
	float z = pQ.z;
	float s = (Quadrant(pQ) == 0 ? 0 : 2 / Quadrant(pQ));
	float wx = s * w * x;
	float wy = s * w * y;
	float wz = s * w * z;
	float xx = s * x * x;
	float xy = s * x * y;
	float xz = s * x * z;
	float yy = s * y * y;
	float yz = s * y * z;
	float zz = s * z * z;

	return Matrix3(1 - (yy + zz), xy + wz, xz - wy,
					xy - wz, 1 - (xx + zz), yz + wx,
					xz + wy, yz - wx, 1 - (xx + yy));
}
static vmath::Matrix4 vmath::QuaternionToMatrix(const vmath::Quaternion& pQ)
{
	float w = pQ.w;
	float x = pQ.x;
	float y = pQ.y;
	float z = pQ.z;
	float s = (Quadrant(pQ) == 0 ? 0 : 2 / Quadrant(pQ));
	float wx = s * w * x;
	float wy = s * w * y;
	float wz = s * w * z;
	float xx = s * x * x;
	float xy = s * x * y;
	float xz = s * x * z;
	float yy = s * y * y;
	float yz = s * y * z;
	float zz = s * z * z;

	return Matrix4(1 - (yy + zz), xy + wz, xz - wy, 0,
					xy - wz, 1 - (xx + zz), yz + wx, 0,
					xz + wy, yz - wx, 1 - (xx + yy), 0,
					0, 0, 0, 1.0f);
}
static vmath::Quaternion vmath::MatrixToQuaternion(const vmath::Matrix3& pM)
{
	float trace = Trace(pM);
	float qw, qx, qy, qz;
	if (trace > 0)
	{
		float S = sqrtf(trace + 1.0f) * 2.0f; // S=4*qw 
		qw = 0.25f * S;
		qx = (pM.z.y - pM.y.z) / S;
		qy = (pM.x.z - pM.z.x) / S;
		qz = (pM.y.x - pM.x.y) / S;
	}
	else if ((pM.x.x > pM.y.y)&(pM.x.x > pM.z.z))
	{
		float S = sqrtf(1.0f + pM.x.x - pM.y.y - pM.z.z) * 2.0f; // S=4*qx 
		qw = (pM.z.y - pM.y.z) / S;
		qx = 0.25f * S;
		qy = (pM.x.y + pM.y.x) / S;
		qz = (pM.x.z + pM.z.x) / S;
	}
	else if (pM.y.y > pM.z.z)
	{
		float S = sqrtf(1.0f + pM.y.y - pM.x.x - pM.z.z) * 2.0f; // S=4*qy
		qw = (pM.x.z - pM.z.x) / S;
		qx = (pM.x.y + pM.y.x) / S;
		qy = 0.25f * S;
		qz = (pM.y.z + pM.z.y) / S;
	}
	else 
	{
		float S = sqrtf(1.0f + pM.z.z - pM.x.x - pM.y.y) * 2.0f; // S=4*qz
		qw = (pM.y.x - pM.x.y) / S;
		qx = (pM.x.z + pM.z.x) / S;
		qy = (pM.y.z + pM.z.y) / S;
		qz = 0.25f * S;
	}

	return Quaternion(qx, qy, qz, qw);
}

/*
vmath::Vector2& vmath::Vector2::operator= (const vmath::Vector3& a)
{
	x = a.x;
	y = a.y;
	return *this;
}
vmath::Vector2& vmath::Vector2::operator= (const vmath::Vector4& a)
{
	x = a.x;
	y = a.y;
	return *this;
}
vmath::Matrix2& vmath::Matrix2::operator= (const vmath::Matrix3& a)
{
	x = a.x;
	y = a.y;
	return *this;
}
vmath::Matrix2& vmath::Matrix2::operator= (const vmath::Matrix4& a)
{
	x = a.x;
	y = a.y;
	return *this;
}
vmath::Vector3& vmath::Vector3::operator= (const vmath::Vector4& a)
{
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}
vmath::Matrix3& vmath::Matrix3::operator= (const vmath::Matrix4& a)
{
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}
*/
#pragma once

#include <cmath>
#include <iostream>
#include <cstdlib> 
#include <cstdint>

namespace Visor
{
	template<typename T>
	struct Matrix4;

	template<typename T>
	struct Matrix3;

	template<typename T>
	struct Vector3;

	// ===== Matrix4 =====
	template<typename T>
	struct Matrix4
	{
	public:
		void transpose();
		Matrix3<T> getUpperLeft() const;
		
		Matrix4<T> operator*(const Matrix4<T>& matrix) const;

		static Matrix4<T> getIdentity();
		static Matrix4<T> getScaling(T scalingX, T scalingY, T scalingZ);
		static Matrix4<T> getRotation(T yaw, T pitch, T roll);
		static Matrix4<T> getInverseRotation(T yaw, T pitch, T roll);
		static Matrix4<T> getTranslation(const Vector3<T>& position);
		static Matrix4<T> getView(const Vector3<T>& position, T yaw, T pitch, T roll);
		static Matrix4<T> getProjection(T fov, T aspectRatio);
		
	public:
		T m[4][4];
	};

	template<typename T>
	void Matrix4<T>::transpose()
	{
		for (uint32_t row = 0; row < 4; ++row)
		{
			for (uint32_t column = row + 1; column < 4; ++column)
			{
				const T tmp = m[row][column];
				m[row][column] = m[column][row];
				m[column][row] = tmp;
			}
		}
	}

	template<typename T>
	Matrix3<T> Matrix4<T>::getUpperLeft() const
	{
		Matrix3<T> result = {};

		for (uint32_t row = 0; row < 3; ++row)
		{
			for (uint32_t column = 0; column < 3; ++column)
			{
				result.m[row][column] = m[row][column];
			}
		}

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::operator*(const Matrix4<T>& matrix) const
	{
		Matrix4<T> result = {};

		for (uint32_t row = 0; row < 4; ++row)
		{
			for (uint32_t column = 0; column < 4; ++column)
			{
				result.m[row][column] = T(0);
				for (uint32_t k = 0; k < 4; ++k)
				{
					result.m[row][column] += m[row][k] * matrix.m[k][column];
				}
			}
		}

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getIdentity()
	{
		Matrix4<T> result = {};

		for (uint32_t i = 0; i < 4; ++i)
		{
			result.m[i][i] = T(1);
		}

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getScaling(T scalingX, T scalingY, T scalingZ)
	{
		Matrix4<T> result = {};

		result.m[0][0] = scalingX;
		result.m[1][1] = scalingY;
		result.m[2][2] = scalingZ;
		result.m[3][3] = T(1);

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getRotation(T yaw, T pitch, T roll)
	{
		// idea : A * B = take the world space transform made by B and apply A -> new transform in world space
		// hence roll then pitch then yaw (yaw * pitch * roll)
		// it wouldn't work otherwise or would "deform" the expected rotations in weird ways

		Matrix4<T> result = {};

		const T cosy = std::cos(yaw);
		const T siny = std::sin(yaw);
		const T cosp = std::cos(pitch);
		const T sinp = std::sin(pitch);
		const T cosr = std::cos(roll);
		const T sinr = std::sin(roll);

		result.m[0][0] = sinp * sinr * siny + cosr * cosy;
		result.m[0][1] = cosr * sinp * siny - cosy * sinr;
		result.m[0][2] = -cosp * siny;
		result.m[0][3] = T(0);

		result.m[1][0] = cosp * sinr;
		result.m[1][1] = cosp * cosr;
		result.m[1][2] = sinp;
		result.m[1][3] = T(0);

		result.m[2][0] = -cosy * sinp * sinr + cosr * siny;
		result.m[2][1] = -cosr * cosy * sinp - sinr * siny;
		result.m[2][2] = cosp * cosy;
		result.m[2][3] = T(0);

		result.m[3][0] = T(0);
		result.m[3][1] = T(0);
		result.m[3][2] = T(0);
		result.m[3][3] = T(1);

		return result;
	}
	template<typename T>
	Matrix4<T> Matrix4<T>::getInverseRotation(T yaw, T pitch, T roll)
	{
		// same idea as rotation, but we need to "undo" each transformation/rotation
		// so we apply each rotation in reverse yaw then pitch then roll (roll * pitch * yaw) but with a negative angle

		Matrix4<T> result = {};

		const T cosy = std::cos(-yaw);
		const T siny = std::sin(-yaw);
		const T cosp = std::cos(-pitch);
		const T sinp = std::sin(-pitch);
		const T cosr = std::cos(-roll);
		const T sinr = std::sin(-roll);

		result.m[0][0] = -sinp * sinr * siny + cosr * cosy;
		result.m[0][1] = -cosp * sinr;
		result.m[0][2] = -cosy * sinp * sinr - cosr * siny;
		result.m[0][3] = T(0);

		result.m[1][0] = cosr * sinp * siny + cosy * sinr;
		result.m[1][1] = cosp * cosr;
		result.m[1][2] = cosr * cosy * sinp - sinr * siny;
		result.m[1][3] = T(0);

		result.m[2][0] = cosp * siny;
		result.m[2][1] = -sinp;
		result.m[2][2] = cosp * cosy;
		result.m[2][3] = T(0);

		result.m[3][0] = T(0);
		result.m[3][1] = T(0);
		result.m[3][2] = T(0);
		result.m[3][3] = T(1);

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getTranslation(const Vector3<T>& position)
	{
		Matrix4<T> result = getIdentity();

		result.m[0][3] = position.x;
		result.m[1][3] = position.y;
		result.m[2][3] = position.z;

		return result;
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getView(const Vector3<T>& position, T yaw, T pitch, T roll)
	{
		Matrix4<T> inverseRotation = getInverseRotation(yaw, pitch, roll);

		Vector3<T> negativePosition = {
			-position.x,
			-position.y,
			-position.z,
		};

		return inverseRotation * getTranslation(negativePosition);
	}

	template<typename T>
	Matrix4<T> Matrix4<T>::getProjection(T fov, T aspectRatio)
	{
		Matrix4<T> result = {};

		const T invTanHalfFov = T(1) / std::tan(fov / T(2));

		const T zNear = T(1);
		const T zFar = T(10000);

		result.m[0][0] = invTanHalfFov / aspectRatio;
		result.m[1][1] = invTanHalfFov;
		result.m[2][2] = T(1);
		result.m[2][3] = -zNear;
		result.m[3][2] = T(1);

		return result;
	}

	// ===== Matrix3 =====
	template<typename T>
	struct Matrix3
	{
	public:
		Matrix3<T> operator*(const Matrix3<T>& matrix) const;
		Vector3<T> operator*(const Vector3<T>& vector) const;

	public:
		T m[3][3];
	};

	template<typename T>
	Matrix3<T> Matrix3<T>::operator*(const Matrix3<T>& matrix) const
	{
		Matrix3<T> result = {};

		for (uint32_t row = 0; row < 3; ++row)
		{
			for (uint32_t column = 0; column < 3; ++column)
			{
				result.m[row][column] = T(0);
				for (uint32_t k = 0; k < 3; ++k)
				{
					result.m[row][column] += m[row][k] * matrix.m[k][column];
				}
			}
		}

		return result;
	}

	template<typename T>
	Vector3<T> Matrix3<T>::operator*(const Vector3<T>& vector) const
	{
		Vector3<T> result = {};

		result.x = m[0][0] * vector.x + m[0][1] * vector.y + m[0][2] * vector.z;
		result.y = m[1][0] * vector.x + m[1][1] * vector.y + m[1][2] * vector.z;
		result.z = m[2][0] * vector.x + m[2][1] * vector.y + m[2][2] * vector.z;

		return result;
	}

	// ===== Vector4 =====
	template<typename T>
	struct Vector4
	{
	public:
		T x;
		T y;
		T z;
		T w;
	};


	// ===== Vector3 =====
	template<typename T>
	struct Vector3
	{
	public:
		T x;
		T y;
		T z;

	public:
		Vector3<T> operator-() const;
		Vector3<T> operator+(const Vector3<T>& vector) const;
		Vector3<T> operator-(const Vector3<T>& vector) const;
		Vector3<T> operator+(T scalar) const;
		Vector3<T> operator-(T scalar) const;
		Vector3<T> operator*(T scalar) const;
		Vector3<T> operator/(T scalar) const;
		bool operator==(const Vector3<T>& vector) const;
		bool operator!=(const Vector3<T>& vector) const;
		T& operator[](uint32_t index);
		const T& operator[](uint32_t index) const;
		
		void normalize();
		T dot(const Vector3<T>& vector) const;
		Vector3<T> cross(const Vector3<T>& vector) const;

		T getNorm2() const;
		T getNorm() const;
	};

	template<typename T>
	Vector3<T> Vector3<T>::operator-() const
	{
		return Vector3<T>{
			-x,
			-y,
			-z 
		};
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator+(const Vector3<T>& vector) const
	{
		Vector3<T> result = {};
		
		result.x = x + vector.x;
		result.y = y + vector.y;
		result.z = z + vector.z;

		return result;
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator-(const Vector3<T>& vector) const
	{
		Vector3<T> result = {};

		result.x = x - vector.x;
		result.y = y - vector.y;
		result.z = z - vector.z;

		return result;
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator+(T scalar) const
	{
		Vector3<T> result = {};

		result.x = x + scalar;
		result.y = y + scalar;
		result.z = z + scalar;

		return result;
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator-(T scalar) const
	{
		Vector3<T> result = {};

		result.x = x - scalar;
		result.y = y - scalar;
		result.z = z - scalar;

		return result;
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator*(T scalar) const
	{
		Vector3<T> result = {};

		result.x = x * scalar;
		result.y = y * scalar;
		result.z = z * scalar;

		return result;
	}

	template<typename T>
	Vector3<T> Vector3<T>::operator/(T scalar) const
	{
		Vector3<T> result = {};

		result.x = x / scalar;
		result.y = y / scalar;
		result.z = z / scalar;

		return result;
	}

	template<typename T>
	bool Vector3<T>::operator==(const Vector3<T>& vector) const
	{
		return x == vector.x && y == vector.y && z == vector.z;
	}

	template<typename T>
	bool Vector3<T>::operator!=(const Vector3<T>& vector) const
	{
		return !(*this == vector);
	}

	template<typename T>
	T& Vector3<T>::operator[](uint32_t index)
	{
		switch (index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default: {
			std::cerr << "invalid access index\n";
			std::exit(EXIT_FAILURE);
		}
		}
	}

	template<typename T>
	const T& Vector3<T>::operator[](uint32_t index) const
	{
		switch (index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default: {
			std::cerr << "invalid access index\n";
			std::exit(EXIT_FAILURE);
		}
		}
	}

	template<typename T>
	void Vector3<T>::normalize()
	{
		const T norm = getNorm();

		if (norm == T(0))
		{
			std::cerr << "normalizing a null vector\n";
			std::exit(EXIT_FAILURE);
		}

		x = x / norm;
		y = y / norm;
		z = z / norm;
	}

	template<typename T>
	T Vector3<T>::dot(const Vector3<T>& vector) const
	{
		return (x * vector.x) + (y * vector.y) + (z * vector.z);
	}

	template<typename T>
	Vector3<T> Vector3<T>::cross(const Vector3<T>& vector) const
	{
		Vector3<T> result = {};

		result.x = y * vector.z - z * vector.y;
		result.y = z * vector.x - x * vector.z;
		result.z = x * vector.y - y * vector.x;

		return result;
	}

	template<typename T>
	T Vector3<T>::getNorm2() const
	{
		return (x * x) + (y * y) + (z * z);
	}

	template<typename T>
	T Vector3<T>::getNorm() const
	{
		return std::sqrt(getNorm2());
	}
}
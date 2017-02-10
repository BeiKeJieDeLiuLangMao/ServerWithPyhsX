/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef NI_COF44_H
#define NI_COF44_H

#include "NxApex.h"
#include "foundation/PxMat44.h"
#include "foundation/PxPlane.h"

namespace physx
{
namespace apex
{

/**
\brief Stores the info needed for the cofactor matrix of a 4x4 homogeneous transformation matrix (implicit last row = 0 0 0 1)
*/
class NiCof44
{
public:
	/**
	\param [in] m can be an arbitrary homogeneoous transformation matrix
	*/
	NiCof44(const PxMat44& m)
	{
		_block33(0, 0) = m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1);
		_block33(0, 1) = m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2);
		_block33(0, 2) = m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0);
		_block33(1, 0) = m(2, 1) * m(0, 2) - m(2, 2) * m(0, 1);
		_block33(1, 1) = m(2, 2) * m(0, 0) - m(2, 0) * m(0, 2);
		_block33(1, 2) = m(2, 0) * m(0, 1) - m(2, 1) * m(0, 0);
		_block33(2, 0) = m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1);
		_block33(2, 1) = m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2);
		_block33(2, 2) = m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
		_block13 = _block33.transformTranspose(-m.getPosition());
		_44 = _block33(0, 0) * m(0, 0) + _block33(0, 1) * m(0, 1) + _block33(0, 2) * m(0, 2);

		if (_44 < 0)
		{
			// det is < 0, we need to negate all values
			// The Cov Matrix divided by the determinant is the same as the inverse transposed of an affine transformation
			// For rotation normals, dividing by the determinant is useless as it gets renormalized afterwards again.
			// If the determinant is negative though, it is important that all values are negated to get the right results.
			_block33 *= -1;
			_block13 *= -1;
			_44 = -_44;
		}
	}

	/**
	\param [in] rt must be pure (proper) rotation and translation
	\param [in] s is any diagonal matrix (typically scale).
	\note The combined transform is assumed to be (rt)*s, i.e. s is applied first
	*/
	NiCof44(const PxMat44& rt, const PxVec3 s)
	{
		const PxVec3 cofS(s.y * s.z, s.z * s.x, s.x * s.y);
		_block33(0, 0) = rt(0, 0) * cofS.x;
		_block33(0, 1) = rt(0, 1) * cofS.y;
		_block33(0, 2) = rt(0, 2) * cofS.z;
		_block33(1, 0) = rt(1, 0) * cofS.x;
		_block33(1, 1) = rt(1, 1) * cofS.y;
		_block33(1, 2) = rt(1, 2) * cofS.z;
		_block33(2, 0) = rt(2, 0) * cofS.x;
		_block33(2, 1) = rt(2, 1) * cofS.y;
		_block33(2, 2) = rt(2, 2) * cofS.z;
		_block13 = _block33.transformTranspose(-rt.getPosition());
		_44 = cofS.x * s.x;

		if (_44 < 0)
		{
			// det is < 0, we need to negate all values, see above
			_block33 *= -1;
			_block13 *= -1;
			_44 = -_44;
		}
	}

	/**
	\brief Transforms a plane equation correctly even when the transformation is not a rotation
	\note If the transformation is not a rotation then the length of the plane's normal vector is not preserved in general.
	*/
	void transform(const physx::PxPlane& src, physx::PxPlane& dst) const
	{
		dst.n = _block33.transform(src.n);
		dst.d = (_block13.dot(src.n)) + _44 * src.d;
	}

	/**
	\brief Transforms a normal correctly even when the transformation is not a rotation
	\note If the transformation is not a rotation then the normal's length is not preserved in general.
	*/
	const PxMat33&	getBlock33() const
	{
		return _block33;
	}

	/**
	\brief The determinant of the original matrix.
	*/
	PxF32			getDeterminant() const
	{
		return _44;
	}

private:
	PxMat33	_block33;
	PxVec3	_block13;
	PxF32	_44;
};

}
} // end namespace physx::apex

#endif
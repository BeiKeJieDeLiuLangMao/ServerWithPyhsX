/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "MeshPainter.h"

#ifdef PX_WINDOWS

#include <PsMathUtils.h>
#include <NxClothingPhysicalMesh.h>
#include <NxApexRenderDebug.h>

#include <PxMat34Legacy.h>

#include <algorithm>
#include <assert.h>

namespace SharedTools
{
struct TetraFace
{
	void set(int _v0, int _v1, int _v2)
	{
		v0 = _v0;
		v1 = _v1;
		v2 = _v2;
		flipped = false;
		if (v0 > v1)
		{
			int v = v0;
			v0 = v1;
			v1 = v;
			flipped = !flipped;
		}
		if (v1 > v2)
		{
			int v = v1;
			v1 = v2;
			v2 = v;
			flipped = !flipped;
		}
		if (v0 > v1)
		{
			int v = v0;
			v0 = v1;
			v1 = v;
			flipped = !flipped;
		}
	}
	bool operator==(const TetraFace& f) const
	{
		return v0 == f.v0 && v1 == f.v1 && v2 == f.v2;
	}
	bool operator < (const TetraFace& f) const
	{
		if (v0 < f.v0)
		{
			return true;
		}
		if (v0 > f.v0)
		{
			return false;
		}
		if (v1 < f.v1)
		{
			return true;
		}
		if (v1 > f.v1)
		{
			return false;
		}
		return v2 < f.v2;
	}
	int v0, v1, v2;
	bool flipped;
};

struct Edge
{
	void set(int v0, int v1, int triNr, int faceNr)
	{
		if (v0 < v1)
		{
			this->v0 = v0;
			this->v1 = v1;
		}
		else
		{
			this->v0 = v1;
			this->v1 = v0;
		}
		this->triNr = triNr;
		this->faceNr = faceNr;
	}
	bool operator==(const Edge& f) const
	{
		return v0 == f.v0 && v1 == f.v1;
	}
	bool operator < (const Edge& f) const
	{
		if (v0 < f.v0)
		{
			return true;
		}
		if (v0 > f.v0)
		{
			return false;
		}
		return v1 < f.v1;
	}
	int v0, v1;
	int triNr;
	int faceNr;
};


MeshPainter::MeshPainter()
{
	clear();
}



MeshPainter::~MeshPainter()
{
	clear();
}



void MeshPainter::clear()
{
	mCurrentMark = 0;
	mTriMarks.clear();
	mVertices.clear();
	mIndices.clear();
	mIndexRanges.clear();
	mNeighbors.clear();
	mNormals.clear();
	mTetraNormals.clear();
	mCollectedTriangles.clear();

	for (unsigned i = 0; i < mFloatBuffers.size(); i++)
	{
		if (mFloatBuffers[i].buffer != NULL && mFloatBuffers[i].allocated)
		{
			delete mFloatBuffers[i].buffer;
		}
	}
	mFloatBuffers.clear();

	mFlagBuffers.clear();

	mPaintRadius = 0.0f;
	mBrushMode = 0;
	mFalloffExponent = 1.0f;
	mTargetValue = 0.8f;
	mScaledTargetValue = 0.8f;

	mLastTriangle = -1;
	mRayOrig = physx::PxVec3(0.0f);
	mRayDir = physx::PxVec3(0.0f);
	mLastRaycastNormal = physx::PxVec3(0.0f);
}



PaintFloatBuffer& MeshPainter::getInternalFloatBuffer(unsigned int id)
{
	for (unsigned i = 0; i < mFloatBuffers.size(); i++)
		if (mFloatBuffers[i].id == id)
		{
			return mFloatBuffers[i];
		}

	mFloatBuffers.resize(mFloatBuffers.size() + 1);
	return mFloatBuffers[mFloatBuffers.size() - 1];
}



PaintFlagBuffer& MeshPainter::getInternalFlagBuffer(unsigned int id)
{
	for (unsigned i = 0; i < mFlagBuffers.size(); i++)
		if (mFlagBuffers[i].id == id)
		{
			return mFlagBuffers[i];
		}

	mFlagBuffers.resize(mFlagBuffers.size() + 1);
	return mFlagBuffers[mFlagBuffers.size() - 1];
}



/*void MeshPainter::allocateFloatBuffer(physx::PxU32 id)
{
	PaintFloatBuffer& fb = getInternalFloatBuffer(id);
	fb.id = id;
	if (fb.buffer) delete fb.buffer;
	fb.buffer = (float*)malloc(mVertices.size() * sizeof(float));
	fb.stride = sizeof(float);
	fb.allocated = true;
}*/



void MeshPainter::clearIndexBufferRange()
{
	mIndexRanges.clear();
}



void MeshPainter::addIndexBufferRange(physx::PxU32 start, physx::PxU32 end)
{
	PX_ASSERT(start < mIndices.size());
	PX_ASSERT(end <= mIndices.size());
	PX_ASSERT((end - start) % 3 == 0);

	IndexBufferRange range;
	range.start = start;
	range.end = end;

	for (physx::PxU32 i = 0; i < mIndexRanges.size(); i++)
	{
		PX_ASSERT(!mIndexRanges[i].isOverlapping(range));
		if (mIndexRanges[i].isOverlapping(range))
		{
			return;    // do not add overlapping ranges!
		}
	}

	mIndexRanges.push_back(range);

	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		mVerticesDisabled[i] = true;
	}

	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		IndexBufferRange range = mIndexRanges[r];
		for (physx::PxU32 i = range.start; i < range.end; i++)
		{
			mVerticesDisabled[mIndices[i]] = false;
		}
	}
}



void MeshPainter::setFloatBuffer(unsigned int id, float* buffer, int stride)
{
	PaintFloatBuffer& fb = getInternalFloatBuffer(id);
	fb.id = id;
	fb.buffer = buffer;
	fb.stride = stride;
	fb.allocated = false;
}



void MeshPainter::setFlagBuffer(unsigned int id, unsigned int* buffer, int stride)
{
	PaintFlagBuffer& fb = getInternalFlagBuffer(id);
	fb.id = id;
	fb.buffer = buffer;
	fb.stride = stride;
}



void* MeshPainter::getFloatBuffer(physx::PxU32 id)
{
	for (unsigned i = 0; i < mFloatBuffers.size(); i++)
		if (mFloatBuffers[i].id == id)
		{
			return mFloatBuffers[i].buffer;
		}
	return NULL;
}



void MeshPainter::initFrom(const physx::apex::NxClothingPhysicalMesh* mesh)
{
	clear();

	if (mesh->isTetrahedralMesh())
	{
		static const int faceIndices[4][3] = {{1, 2, 3}, {2, 0, 3}, {0, 1, 3}, {2, 1, 0}};

		unsigned numVerts = mesh->getNumVertices();
		mVertices.resize(numVerts);
		mVerticesDisabled.resize(numVerts, false);
		mesh->getVertices(&mVertices[0], sizeof(physx::PxVec3));
		mTetraNormals.resize(numVerts);
		mesh->getNormals(&mTetraNormals[0], sizeof(physx::PxVec3));

		// extract surface triangle mesh
		std::vector<physx::PxU32> tetIndices;
		unsigned numIndices = mesh->getNumIndices();
		tetIndices.resize(numIndices);
		mesh->getIndices(&tetIndices[0], sizeof(physx::PxU32));

		std::vector<TetraFace> faces;
		TetraFace face;

		for (unsigned i = 0; i < numIndices; i += 4)
		{
			for (unsigned j = 0; j < 4; j++)
			{
				face.set(
				    (int)tetIndices[i + (unsigned)faceIndices[j][0]],
				    (int)tetIndices[i + (unsigned)faceIndices[j][1]],
				    (int)tetIndices[i + (unsigned)faceIndices[j][2]]
				);
				faces.push_back(face);
			}
		}

		std::sort(faces.begin(), faces.end());

		physx::PxU32 numFaces = (physx::PxU32)faces.size();
		physx::PxU32 i = 0;

		while (i < numFaces)
		{
			TetraFace& f = faces[i];
			i++;
			if (i < numFaces && !(faces[i] == f))
			{
				if (f.flipped)
				{
					mIndices.push_back((physx::PxU32)f.v0);
					mIndices.push_back((physx::PxU32)f.v2);
					mIndices.push_back((physx::PxU32)f.v1);
				}
				else
				{
					mIndices.push_back((physx::PxU32)f.v0);
					mIndices.push_back((physx::PxU32)f.v1);
					mIndices.push_back((physx::PxU32)f.v2);
				}
			}
			else
			{
				while (i < numFaces && faces[i] == f)
				{
					i++;
				}
			}
		}
	}
	else
	{
		unsigned numVerts = mesh->getNumVertices();
		mVertices.resize(numVerts);
		mVerticesDisabled.resize(numVerts, false);
		mesh->getVertices(&mVertices[0], sizeof(physx::PxVec3));

		unsigned numIndices = mesh->getNumIndices();
		mIndices.resize(numIndices);
		mesh->getIndices(&mIndices[0], sizeof(physx::PxU32));
	}

	clearIndexBufferRange();
	addIndexBufferRange(0, (physx::PxU32)mIndices.size());
	complete();
}



void MeshPainter::initFrom(const physx::PxVec3* vertices, int numVertices, int vertexStride, const physx::PxU32* indices, int numIndices, int indexStride)
{
	clear();

	mVertices.resize((unsigned)numVertices);
	mVerticesDisabled.resize((unsigned)numVertices, false);
	const physx::PxU8* p = (physx::PxU8*)vertices;
	for (int i = 0; i < numVertices; i++, p += vertexStride)
	{
		mVertices[(unsigned)i] = *((physx::PxVec3*)p);
	}

	mIndices.resize((unsigned)numIndices);
	p = (physx::PxU8*)indices;
	for (int i = 0; i < numIndices; i++, p += indexStride)
	{
		mIndices[(unsigned)i] = *((physx::PxU32*)p);
	}

	clearIndexBufferRange();
	addIndexBufferRange(0, (physx::PxU32)mIndices.size());
	complete();
}



void MeshPainter::complete()
{
	computeNormals();
	createNeighborInfo();
	mTriMarks.resize(mIndices.size() / 3, 0);
	computeSiblingInfo(0.0f);
}



void MeshPainter::computeNormals()
{
	const physx::PxU32 numVerts = (physx::PxU32)mVertices.size();
	mNormals.resize(numVerts);
	memset(&mNormals[0], 0, sizeof(physx::PxVec3) * numVerts);

	for (physx::PxU32 i = 0; i < mIndices.size(); i += 3)
	{
		const physx::PxU32 i0 = mIndices[i + 0];
		const physx::PxU32 i1 = mIndices[i + 1];
		const physx::PxU32 i2 = mIndices[i + 2];
		physx::PxVec3 n = (mVertices[i1] - mVertices[i0]).cross(mVertices[i2] - mVertices[i0]);
		mNormals[i0] += n;
		mNormals[i1] += n;
		mNormals[i2] += n;
	}

	for (physx::PxU32 i = 0; i < numVerts; i++)
	{
		mNormals[i].normalize();
	}
}



void MeshPainter::createNeighborInfo()
{
	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);

	std::vector<Edge> edges;
	Edge edge;
	for (physx::PxU32 i = 0; i < mIndices.size(); i += 3)
	{
		for (physx::PxU32 j = 0; j < 3; j++)
		{
			edge.set((physx::PxI32)mIndices[i + j], (physx::PxI32)mIndices[i + (j + 1) % 3], (physx::PxI32)i / 3, (physx::PxI32)j);
			edges.push_back(edge);
		}
	}

	std::sort(edges.begin(), edges.end());

	physx::PxU32 numEdges = (physx::PxU32)edges.size();
	physx::PxU32 i = 0;
	while (i < numEdges)
	{
		Edge& e0 = edges[i];
		i++;
		if (i < numEdges && edges[i] == e0)
		{
			Edge& e1 = edges[i];
			mNeighbors[physx::PxU32(3 * e0.triNr + e0.faceNr)] = e1.triNr;
			mNeighbors[physx::PxU32(3 * e1.triNr + e1.faceNr)] = e0.triNr;
		}
		while (i < numEdges && edges[i] == e0)
		{
			i++;
		}
	}
}



bool MeshPainter::rayCast(int& triNr, float& t) const
{
	t = PX_MAX_F32;
	triNr = -1;
	mLastRaycastNormal = physx::PxVec3(0.0f);
	mLastRaycastPos = physx::PxVec3(0.0f);

	if (mRayDir.isZero())
	{
		return false;
	}

	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		const IndexBufferRange range = mIndexRanges[r];
		for (physx::PxU32 i = range.start; i < range.end; i += 3)
		{
			const physx::PxVec3& p0 = mVertices[mIndices[i]];
			const physx::PxVec3& p1 = mVertices[mIndices[i + 1]];
			const physx::PxVec3& p2 = mVertices[mIndices[i + 2]];
			float triT, u, v;

			bool hit = rayTriangleIntersection(mRayOrig, mRayDir, p0, p1, p2, triT, u, v);
			if (!hit)
			{
				continue;
			}
			if (triT < t)
			{
				t = triT;
				triNr = (physx::PxI32)i / 3;
				mLastRaycastNormal = (p1 - p0).cross(p2 - p0);
				mLastRaycastPos = mRayOrig + mRayDir * t;
			}
		}
	}
	// smooth normals would be nicer...
	mLastRaycastNormal.normalize();
	return triNr >= 0;
}



bool MeshPainter::rayTriangleIntersection(const physx::PxVec3& orig, const physx::PxVec3& dir,
        const physx::PxVec3& a, const physx::PxVec3& b, const physx::PxVec3& c,
        float& t, float& u, float& v) const
{
	physx::PxVec3 edge1, edge2, tvec, pvec, qvec;
	float det, inv_det;

	edge1 = b - a;
	edge2 = c - a;
	pvec = dir.cross(edge2);

	/* if determinant is near zero, ray lies in plane of triangle */
	det = edge1.dot(pvec);

	if (det == 0.0f)
	{
		return false;
	}
	inv_det = 1.0f / det;

	/* calculate distance from vert0 to ray origin */
	tvec = orig - a;

	/* calculate U parameter and test bounds */
	u = tvec.dot(pvec) * inv_det;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	/* prepare to test V parameter */
	qvec = tvec.cross(edge1);

	/* calculate V parameter and test bounds */
	v = dir.dot(qvec) * inv_det;
	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	/* calculate t, ray intersects triangle */
	t = edge2.dot(qvec) * inv_det;

	return true;
}



physx::PxVec3 MeshPainter::getTriangleCenter(int triNr) const
{
	const physx::PxVec3& p0 = mVertices[mIndices[3 * (physx::PxU32)triNr + 0]];
	const physx::PxVec3& p1 = mVertices[mIndices[3 * (physx::PxU32)triNr + 1]];
	const physx::PxVec3& p2 = mVertices[mIndices[3 * (physx::PxU32)triNr + 2]];
	return (p0 + p1 + p2) / 3.0f;
}



physx::PxVec3 MeshPainter::getTriangleNormal(int triNr) const
{
	const physx::PxVec3& p0 = mVertices[mIndices[3 * (physx::PxU32)triNr + 0]];
	const physx::PxVec3& p1 = mVertices[mIndices[3 * (physx::PxU32)triNr + 1]];
	const physx::PxVec3& p2 = mVertices[mIndices[3 * (physx::PxU32)triNr + 2]];
	physx::PxVec3 normal = (p1 - p0).cross(p2 - p0);
	normal.normalize();
	return normal;
}



void MeshPainter::collectTriangles() const
{
	// Dijkstra along the mesh
	mCollectedTriangles.clear();
	//int triNr;
	//float t;
	if (mLastTriangle == -1)
	{
		return;
	}

	//if (!rayCast(triNr, t))
	//	return;

	mCurrentMark++;
	std::vector<DistTriPair> heap;

	DistTriPair start;
	start.set(mLastTriangle, 0.0f);
	heap.push_back(start);

	while (!heap.empty())
	{
		std::pop_heap(heap.begin(), heap.end());

		DistTriPair dt = heap[heap.size() - 1];
		heap.pop_back();

		if (mTriMarks[(physx::PxU32)dt.triNr] == mCurrentMark)
		{
			continue;
		}

		mTriMarks[(physx::PxU32)dt.triNr] = mCurrentMark;
		mCollectedTriangles.push_back(dt);
		physx::PxVec3 center = getTriangleCenter(dt.triNr);

		for (physx::PxU32 i = 0; i < 3; i++)
		{
			int adjNr = mNeighbors[3 * (physx::PxU32)dt.triNr + i];
			if (!isValidRange(adjNr * 3))
			{
				continue;
			}

			if (mTriMarks[(physx::PxU32)adjNr] == mCurrentMark)
			{
				continue;
			}

			physx::PxVec3 adjCenter = getTriangleCenter(adjNr);
			DistTriPair adj;
			adj.set(adjNr, dt.dist - (center - adjCenter).magnitude());	// heap sorts large -> small, we need the opposite
			if (-adj.dist > mPaintRadius)
			{
				continue;
			}

			physx::PxVec3 adjNormal = getTriangleNormal(adjNr);

			// stop at 90 degrees
			const float angle = physx::PxCos(physx::degToRad(70.0f));
			if (adjNormal.dot(mLastRaycastNormal) < angle)
			{
				continue;
			}

			heap.push_back(adj);

			std::push_heap(heap.begin(), heap.end());

		}
	}
}



bool MeshPainter::isValidRange(int vertexNumber) const
{
	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		const IndexBufferRange& range = mIndexRanges[r];
		if ((vertexNumber >= (physx::PxI32)range.start) && (vertexNumber < (physx::PxI32)range.end))
		{
			return true;
		}
	}
	return false;
}



bool MeshPainter::IndexBufferRange::isOverlapping(const IndexBufferRange& other) const
{
	if (other.start >= end)
	{
		return false;
	}

	if (start >= other.end)
	{
		return false;
	}

	return true;
}



void MeshPainter::changeRadius(float paintRadius)
{
	mPaintRadius = paintRadius;
}



void MeshPainter::setRayAndRadius(const physx::PxVec3& rayOrig, const physx::PxVec3& rayDir, float paintRadius, int brushMode, float falloffExponent, float scaledTargetValue, float targetColor)
{
	float t;
	mRayOrig = rayOrig;
	mRayDir = rayDir;
	rayCast(mLastTriangle, t);
	mPaintRadius = paintRadius;
	mBrushMode = brushMode;
	mFalloffExponent = falloffExponent;
	mScaledTargetValue = scaledTargetValue;
	mBrushColor = targetColor;
}



void MeshPainter::paintFloat(unsigned int id, float min, float max, float target) const
{
	mTargetValue = target;

	if (mLastRaycastNormal.isZero() && mPaintRadius >= 0)
	{
		return;
	}

	if (!mLastRaycastNormal.isZero() && mPaintRadius <= 0)
	{
		return;
	}

	int bufferNr = -1;
	for (size_t i = 0; i < mFloatBuffers.size(); i++)
	{
		if (mFloatBuffers[i].id == id)
		{
			bufferNr = (int)i;
		}
	}

	if (bufferNr < 0)
	{
		return;
	}

	if (mBrushMode == 1)
	{
		// sphere painting
		// also used for flooding, with a negative radius
		const float paintRadius2 = mPaintRadius * mPaintRadius;
		physx::PxVec3 origProj = mRayOrig - mRayDir * mRayDir.dot(mRayOrig);

		const unsigned int numVertices = (unsigned int)mVertices.size();
		for (unsigned int i = 0; i < numVertices; i++)
		{
			if (mVerticesDisabled[i])
			{
				continue;
			}

			// negative radius means flood all vertices
			float dist2 = 0;
			if (mPaintRadius >= 0.0f)
			{
				// this is the sphere
				dist2 = (mLastRaycastPos - mVertices[i]).magnitudeSquared();

				if (dist2 > paintRadius2)
				{
					continue;
				}
			}

			float relative = 1.0f - (physx::PxSqrt(dist2) / mPaintRadius);
			PX_ASSERT(relative >= 0.0f);
			if (relative < 0)
			{
				continue;
			}
			PX_ASSERT(relative <= 1.0f);

			relative = physx::PxPow(relative, mFalloffExponent);

			float& f = mFloatBuffers[(physx::PxU32)bufferNr][i];
			f = physx::PxClamp(relative * target + (1.0f - relative) * f, min, max);
		}
	}
	else if (mBrushMode == 0)
	{
		// flat painting
		collectTriangles();
		mCollectedVertices.clear();
		for (physx::PxU32 i = 0; i < mCollectedTriangles.size(); i++)
		{
			DistTriPair& dt = mCollectedTriangles[i];
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				const physx::PxU32 index = mIndices[3 * (physx::PxI32)dt.triNr + j];
				bool found = false;
				for (physx::PxU32 k = 0; k < mCollectedVertices.size(); k++)
				{
					if (mCollectedVertices[k] == index)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					mCollectedVertices.push_back(index);
				}
			}
		}

		for (physx::PxU32 i = 0; i < mCollectedVertices.size(); i++)
		{
			float dist2 = (mLastRaycastPos - mVertices[mCollectedVertices[i]]).magnitudeSquared();
			float relative = 1.0f - (physx::PxSqrt(dist2) / mPaintRadius);
			if (relative < 0)
			{
				continue;
			}

			relative = physx::PxPow(relative, mFalloffExponent);

			float& f = mFloatBuffers[(physx::PxU32)bufferNr][(physx::PxI32)mCollectedVertices[i]];
			f = physx::PxClamp(relative * target + (1.0f - relative) * f, min, max);
		}
	}
	else
	{
		// smooth painting
		PX_ASSERT(mBrushMode == 2);
		collectTriangles();

		mSmoothingCollectedIndices.clear();

		mCollectedVertices.clear();
		for (physx::PxU32 i = 0; i < mCollectedTriangles.size(); i++)
		{
			DistTriPair& dt = mCollectedTriangles[i];
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				const physx::PxU32 index = mIndices[3 * (physx::PxU32)dt.triNr + j];
				bool found = false;
				for (physx::PxU32 k = 0; k < mCollectedVertices.size(); k++)
				{
					if (mCollectedVertices[k] == index)
					{
						mSmoothingCollectedIndices.push_back(k);
						found = true;
						break;
					}
				}
				if (!found)
				{
					mSmoothingCollectedIndices.push_back((physx::PxU32)mCollectedVertices.size());
					mCollectedVertices.push_back(index);
				}
			}
		}

		if (mCollectedVerticesFloats.size() < mCollectedVertices.size() * 2)
		{
			mCollectedVerticesFloats.resize(mCollectedVertices.size() * 2, 0.0f);
		}
		else
		{
			memset(&mCollectedVerticesFloats[0], 0, sizeof(float) * mCollectedVertices.size() * 2);
		}

		for (physx::PxU32 i = 0; i < mCollectedTriangles.size(); i++)
		{
			DistTriPair& dt = mCollectedTriangles[i];

			// generate the average
			float average = 0.0f;
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				const physx::PxU32 index = mIndices[3 * (physx::PxU32)dt.triNr + j];
				average += mFloatBuffers[(physx::PxU32)bufferNr][index];
			}
			average /= 3.0f;

			float relative = 1.0f - (dt.dist / mPaintRadius);

			if (relative < 0)
			{
				continue;
			}

			relative = physx::PxPow(relative, mFalloffExponent);

			for (physx::PxU32 j = 0; j < 3; j++)
			{
				const physx::PxU32 index = mIndices[3 * (physx::PxU32)dt.triNr + j];
				const float source = mFloatBuffers[(physx::PxU32)bufferNr][index];

				const physx::PxU32 targetIndex = mSmoothingCollectedIndices[i * 3 + j];
				mCollectedVerticesFloats[targetIndex * 2] += relative * average + (1.0f - relative) * source;
				mCollectedVerticesFloats[targetIndex * 2 + 1] += 1.0f;
			}
		}

		for (physx::PxU32 i = 0; i < mCollectedVertices.size(); i++)
		{
			if (mFloatBuffers[(physx::PxU32)bufferNr][mCollectedVertices[i]] >= 0.0f)
			{
				mFloatBuffers[(physx::PxU32)bufferNr][mCollectedVertices[i]] = mCollectedVerticesFloats[i * 2] / mCollectedVerticesFloats[i * 2 + 1];
			}
		}
	}
}




void MeshPainter::paintFlag(unsigned int id, unsigned int flag, bool useAND) const
{
	mTargetValue = 1.0f;
	PX_ASSERT(mFalloffExponent == 0.0f);

	if (mLastRaycastNormal.isZero() && mPaintRadius >= 0)
	{
		return;
	}

	if (!mLastRaycastNormal.isZero() && mPaintRadius <= 0)
	{
		return;
	}

	int bufferNr = -1;
	for (size_t i = 0; i < mFlagBuffers.size(); i++)
	{
		if (mFlagBuffers[i].id == id)
		{
			bufferNr = (int)i;
		}
	}

	if (bufferNr < 0)
	{
		return;
	}

	if (mBrushMode == 1)
	{
		// sphere painting
		const float paintRadius2 = mPaintRadius * mPaintRadius;
		physx::PxVec3 origProj = mRayOrig - mRayDir * mRayDir.dot(mRayOrig);

		for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		{
			if (mVerticesDisabled[i])
			{
				continue;
			}

			// negative radius means flood  all vertices
			float dist2 = 0;
			if (mPaintRadius >= 0.0f)
			{
				// this is the sphere
				dist2 = (mLastRaycastPos - mVertices[i]).magnitudeSquared();

				if (dist2 > paintRadius2)
				{
					continue;
				}
			}

			unsigned int& u = mFlagBuffers[(physx::PxU32)bufferNr][i];
			if (useAND)
			{
				u &= flag;
			}
			else
			{
				u |= flag;
			}
		}
	}
	else if (mBrushMode == 0)
	{
		// flat painting
		collectTriangles();
		mCollectedVertices.clear();
		for (physx::PxU32 i = 0; i < mCollectedTriangles.size(); i++)
		{
			DistTriPair& dt = mCollectedTriangles[i];
			for (int j = 0; j < 3; j++)
			{
				const physx::PxU32 index = mIndices[3 * (physx::PxU32)dt.triNr + j];
				bool found = false;
				for (size_t k = 0; k < mCollectedVertices.size(); k++)
				{
					if (mCollectedVertices[k] == index)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					mCollectedVertices.push_back(index);
				}
			}
		}

		const float paintRadius2 = mPaintRadius * mPaintRadius;

		for (physx::PxU32 i = 0; i < mCollectedVertices.size(); i++)
		{
			float dist2 = (mLastRaycastPos - mVertices[mCollectedVertices[i]]).magnitudeSquared();
			if (dist2 > paintRadius2)
			{
				continue;
			}

			unsigned int& u = mFlagBuffers[(physx::PxU32)bufferNr][mCollectedVertices[i]];
			if (useAND)
			{
				u &= flag;
			}
			else
			{
				u |= flag;
			}
		}
	}
}



void MeshPainter::smoothFloat(physx::PxU32 id, float smoothingFactor, physx::PxU32 numIterations) const
{
	int bufferNr = -1;
	for (physx::PxU32 i = 0; i < mFloatBuffers.size(); i++)
		if (mFloatBuffers[i].id == id)
		{
			bufferNr = (physx::PxI32)i;
		}

	if (bufferNr < 0)
	{
		return;
	}


	struct Edge
	{
		Edge(physx::PxU32 _v1, physx::PxU32 _v2, float _l)
		{
			v1 = physx::PxMin(_v1, _v2);
			v2 = physx::PxMax(_v1, _v2);
			invLength = 1.0f / _l;
		}
		physx::PxU32 v1;
		physx::PxU32 v2;
		float invLength;
		bool operator<(const Edge& other) const
		{
			if (v1 == other.v1)
			{
				return v2 < other.v2;
			}
			return v1 < other.v1;
		}
		bool operator==(const Edge& other) const
		{
			return v1 == other.v1 && v2 == other.v2;
		}
	};

	int totalSize = 0;
	for (physx::PxU32 i = 0; i < mIndexRanges.size(); i++)
	{
		totalSize += mIndexRanges[i].end - mIndexRanges[i].start;
	}

	std::vector<Edge> edges;
	edges.reserve((physx::PxU32)totalSize);
	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		for (physx::PxU32 i = mIndexRanges[r].start; i < mIndexRanges[r].end; i += 3)
		{
			Edge e1(mIndices[i + 0], mIndices[i + 1], (mVertices[mIndices[i + 0]] - mVertices[mIndices[i + 1]]).magnitude());
			Edge e2(mIndices[i + 1], mIndices[i + 2], (mVertices[mIndices[i + 1]] - mVertices[mIndices[i + 2]]).magnitude());
			Edge e3(mIndices[i + 2], mIndices[i + 0], (mVertices[mIndices[i + 2]] - mVertices[mIndices[i + 0]]).magnitude());
			edges.push_back(e1);
			edges.push_back(e2);
			edges.push_back(e3);
		}
	}

	std::sort(edges.begin(), edges.end());

	const PaintFloatBuffer& buffer = mFloatBuffers[(physx::PxU32)bufferNr];

	std::vector<float> newValues(mVertices.size(), 0.0f);
	std::vector<float> newWeights(mVertices.size(), 0.0f);

	for (physx::PxU32 iteration = 0; iteration < numIterations; iteration++)
	{
		for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		{
			newValues[i] = buffer[i];
			newWeights[i] = 1.0f;
		}

		for (physx::PxU32 i = 0; i < edges.size(); i++)
		{
			physx::PxU32 j = i;
			while (j < edges.size() && edges[j] == edges[i])
			{
				i = j;
				j++;
			}

			// maxDistance < 0 is a drain, collisionFactor < 0 is not!
			if (id != 1 || buffer[edges[i].v1] > 0.0f && buffer[edges[i].v2] > 0.0f)
			{
				const float factor = edges[i].invLength * smoothingFactor;
				if (buffer[edges[i].v1] >= 0.0f)
				{
					newValues[edges[i].v1] += buffer[edges[i].v2] * factor;
					newWeights[edges[i].v1] += factor;
				}

				if (buffer[edges[i].v2] >= 0.0f)
				{
					newValues[edges[i].v2] += buffer[edges[i].v1] * factor;
					newWeights[edges[i].v2] += factor;
				}
			}
		}

		for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		{
			if (newWeights[i] > 0.0f)
			{
				buffer[i] = newValues[i] / newWeights[i];
			}
		}

		physx::PxU32 i = 0;
		while (i < mSiblings.size())
		{
			float avg = 0.0f;
			physx::PxU32 num = 0;
			physx::PxU32 j = i;
			while (mSiblings[j] >= 0)
			{
				avg += buffer[mSiblings[j]];
				num++;
				j++;
			}
			PX_ASSERT(num > 0);
			avg /= num;
			j = i;
			while (mSiblings[j] >= 0)
			{
				buffer[mSiblings[j]] = avg;
				j++;
			}
			i = j + 1;
		}
	}
}



void MeshPainter::smoothFloatFast(physx::PxU32 id, physx::PxU32 numIterations) const
{
	int bufferNr = -1;
	for (physx::PxU32 i = 0; i < mFloatBuffers.size(); i++)
		if (mFloatBuffers[i].id == id)
		{
			bufferNr = (physx::PxI32)i;
		}

	if (bufferNr < 0)
	{
		return;
	}

	const PaintFloatBuffer& buffer = mFloatBuffers[(physx::PxU32)bufferNr];

	float min0 = PX_MAX_F32;
	float max0 = -PX_MAX_F32;
	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		for (physx::PxU32 i = mIndexRanges[r].start; i < mIndexRanges[r].end; i += 3)
		{
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				const float b = buffer[mIndices[i + j]];
				if (id != 0 || b >= 0.0f)
				{
					min0 = physx::PxMin(min0, b);
					max0 = physx::PxMax(max0, b);
				}
			}
		}
	}
	if (min0 == PX_MAX_F32)
	{
		return;
	}

	for (physx::PxU32 iteration = 0; iteration < numIterations; iteration++)
	{
		for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
		{
			for (physx::PxU32 i = mIndexRanges[r].start; i < mIndexRanges[r].end; i += 3)
			{
				float avg = 0.0f;
				//			physx::PxU32 num = 0;
				for (physx::PxU32 j = 0; j < 3; j++)
				{
					if (id == 0)
					{
						avg += physx::PxMax(buffer[mIndices[i + j]], 0.01f);
					}
					else
					{
						avg += buffer[mIndices[i + j]];
					}
				}
				avg /= 3.0f;

				for (physx::PxU32 j = 0; j < 3; j++)
				{
					float& b = buffer[mIndices[i + j]];

					if (id != 0 || (b >= 0.0f && b < 0.95f * max0))
					{
						b = avg;
					}
				}
			}
		}

		for (physx::PxU32 i = 0; i < mSiblings.size();)
		{
			float avg = 0.0f;
			physx::PxU32 num = 0;
			physx::PxU32 j = i;
			while (mSiblings[j] >= 0)
			{
				avg += buffer[mSiblings[j]];
				num++;
				j++;
			}
			PX_ASSERT(num > 0);
			avg /= num;
			j = i;
			while (mSiblings[j] >= 0)
			{
				buffer[mSiblings[j]] = avg;
				j++;
			}
			i = j + 1;
		}
	}

	float min1 = PX_MAX_F32;
	float max1 = -PX_MAX_F32;
	for (physx::PxU32 r = 0; r < mIndexRanges.size(); r++)
	{
		for (physx::PxU32 i = mIndexRanges[r].start; i < mIndexRanges[r].end; i += 3)
		{
			for (physx::PxU32 j = 0; j < 3; j++)
			{
				float b = buffer[mIndices[i + j]];
				if (id != 0 || b >= 0.0f)
				{
					min1 = physx::PxMin(min1, b);
					max1 = physx::PxMax(max1, b);
				}
			}
		}
	}

	std::vector<bool> marked;
	marked.resize(mVertices.size(), false);

	//float s = (max1 - min1);
	//if (s > 0.0f)
	//{
	//	s = (max0 - min0) / s;
	//	for (physx::PxU32 i = mIndicesStart; i < mIndicesEnd; i+=3)
	//	{
	//		for (physx::PxU32 j = 0; j < 3; j++)
	//		{
	//			physx::PxU32 index = mIndices[i+j];
	//			if (!marked[index])
	//			{
	//				marked[index] = true;
	//				if (id != 0 || buffer[index] >= 0.0f)
	//					buffer[index] = min0 + (buffer[index] - min1) * s;
	//			}
	//		}
	//	}
	//}
}



void MeshPainter::drawBrush(physx::apex::NxApexRenderDebug* batcher) const
{
	PX_ASSERT(batcher != NULL);
	if (mPaintRadius == 0 || mLastRaycastNormal.isZero() || batcher == NULL)
	{
		return;
	}

	physx::PxU32 brushColor;
	if (mBrushColor < 0.0f || mBrushColor > 1.0f || mTargetValue < 0.0f)
	{
		brushColor = batcher->getDebugColor(physx::DebugColors::Red);
	}
	else
	{
		brushColor = batcher->getDebugColor(mBrushColor, mBrushColor, mBrushColor);
	}

	batcher->setCurrentColor(brushColor);

	physx::PxMat34Legacy transform;
	transform.t = mLastRaycastPos;
	{
		const physx::PxVec3 up(0.0f, 1.0f, 0.0f);
		physx::PxVec3 axis = mLastRaycastNormal.cross(up);
		if (axis.isZero())
		{
			axis = physx::PxVec3(0.0f, 1.0f, 0.0f);
		}
		else
		{
			axis.normalize();
		}
		physx::PxF32 angle = physx::PxAcos(mLastRaycastNormal.dot(up));
		physx::PxQuat q(-angle, axis);
		transform.M.fromQuat(q);

		physx::PxMat33Legacy scale;
		scale.setIdentity();
		scale.multiplyDiagonal(physx::PxVec3(mPaintRadius, mPaintRadius, mPaintRadius));
		transform.M = transform.M * scale;
	}
	batcher->debugLine(mLastRaycastPos, mLastRaycastPos + mLastRaycastNormal * mScaledTargetValue);

	const physx::PxU32 arcSize = 20;
	physx::PxVec3 lastPos(1.0f, 0.0f, 0.0f);
	lastPos = transform * lastPos;
	for (physx::PxU32 i = 1; i <= arcSize; i++)
	{
		const float angle = i * physx::PxTwoPi / (float)arcSize;
		physx::PxVec3 pos(physx::PxCos(angle), 0.0f, physx::PxSin(angle));
		pos = transform * pos;

		batcher->debugLine(lastPos, pos);

		lastPos = pos;
	}

	const physx::PxU32 numSteps = 10;
	physx::PxF32 lastHeight = mScaledTargetValue / mPaintRadius;
	physx::PxF32 lastT = 0.0f;
	for (physx::PxU32 i = 1; i <= numSteps; i++)
	{
		const float t = (float)i / (float)numSteps;

		const physx::PxF32 height = physx::PxPow(1.0f - t, mFalloffExponent) * mScaledTargetValue / mPaintRadius;

		batcher->debugLine(transform * physx::PxVec3(t, height, 0), transform * physx::PxVec3(lastT, lastHeight, 0));
		batcher->debugLine(transform * physx::PxVec3(-t, height, 0), transform * physx::PxVec3(-lastT, lastHeight, 0));
		batcher->debugLine(transform * physx::PxVec3(0, height, t), transform * physx::PxVec3(0, lastHeight, lastT));
		batcher->debugLine(transform * physx::PxVec3(0, height, -t), transform * physx::PxVec3(0, lastHeight, -lastT));

		lastT = t;
		lastHeight = height;
	}

	if (mFalloffExponent == 0)
	{
		const physx::PxF32 height = mScaledTargetValue / mPaintRadius;
		lastPos = transform * physx::PxVec3(1.0f, height, 0.0f);
		for (physx::PxU32 i = 1; i <= arcSize; i++)
		{
			const float angle = i * physx::PxTwoPi / (float)arcSize;
			physx::PxVec3 pos(physx::PxCos(angle), height, physx::PxSin(angle));
			pos = transform * pos;

			batcher->debugLine(lastPos, pos);

			lastPos = pos;
		}

		batcher->debugLine(transform * physx::PxVec3(1, 0, 0), transform * physx::PxVec3(1, height, 0));
		batcher->debugLine(transform * physx::PxVec3(-1, 0, 0), transform * physx::PxVec3(-1, height, 0));
		batcher->debugLine(transform * physx::PxVec3(0, 0, 1), transform * physx::PxVec3(0, height, 1));
		batcher->debugLine(transform * physx::PxVec3(0, 0, -1), transform * physx::PxVec3(0, height, -1));
	}
}



void MeshPainter::computeSiblingInfo(float distanceThreshold)
{
	physx::PxU32 numVerts = (physx::PxU32)mVertices.size();

	// select primary axis
	physx::PxBounds3 bounds;
	bounds.setEmpty();
	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
	{
		bounds.include(mVertices[i]);
	}


	physx::PxVec3 extents = bounds.getExtents();
	int axis;
	if (extents.x > extents.y && extents.x > extents.z)
	{
		axis = 0;
	}
	else if (extents.y > extents.z)
	{
		axis = 1;
	}
	else
	{
		axis = 2;
	}

	// create sorted vertex list
	struct VertRef
	{
		physx::PxU32 vertNr;
		float val;
		bool operator < (const VertRef& v) const
		{
			return val < v.val;
		}
	};

	std::vector<VertRef> refs;
	refs.resize(numVerts);
	for (physx::PxU32 i = 0; i < numVerts; i++)
	{
		refs[i].vertNr = i;
		refs[i].val = mVertices[i][axis];
	}
	std::sort(refs.begin(), refs.end());

	// collect siblings
	mFirstSibling.resize(numVerts, -1);
	mSiblings.clear();

	float d2 = distanceThreshold * distanceThreshold;

	for (physx::PxU32 i = 0; i < numVerts; i++)
	{
		physx::PxU32 vi = refs[i].vertNr;
		if (mFirstSibling[vi] >= 0)
		{
			continue;
		}

		physx::PxI32 firstSibling = (physx::PxI32)mSiblings.size();
		physx::PxU32 numSiblings = 0;

		for (physx::PxU32 j = i + 1; j < numVerts; j++)
		{
			if (refs[j].val - refs[i].val > distanceThreshold)
			{
				break;
			}
			physx::PxU32 vj = refs[j].vertNr;
			if ((mVertices[vi] - mVertices[vj]).magnitudeSquared() <= d2)
			{
				mFirstSibling[vj] = firstSibling;
				mSiblings.push_back((physx::PxI32)vj);
				numSiblings++;
			}
		}
		if (numSiblings > 0)
		{
			mFirstSibling[vi] = firstSibling;
			mSiblings.push_back((physx::PxI32)vi);
			mSiblings.push_back(-1);		// end marker
		}
	}
}

} //namespace SharedTools

#endif // PX_WINDOWS

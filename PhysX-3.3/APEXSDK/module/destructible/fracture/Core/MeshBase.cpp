/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "RTdef.h"
#if RT_COMPILE
#include <PxBounds3.h>

#include "MeshBase.h"
#include <algorithm>

namespace physx
{
namespace fracture
{
namespace base
{

// ----------------------------------------------------------------------
void Mesh::updateNormals()
{
	if (mVertices.empty())
		return;

	PxVec3 zero(0.0f, 0.0f, 0.0f);

	mNormals.resize(mVertices.size());
	for (physx::PxU32 i = 0; i < mNormals.size(); i++) 
		mNormals[i] = zero;
	PxVec3 n;

	PxU32 *idx = &mIndices[0];
	physx::PxU32 numTriangles = mIndices.size() / 3;
	for (physx::PxU32 i = 0; i < numTriangles; i++) {
		PxU32 i0 = *idx++;
		PxU32 i1 = *idx++;
		PxU32 i2 = *idx++;
		n = (mVertices[i1] - mVertices[i0]).cross(mVertices[i2] - mVertices[i0]);
		mNormals[i0] += n;
		mNormals[i1] += n;
		mNormals[i2] += n;
	}
	for (physx::PxU32 i = 0; i < mNormals.size(); i++) {
		if (!mNormals[i].isZero())
			mNormals[i].normalize();
	}
}

// ------------------------------------------------------------------------------
void Mesh::getBounds(PxBounds3 &bounds, int subMeshNr) const
{
	bounds.setEmpty();
	if (subMeshNr < 0 || subMeshNr >= (int)mSubMeshes.size()) {
		for (physx::PxU32 i = 0; i < mVertices.size(); i++)
			bounds.include(mVertices[i]);
	}
	else {
		const SubMesh &sm = mSubMeshes[(physx::PxU32)subMeshNr];
		for (int i = 0; i < sm.numIndices; i++) {
			bounds.include(mVertices[(physx::PxU32)mIndices[physx::PxU32(sm.firstIndex + i)]]);
		}
	}
}

// ------------------------------------------------------------------------------
void Mesh::normalize(const PxVec3 &center, float diagonal)
{
	if (mVertices.size() < 3)
		return;

	PxBounds3 bounds;
	getBounds(bounds);

	float s = diagonal / bounds.getDimensions().magnitude();
	PxVec3 c = 0.5f * (bounds.minimum + bounds.maximum);

	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		mVertices[i] = center + (mVertices[i] - c) * s;
}

// ------------------------------------------------------------------------------
void Mesh::scale(float diagonal)
{
	if (mVertices.size() < 3)
		return;

	PxBounds3 bounds;
	getBounds(bounds);

	float s = diagonal / bounds.getDimensions().magnitude();
	for (physx::PxU32 i = 0; i < mVertices.size(); i++)
		mVertices[i] *= s;
}

// --------------------------------------------------------------------------------------------
struct IdEdge {
	void set(PxU32 &i0, PxU32 &i1, int faceNr, int edgeNr) {
		if (i0 < i1) { this->i0 = i0; this->i1 = i1; } 
		else { this->i0 = i1; this->i1 = i0; }
		this->faceNr = faceNr; this->edgeNr = edgeNr;
	}
	bool operator < (const IdEdge &e) const {
		if (i0 < e.i0) return true;
		if (i0 == e.i0 && i1 < e.i1) return true;
		return false;
	}
	bool operator == (const IdEdge &e) const { return i0 == e.i0 && i1 == e.i1; }
	PxU32 i0,i1;
	int faceNr, edgeNr;
};

// ------------------------------------------------------------------------------
bool Mesh::computeNeighbors()
{
	physx::PxU32 numTris = mIndices.size() / 3;

	shdfnd::Array<IdEdge> edges(3*numTris);

	for (physx::PxU32 i = 0; i < numTris; i++) {
		for (physx::PxU32 j = 0; j < 3; j++) 
			edges[3*i+j].set(mIndices[3*i+j], mIndices[3*i + (j+1)%3], (physx::PxI32)i, (physx::PxI32)j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	physx::PxU32 i = 0;
	while (i < edges.size()) {
		IdEdge &e0 = edges[i];
		i++;
		if (i < edges.size() && edges[i] == e0) {
			IdEdge &e1 = edges[i];
			mNeighbors[physx::PxU32(3* e0.faceNr + e0.edgeNr)] = e1.faceNr;
			mNeighbors[physx::PxU32(3* e1.faceNr + e1.edgeNr)] = e0.faceNr;
			i++;
		}
		while (i < edges.size() && edges[i] == e0) {
			manifold = false;
			i++;
		}
	}
	return manifold;
}

// --------------------------------------------------------------------------------------------
struct PosEdge {
	// not using indices for edge match but positions
	// so duplicated vertices are handled as one vertex
	static bool smaller(const PxVec3 &v0, const PxVec3 &v1) {
		if (v0.x < v1.x) return true;
		if (v0.x > v1.x) return false;
		if (v0.y < v1.y) return true;
		if (v0.y > v1.y) return false;
		return (v0.z < v1.z);
	}
	void set(const PxVec3 &v0, const PxVec3 &v1, int faceNr, int edgeNr) {
		if (smaller(v0,v1)) { this->v0 = v0; this->v1 = v1; } 
		else { this->v0 = v1; this->v1 = v0; }
		this->faceNr = faceNr; this->edgeNr = edgeNr;
	}
	bool operator < (const PosEdge &e) const {
		if (smaller(v0, e.v0)) return true;
		if (v0 == e.v0 && smaller(v1, e.v1)) return true;
		return false;
	}
	bool operator == (const PosEdge &e) const { return v0 == e.v0 && v1 == e.v1; }
	PxVec3 v0,v1;
	int faceNr, edgeNr;
};

// ------------------------------------------------------------------------------
bool Mesh::computeWeldedNeighbors()
{
	physx::PxU32 numTris = mIndices.size() / 3;
	shdfnd::Array<PosEdge> edges(3*numTris);

	for (physx::PxU32 i = 0; i < numTris; i++) {
		for (physx::PxU32 j = 0; j < 3; j++) 
			edges[3*i+j].set(mVertices[(physx::PxU32)mIndices[3*i+j]], mVertices[(physx::PxU32)mIndices[3*i + (j+1)%3]], (physx::PxI32)i, (physx::PxI32)j);
	}
	std::sort(edges.begin(), edges.end());

	mNeighbors.clear();
	mNeighbors.resize(mIndices.size(), -1);
	bool manifold = true;
	physx::PxU32 i = 0;
	while (i < edges.size()) {
		PosEdge &e0 = edges[i];
		i++;
		if (i < edges.size() && edges[i] == e0) {
			PosEdge &e1 = edges[i];
			mNeighbors[physx::PxU32(3* e0.faceNr + e0.edgeNr)] = e1.faceNr;
			mNeighbors[physx::PxU32(3* e1.faceNr + e1.edgeNr)] = e0.faceNr;
			i++;
		}
		while (i < edges.size() && edges[i] == e0) {
			manifold = false;
			i++;
		}
	}
	return manifold;
}

void Mesh::flipV()
{
	for(PxU32 i = 0; i < mTexCoords.size(); i++)
	{
		mTexCoords[i].y = 1.0f - mTexCoords[i].y;
	}
}

}
}
}
#endif
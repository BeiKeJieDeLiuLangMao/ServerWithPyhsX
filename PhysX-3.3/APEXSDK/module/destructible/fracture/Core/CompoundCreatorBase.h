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
#ifndef COMPOUND_CREATOR_BASE_H
#define COMPOUND_CREATOR_BASE_H

// Matthias Muller-Fischer

#include <PxVec3.h>
#include <PxTransform.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

#include "CompoundGeometryBase.h"

namespace physx
{
namespace fracture
{
namespace base
{
	class SimScene;

// ---------------------------------------------------------------------------------------
class CompoundCreator : public ::physx::shdfnd::UserAllocated {
	friend class physx::fracture::base::SimScene;
public:
	// direct
	void createTorus(float r0, float r1, int numSegs0, int numSegs1, const PxTransform *trans = NULL);
	void createCylinder(float r, float h, int numSegs, const PxTransform *trans = NULL);
	void createBox(const PxVec3 &dims, const PxTransform *trans = NULL);
	void createSphere(const PxVec3 &dims, int resolution = 5, const PxTransform *trans = NULL);
	void fromTetraMesh(const shdfnd::Array<PxVec3> &tetVerts, const shdfnd::Array<int> &tetIndices);

	const CompoundGeometry &getGeometry() { return mGeom; }

	virtual void debugDraw() {}

protected:
	CompoundCreator(SimScene* scene): mScene(scene) {}
	virtual ~CompoundCreator() {}

	SimScene* mScene;

	static int tetFaceIds[4][3];
	static int tetEdgeVerts[6][2];
	static int tetFaceEdges[4][3];

	void computeTetNeighbors();
	void computeTetEdges();
	void colorTets();
	void colorsToConvexes();

	bool tetHasColor(int tetNr, int color);
	bool tetAddColor(int tetNr, int color);
	bool tetRemoveColor(int tetNr, int color);
	int  tetNumColors(int tetNr);
	void deleteColors();

	bool tryTet(int tetNr, int color);

	// from tet mesh
	shdfnd::Array<PxVec3> mTetVertices;
	shdfnd::Array<int> mTetIndices;
	shdfnd::Array<int> mTetNeighbors;

	shdfnd::Array<int> mTetFirstColor;
	struct Color {
		int color;
		int next;
	};
	int mTetColorsFirstEmpty;
	shdfnd::Array<Color> mTetColors;

	struct TetEdge {
		void init(int i0, int i1) { this->i0 = i0; this->i1 = i1; firstTet = 0; numTets = 0; onSurface = false; }
		int i0, i1;
		int firstTet, numTets;
		bool onSurface;
	};
	shdfnd::Array<TetEdge> mTetEdges;
	shdfnd::Array<int> mTetEdgeNrs;
	shdfnd::Array<int> mEdgeTetNrs;
	shdfnd::Array<float> mEdgeTetAngles;

	shdfnd::Array<int> mAddedTets;
	shdfnd::Array<int> mTestEdges;

	// input
	shdfnd::Array<PxVec3> mVertices;
	shdfnd::Array<int> mIndices;

	// output
	CompoundGeometry mGeom;
};

}}}

#endif
#endif

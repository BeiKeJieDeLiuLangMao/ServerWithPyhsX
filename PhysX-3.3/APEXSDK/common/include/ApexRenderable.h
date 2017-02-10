/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef APEX_RENDERABLE_H
#define APEX_RENDERABLE_H

#include "PsShare.h"
#include "PsMutex.h"

#include "foundation/PxBounds3.h"

namespace physx
{
namespace apex
{

/**
	Base class for implementations of NxApexRenderable classes
*/

class ApexRenderable
{
public:
	ApexRenderable()
	{
		mRenderBounds.setEmpty();
	}
	~ApexRenderable()
	{
		// the PS3 Mutex cannot be unlocked without first being locked, so grab the lock
		if (renderDataTryLock())
		{
			renderDataUnLock();
		}
		else
		{
			// someone is holding the lock and should not be, so assert
			PX_ALWAYS_ASSERT();
		}
	}
	void renderDataLock()
	{
		mRenderDataLock.lock();
	}
	void renderDataUnLock()
	{
		mRenderDataLock.unlock();
	}
	bool renderDataTryLock()
	{
		return mRenderDataLock.trylock();
	}
	const physx::PxBounds3&	getBounds() const
	{
		return mRenderBounds;
	}

protected:
	//physx::Mutex		mRenderDataLock;
	// Converting to be a PS3 SPU-friendly lock
	// On PC this is the same as a Mutex, on PS3 it is a 128b (!) aligned U32. Subclasses might get bigger on PS3 and they
	// are most likely distributed over more than one cache line.
	physx::AtomicLock	mRenderDataLock;

	physx::PxBounds3	mRenderBounds;
};

}
} // end namespace physx::apex

#endif // APEX_RENDERABLE_H

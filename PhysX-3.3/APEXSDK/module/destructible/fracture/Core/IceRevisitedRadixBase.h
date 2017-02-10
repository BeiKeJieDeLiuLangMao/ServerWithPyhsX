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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		IceRevisitedRadix.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERADIXSORT_BASE_H__
#define __ICERADIXSORT_BASE_H__

#include "PxSimpleTypes.h"
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

	class RadixSort : public ::physx::shdfnd::UserAllocated
	{
		public:
		// Constructor/Destructor
								RadixSort();
								virtual ~RadixSort();
		// Sorting methods
				RadixSort&		Sort(const PxU32* input, PxU32 nb, bool signedvalues=true);
				RadixSort&		Sort(const float* input, PxU32 nb);

		//! Access to results. mRanks is a list of indices in sorted order, i.e. in the order you may further process your data
		inline	PxU32*			GetRanks()			const	{ return mRanks;		}

		//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
		inline	PxU32*			GetRecyclable()		const	{ return mRanks2;		}

		// Stats
				PxU32			GetUsedRam()		const;
		//! Returns the total number of calls to the radix sorter.
		inline	PxU32			GetNbTotalCalls()	const	{ return mTotalCalls;	}
		//! Returns the number of premature exits due to temporal coherence.
		inline PxU32			GetNbHits()			const	{ return mNbHits;		}

		private:

				PxU32			mHistogram[256*4];			//!< Counters for each byte
				PxU32			mOffset[256];				//!< Offsets (nearly a cumulative distribution function)
				PxU32			mCurrentSize;				//!< Current size of the indices list

				PxU32           mRanksSize;
				PxU32*			mRanks;				//!< Two lists, swapped each pass
				PxU32*			mRanks2;
		// Stats
				PxU32			mTotalCalls;
				PxU32			mNbHits;
		// Internal methods
				void			CheckResize(PxU32 nb);
				bool			Resize(PxU32 nb);
	};

}
}
}

#endif // __ICERADIXSORT_H__
#endif

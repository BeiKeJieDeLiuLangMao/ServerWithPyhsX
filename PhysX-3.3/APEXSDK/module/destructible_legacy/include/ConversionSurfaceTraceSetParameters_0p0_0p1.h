/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef CONVERSIONSURFACETRACESETPARAMETERS_0P0_0P1H_H
#define CONVERSIONSURFACETRACESETPARAMETERS_0P0_0P1H_H

#include "ParamConversionTemplate.h"
#include "SurfaceTraceSetParameters_0p0.h"
#include "SurfaceTraceSetParameters_0p1.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<SurfaceTraceSetParameters_0p0, SurfaceTraceSetParameters_0p1, 0, 1> ConversionSurfaceTraceSetParameters_0p0_0p1Parent;

class ConversionSurfaceTraceSetParameters_0p0_0p1: ConversionSurfaceTraceSetParameters_0p0_0p1Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionSurfaceTraceSetParameters_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionSurfaceTraceSetParameters_0p0_0p1)(t) : 0;
	}

protected:
	ConversionSurfaceTraceSetParameters_0p0_0p1(NxParameterized::Traits* t) : ConversionSurfaceTraceSetParameters_0p0_0p1Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (PxU32)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		//TODO:
		//	Write custom conversion code here using mNewData and mLegacyData members.
		//
		//	Note that
		//		- mNewData has already been initialized with default values
		//		- same-named/same-typed members have already been copied
		//			from mLegacyData to mNewData
		//		- included references were moved to mNewData
		//			(and updated to preferred versions according to getPreferredVersions)
		//
		//	For more info see the versioning wiki.

		return true;
	}
};

}
}
} // namespace physx::apex

#endif

/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef __CONVERSIONCLOTHINGCOOKEDPARAM_0P0_0P1H__
#define __CONVERSIONCLOTHINGCOOKEDPARAM_0P0_0P1H__

#include "ParamConversionTemplate.h"
#include "ClothingCookedParam_0p0.h"
#include "ClothingCookedParam_0p1.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<ClothingCookedParam_0p0, ClothingCookedParam_0p1, 0, 1> ConversionClothingCookedParam_0p0_0p1Parent;

class ConversionClothingCookedParam_0p0_0p1: ConversionClothingCookedParam_0p0_0p1Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingCookedParam_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingCookedParam_0p0_0p1)(t) : 0;
	}

protected:
	ConversionClothingCookedParam_0p0_0p1(NxParameterized::Traits* t) : ConversionClothingCookedParam_0p0_0p1Parent(t) {}

	bool convert()
	{
		//TODO:
		//	write custom conversion code here using mNewData and mLegacyData members
		//	note that members with same names were already copied in parent converter
		// and mNewData was already initialized with default values

		mNewData->cookedDataVersion = 284;

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif

/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef APEX_RENDER_DEBUG_H
#define APEX_RENDER_DEBUG_H

#include "NiApexRenderDebug.h"
#include "PxProcessRenderDebug.h"

namespace physx
{
namespace apex
{

class ApexSDK;

NiApexRenderDebug* createNiApexRenderDebug(ApexSDK* a, physx::PxProcessRenderDebug* debugChannel);
void				releaseNiApexRenderDebug(NiApexRenderDebug* n);

}
} // end namespace physx::apex

#endif

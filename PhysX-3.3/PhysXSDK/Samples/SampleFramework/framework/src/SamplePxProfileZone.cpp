/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#include "SamplePxProfileZone.h"

#ifdef PHYSX_PROFILE_SDK

using namespace physx::profile;
using namespace physx;

#define AG_PERFMON_EVENT_DEFINITION_HEADER "AgPerfMonEventDefs.h"
#include "SampleProfileDefineAgPerfmonEventInfo.h"
#undef AG_PERFMON_EVENT_DEFINITION_HEADER

SampleProfileEventNameProvider gProfileNameProvider;
PxProfileNames SampleProfileEventNameProvider::getProfileNames() const { return gPxProfileNames; }

#endif

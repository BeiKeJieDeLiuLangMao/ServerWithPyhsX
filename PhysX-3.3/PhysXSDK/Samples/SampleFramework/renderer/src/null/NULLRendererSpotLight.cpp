/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#include <RendererConfig.h>

#include "NULLRendererSpotLight.h"

using namespace SampleRenderer;

NullRendererSpotLight::NullRendererSpotLight(const RendererSpotLightDesc &desc) :
	RendererSpotLight(desc)
{

}

NullRendererSpotLight::~NullRendererSpotLight(void)
{

}

void NullRendererSpotLight::bind(void) const
{
}

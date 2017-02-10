/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#ifndef COCOA_APP_H
#define COCOA_APP_H

#include <Cocoa/Cocoa.h>

@interface CocoaApp : NSApplication
{
  @private
}

- (id)init;
- (void)runOnce: (BOOL) blockTillEvent;

@end 

#endif 

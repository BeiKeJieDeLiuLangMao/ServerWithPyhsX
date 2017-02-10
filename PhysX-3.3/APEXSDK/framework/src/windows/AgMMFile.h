/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef __AG_MM_FILE__
#define __AG_MM_FILE__

#include "windows/PsWindowsInclude.h"

namespace physx
{
	class AgMMFile
	{
	public:
		AgMMFile();
		AgMMFile(char *name, unsigned int size, bool &alreadyExists);
		void create(char *name, unsigned int size, bool &alreadyExists);
		unsigned int getSize() {return mSize;};
		void * getAddr() {return mAddr;};
		void destroy();
		~AgMMFile();

	private:
		void *mAddr;
		unsigned int mSize;
		HANDLE mFileH;
	};
}

#endif	// __AG_MM_FILE__

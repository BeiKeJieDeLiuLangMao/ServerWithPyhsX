/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include <PsString.h>
#include "foundation/PxAssert.h"

#include <DirEntry.h>

#include "Find.h"

namespace physx {
namespace apex {

void Find(const char* root, FileHandler& f, const char** ignoredFiles)
{
	if (!root)
		return;

	// Fix slashes
	char goodRoot[128];
	strcpy(goodRoot, root);
#if defined PX_WINDOWS || defined PX_X360
	for (char* p = goodRoot; *p; ++p)
	{
		if ('/' == *p)
			*p = '\\';
	}
#endif

	physx::DirEntry dentry;
	if (!physx::DirEntry::GetFirstEntry(goodRoot, dentry))
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	for (; !dentry.isDone(); dentry.next())
	{
		const char* filename = dentry.getName();

		if (!filename || 0 == strcmp(".", filename) || 0 == strcmp("..", filename))
			continue;

		bool doSkip = false;
		for (size_t i = 0; ignoredFiles && ignoredFiles[i]; ++i)
		{
			if (0 == strcmp(filename, ignoredFiles[i]))
			{
				doSkip = true;
				break;
			}
		}

		if (doSkip)
			continue;

		char tmp[128];
		physx::string::sprintf_s(tmp, sizeof(tmp), "%s/%s", goodRoot, filename);

#if defined PX_WINDOWS || defined PX_X360
	for (char* p = tmp; *p; ++p)
	{
		if ('/' == *p)
			*p = '\\';
	}
#endif

		if (dentry.isDirectory())
		{
			Find(tmp, f, ignoredFiles);
			continue;
		}

		f.handle(tmp);
	}
}

}}

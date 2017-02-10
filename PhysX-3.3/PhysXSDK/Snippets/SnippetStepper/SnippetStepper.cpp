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

// ****************************************************************************
// This snippet illustrates kinematic actor updates in a substepped simulation.
//
// It uses chained continuation tasks that call fetchResults and run simulation steps. 
// The scene consists of a kinematic platform interacting with a dynamic 
// sphere. The kinematic actor's target pose is updated before every substep.
// ****************************************************************************

#include <new>

#include "PxPhysicsAPI.h"

#include "../SnippetCommon/SnippetPrint.h"
#include "../SnippetCommon/SnippetPVD.h"
#include "../SnippetUtils/SnippetUtils.h"

using namespace physx;
using namespace SnippetUtils;

// The usual PhysX resources.
PxDefaultAllocator			gAllocator;
PxDefaultErrorCallback		gErrorCallback;

PxFoundation*				gFoundation		= NULL;
PxPhysics*					gPhysics		= NULL;
PxMaterial*					gMaterial		= NULL;

PxDefaultCpuDispatcher*		gDispatcher		= NULL;
PxScene*					gScene			= NULL;
PxRigidDynamic*				gKinematic		= NULL;

PxVisualDebuggerConnection*	gConnection		= NULL;

// A very simple substepping policy: just take 2 60Hz substeps per step.
static const PxReal			SUBSTEP_LENGTH	= 1.0f/60.0f;
static const PxU32			NUM_STEPS		= 1000;
static const PxI32			NUM_SUBSTEPS	= 2;

// Context for keeping track of the stepper state.
struct StepContext
{
	class SubstepCompletionTask*	taskPool;
	Sync*							completionSync;
	PxI32							nbSubstepsFinished;
	volatile PxI32					nbTasksDestroyed;
} gStepContext;

// Completion task for running a substep.

// The following sequencing is guaranteed:
// * the section of the run() method up to the removeReference() in startNextSubstep() will execute prior  
//   to the run() method of the task submitted by startNextSubstep()
// * the run() method of a task will run before its release() method
//
// Any work done by a task after releasing the next task (via removeReference()) could end up running in 
// parallel with that task, if simulate() completes sufficiently quickly or there is a context switch. In order
// to prevent races, it is therefore recommended that a completion task do no work after releasing the next.

class SubstepCompletionTask : public PxLightCpuTask
{
public:
	SubstepCompletionTask()
	{
		mTm = gScene->getTaskManager();
	}

	void run() 
	{		
		void startNextSubstep();

		gScene->fetchResults(true);
		if(++gStepContext.nbSubstepsFinished < NUM_SUBSTEPS)
			startNextSubstep();
	}

	void release()
	{
		this->~SubstepCompletionTask();

		// If we're done with all the substeps , synchronize with the main thread. In a real application 
		// we would most likely run dependent tasks instead.

		// We can only signal completion once all substepping resources are cleaned up.  
		// Release() calls may run concurrently or out of order, so we use an atomic counter.

		if(atomicIncrement(&gStepContext.nbTasksDestroyed) == NUM_SUBSTEPS)
			syncSet(gStepContext.completionSync);
	}

	const char* getName() const { return "Substep Completion Task"; }
};


// Update the sim inputs and start the next PhysX substep.

void startNextSubstep()
{
	// Compute new target pose for the kinematic at the end of the substep.

	static PxReal sTotalSeconds		= 0.0f;
	sTotalSeconds += SUBSTEP_LENGTH;

	const PxReal period		= 4.0f;
	const PxReal amplitude	= 10.0f;
	const PxReal angVel		= PxTwoPi/period;

	PxReal yPos = PxSin(angVel * sTotalSeconds) * amplitude;
	gKinematic->setKinematicTarget(PxTransform(0.0f, yPos, 0.0f));

	// Create a completion task and set its reference count to 1. This way we can safely submit it to simulate()
	// and, even if we get context-switched and simulate() completes before we get back, the task's run()
	// method will not execute until we're ready.

	SubstepCompletionTask* nextCompletion = new (gStepContext.taskPool+gStepContext.nbSubstepsFinished) SubstepCompletionTask();
	nextCompletion->addReference();

	// Kick off the sim with the new completion task. Once this call returns, worker threads will update the PhysX 
	// state in parallel with the rest of this function.

	gScene->simulate(SUBSTEP_LENGTH, nextCompletion);

	// We can do things here that can run in parallel with the simulation, but must happen before the next task's
	// run method executes. In this snippet, there's nothing to do...
	
	// Finally, remove the reference that prevents the next completion task running.
	
	nextCompletion->removeReference();
}



void runPhysics()
{
	// Initialize the substepping context.
	syncReset(gStepContext.completionSync);
	gStepContext.nbSubstepsFinished = 0;
	gStepContext.nbTasksDestroyed = 0;

	// Start the first substep, then wait for the last one to finish.
	startNextSubstep();
	syncWait(gStepContext.completionSync);
}

void initPhysics()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	PxProfileZoneManager* profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(gFoundation);
	gPhysics = PxCreateBasePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, profileZoneManager);

	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.2f);

	if(gPhysics->getPvdConnectionManager())
	{
		gPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONSTRAINTS, true);
		gPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
		gPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
		gConnection = PxVisualDebuggerExt::createConnection(gPhysics->getPvdConnectionManager(), PVD_HOST, 5425, 10);
	}

	PxSceneDesc desc(gPhysics->getTolerancesScale());
	desc.filterShader = PxDefaultSimulationFilterShader;
	desc.cpuDispatcher = gDispatcher = PxDefaultCpuDispatcherCreate(2);
	desc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gScene = gPhysics->createScene(desc);

	gKinematic = PxCreateKinematic(*gPhysics, PxTransform(PxIdentity), PxBoxGeometry(5.0f, 1.0f, 5.0f), *gMaterial, 1.0f);
	gScene->addActor(*gKinematic);

	PxRigidDynamic* sphere = PxCreateDynamic(*gPhysics, PxTransform(0.0f, 5.0f, 0.0f), PxSphereGeometry(1.0f), *gMaterial, 1.0f);
	gScene->addActor(*sphere);
}

void cleanupPhysics()
{
	if(NULL != gConnection)
	{
		gConnection->lockOutStream().flush();
		gConnection->unlockOutStream();
		gConnection->disconnect();
		gConnection->release();
	}
	gDispatcher->release();
	gPhysics->release();
	gFoundation->release();
}

int snippetMain(int, const char*const*)
{
	initPhysics();

	// Storage and synchronization for substepping.
	gStepContext.taskPool = reinterpret_cast<SubstepCompletionTask*>(malloc(NUM_SUBSTEPS * sizeof(SubstepCompletionTask)));
	gStepContext.completionSync = syncCreate();

	for(PxU32 i=0; i<NUM_STEPS; i++)
		runPhysics();

	syncRelease(gStepContext.completionSync);
	free(gStepContext.taskPool);

	cleanupPhysics();

	printf("SnippetStepper done.\n");

	return 0;
}

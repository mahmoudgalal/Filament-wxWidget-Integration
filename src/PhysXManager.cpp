#include "PhysXManager.h"
#include<iostream>

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace std;

PhysXManager::PhysXManager()
{
	cout << "initializePhysX" << endl;
	initializePhysX();
}

PhysXManager::~PhysXManager()
{
	cout << "Cleaning PhysX" << endl;
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	if (gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();	gPvd = NULL;
		PX_RELEASE(transport);
	}
	PX_RELEASE(gFoundation);
}

void PhysXManager::simulate(float delta)
{
	gScene->simulate(delta);
	gScene->fetchResults(true);
}

void PhysXManager::createCube(float x, float y, float z, const Node* node, const PxVec3& velocity)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(1.0f, 1.0f, 1.0f), *gMaterial);
	PxTransform localTm(x, y, z) ;
	PxRigidDynamic* body = gPhysics->createRigidDynamic(localTm);
	body->attachShape(*shape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	body->setLinearVelocity(velocity);
	body->setAngularDamping(0.5f);
	body->setName(node->getName().c_str());
	gScene->addActor(*body);
	body->userData = (void*)node;
	shape->release();
}

void PhysXManager::createStaticCube(const PxVec3& pos, const PxVec3& scale, const Node* node, const PxVec3& velocity)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(scale.x, scale.y, scale.z), *gMaterial);
	PxTransform localTm(pos.x, pos.y, pos.z);
	PxRigidStatic* body = gPhysics->createRigidStatic(localTm);
	body->attachShape(*shape);
	//PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	gScene->addActor(*body);
	body->setName(node->getName().c_str());
	body->userData = (void*)node;
	shape->release();
}

void PhysXManager::createSphere(float x, float y, float z, float radius, const Node* node, const PxVec3& velocity)
{
	PxSphereGeometry g(radius);
	PxTransform localTm(x, y, z);
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, localTm, g, *gMaterial, 10.0f);
	dynamic->setLinearVelocity(velocity);
	dynamic->setAngularDamping(0.5f);
	dynamic->setName(node->getName().c_str());
	dynamic->userData = (void*)node;
	gScene->addActor(*dynamic);
}

void PhysXManager::updateNodes()
{
	PxU32 nbActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
	if (nbActors)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
		for (auto& actor : actors)
		{
			const bool sleeping = actor->is<PxRigidDynamic>() ? actor->is<PxRigidDynamic>()->isSleeping() : false;
			auto node = reinterpret_cast<Node*>(actor->userData);
			if (node != nullptr ) {
				const PxMat44 transform = actor->getGlobalPose();				
				node->setPose(&transform.column0.x);
				node->onSleep(sleeping);
			}
		}
	}
}

void PhysXManager::rayCast(PxVec3& origin, PxVec3& unitDir)
{
	PxReal maxDistance = 2000;            // [in] Raycast max distance
	PxRaycastBuffer hit;                 // [out] Raycast results

	// Raycast against all static & dynamic objects (no filtering)
	// The main result from this call is the closest hit, stored in the 'hit.block' structure
	bool status = gScene->raycast(origin, unitDir, maxDistance, hit);
	
	if (status) {
		std::cout << "Item Picked:" << hit.block.actor->getName()<<std::endl;
		auto actor = hit.block.actor->is<PxRigidDynamic>();
		if(actor)
		PxRigidBodyExt::addForceAtLocalPos(*actor, PxVec3(0.1, 900.0f, 120.3f),
			PxVec3(0.1, 0.50f, 0.3f), PxForceMode::eIMPULSE);
	}		
}

void PhysXManager::initializePhysX()
{
	
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	groundPlane->setName("Ground_Plane");
	gScene->addActor(*groundPlane);
}



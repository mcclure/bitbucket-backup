/*
 *  physics.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/25/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "physics.h"

#define TOOFAR(x,a) (fabs(x) > (a)/2+0.5-1+inset)

struct FindOneCallback : public btCollisionWorld::ContactResultCallback {
	ExtCollisionScene *parent;
	Vector3 center;
	Number inset;
	SceneEntity *found;
	
	FindOneCallback(ExtCollisionScene *_parent, Vector3 _center, Number _inset)
		: btCollisionWorld::ContactResultCallback(), parent(_parent), center(_center), inset(_inset), found(NULL) {}
	
	virtual btScalar addSingleResult(btManifoldPoint& cp,
									 const btCollisionObject* colObj0,int partId0,int index0,
									 const btCollisionObject* colObj1,int partId1,int index1) {
		SceneEntity *candidate = parent->getCollisionEntityByObject(const_cast<btCollisionObject*>(colObj1))->getSceneEntity();
		Vector3 candidate_center = candidate->getPosition();
		
//		ERR("%p==%p, x %lf y %lf z %lf inset %lf\n", colObj0, colObj1, (double)candidate->bBox.x, (double)candidate->bBox.y, (double)candidate->bBox.z, inset);
//		ERR("x %lf\ty %lf\tz %lf\t", (double)fabs(center.x-candidate_center.x), (double)fabs(center.y-candidate_center.y), (double)fabs(center.z-candidate_center.z));
		if (!(TOOFAR(center.x-candidate_center.x, candidate->bBox.x) || TOOFAR(center.y-candidate_center.y, candidate->bBox.y) || TOOFAR(center.z-candidate_center.z, candidate->bBox.z))) {
			found = candidate;
//			ERR("COLLIDE");
		}
//		ERR("\n");
		return 0;
	}
};

SceneEntity *ExtCollisionScene::collidesWith(SceneEntity *test, Number inset) {
	CollisionSceneEntity *pse = getCollisionByScreenEntity(test);
	FindOneCallback callback(this, test->getPosition(), inset);
	
	pse->lastPosition = test->getPosition(); // Maybe it's moved?
	pse->Update();
	
	world->contactTest(pse->collisionObject, callback);
//	ERR("DONE\n");
	
	return callback.found;
}
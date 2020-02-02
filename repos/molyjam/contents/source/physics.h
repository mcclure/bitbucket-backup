#ifndef _PHYSICS_H
#define _PHYSICS_H

/*
 *  physics.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/25/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "program.h"

class ExtCollisionScene : public PhysicsScene {
public:
	ExtCollisionScene() : PhysicsScene() {}
	SceneEntity *collidesWith(SceneEntity *test, Number inset);
};

#endif /* _VOXEL_LOADER_H */
/*
 *  physics.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/25/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#include "program.h"

class ExtCollisionScene : public CollisionScene {
public:
	ExtCollisionScene() : CollisionScene() {}
	SceneEntity *collidesWith(SceneEntity *test, Number inset);
};
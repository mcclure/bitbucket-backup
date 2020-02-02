#ifndef _CUBER_H
#define _CUBER_H

/*
 *  cuber.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 5/9/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include "vox.h"

struct Cuber : public Scene {
	Vox *vox;
	static Mesh *commonCube();
	Cuber(Vox *_vox = NULL, bool _virtualScene = false);
	virtual void Render(Camera *targetCamera = NULL);
};

#endif /* _CUBER_H */
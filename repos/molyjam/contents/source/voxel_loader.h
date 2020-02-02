#ifndef _VOXEL_LOADER_H
#define _VOXEL_LOADER_H

/*
 *  voxel_loader.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/24/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

struct voxel_loader {
	virtual void voxel(int x, int y, int z, int xd, int yd, int zd, unsigned int color) {}
	virtual bool load(const string &filename); // true for success
};

struct physics_voxel_loader : public voxel_loader {
	ExtCollisionScene *_scene; room_auto *room;
	physics_voxel_loader(ExtCollisionScene *__scene = NULL, room_auto *_room = NULL);
	ExtCollisionScene *scene();
	virtual void voxel(int x, int y, int z, int xd, int yd, int zd, unsigned int color);
};

#endif /* _VOXEL_LOADER_H */
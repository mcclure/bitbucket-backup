/*
 *  voxel_loader.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/24/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#define protected public // Blame Ivan.

#include "program.h"
#include "physics.h"
#include "voxel_loader.h"
#include "physfs.h"
#include "lodepng.h"
#include "bridge.h"

bool voxel_loader::load(const string &filename) { // true for success
	//load and decode
	std::vector<unsigned char> buffer, image;
	int w,h; // 2D image
	int xdim=0,ydim=0,zdim=0; // 3D image
	
	PHYSFS_file *f = PHYSFS_openRead(filename.c_str());
	if (!f) return false;
	int fsize = PHYSFS_fileLength(f);
	buffer.resize(fsize);
	PHYSFS_read (f, &buffer[0], 1, fsize);
	PHYSFS_close(f);
	
	LodePNG::Decoder decoder;
	decoder.decode(image, buffer.empty() ? 0 : &buffer[0], (unsigned)buffer.size()); //decode the png
	
	//if there's an error, display it, otherwise display information about the image
	if(decoder.hasError()) {
		ERR("Error %d: %s\n", (int)decoder.getError(), LodePNG_error_text(decoder.getError()));
		return false;
	}

	w = decoder.getWidth();
	h = decoder.getHeight();
	ERR("Loading voxels: 2D: Width %d height %d\n", w, h);
	
	for(size_t i = 0; i < decoder.getInfoPng().text.num; i++) {
		ERR("%s: %s\n", decoder.getInfoPng().text.keys[i], decoder.getInfoPng().text.strings[i]);
		if (string("VoxelGridDimX") == decoder.getInfoPng().text.keys[i])
			xdim = atoi(decoder.getInfoPng().text.strings[i]);
		if (string("VoxelGridDimY") == decoder.getInfoPng().text.keys[i])
			ydim = atoi(decoder.getInfoPng().text.strings[i]);
		if (string("VoxelGridDimZ") == decoder.getInfoPng().text.keys[i])
			zdim = atoi(decoder.getInfoPng().text.strings[i]);
	}
	ERR("Loading voxels: 3D: x %d height %d width %d\n", xdim, ydim, zdim);
	
	if (!xdim || !ydim || !zdim)
		return false;
	
#define GETCOORDS(_x,_y, _3x,_3y,_3z) int _x = _3z*xdim + _3x, _y = ydim - _3y - 1
	
	uint32_t *image2 = (uint32_t *)&image[0];
	for(int x = 0; x < xdim; x++) {
		for(int y = 0; y < ydim; y++) {
			for(int z = 0; z < zdim; z++) {
				GETCOORDS(x2d,y2d, x,y,z);
				unsigned int color = image2[y2d * w + x2d]; // TODO: WON'T SUPPORT PPC ENDIAN!!
				
				if (color) {
					int xd = 0, yd = 0, zd = 0;
					bool failed = false;
				
					for(int x2 = x; x2 < xdim; x2++) {
						GETCOORDS(x2d2,y2d2, x2,y,z);
						unsigned int color2 = image2[y2d2 * w + x2d2];
						if (color != color2) break;
						xd++;
					}
					
					for(int y2 = y; y2 < ydim; y2++) {
						for(int x2 = x; x2 < x+xd; x2++) {
							GETCOORDS(x2d2,y2d2, x2,y2,z);
							unsigned int color2 = image2[y2d2 * w + x2d2];
							if (color != color2) { failed = true; break; }
						}
						if (failed) break;
						yd++;
					}
					
					failed = false;
					for(int z2 = z; z2 < zdim; z2++) {
						for(int y2 = y; y2 < y+yd; y2++) {
							for(int x2 = x; x2 < x+xd; x2++) {
								GETCOORDS(x2d2,y2d2, x2,y2,z2);
								unsigned int color2 = image2[y2d2 * w + x2d2];
								if (color != color2) { failed = true; break; }
							}
							if (failed) break;
						}
						if (failed) break;
						zd++;
					}
					
					for(int z2 = z; z2 < z+zd; z2++) {
						for(int y2 = y; y2 < y+yd; y2++) {
							for(int x2 = x; x2 < x+xd; x2++) {
								GETCOORDS(x2d2,y2d2, x2,y2,z2);
								image2[y2d2 * w + x2d2] = 0;
							}
						}
					}
					
					ERR("%d,%d,%d BLOCK SIZE HERE %d,%d,%d\n",x,y,z,xd,yd,zd);
					
					voxel(x, y, z, xd, yd, zd, color);
				}
			}
		}
	}
	
	return true;
}

void createTiledBox(Mesh *m, Number w, Number d, Number h) {
	Polycode::Polygon *polygon = new Polycode::Polygon();
	polygon->addVertex(w,0,h, w, h);
	polygon->addVertex(0,0,h, w, 0);
	polygon->addVertex(0,0,0,0,0);
	polygon->addVertex(w,0,0,0,h);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(w,d,h, w, h);
	polygon->addVertex(w,d,0, w, 0);
	polygon->addVertex(0,d,0,0,0);
	polygon->addVertex(0,d,h,0,h);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,d,0,0,d);
	polygon->addVertex(w,d,0, w, d);
	polygon->addVertex(w,0,0, w, 0);
	polygon->addVertex(0,0,0,0,0);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,0,h,0,0);
	polygon->addVertex(w,0,h, w, 0);
	polygon->addVertex(w,d,h, w, d);
	polygon->addVertex(0,d,h,0,d);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,0,h,0,d);
	polygon->addVertex(0,d,h, h, d);
	polygon->addVertex(0,d,0, h, 0);
	polygon->addVertex(0,0,0,0,0);
	m->addPolygon(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(w,0,h,0,d);
	polygon->addVertex(w,0,0, h, d);
	polygon->addVertex(w,d,0, h, 0);
	polygon->addVertex(w,d,h,0,0);
	m->addPolygon(polygon);
	
	for(int i=0; i < m->polygons.size(); i++) {
		for(int j=0; j < m->polygons[i]->getVertexCount(); j++) {
			m->polygons[i]->getVertex(j)->x = m->polygons[i]->getVertex(j)->x - (w/2.0f);
			m->polygons[i]->getVertex(j)->y = m->polygons[i]->getVertex(j)->y - (d/2.0f);
			m->polygons[i]->getVertex(j)->z = m->polygons[i]->getVertex(j)->z - (h/2.0f);	
		}
	}
	
	m->calculateNormals();
	m->calculateTangents();
	m->arrayDirtyMap[RenderDataArray::VERTEX_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::COLOR_DATA_ARRAY] = true;				
	m->arrayDirtyMap[RenderDataArray::TEXCOORD_DATA_ARRAY] = true;						
	m->arrayDirtyMap[RenderDataArray::NORMAL_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::TANGENT_DATA_ARRAY] = true;									
}

physics_voxel_loader::physics_voxel_loader(ExtCollisionScene *__scene, room_auto *_room) : _scene(__scene), room(_room) {}

ExtCollisionScene *physics_voxel_loader::scene() {
	if (!_scene) _scene = new ExtCollisionScene();
	return _scene;
}

#define C_PLAYER 0xFFFFFFFF
#define C_DIAMOND 0xFF000000

void physics_voxel_loader::voxel(int _x, int _y, int _z, int xd, int yd, int zd, unsigned int color) {
//	ERR("%d, %d, %d, %x\n", x, y, z, color);
	if (!room) return;
	double x = _x + xd*0.5, y = _y + yd*0.5, z = _z + zd*0.5;
	
	if (color) {
		if (color == C_PLAYER) {
			room->playerAt = Vector3(x,y,z);
		} else if (color == C_DIAMOND) {
			SceneEcho *box = new SceneEcho(room->block);
			box->setPosition(x, y, z);
			box->setColor(1.0,1.0,1.0,1.0);
//			box->setColor(random()/RANDOM_MAX,random()/RANDOM_MAX,random()/RANDOM_MAX,1.0);
			scene()->addCollisionChild(box, CollisionSceneEntity::SHAPE_BOX);
			box->custEntityType = "exit";
			room->owned_screen.push_back(box);
//			room->diamondsAt.push_back(Vector3(x,y,z));
		} else {
			SceneMesh *box = new SceneMesh(Mesh::QUAD_MESH);
			createTiledBox( box->getMesh(), xd,yd,zd);
			box->bBox = box->getMesh()->calculateBBox();
			box->setPosition(x, y, z);
			if ((color & C_DIAMOND) != C_DIAMOND) { // If alpha<1.0
				int swerveby = (color >> 24) & 0xF;
				int swerveas = (color >> 28) & 0xF;
				room->diamondsAt.push_back(Vector3(swerveby,swerveas,0));
				room->carpetsAt.push_back(box);
				color |= C_DIAMOND;
			}
			box->setColor(color);
			box->setMaterialByName("CubeMaterial");
			scene()->addCollisionChild(box, CollisionSceneEntity::SHAPE_BOX);			
			room->owned_screen.push_back(box);
		}
	}
}
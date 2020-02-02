/*
 *  cuber.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 5/9/12.
 *  Copyright 2012 Run Hello. All rights reserved.
 *
 */

#include "cuber.h"

Cuber::Cuber(Vox *_vox, bool _virtualScene) : Scene(_virtualScene), vox(_vox) {
}

void createTiledBox(Mesh *m, Number w, Number d, Number h) { // Note: Mesh owns polygons
	vector <Polycode::Polygon*> polygons;
	
	Polycode::Polygon *polygon = new Polycode::Polygon();
	polygon->addVertex(w,0,h, w, h);
	polygon->addVertex(0,0,h, w, 0);
	polygon->addVertex(0,0,0,0,0);
	polygon->addVertex(w,0,0,0,h);
	polygons.push_back(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(w,d,h, w, h);
	polygon->addVertex(w,d,0, w, 0);
	polygon->addVertex(0,d,0,0,0);
	polygon->addVertex(0,d,h,0,h);
	polygons.push_back(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,d,0,0,d);
	polygon->addVertex(w,d,0, w, d);
	polygon->addVertex(w,0,0, w, 0);
	polygon->addVertex(0,0,0,0,0);
	polygons.push_back(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,0,h,0,0);
	polygon->addVertex(w,0,h, w, 0);
	polygon->addVertex(w,d,h, w, d);
	polygon->addVertex(0,d,h,0,d);
	polygons.push_back(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(0,0,h,0,d);
	polygon->addVertex(0,d,h, h, d);
	polygon->addVertex(0,d,0, h, 0);
	polygon->addVertex(0,0,0,0,0);
	polygons.push_back(polygon);
	
	polygon = new Polycode::Polygon();
	polygon->addVertex(w,0,h,0,d);
	polygon->addVertex(w,0,0, h, d);
	polygon->addVertex(w,d,0, h, 0);
	polygon->addVertex(w,d,h,0,0);
	polygons.push_back(polygon);
	
	for(int i=0; i < polygons.size(); i++) {
		for(int j=0; j < polygons[i]->getVertexCount(); j++) {
			polygons[i]->getVertex(j)->x = polygons[i]->getVertex(j)->x - (w/2.0f);
			polygons[i]->getVertex(j)->y = polygons[i]->getVertex(j)->y - (d/2.0f);
			polygons[i]->getVertex(j)->z = polygons[i]->getVertex(j)->z - (h/2.0f);	
		}
	}
	
	for(int c = 0; c < polygons.size(); c++)
		m->addPolygon(polygons[c]);
	
	m->calculateNormals();
	m->calculateTangents();
	m->arrayDirtyMap[RenderDataArray::VERTEX_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::COLOR_DATA_ARRAY] = true;				
	m->arrayDirtyMap[RenderDataArray::TEXCOORD_DATA_ARRAY] = true;						
	m->arrayDirtyMap[RenderDataArray::NORMAL_DATA_ARRAY] = true;		
	m->arrayDirtyMap[RenderDataArray::TANGENT_DATA_ARRAY] = true;									
}

Mesh *Cuber::commonCube() {
	static Mesh *__mesh = NULL;
	if (!__mesh) {
		__mesh = new Mesh(Mesh::QUAD_MESH);
		createTiledBox(__mesh, 1,1,1);
	}	
	return __mesh;
}

void Cuber::Render(Camera *targetCamera) {
	if (!vox) return;
	
	if(!targetCamera && !activeCamera)
		return;
	
	if(!targetCamera)
		targetCamera = activeCamera;
	
	Renderer *renderer = CoreServices::getInstance()->getRenderer();
	
	if(useClearColor)
		renderer->setClearColor(clearColor.r,clearColor.g,clearColor.b);	
	
	targetCamera->rebuildTransformMatrix();
	
	targetCamera->doCameraTransform();
	targetCamera->buildFrustrumPlanes();
	
	if(targetCamera->getOrthoMode()) {
		renderer->_setOrthoMode();
	}
		
	// DRAW
	
	Mesh *mesh = commonCube();
	
	Number bx = -vox->xdim/2.0, by = -vox->ydim/2.0, bz = -vox->zdim/2.0;
	for(int z = 0; z < vox->zdim; z++) {
		for(int y = 0; y < vox->ydim; y++) {
			for(int x = 0; x < vox->xdim; x++) {
				const Color &color = vox->get(x,y,z);
				if (color.a == 0)
					continue;
				renderer->pushMatrix();
				renderer->translate3D(bx+x,by+vox->ydim-y-1,bz+z);
				renderer->setVertexColor(color.r,color.g,color.b,color.a);
				renderer->setTexture(NULL);
				renderer->pushDataArrayForMesh(mesh, RenderDataArray::VERTEX_DATA_ARRAY);
			//	renderer->pushDataArrayForMesh(mesh, RenderDataArray::TEXCOORD_DATA_ARRAY);
				renderer->drawArrays(mesh->getMeshType());
				renderer->popMatrix();
			}
		}
	}
	
	// FINISH DRAW
	
	for(int i=0; i<entities.size();i++) {
		if(entities[i]->getBBoxRadius() > 0) {
			if(targetCamera->isSphereInFrustrum((entities[i]->getPosition()), entities[i]->getBBoxRadius()))
				entities[i]->transformAndRender();
		} else {
			entities[i]->transformAndRender();		
		}
	}
	
	if(targetCamera->getOrthoMode()) {
		CoreServices::getInstance()->getRenderer()->setPerspectiveMode();
	}
	
};
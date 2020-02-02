#ifndef _SVGLOADER_H
#define _SVGLOADER_H

/*
 *  svgload.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 12/5/11.
 *  Copyright 2011 Run Hello. All rights reserved.
 *
 */

class TiXmlElement;

class svgtransform;

enum svgtype {
	svg_rect,
	svg_circle,
	svg_mesh,
	svg_floater,
};

class svgloader {
protected:
	Screen *_screen;
	virtual Screen *createScreen();
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
	virtual bool loadGroup(TiXmlElement *group, const svgtransform &parent_transform);
	virtual bool loadXml(TiXmlElement *xml, const svgtransform &parent_transform);
	virtual bool loadRootXml(TiXmlElement *xml, const svgtransform &parent_transform);
public:
	svgloader(Screen *__screen = NULL);
	Screen *screen();
	virtual bool load(const string &filename); // true for success
};

class physics_svgloader : public svgloader {
#if 0
protected:
	virtual PhysicsScreen *createScreen();
	virtual bool addChild(ScreenEntity *shape, TiXmlElement *xml, svgtype kind, const svgtransform &transform);
public:
	physics_svgloader(PhysicsScreen *__screen = NULL);
	PhysicsScreen *physics() { return (PhysicsScreen *)screen(); }	
#endif
};

#endif /* _SVGLOADER_H */
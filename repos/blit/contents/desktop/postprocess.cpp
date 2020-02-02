/*
 *  postprocess.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/6/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "kludge.h"
#include "game.h"
#include "display_ent.h"
#include "test_ent.h"
#include "program.h"
#include "glCommon.h"
#include "glCommonMatrix.h"
#include "postprocess.h"

void framebuffer_source::vacate() {
	jcDeleteFramebuffers(1, &framebuffer); framebuffer = 0;
	jcDeleteRenderbuffers(1, &renderbuffer); renderbuffer = 0;
}

void framebuffer_source::create() {
	// TODO: Fix those constants
	jcGenFramebuffers(1, &framebuffer);
	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer);
	jcGenRenderbuffers(1, &renderbuffer);
	jcBindRenderbuffer(GL_RENDERBUFFER_EXT, renderbuffer);
	jcRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, outTexture->twidth, outTexture->theight); // Should be width, height?
	jcFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer);
	
	jcFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, outTexture->texture, 0);
	
	GLERR("RTT gen");
	
#ifdef SELF_EDIT
	GLenum status = jcCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	ERR("Framebuffer status %d (desired %d)\n", (int)status, (int)GL_FRAMEBUFFER_COMPLETE_EXT);
#endif

	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}

void framebuffer_source::enter() {
	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer);
	GLERR("RTT enter");
	// DO SOMETHING WITH VIEWPORTS?!
}

void framebuffer_source::exit() {
	jcBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}

void framebuffer_source::framebuffer_swap(framebuffer_source *other) {
	outTexture->texture_swap(other->outTexture);

	GLuint temp = framebuffer;
	framebuffer = other->framebuffer;
	other->framebuffer = temp;
	
	temp = renderbuffer;
	renderbuffer = other->renderbuffer;
	other->renderbuffer = temp;
}

// Is this useful?
cpVect framebuffer_source::size() {
	return cpv(outTexture->twidth, outTexture->theight);
}

void target_ent::display(drawing *) {	
	drawing d(framebuffer->outTexture);
	
	enter();
	
	// IS THIS NEEDED ?!
	rinseGl();
	glClearColor(0.0,0.0,0.0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, framebuffer->outTexture->sub_width(), framebuffer->outTexture->sub_height());
//	goOrtho();
//  jcLoadIdentity();
	
	ent::display(&d); // SKIP OVER DISPLAY_ENT
	
	d.execute();
	
	exit();
	
	// Note: assuming "return to baseline", which may not be accurate (say if target_ents nest?)
	glViewport(0, 0, surfacew, surfaceh);
	goOrtho();
	jcLoadIdentity();
	
	GLERR("RTT frame");
}

void target_ent::enter() {
	if (framebuffer) framebuffer->enter();
}

void target_ent::exit() {
	if (framebuffer) framebuffer->exit();
}

texture_slice *simple_texture(int w, int h) {
	texture_slice *drawInto = new texture_slice();
	drawInto->init(w, h);
	drawInto->construct();
	return drawInto;
}

texture_slice *filter::make_texture(int c, int of) {
	return simple_texture(pw, ph);
}

ent *filter::make_splatter(texture_slice *from, stateoid *with, int c, int of) {
	return new splatter(new texture_source(from), true, with);
}

void filter::inserting() {
	vector <stateoid *> statesFrom = stateSetter;
	if (!statesFrom.size()) statesFrom.push_back(NULL); // Always ensure at least one target
	ent *lastSource = source;
	
	int of = statesFrom.size();
	for(int c = 0; c < of; c++) {
		texture_slice *drawInto = make_texture(c, of);

		target_ent *t = new target_ent(drawInto, true);
		t->insert(this, ENT_PRIO_FBTARGET);
		target.push_back(t);
		
		if (lastSource)
			lastSource->insert(t);
		lastSource = make_splatter(t->display_texture(), statesFrom[c], c, of);
	}
	lastSource->insert(this);
}

ent *fixed_filter::make_splatter(texture_slice *from, stateoid *with, int c, int of) {
	if (c < of-1)
		return filter::make_splatter(from, with, c, of);
	else
		return new fixed_splatter(new texture_source(from), true, with, size, offset);
}
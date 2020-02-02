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
#include <plaid/audio.h>
#include <plaid/audio/synth.h>
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
	
	GLenum status = jcCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	ERR("Framebuffer status %d (desired %d)\n", (int)status, (int)GL_FRAMEBUFFER_COMPLETE_EXT);
	
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
	drawing d;
	d.size(framebuffer->outTexture);
	
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
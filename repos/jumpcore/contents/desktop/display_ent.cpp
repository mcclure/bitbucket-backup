/*
 *  display_ent.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 10/28/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "display_ent.h"
#include "glCommon.h"
#include "glCommonMatrix.h"
#include "input.h"
#include "text_display.h"
#include "internalfile.h"

ent *global_ent = new display_ent();

// Infrastructure

texture_source::texture_source(texture_slice *s, bool own) : code(display_code_unique()) {
	texture_load(s, ownTexture);
}
texture_source::texture_source(const char *name) : code(display_code_unique()) {
	texture_load(name);
}
texture_source::~texture_source() {
	if (ownTexture) delete outTexture;
}
void texture_source::texture_load(texture_slice *s, bool own) {
	outTexture = s;
	ownTexture = own;
}
void texture_source::texture_load(const char *name) {
	texture_slice *s = new texture_slice();
	s->load(name);
	texture_load(s,true);
}

// Ents

void lockout::display(drawing *) {
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	jcImmediateColor4f(0.0,0.0,0.0,1.0);
	drawText("Sorry, an OpenGL 2.0 compatible video card is required.", 0, 0.1, 0, true, true);
	drawText("Press esc to quit.", 0, -0.1, 0, true, true);
}

void display_ent::display(drawing *) {
	jcPushMatrix(); // When can I remove these?
	drawing d;
	
	ent::display(&d);
	
	d.execute();
	jcPopMatrix();
	GLERR("display");
}

void eraser::display(drawing *) {
	glClearColor(r,g,b,a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void splatter::display(drawing *d) {
	cpVect from = cpv(-d->w, d->h), to = cpv(d->w, -d->h);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(src->code, true, false, src->outTexture->texture);
	into.stateSetter = stateSetter;
	into.push4(verts, WHITE, src->outTexture);
}

void fixed_splatter::display(drawing *d) {
	cpVect from = cpvadd(cpvscale(size, cpv(-1, 1)), offset), to = cpvadd(cpvscale(size, cpv(1, -1)), offset);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(src->code, true, false, src->outTexture->texture);
	into.stateSetter = stateSetter;
	into.push4(verts, WHITE, src->outTexture);
}

void boxer::display(drawing *d) {
	cpVect from = cpvadd(cpvscale(size, cpv(-1, 1)), offset), to = cpvadd(cpvscale(size, cpv(1, -1)), offset);
	cpVect verts[4] = RECT(from, to);
	quadpile &into = d->get_quadpile(code, false, false);
	into.push4(verts);
}

// misc


extern "C" {
	int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
}
#define SCREENSHOTTER_SCRATCH_MAX 128

screenshotter::screenshotter(uint32_t _trigger, texture_source *_texture, string basename)
		: ent(), snapshots(1), trigger(_trigger), texture(_texture) {
	char filename_scratch[SCREENSHOTTER_SCRATCH_MAX];

	time_t rawtime;
	struct tm * timeinfo;
	
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	basename = basename + "-%m.%d.%Y-%H.%M.%S-%%d.png"; // Judge me if you must
	strftime(filename_scratch, SCREENSHOTTER_SCRATCH_MAX, basename.c_str(), timeinfo);
	filename = filename_scratch;
}

void screenshotter::input(InputData *data) {
	if (data->inputcode == trigger) {
		quickTake(texture ? texture->outTexture : NULL);
	}
	
	ent::input(data);
}

void screenshotter::quickTake(texture_slice *from) {
	char filename2[FILENAMESIZE];
	userPath(filename2, filename.c_str(), snapshots++);

	take(filename2, from ? from->texture : 0);
	
}

// FIXME: Is YFLIP broken in nil txture case?
void screenshotter::take(string filename, GLuint texture) {
	int textureWidth, textureHeight;
	glBindTexture(GL_TEXTURE_2D, texture);
	if (texture) {
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
	} else {
		textureWidth = surfacew;
		textureHeight = surfaceh;
	}
	int size = textureWidth*textureHeight;
	vector<unsigned int> data;
	data.resize(size);

	if (texture) {
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	} else {
		glReadPixels(0, 0, textureWidth, textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	}
	GLERR("Snapshot");
	
	if (!YFLIP) {
		// Swap columns top to bottom
		vector<unsigned int> dataRow;
		dataRow.resize(textureWidth);
		for(int y = 0; y < textureHeight/2; y++) {
			int topRow = y*textureWidth;
			int botRow = (textureHeight-y-1)*textureWidth;
			memcpy(&dataRow[0],   &data[topRow], textureWidth*sizeof(&data[0]));
			memcpy(&data[topRow], &data[botRow], textureWidth*sizeof(&data[0]));
			memcpy(&data[botRow], &dataRow[0],   textureWidth*sizeof(&data[0]));
		}
	}

	stbi_write_png(filename.c_str(), textureWidth, textureHeight, 4, &data[0], 4*textureWidth);
}

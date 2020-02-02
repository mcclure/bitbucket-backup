/*
 *  voxel_loader.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/24/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#include "program.h"
#include "cuber.h"
#include "vox.h"
#include "physfs.h"
#include "lodepng.h"
#include "bridge.h"

#define HYPERVERBAL 1

#if HYPERVERBAL
#define HYPERERR ERR
#else
#define HYPERERR EAT
#endif

static Color pure_transparent(0.0,0.0,0.0,0.0);

Vox::Vox(int _xdim, int _ydim, int _zdim) {
	resize(_xdim, _ydim, _zdim);
}

void Vox::resize(int _xdim, int _ydim, int _zdim) {
	xdim = _xdim; ydim = _ydim; zdim = _zdim;
	data.resize(xdim*ydim*zdim);
}

Vox *Vox::load(const string &filename, uint32_t transparent) { // true for success
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
	ERR("Loading voxels %s: 2D: Width %d height %d\n", filename.c_str(), w, h);
	
	xdim = w; ydim = h; zdim = 1;
	
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
		return NULL;
	
#define GETCOORDS(_x,_y, _3x,_3y,_3z) int _x = _3z*xdim + _3x, _y = _3y
	
	HYPERERR("----------------\n");
	
	Vox *result = new Vox(xdim, ydim, zdim);
	uint32_t *image2 = (uint32_t *)&image[0];
	for(int z = 0; z < zdim; z++) {
		for(int y = 0; y < ydim; y++) {
			for(int x = 0; x < xdim; x++) {
				GETCOORDS(x2d,y2d, x,y,z);
				unsigned int color = image2[y2d * w + x2d]; // TODO: WON'T SUPPORT PPC ENDIAN!!
				
				Color pcolor(color);
				if (color != transparent && pcolor.a) {
					result->set(pcolor, x, y, z);
					HYPERERR("X ");
				} else {
					result->set(Color(0.0,0.0,0.0,0.0), x, y, z);
					HYPERERR("  ");
				}
			}
			HYPERERR("\n");
		}
		HYPERERR("----------------\n");
	}
	
	return result;
}

void Vox::clear(int tx, int ty, int tz, int dx, int dy, int dz) {
	adjust(dx,dy,dz);
	for(int z = 0; z < dz && tz+z < zdim; z++) {
		if (tz+z<0) continue;
		for(int y = 0; y < dy && ty+y < ydim; y++) {
			if (ty+y<0) continue;
			for(int x = 0; x < dx && tx+x < xdim; x++) {
				if (tx+x<0) continue;
				set(pure_transparent,tx+x,ty+y,tz+z);
			}
		}
	}
}

// Notice: zflip turns sideways and *also* reverses the d direction when blitting "into"
void Vox::xzblit(bool xzcenter, bool xzflip, const Vox *src, int tx, int ty, int tz, int sx, int sy, int sz, int dx, int dy, int dz) {
	src->adjust(dx,dy,dz);
	if (xzcenter) {
		if (!xzflip) {
			tx -= dx/2; tz -= dz/2;
		} else {
			tx -= dz/2; tz += dx/2;
		}
	}
	for(int z = 0; z < dz && sz+z < src->zdim; z++) {
		for(int y = 0; y < dy && ty+y < ydim && sy+y < src->ydim; y++) {
			if (ty+y<0 || sy+y<0) continue;
			for(int x = 0; x < dx && sx+x < src->xdim; x++) {
				int txx, tzz;
				if (!xzflip) {
					txx = tx+x; tzz = tz+z;
				} else {
					txx = tx+z; tzz = tz-x;
				}
				if (tzz<0 || sz+z<0) continue; // Underflow z, skip foward toward next z loop
				if (!(tzz < zdim)) continue; // Overflow z, skip forward toward next z loop
				if (txx<0 || sx+x<0) continue; // Underflow x, skip to next x loop
				if (!(txx < xdim)) break; // Overflow x, skip to next x loop
				
				const Color &setcolor = src->get(sx+x,sy+y,sz+z);
				if (setcolor.a>0)
					set(setcolor,txx,ty+y,tzz);
			}
		}
	}
	
}

void Vox::blit(const Vox *src, int tx, int ty, int tz, int sx, int sy, int sz, int dx, int dy, int dz) {
	xzblit(false, false, src,tx,ty,tz,sx,sy,sz,dx,dy,dz);
}
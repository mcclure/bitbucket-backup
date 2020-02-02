#ifndef _VOX_H
#define _VOX_H

/*
 *  vox.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 2/24/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#include "program.h"

struct Vox {
	int xdim; int ydim; int zdim;
	vector<Color> data;
	Vox(int _xdim = 0, int _ydim = 0, int _zdim = 0);
	inline int index(int x, int y, int z = 0) const { return z*(xdim*ydim) + y*xdim + x; }
	inline Color get(int x, int y, int z = 0) const { return data[index(x,y,z)]; }
	inline void set(const Color &n, int x, int y, int z = 0) { data[index(x,y,z)] = n; }
	void resize(int _xdim = 0, int _ydim = 0, int _zdim = 0);
	void clear(int tx = 0, int ty = 0, int tz = 0, int dx = -1, int dy = -1, int dz = -1);
	void xzblit(bool center, bool xzflip, const Vox *src, int tx, int ty, int tz = 0, int sx = 0, int sy = 0, int sz = 0, int dx = -1, int dy = -1, int dz = -1);
	void blit(const Vox *src, int tx, int ty, int tz = 0, int sx = 0, int sy = 0, int sz = 0, int dx = -1, int dy = -1, int dz = -1);
	static Vox *load(const string &filename, uint32_t transparent = 0);
	inline void adjust(int &dx, int &dy, int &dz) const {
		if (dx < 0) dx = xdim;
		if (dy < 0) dy = ydim;
		if (dz < 0) dz = zdim;
	}
};

#endif /* _VOX_H */
#ifndef CP_RECT_H
#define CP_RECT_H

// Include cpVect.h first

// A rectangle class based on cpVect. Not the same thing as a cpBB.
// Any non-inlined methods are defined in util_display.cpp

inline cpVect xflip(cpVect v) { v.x = -v.x; return v; }
inline cpVect yflip(cpVect v) { v.y = -v.y; return v; }

struct cpRect;
cpRect cpr(cpVect,cpVect);

struct cpRect {
	cpVect center;
	cpVect rad;
	
	inline cpVect size() { return cpvmult(rad,2); }
	// Upper left, Upper Right, Down left etc using OpenGL coords
	inline cpVect ul() { return cpvadd(center,xflip(rad)); }
	inline cpVect ur() { return cpvadd(center,rad); }
	inline cpVect dl() { return cpvadd(center,cpvneg(rad)); }
	inline cpVect dr() { return cpvadd(center,yflip(rad)); }
	
	inline cpFloat u() { return center.y+rad.y; }
	inline cpFloat d() { return center.y-rad.y; }
	inline cpFloat l() { return center.x-rad.x; }
	inline cpFloat r() { return center.x+rad.x; }
	
	inline cpVect vert(int at) {
		switch (at) {
			case 0: return dl();
			case 1: return ul();
			case 2: return ur();
			case 3: return dr();
		}
	}
	inline void fill(cpVect *v) {
		v[0] = dl(); v[1] = ul(); v[2] = ur(); v[3] = dr();
	}
	
	inline cpRect translate(cpVect by) {
		return cpr(cpvadd(center,by), rad);
	}
	
	inline cpRect scale(cpFloat factor) { // Toward 0
		return cpr(cpvmult(center,factor), cpvmult(rad,factor));
	}
	
	inline cpRect inset(cpFloat factor) {
		return cpr(center, cpvmult(rad,factor));
	}
	
	inline cpRect inset_fixed(cpFloat by) {
		cpVect r = cpv(max<cpFloat>(0,rad.x-by*2),
					   max<cpFloat>(0,rad.y-by*2));
		return cpr(center, r);
	}

	inline cpRect align_u(float to) {
		cpRect r = *this;
		r.center.y += to-(r.center.y+r.rad.y);
		return r;
	}
	inline cpRect align_d(float to) {
		cpRect r = *this;
		r.center.y += to-(r.center.y-r.rad.y);
		return r;
	}
	inline cpRect align_l(float to) {
		cpRect r = *this;
		r.center.x += to-(r.center.x-r.rad.x);
		return r;
	}
	inline cpRect align_r(float to) {
		cpRect r = *this;
		r.center.x += to-(r.center.x+r.rad.x);
		return r;
	}		
	
	inline cpRect set_u(float to) {
		cpRect r = *this;
		cpFloat offset = (to-(r.center.y+r.rad.y))/2;
		r.center.y += offset;
		r.rad.y += offset;
		return r;
	}
	inline cpRect set_d(float to) {
		cpRect r = *this;
		cpFloat offset = (to-(r.center.y-r.rad.y))/2;
		r.center.y += offset;
		r.rad.y -= offset;
		return r;
	}
	inline cpRect set_l(float to) {
		cpRect r = *this;
		cpFloat offset = (to-(r.center.x-r.rad.x))/2;
		r.center.x += offset;
		r.rad.x -= offset;
		return r;
	}
	inline cpRect set_r(float to) {
		cpRect r = *this;
		cpFloat offset = (to-(r.center.x+r.rad.x))/2;
		r.center.x += offset;
		r.rad.x += offset;
		return r;
	}	
	
	inline cpRect reflect_x() {
		cpRect r = *this;
		r.center.x = -r.center.x;
		return r;
	}
	
	inline cpRect reflect_y() {
		cpRect r = *this;
		r.center.y = -r.center.y;
		return r;
	}
	
	static inline cpRect square(cpFloat r) {
		return cpr(cpvzero, cpv(r,r));
	}
};

inline cpRect cpr(cpVect center,cpVect rad) {
	cpRect r = {center, rad};
	return r;
}

inline cpRect cpbounds(cpVect ul, cpVect dr) {
	cpRect temp = {cpvzero, cpvzero};
	cpFloat u,d,l,r;
	if (ul.x > dr.x) {
		r = ul.x; l = dr.x;
	} else {
		l = ul.x; r = dr.x;
	}
	if (ul.y > dr.y) {
		u = ul.y; d = dr.y;
	} else {
		d = ul.y; u = dr.y;
	}
	return temp.set_u(u).set_l(l).set_d(d).set_r(r);
}

#endif // CP_RECT_H
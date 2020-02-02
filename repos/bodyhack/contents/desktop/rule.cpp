/*
 *  rule.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/25/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "program.h"
#include "rule.h"

float rule::power() {
	float a = 0;
	for(int c = 0; c < 9; c++)
		a += data[c];
	return a;
}

void rule::norm() {
	float p = power();
	if (p) {
		for(int c = 0; c < 9; c++)
			data[c] /= p;
	}
}

void rule::offset_norm(float target) {
	float p = power();
	p = (target - p)/9;
	for(int c = 0; c < 9; c++)
		data[c] += p;
}

void rule::bounded(float &low, float &high) {
	low = data[0]; high = data[0];
	for(int c = 1; c < 9; c++) {
		float &f = data[c];
		if (f < low) low = f;
		if (f > high) high = f;
	}
}

void rulecross(rule &r, float intens, float skew) {
	for(int y = 0; y < 3; y++) {
		for(int x = 0; x < 3; x++) {
			if (x != 1 || y != 1) {
				r.v(x,y) += (intens + skew*(y-1)) * ((x+y+1)%2?-1:1);
			}
		}
	}
}

void rulechaos(rule &r) {
	for(int y = 0; y < 3; y++) {
		for(int x = 0; x < 3; x++) {
			unsigned int k = random();
			r.v(x,y) = k * (k&1?-1:1);
		}
	}
	r.norm();
}

void ruleblur(rule &r) {
	for(int y = 0; y < 3; y++) {
		for(int x = 0; x < 3; x++) {
			r.v(x,y) = (x == 1 && y == 1 ? 9 : 1);
		}
	}
	r.norm();
}
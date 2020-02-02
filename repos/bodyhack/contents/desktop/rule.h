#ifndef _RULE_H
#define _RULE_H

/*
 *  rule.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 11/25/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */



struct rule {
	rule(float aa = 0, float ab = 0, float ac = 0, float ba = 0, float bb = 1, float bc = 0, float ca = 0, float cb = 0, float cc = 0) {
		data[0] = aa; data[1] = ab; data[2] = ac; data[3] = ba; data[4] = bb; data[5] = bc; data[6] = ca; data[7] = cb; data[8] = cc;
	}
	GLfloat data[9];
	void unload(GLfloat *into, int offset = 0) {
		memcpy(into + offset*9, data, sizeof(data));
	}
	float &v(const int x, const int y) {
		return data[y*3+x];
	}
	void bounded(float &low, float &high);
	void print() {
		for(int c = 0; c < 9; c++) {
			ERR("%4f", data[c]);
			if ((c+1)%3) ERR("\t"); else if (c<8) ERR("\n");
		}
		ERR("\tp=%f\n\n", power());
	}
	float power();
	void norm();
	void offset_norm(float target=1);
};

void rulecross(rule &r, float intens, float skew);
void rulechaos(rule &r);
void ruleblur(rule &r);

#endif
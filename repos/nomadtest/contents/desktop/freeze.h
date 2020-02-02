/*
 *  freeze.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/8/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _FREEZE_H
#define _FREEZE_H

#include <stdio.h>
#include <kludge.h>

#define F_WRITE(x) (sizeof(x) == fwrite((char *) &(x), 1, sizeof(x), f))
#define F_READ(x) (sizeof(x) == fread((char *) &(x), 1, sizeof(x), f))
#define F_STRINGMAX 256

inline void f_writes(FILE *f, const string &s) {
	uint32_t tempCount = s.length();
	F_WRITE(tempCount);
	fwrite(s.c_str(), 1, tempCount, f);
}

inline string f_reads(FILE *f) {
	uint32_t tempCount;
	F_READ(tempCount);
	int readto = min<int>(F_STRINGMAX,tempCount);
	char filename[F_STRINGMAX]; // Wait WTF, no
	fread(filename, 1, readto, f);
	if (readto < tempCount)
		fseek(f, tempCount-readto, SEEK_CUR);
	return string(filename, readto);
}

template<typename T>
inline void f_write(FILE *f, T v) {
	F_WRITE(v);
}

template<typename T>
inline T f_read(FILE *f) {
	T temp;
	F_READ(temp);
	return temp;
}

inline void f_write32(FILE *f, uint32_t v) { f_write(f, v); }
inline uint32_t f_read32(FILE *f) { return f_read<uint32_t>(f); }

inline void f_write8(FILE *f, uint8_t v) { f_write(f, v); }
inline uint32_t f_read8(FILE *f) { return f_read<uint8_t>(f); }

struct freeze {
	virtual void f_load(FILE *f) {}
	virtual void f_loadfile(const string &path);
	virtual void f_save(FILE *f) {}
	virtual void f_savefile(const string &path);
};

#endif
#ifndef _DCLIB_H
#define _DCLIB_H
/*
 *  dclib.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 8/15/11.
 *  Copyright 2011 Andi McClure. All rights reserved.
 *
 */

#define METRO_AT_START 0.4

struct sounding;

struct cpBody;
struct subtexture_slice;
struct chip;
struct guidedata;
typedef hash_map<int,cpBody *>::iterator chip_iter;

struct bodydata {
    unsigned int collision_type;
    bodydata(unsigned int _collision_type) : collision_type(_collision_type) {}
};

struct sample_inst {
	string name;
	double *samp;
	int len;
	int pmod;
	int suit, card;
	bool blanked;
	subtexture_slice *icon;
	sample_inst(const char *_name = NULL, double *_samp = NULL, int _len = 0, int _pmod = 0, subtexture_slice *_icon = 0) : name(_name), samp(_samp), len(_len), pmod(_pmod), suit(0), card(0), blanked(false), icon(_icon) {}
	string shortname() { return name; } // For now same as long name
};

struct chip_payload {
	int p;
	sample_inst *samp;
	chip_payload(int _p, sample_inst *_samp = NULL) : p(_p), samp(_samp) {}
	subtexture_slice *icon();
	double h();
	double f();
	bool oversize();
	string name();
	void set_blank(bool _blank) { if (samp) samp->blanked = _blank; }
};

struct chip : public bodydata {
    unsigned int id;
	chip_payload ip;
    double when;
	bool wildfire;
    unsigned int fired;
	unsigned int updated;
	
	chip();
	static unsigned int updated_gen;
	static unsigned int updated_now() { return ++updated_gen; }
};

#define BDTYPE(d) (((bodydata *)d)->collision_type)


struct circle {

	sounding *soundroot;
	
	double nowat;

	double amp;
	
	int playing;
	
	hash_map<int,cpBody *> chips;
	
	pthread_mutex_t live_mutex;
	
	double metro_at;
	
	bool soundpaused;
	
	bool toggle_fire;
	
	circle();
	circle(const circle &);
	
	// The "tempo". You probably want to redefine this.
	int samplesPerLoop() { return int(200000.0/(METRO_AT_START) * metro_at); }
	
	// Duration of "synthesized" (non-sample) notes
	int chimeDuration() { return 1000; }
};

extern circle mainCircle;

#endif /* DCLIB_H */
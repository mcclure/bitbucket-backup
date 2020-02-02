/*
 *  text_ent.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 8/24/14.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#ifndef _TEXT_ENT_H
#define _TEXT_ENT_H

#include "kludge.h"
#include "display.h"
#include "text_display.h"
#include "ent.h"

// Dynamic text floater
struct text_ent : public ent {
	float x, y;
	bool xcenter, ycenter;
	
	virtual string text() = 0;
	
	text_ent(float _x=0, float _y=0, bool _xcenter=false, bool _ycenter=false) : ent(), x(_x), y(_y), xcenter(_xcenter), ycenter(_ycenter) {}
	void display(drawing *d) {
		jcImmediateColor4f(1.0,1.0,1.0,1.0);	// For debug
		drawText(text(), x, y, 0, xcenter, ycenter);
		ent::display(d);
	}
};

// Fixed-string text floater
struct string_ent : public text_ent {
	string constant_text;

	virtual string text() { return constant_text; }
	
	string_ent(const string &_text, float _x=0, float _y=0, bool _xcenter=false, bool _ycenter=false) : text_ent(_x, _y, _xcenter, _ycenter), constant_text(_text) {}
};

// Multiline/wrap screen -- support

#if 0 // Hyperverbal debug for word wrap
#define TEXTENT_HYPERERR(...) ERR (__VA_ARGS__)
#else
#define TEXTENT_HYPERERR(...)
#endif

#define TEXTENT_STATUS(x) TEXTENT_HYPERERR("%s: \"%s\" , \"%s\"\n", x, first.substr(0, firstTo).c_str(), first.substr(restFrom).c_str())

int maxLines() {
	return 2/textHeight();
}

bool linefits(const string &test) {
	static bool haveMargin = false;
	static float margin;
	if (!haveMargin) {
		margin = 2/aspect - textWidth("SPACE");
		haveMargin = true;
	}
	return textWidth(test) < margin;
}

void adjustSpace(const string &str, int &pos, int *pos2, bool spaces, int step, int offset = 0) {
	while ( !( (step < 0 && (pos+offset) <= 0) || (step > 0 && (pos+offset) >= str.size()-1) ) // Don't go off bounds
		&& ( (!spaces) ^ bool(isspace(str[pos+offset])) ) ) { // Meets criteria
		pos += step;
		if (pos2) *pos2 += step;
	}
}

void linesplit(string &first, string &rest) {
	if (linefits(first))
		return;
	
	// Find knownGood
	int knownGood = 0;
	while (knownGood < first.size() && linefits(first.substr(0, knownGood+1)))
		knownGood++;
	
	int firstTo = knownGood;
	int restFrom = knownGood;
	
	if (restFrom < first.size()) {
		if (isspace(first[restFrom])) {
			TEXTENT_STATUS("Trim spaces line2");
			adjustSpace(first, restFrom, NULL, true, 1);
		} else {
			TEXTENT_STATUS("Move for letters");
			adjustSpace(first, firstTo, &restFrom, false, -1, -1);
		}
		
		TEXTENT_STATUS("Trim spaces line1");
		adjustSpace(first, firstTo, NULL, true, -1, -1);
		TEXTENT_STATUS("DONE"); TEXTENT_HYPERERR("\n");
	}
	
	rest = first.substr(restFrom);
	first = first.substr(0, firstTo);
}

// Multiline/wrap screen -- ents

struct scroller_ent : public ent {
	bool xcenter;
	float hCache; int maxCache;
	list<string> lines;
	scroller_ent(bool _xcenter = true) : ent(), xcenter(_xcenter) {
		hCache = textHeight();
		maxCache = maxLines();
	}
	void pushSanitizedLine(const string &line) {
		lines.push_back(line);
		if (lines.size() >= maxCache)
			lines.pop_front();
	}
	virtual void pushLine(const string &text) {
		string first = text;
		while (1) {
			string rest;
			linesplit(first, rest);
			pushSanitizedLine(first);
			if (!rest.size())
				break;
			first = rest;
		}
	}
	void display(drawing *d) {
		jcImmediateColor4f(1.0,1.0,1.0,1.0);	// For debug
		float x = xcenter ? 0 : -1/aspect;
		int c = 1;
		for(list<string>::iterator i = lines.begin(); i != lines.end(); i++) {
			float y = hCache*c;
			drawText(*i, x, 1-y, 0, xcenter, false);
			c++;
		}
		ent::display(d);
	}
	
	void clear() { lines.clear(); }
	void vcenter() {
		int y = (maxCache - lines.size())/2;
		for(int c = 0; c < y; c++)
			lines.push_front("");
	}
};

// Super cheesy, scroller that forces to CAPS
struct capital_scroller_ent : public scroller_ent {
	capital_scroller_ent(bool _xcenter = true) : scroller_ent(_xcenter) {
		hCache = textHeight();
		maxCache = maxLines();	
	}
	void pushLine(const string &text) {
		string outline;
		for(int c = 0; c < text.length(); c++) {
			outline += toupper(text[c]);
		}
		scroller_ent::pushLine(outline);
	}
};

#endif

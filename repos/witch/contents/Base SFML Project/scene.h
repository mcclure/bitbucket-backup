#pragma once

/*
 *  scene.h
 *  SFML
 *
 *  Created by Andi McClure on 4/27/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#include "audioObjects.h"
#include "postprocess.h"

enum SpringRules {
	SPRING_FREE = 0,
	SPRING_CENTERED = 1,
	SPRING_GROUPED = 2,
	SPRING_CENTERED_AND_GROUPED = 3,
	SPRING_AUTOLINK_ANCHORS = 4,
	SPRING_LINK_GROUP_PLUS_ANCHORS = 6,
};

enum ProcessorSetup {
	PROCESS_NONE,
	PROCESS_FADE,
	PROCESS_BLEED,
	PROCESS_BLEEDTHENFADE,
	PROCESS_LIGHTING
};

enum Song {
	SONG_NONE,
	SONG_BUZZER,
	SONG_WANDER
};

struct Scene {
	string groupImage;
	vector<SpringRules> springRules;
	int particleOften;
	
	string colorImage;
	bool colorVary;
	ProcessorSetup processorSetup;
	
	string backgroundColorImage;

	Song song;
	float songFrequency;
	int songWanderers;
	
	Scene() : processorSetup(PROCESS_NONE), particleOften(0), song(SONG_NONE), songFrequency(0), songWanderers(0) {}
	SpringRules springRule(int idx) { return springRules.size() > idx ? springRules[idx] : SPRING_FREE; }
	void setSpringRules(int idx, SpringRules value) { 
		if (idx >= springRules.size()) springRules.resize(idx+1); 
		springRules[idx] = value;
		if ((value == SPRING_FREE) || (value == SPRING_CENTERED))
			groupTuning[idx].applySpringAttachmentForces = false;
		else
			groupTuning[idx].applySpringAttachmentForces = true;
	}
};

extern Scene *scene;
void setupScene();
void teardownScene();
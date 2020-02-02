/*
 *  input_mapping.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 8/30/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#ifndef _INPUT_MAPPING_H
#define _INPUT_MAPPING_H

#include "kludge.h"
#include "ent.h"
#include "input.h"
#include "json/json-forwards.h"

struct InputWishSpec {
	string value;
	uint32_t inputcode;
	uint32_t axiscode;
	InputWishSpec(const string &_value, uint32_t _inputcode, uint32_t _axiscode)
		: value(_value), inputcode(_inputcode), axiscode(_axiscode) {}
};

struct MappingState {
	vector<InputRuleSpec> currentRules;
	vector<InputWishSpec> currentWishes;
};

struct ControllerLabels {
	ControllerLabels() : fallback(NULL) {}
	hash_map<string, string> labels;
	ControllerLabels *fallback;
};

struct input_mapper : public ent {
	bool found_controllers;
	input_mapper();
	vector<MappingState> stack;
	vector<InputRuleSpec> lastSet;
	void clear();
	void deassign();
	void assign();
	void push (bool inherit = true);
	void pop();
	void addRule(const InputRuleSpec &rule);
	void addKeyboard(SDL_Scancode  value, uint32_t inputcode, uint32_t axiscode = AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	void addJoystick(const string &value, uint32_t inputcode, uint32_t axiscode = AXISCODE_HIGH_MASK|AXISCODE_ZERO_MASK);
	void inserting();
	MappingState &state();
	void input(InputData *);
	
	void registerLabels(const string &name, const Json::Value &json, const string &inherit = string());
	
	bool hasController() { return !firstController.empty(); }
	string firstController;
	hash_map<string, ControllerLabels *> labels;
	string label(const string &button, const string &controller = string());
	string label(SDL_Scancode value);
};

#endif /* _INPUT_MAPPING_H */
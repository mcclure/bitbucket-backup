/*
 *  markov.h
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 1/24/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#include "program.h"

// Notice: lua friendly function first, c++ friendly function second.
struct MarkovModel {
	int hits(String found) { return hits(found.getSTLString()); }
	int hits(const string &found);
	void loadTraining(String inp) { loadTraining(inp.getSTLString()); }
	void loadTraining(const string &inp);
	void loadTrainingFile(String &inp) { loadTrainingFile(inp.getSTLString()); }
	void loadTrainingFile(const string &inp);
	void history_set(String history) { history_set(history.getSTLString()); }
	void history_set(const string &history);
	Number probability(String of) { return probability(of[0]); }
	double probability(char of);
	char probability_max();
	String probability_max_string();
	char guess_next();
	String guess_next_string();
	MarkovModel(int _cap);
protected:
	int cap;
	hash_map <char, char> alphabet;
	hash_map <string, int> record;
	typedef hash_map<char,char>::iterator alphabet_iter;
	
	string cache_history;
	hash_map <char, double> cache_probability;
};
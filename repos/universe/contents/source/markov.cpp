/*
 *  markov.cpp
 *  PolycodeTemplate
 *
 *  Created by Andi McClure on 1/24/12.
 *  Copyright 2012 Tomatogon. All rights reserved.
 *
 */

#include "markov.h"

#if 0 // Hyperverbal
#define HYPERERR ERR
#else
#define HYPERERR EAT
#endif

MarkovModel::MarkovModel(int _cap) : cap(_cap) {}

int MarkovModel::hits(const string &found) {
	if (!record.count(found)) {
		return 0;
	} else {
		return record[found];
	}
}

void MarkovModel::loadTraining(const string &in) {
	for(int c = 0; c < in.size(); c++) {
		const char &ch = in[c];
		alphabet[ch] = in[ch];
		if (!c) continue; // Forget letter 0.
		
		for(int d = 0; d <= c && d < cap; d++) {
			string sub(in, c-d, d);
			record[sub]++;
		}
	}
	cache_history.clear();
	cache_probability.clear();
}

string help_attempt(const string &path);

void MarkovModel::loadTrainingFile(const string &in) {
	loadTraining(help_attempt(in));
}

void MarkovModel::history_set(const string &history) {
	if (cache_history != history) {
		cache_history = history;
		cache_probability.clear();
		for(int chunk = min<int>(history.size(), cap); chunk >= 1; chunk--) {
			string compare(history,history.size()-chunk);
			vector <char> letters;
			vector <int> counts;
			int total = 0;
			for(alphabet_iter i = alphabet.begin(); i != alphabet.end(); i++) {
				string sound = compare + i->first;
				int count = hits(sound);
				letters.push_back(i->first);
				counts.push_back(count);
				total += count;
			}
			if (total) {
				HYPERERR("\nFILLING CACHE\n"); 
				for(int c = 0; c < letters.size(); c++) {
					HYPERERR("Prefix '%s': add '%c': %d/%d = %lf\n", compare.c_str(), letters[c], counts[c], total, double(counts[c])/total);
					cache_probability[letters[c]] = double(counts[c])/total;
				}
				break;
			}
		}
	}	
}

double MarkovModel::probability(char of) {
	return cache_probability[of];
}

char MarkovModel::probability_max() {
	char max_is = '\0';
	double max_prob = -1;
	for(alphabet_iter i = alphabet.begin(); i != alphabet.end(); i++) {
		double this_prob = probability(i->first);
		if (this_prob > max_prob) {
			max_prob = this_prob;
			max_is = i->first;
		}
	}
	return max_is;
}

String MarkovModel::probability_max_string() {
	const char s[2] = {probability_max(), '\0'};
	return String(s);
}

char MarkovModel::guess_next() {
	vector<char> letter;
	vector<double> prob;
	double prob_accum = 0;
	for(alphabet_iter i = alphabet.begin(); i != alphabet.end(); i++) {
		double this_prob = probability(i->first);
		prob_accum += this_prob;
		letter.push_back(i->first);
		prob.push_back(prob_accum);
	}
	double guess = random()/double(RANDOM_MAX);
	for(int c = 0; c < prob.size(); c++) {
		if (prob[c] > guess)
			return letter[c];
	}
	return letter.back();
}

String MarkovModel::guess_next_string() {
	const char s[2] = {guess_next(), '\0'};
	return String(s);
}
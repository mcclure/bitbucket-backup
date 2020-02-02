/*
 *  plaidext.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 12/23/13.
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "plaidext.h"

void NoiseSynth::pull(AudioChunk &chunk, const State &a, const State &b) {
	int32_t *i = chunk.start(0), *e = chunk.end(0);

	while (i != e) {
		*i = float(random())/RANDOM_MAX * AudioFormat::INT24_CLIP;
		i++;
	}

	//Copy signal into all other channels
	for (uint32_t c = chunk.channels()-1; c > 0; --c)
	{
		std::memcpy(chunk.start(c), chunk.start(0), 4*chunk.length());
	}
}

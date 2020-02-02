/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        speed.cpp ( Live Once Library, C++ )
 *
 * COMMENTS:
 */

#include "speed.h"

PHASE_NAMES	glbCurPhase;

int		glbGameMoves = 0;

#define PHASE_LEN	5

// These are very precisely defined!
const PHASE_NAMES glbPhaseOrder[PHASE_LEN] =
{
    PHASE_FAST,
    PHASE_NORMAL,
    PHASE_SLOW,
    PHASE_QUICK,
    PHASE_NORMAL
};

void 
spd_init()
{
    glbGameMoves = 0;
    spd_inctime();
}

void
spd_inctime()
{
    glbGameMoves++;
    glbCurPhase = glbPhaseOrder[glbGameMoves % PHASE_LEN];
}

int
spd_gettime()
{
    return glbGameMoves;
}

void
spd_settime(int time)
{
    glbGameMoves = time;
    glbCurPhase = glbPhaseOrder[glbGameMoves % PHASE_LEN];
}

PHASE_NAMES
spd_getphase()
{
    return glbCurPhase;
}

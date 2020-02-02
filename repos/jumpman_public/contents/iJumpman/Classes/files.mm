/*
 *  files.mm
 *  iJumpman
 *
 *  Created by Andi McClure on 7/19/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "chipmunk.h"
#include "iJumpman.h"
#include <fstream>
#include <sys/stat.h>

uint64_t SDL_GetTicks();
void retilt(spaceinfo *s, cpShape *shape, int tiltnew);
string scoreKeyFor(LoadedFrom from, string filename);
extern uint64_t subTimerStartsAt;

#define WRITE(x) f.write((char *) &(x), sizeof(x))
#define READ(x) f.read((char *) &(x), sizeof(x))

bool didHibernate = false;
bool wantHibernate = false;

#if FOG
extern float fogf;
#endif

inline void s_writes(ofstream &f, string s) {
	int tempCount = s.length();
	WRITE(tempCount);
	f.write(s.c_str(), tempCount);
}

inline string s_reads(ifstream &f) {
	int tempCount;
	READ(tempCount);
	if (tempCount > FILENAMESIZE) return string();
	char filename[FILENAMESIZE];
	f.read(filename, tempCount);
	return string(filename, tempCount);
}

#define WRITES(x) s_writes(f, (x))
#define READS(x) (x) = s_reads(f)

template< typename T> inline void s_writev(ofstream &f, vector<T> &v) {
	int tempCount = v.size();
	WRITE(tempCount);
	if (!tempCount) return;
	f.write((char *)&v[0], tempCount*sizeof(T));
}

template < typename T > inline void s_readv(ifstream &f, vector<T> &v) {
	int tempCount;
	READ(tempCount);
	v.resize(tempCount);
	if (!tempCount) return;
	f.read((char *)&v[0], tempCount*sizeof(T));
}

#define WRITEV(x) s_writev(f, (x))
#define READV(x) s_readv(f, (x))

static const bool vfalse = false, vtrue = true;

// Not a good place for this?
void userPath(char *dst) {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, 
														 NSUserDomainMask, YES); 
	NSString *documentsDirectory = [paths objectAtIndex:0];
	
	snprintf(dst, FILENAMESIZE, "%s", [documentsDirectory cStringUsingEncoding:NSASCIIStringEncoding]);
}

void CachePath(char *dst, const char *fmt, int arg1 = 0, int arg2 = 0) {
	char filename[FILENAMESIZE];
	snprintf(filename, FILENAMESIZE, fmt, arg1, arg2);
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, 
														 NSUserDomainMask, YES); 
	NSString *documentsDirectory = [paths objectAtIndex:0];
	
	snprintf(dst, FILENAMESIZE, "%s/%s", [documentsDirectory cStringUsingEncoding:NSASCIIStringEncoding], filename);
	
	ERR("cache:%s\n", dst); // just curious
}

int objCount = 0; // Does Chipmunk really not offer a way to do this?
void countObject(void *ptr, void *) { 
if (((cpShape*)ptr)->collision_type == C_JUMPMAN) return; // Urg, and this was almost sorta elegant until I added this
objCount++;
}

// Darn vtables, I want to use memcpy.

enemy_info::enemy_info(ifstream &f) {
	READ(lastWhichFrame);
	READ(lastWhichFrame);
	READ(lastReflect);
	READ(noRot);
	READ(tiltid);
	READ(tiltref);
	READ(tiltg);
	READ(megaOffset);
}

void enemy_info::hib(ofstream &f) {
	WRITE(lastWhichFrame);
	WRITE(lastWhichFrame);
	WRITE(lastReflect);
	WRITE(noRot);
	WRITE(tiltid);
	WRITE(tiltref);
	WRITE(tiltg);
	WRITE(megaOffset);
}

ing_info::ing_info(ifstream &f) : enemy_info(f) {
//	READ(runningSince); // Let it be 0 or there will be graphical glitches
    runningSince = 0;
}

void ing_info::hib(ofstream &f) {
	enemy_info::hib(f);
//	WRITE(runningSince);
}

loose_info::loose_info(ifstream &f) : enemy_info(f) {
	READ(m);
	READ(i);
}

void loose_info::hib(ofstream &f) {
	enemy_info::hib(f);
	WRITE(m);
	WRITE(i);
}

// Maybe more general than it needs to be?

void hibObject(void *ptr, void *_data)
{
	cpShape *shape = (cpShape*)ptr;
	ofstream &f = *((ofstream *)_data);
		
	if (shape->collision_type == C_JUMPMAN) return; // FIXME WTF
	
	WRITE(shape->collision_type);
	
	cpBody *body = shape->body;
	if (!body || body == level[jumpman_s].staticBody)
		WRITE(vfalse);
	else {
		WRITE(vtrue);
		WRITE(body->p);
		WRITE(body->v);
		WRITE(body->a);
		WRITE(body->m);
		WRITE(body->i);
	}
	
	if (C_BALLTYPE(shape->collision_type)) {
		cpCircleShape *circle = (cpCircleShape *)shape;
		WRITE(circle->r);
	} else { // TODO: u, e
		cpPolyShape *poly = (cpPolyShape *)shape; // TODO: must store shape->p for shrapnel?
		int num = poly->numVerts;
		cpVect *verts = poly->verts;

		WRITE(num);
		for (int c = 0; c < num; c++) // It's always 4! Why bother?
			WRITE(verts[c]);
	}
	WRITE(shape->u);
	WRITE(shape->e);
	WRITE(shape->layers);
	
	switch(shape->collision_type) {
		case C_JUMPMAN: case C_MARCH: case C_STICKY: case C_SWOOP: case C_BALL: case C_ANGRY: case C_HAPPY:
		case C_ING:
		case C_LOOSE: case C_LOOSE2: case C_LOOSE3: {
			enemy_info *info = (enemy_info *)body->data;
			info->hib(f);
		} break;
		case C_BOMB: case C_BOMB2: {
			enemy_info *info = (enemy_info *)shape->data;
			info->hib(f);
		} break;
		default:break;
	}
}

void loadObject(ifstream &f, bool active) {
	unsigned int collision_type;
	bool hasBody;
	cpFloat tempFloat;
	
	READ(collision_type);
	
	cpBody *body = NULL;
	
	READ(hasBody);
	if (!hasBody) {
		body = level[jumpman_s].staticBody;
	} else {
		body = cpBodyNew(INFINITY, INFINITY);
		READ(body->p);
		READ(body->v);
		READ(tempFloat); cpBodySetAngle(body, tempFloat);
		READ(tempFloat); cpBodySetMass(body, tempFloat);
		READ(tempFloat); cpBodySetMoment(body, tempFloat);
	}

	cpShape *shape = NULL;
	
	if (C_BALLTYPE(collision_type)) {
		READ(tempFloat);
		shape = cpCircleShapeNew(body, tempFloat, cpvzero);
	} else {
		int num;
		cpVect seg_verts[4];
		cpVect offset = cpvzero;
		
		READ(num); num = ::min<unsigned int>(num, 4); // Again, more than 4 will never happen, but...
		for(int c = 0; c < num; c++)
			READ(seg_verts[c]);
		
		shape = cpPolyShapeNew(body, num, seg_verts, offset);
	}
	READ(shape->u);
	READ(shape->e);
	READ(shape->layers);
	
	shape->collision_type = collision_type;
	switch(collision_type) { // "nonportable"
		case C_JUMPMAN: case C_MARCH: case C_STICKY: case C_SWOOP: case C_BALL: case C_ANGRY: case C_HAPPY: case C_BIRD:
			body->data = new enemy_info(f); break;
		case C_ING:
			body->data = new ing_info(f); break;			
		case C_LOOSE: case C_LOOSE2: case C_LOOSE3:
			body->data = new loose_info(f); break;			
		case C_BOMB: case C_BOMB2:
			shape->data = new enemy_info(f); break;
		default:break;
	}
	
	if (active && hasBody); // TODO: Remove this stuff when I've verified tilt works.
		ERR(":");
	if (active && hasBody && body->data)
		ERR("%x: %d\n", shape->collision_type, ((enemy_info*)body->data)->tiltid); // 
	
	if (active && hasBody)
		retilt(&level[jumpman_s], shape, body->data ? ((enemy_info*)body->data)->tiltid : 0);
	
	if (active)
		cpSpaceAddShape(level[jumpman_s].space, shape);
	else
		cpSpaceAddStaticShape(level[jumpman_s].space, shape);
}

void SaveHibernate()
{
	char filename[FILENAMESIZE];
    
    CachePath(filename, "warning.obj"); // Safe now.
	unlink(filename);
    
	CachePath(filename, "current.obj");
	ofstream f;
	
    if (jumpman_s >= level.size())
        return;
    if (doingEnding)
        return; // WTF
    
	f.open(filename, ios_base::out | ios_base::binary | ios_base::trunc);
	if (!f.fail()) {
		float tempFloat;
		// TODO: timers, anything editor related? X : Version, level name, jumpman_state, jumpman chassis, repeat_s, vertexes, on_ground (correctly?), tilts (tested?)
	
		tempFloat = COMPILED_VERSION; WRITE(tempFloat);
		
        WRITE(lastLoadedFrom);
		WRITES(lastLoadedFilename);
        		
#if FOG
        WRITE(fogf);
#endif
        
		WRITE(jumpman_s);
		WRITE(jumpman_flag);
		
		WRITE(jumpman_x); // misc state
		WRITE(jumpman_y);
		WRITE(jumpman_l);
		WRITE(roten);
		WRITE(level[jumpman_s].space->gravity);
		WRITE(level[jumpman_s].repeat_every);
		WRITE(level[jumpman_s].staticBody->a);
        WRITE(level[jumpman_s].rots);
        WRITE(level[jumpman_s].dontrot);
		WRITE(level[jumpman_s].has_norots);
		WRITE(level[jumpman_s].base_width);
		WRITE(level[jumpman_s].anytilts);
		WRITE(input_power_last_facing); // This and "jumping" should only effect display?
        WRITE(input_power_modifier);
		WRITE(jumping);
		WRITE(wantrot);
		WRITE(rotstep);
		WRITE(rescan);
		WRITE(surplusSpin);
		WRITE(doingInvincible);
		WRITE(pantype);
		WRITE(jumpmanstate);
		WRITE(jumpman_unpinned);
		WRITE(panf); WRITE(pant); WRITE(panfz); WRITE(pantz); WRITE(panfr); WRITE(pantr); WRITE(pantt); WRITE(panct); WRITE(jumptt); WRITE(jumpct);
		
		WRITE(currentSkip); WRITE(willSkipNext);
        WRITE(currentSkipX); WRITE(currentSkipY);
        WRITE(skipOnRebirth); WRITE(exitedBySkipping); WRITE(flagIsSkipPoisoned);
        
        WRITE(exit_grav_adjust);
        WRITE(rotting_s); WRITE(want_rotting_s);
        WRITE(grav_adjust_stashed_roten);
        
        {
            uint64_t timeRightNow = SDL_GetTicks();
            WRITE(timeRightNow); // "Shut down time"
        }
        WRITE(haveTimerData); WRITE(wantTimerData); WRITE(censorTimerData);
        WRITE(timerStartsAt); WRITE(timerEndsAt); WRITE(timerPausedAt); WRITE(timerLives);
        WRITE(subTimerStartsAt); WRITE(subTimerLives);
        
        // Wrote myself into a corner, can't use hibObject on Jumpman
		enemy_info *chassisData = (enemy_info*)chassis->data;
		WRITE(chassis->p);
		WRITE(chassis->v);
		WRITE(chassis->a);
		WRITE(chassisShape->layers);
		WRITE(chassisData->lastWhichFrame);
		WRITE(chassisData->lastReflect);
		WRITE(chassisData->tiltid);
		WRITE(chassisData->tiltref);
		WRITE(chassisData->tiltg);

		// -- Statics
		objCount = 0;
		cpSpaceHashEach(level[jumpman_s].space->staticShapes, &countObject, NULL);
		WRITE(objCount);
		cpSpaceHashEach(level[jumpman_s].space->staticShapes, &hibObject, &f);
		
		// -- Actives
		objCount = 0;
		cpSpaceHashEach(level[jumpman_s].space->activeShapes, &countObject, NULL);
		WRITE(objCount);
		cpSpaceHashEach(level[jumpman_s].space->activeShapes, &hibObject, &f);
		
		// -- Vertices
		WRITEV(level[jumpman_s].staticVertex);
		WRITEV(level[jumpman_s].lavaVertex);
		WRITEV(level[jumpman_s].zoneVertex);
		WRITEV(level[jumpman_s].kludgeVertex);
		WRITEV(level[jumpman_s].megaVertex);
	}
}

bool LoadHibernate() // true for success
{ 
    char filename[FILENAMESIZE];
    
    // If "warning file" present, we crashed last time and don't want to write this again.
    CachePath(filename, "warning.obj");
    { FILE *f = fopen(filename, "r"); if (f) { ERR("Found warning.obj! dumping.\n"); LoseHibernate(); return false; } }
    
    try {    
        CachePath(filename, "current.obj");

        ifstream f;
        f.exceptions( ios::eofbit | ios::failbit | ios::badbit );
        
        f.open(filename, ios_base::in | ios_base::binary);

        ERR("HAVE CACHE\n");
        CachePath(filename, "warning.obj");
        { FILE *f = fopen(filename, "w+"); if (f) fclose(f); } // touch warning.obj
        
        float tempFloat; 
            
        didHibernate = true;
        wantHibernate = false; // Cuz it'll just be the same file written.
        
        READ(tempFloat); // Version. TODO: Reject file if we don't like this number.
        
        LoadedFrom from;
        string loadedFilename;
        
        READ(from);
        READS(loadedFilename);
#if FOG
        READ(fogf);
#endif
        
        ERR("loaded [%s]\n", loadedFilename.c_str());
        
        int want_jumpman_s, want_jumpman_flag;
        READ(want_jumpman_s);
        READ(want_jumpman_flag);
        
        clearEverything();
        jumpman_s = want_jumpman_s;
        blockload_s = jumpman_s;
        
        bombExcept = true;
        loadGame(from, loadedFilename.c_str(), want_jumpman_flag);
        bombExcept = false;
        
        lastLoadedFrom = from;
        lastLoadedFilename = loadedFilename;
        
        scoreKeyIs(lastLoadedFrom, lastLoadedFilename);
        
        jumpman_s = blockload_s;
        jumpman_d = level[jumpman_s].deep+1;
        blockload_s = -1;
        
        jumpman_flag = want_jumpman_flag;
        
        READ(jumpman_x); // misc state
        READ(jumpman_y);
        READ(jumpman_l);
        READ(roten);
        READ(level[jumpman_s].space->gravity);
        READ(level[jumpman_s].repeat_every);
        READ(tempFloat); cpBodySetAngle(level[jumpman_s].staticBody, tempFloat);	
        READ(level[jumpman_s].rots);
        READ(level[jumpman_s].dontrot);
        READ(level[jumpman_s].has_norots);
        READ(level[jumpman_s].base_width);
        READ(level[jumpman_s].anytilts);
        READ(input_power_last_facing);
        READ(input_power_modifier);
        READ(jumping);
        READ(wantrot);
        READ(rotstep);
        READ(rescan);
        READ(surplusSpin);
        READ(doingInvincible);
        READ(pantype);
        READ(jumpmanstate);
        READ(jumpman_unpinned);
        READ(panf); READ(pant); READ(panfz); READ(pantz); READ(panfr); READ(pantr); READ(pantt); READ(panct); READ(jumptt); READ(jumpct);
        
        READ(currentSkip); READ(willSkipNext);
        READ(currentSkipX); READ(currentSkipY);
        READ(skipOnRebirth); READ(exitedBySkipping); READ(flagIsSkipPoisoned);     
        
        READ(exit_grav_adjust);
        READ(rotting_s); READ(want_rotting_s);
        READ(grav_adjust_stashed_roten);
        
        {
            extern uint64_t timerZero;
            uint64_t hibWrittenAt;
            READ(hibWrittenAt);
            ERR("TIMER ZERO ORIGINAL: %u", (uint)timerZero);
            timerZero -= hibWrittenAt;
            ERR(" NOW: %u\n", (uint)timerZero);
        }
        
        READ(haveTimerData); READ(wantTimerData); READ(censorTimerData);
        READ(timerStartsAt); READ(timerEndsAt); READ(timerPausedAt); READ(timerLives);
        READ(subTimerStartsAt); READ(subTimerLives);        
        
        // Wrote myself into a corner, can't use loadObject on Jumpman
        enemy_info *chassisData = (enemy_info*)chassis->data;
        READ(chassis->p);
        READ(chassis->v);
        READ(tempFloat); cpBodySetAngle(chassis, tempFloat);
        READ(chassisShape->layers);
        READ(chassisData->lastWhichFrame);
        READ(chassisData->lastReflect);
        READ(chassisData->tiltid);
        READ(chassisData->tiltref);
        READ(chassisData->tiltg);
        
        int readCount;
        
        // -- Statics
        READ(readCount);
        for(int c = 0; c < readCount; c++)
            loadObject(f, false);
        
        // -- Actives
        READ(readCount);
        for(int c = 0; c < readCount; c++)
            loadObject(f, true);
        
        // -- Vertices
        READV(level[jumpman_s].staticVertex);
        READV(level[jumpman_s].lavaVertex);
        READV(level[jumpman_s].zoneVertex);
        READV(level[jumpman_s].kludgeVertex);
        READV(level[jumpman_s].megaVertex);

        quadpile::megaIndexEnsure(level[jumpman_s].staticVertex.size()/(4*STATIC_STRIDE));
        quadpile::megaIndexEnsure(level[jumpman_s].lavaVertex.size()/(4*LAVA_STRIDE));
        quadpile::megaIndexEnsure(level[jumpman_s].zoneVertex.size()/(4*LAVA_STRIDE));
        quadpile::megaIndexEnsure(level[jumpman_s].kludgeVertex.size()/(4*MEGA_STRIDE));
        quadpile::megaIndexEnsure(level[jumpman_s].megaVertex.size()/(4*MEGA_STRIDE));        

        if (!jumpman_unpinned) {
            retilt(&level[jumpman_s], chassisShape, chassisData->tiltid);
            cpSpaceAddShape(level[jumpman_s].space, chassisShape);
        }
        
        unlink(filename); // Oh hey we got all the way here and didn't crash.

        return true;
    } catch ( exception &e ) {
       ERR("EXCEPT(%s)\n",e.what());
        bombExcept = false;
    }
    
    return false;
}

void LoseHibernate() {
	char filename[FILENAMESIZE];

	CachePath(filename, "current.obj"); 
	unlink(filename);
    
	CachePath(filename, "warning.obj"); 
	unlink(filename);
}

void LoseHighScores() {
    char optfile[FILENAMESIZE];
    char optfile2[FILENAMESIZE];
    userPath(optfile2); // What is this towers of hanoi nonsense
    snprintf(optfile, FILENAMESIZE, "%s/%s", optfile2, "jumpman.sav");    
	unlink(optfile);
}

void SaveHighScores() {
    if (!readyScores || !dirtyScores)
        return;
    
    char optfile[FILENAMESIZE];    
    {
        char optfile2[FILENAMESIZE];
        userPath(optfile2); // What is this towers of hanoi nonsense
        snprintf(optfile, FILENAMESIZE, "%s/%s", optfile2, "jumpman.sav");
    }
    
	ofstream f;
	unsigned int temp;
	f.open(optfile, ios_base::out | ios_base::binary | ios_base::trunc);
    ERR("ZZZZ File? %d\n", (int)scores.size());
	if (!f.fail()) {
        float tempFloat = COMPILED_VERSION; f.write((char *)&tempFloat, sizeof(tempFloat));
        
		temp = htonl(scores.size()); f.write((char *)&temp, sizeof(temp));
		for(hash_map<string, pair<scoreinfo, scoreinfo> >::iterator b = scores.begin(); b != scores.end(); b++) {
			int namelen = (*b).first.size();
			if (namelen > FILENAMESIZE)
				namelen = FILENAMESIZE;
			temp = htonl(namelen); f.write((char *)&temp, sizeof(temp));
			f.write((*b).first.c_str(), namelen);
			
            temp = htonl( skips[(*b).first] ); f.write((char *)&temp, sizeof(temp));
            
            cpVect tempVect = skips_entry[(*b).first]; f.write((char *)&tempVect, sizeof(tempVect));
            
			int listlen = (*b).second.first.time.size();
            ERR("ZZZZ name %s listlen out %d\n", (*b).first.c_str(), listlen);
			temp = htonl(listlen); f.write((char *)&temp, sizeof(temp));
			for(int c = 0; c < listlen; c++) {
				temp = htonl( (*b).second.first.time[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.first.deaths[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.second.time[c] ); f.write((char *)&temp, sizeof(temp));
				temp = htonl( (*b).second.second.deaths[c] ); f.write((char *)&temp, sizeof(temp));
			}
		}	
        ERR("ZZZZ File! %d\n", (int)scores.size());
	}    
    
    dirtyScores = false;
}

void LoadHighScores() {    
    bool failure = false;
    char optfile[FILENAMESIZE];    
    {
    char optfile2[FILENAMESIZE];
    userPath(optfile2); // What is this towers of hanoi nonsense
    snprintf(optfile, FILENAMESIZE, "%s/%s", optfile2, "jumpman.sav");
    }
    
    ifstream f;
    f.open(optfile, ios_base::in | ios_base::binary);
    if (!f.fail()) {
        f.seekg (0, ios::end);
        ERR("ZZZZ File size %d\n", (int)f.tellg());
        if (sizeof(int) <= f.tellg()) {
            f.seekg (0, ios::beg);
            unsigned int count;
            
            float tempFloat; f.read((char *)&tempFloat, sizeof(tempFloat)); // Seek past version
            
            f.read((char *)&count, sizeof(count)); count = ntohl(count);
            ERR("ZZZZ count %d\n", count);
            for(int c = 0; c < count; c++) {
                unsigned int namelen, listlen;
                unsigned int temp;

                char filename[FILENAMESIZE+1];
                
                f.read((char *)&namelen, sizeof(namelen)); namelen = ntohl(namelen);
                if (namelen > FILENAMESIZE)
                    break;
                f.read(filename, namelen);
                filename[namelen] = '\0';
                
                ERR("ZZZZ File: %s\n", filename);
                
                pair<scoreinfo,scoreinfo> &s = scores[filename];
                
                f.read((char *)&temp, sizeof(temp)); skips[filename] = ntohl(temp);
                ERR("ZZZZ skip at %d\n", skips[filename]);
                
                cpVect tempVect;
                f.read((char *)&tempVect, sizeof(tempVect)); skips_entry[filename] = tempVect;
                ERR("ZZZZ skip entry at (%f,%f)\n", tempVect.x, tempVect.y);
                
                f.read((char *)&listlen, sizeof(listlen)); listlen = ntohl(listlen);
                ERR("ZZZZ list %d\n", listlen);					
                for(int d = 0; d < listlen; d++) { 	// Size will be enforced by push_back.
                    
                    f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
                    s.first.time.push_back(temp);
                    ERR(" ZZZZ a: %d", temp);
                    
                    f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
                    s.first.deaths.push_back(temp);
                    ERR(" ZZZZ b: %d", temp);
                    
                    f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
                    s.second.time.push_back(temp);
                    ERR(" ZZZZ c: %d", temp);
                    
                    f.read((char *)&temp, sizeof(temp)); temp = ntohl(temp);
                    s.second.deaths.push_back(temp);
                    ERR(" ZZZZ d: %d\n", temp);
                }
            }
        } else failure = true;
    } else failure = true;
    if (failure) {		
        // We have no high scores. Is this a problem?
    }
    
    haveWonGame = (scores[scoreKeyFor(FromInternal, "Main.jmp")].first.time.size() >= 10); // I.E. if you've won the game, you have 10 high scores for Main.jmp
//ERR("Main.jmp has: %d paths %s\n", scores["/"].first.time.size(), haveWonGame?"(Winner)":"");

    readyScores = true;
}
/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	Smart Kobold Development
 *
 * NAME:        firefly.h ( Jacob's Matrix, C++ )
 *
 * COMMENTS:
 *		Runs a blood dripping simulation
 *		Allows you to get pure info dumps of the state.
 */

#ifndef __fire__
#define __fire__

#include <libtcod.hpp>

#include "thread.h"
#include "queue.h"
#include "mygba.h"
#include "vec2.h"
#include "vec3.h"
#include "ptrlist.h"

class THREAD;
class FIREFIELD;

enum FireCurve
{
    FIRE_BLACKBODY,
    FIRE_ICE,
    FIRE_MONO,
    FIRE_HEALTH,
    FIRE_MANA,
    FIRE_GOLD,
    FIRE_POISON,
    FIRE_BLOOD
};

class PARTICLE
{
public:
    PARTICLE() { myP = 0; myV = 0; myState = 0; myId = 0; }
    explicit PARTICLE(int) { myP = 0; myV = 0; myState = 0; myId = 0; }

    // Sort by y!
    bool		operator<(const PARTICLE &cmp) const
    { if (myP.y() < cmp.myP.y()) return true;
      if (myP.y() > cmp.myP.y()) return false;
      if (myP.x() < cmp.myP.x()) return true;
      if (myP.x() > cmp.myP.x()) return false;
      if (myP.z() < cmp.myP.z()) return true;
      if (myP.z() > cmp.myP.z()) return false;
      return false;
    }

    VEC3		myP, myV;
    int			myState;
    int			myId;
};

typedef PTRLIST<PARTICLE> PARTICLELIST;

class FIRETEX
{
public:
    FIRETEX(int w, int h);

    void		loadFile(const char *fname);

    void		incRef();
    void		decRef();

    u8			getR(int x, int y) const
			{ return myRGB[x*3+y*myW*3]; }
    u8			getG(int x, int y) const
			{ return myRGB[x*3+y*myW*3+1]; }
    u8			getB(int x, int y) const
			{ return myRGB[x*3+y*myW*3+2]; }

    void		setRGB(int x, int y, u8 r, u8 g, u8 b)
			{
			    myRGB[x*3+y*myW*3] = r;
			    myRGB[x*3+y*myW*3+1] = g;
			    myRGB[x*3+y*myW*3+2] = b;
			}

    static void		firecurve(FireCurve firetype, float heat,
				u8 *rgb);

    void		buildFromConstant(bool horizontal, float percent, FireCurve firetype);
    void		buildFromParticles(const PARTICLELIST &particles, FireCurve firetype);
    void		buildFromParticlesMasked(const PARTICLELIST &particles, FireCurve firetype, FIRETEX *mask);

    int			width() const { return myW; }
    int			height() const { return myH; }

    void		redraw(int sx, int sy) const;

private:
    ~FIRETEX();

    ATOMIC_INT32		myRefCnt;

    u8			*myRGB;
    int			 myW, myH;
};

class FIREFLY
{
public:
    FIREFLY(int count, int tw, int th, FireCurve firetype, const char *mask, bool horizontal);
    ~FIREFLY();

    void		 updateTex(FIRETEX *tex);
    // Returns a FIRETEX.  You must invoke decRef() on it.
    FIRETEX		*getTex();

    int			 width() const { return myTexW; }
    int			 height() const { return myTexH; }

    // Bad locks for the win!
    void		 setParticleCount(int newcount)
    { myPendingCount = newcount; myPendingCountPending = true; }

    // Proper queue!
    void		 postParticleChange(int partchange) { myChangeQueue.append(partchange); }

    int			 particleCount() const { return myParticles.entries(); }
    bool		 useBarGraph() const { return myUseBarGraph; }
    void		 setBarGraph(bool usebar) { myUseBarGraph = usebar;
						    myOldRatio = -1.0f; }

    void		 setRatioLiving(float size);

    int 		 addNewParticle(int width);
    void		 removeParticle();

    // Public only for call back conveninece.
    void		 mainLoop();

    void		 setFireType(FireCurve firetype)
			 { if (myFireType != firetype)
			     myOldRatio = -1.0f;
			    myFireType = firetype; }

    bool		 horizontal() const { return myHorizontal; }
    void		 setHorizontal(bool hor) { myHorizontal = hor; }

private:
    THREAD		*myThread;

    TCODNoise		*myNoise[3];

    PARTICLELIST	 myParticles;

    FIRETEX		*myTex;
    LOCK		 myTexLock;

    FIRETEX		*myMask;
    FireCurve		 myFireType;

    // Disaply res
    int			 myTexW, myTexH;

    float		 myRatioLiving;
    float		 myOldRatio;

    bool		 myPendingCountPending;
    int			 myPendingCount;

    bool		 myHorizontal;
    bool		 myUseBarGraph;

    int			 myParticlesToAdd;
    int			 myParticlesToRemove;

    QUEUE<int>		 myChangeQueue;
};

#endif

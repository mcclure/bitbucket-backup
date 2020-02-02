/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mapdata.h ( Save Scummer Library, C++ )
 *
 * COMMENTS:
 *	Contains an array of floats which can be copy-on-writed.
 */

#ifndef __mapdata__
#define __mapdata__

#include "mygba.h"

#include <iostream>
using namespace std;

class MAPDATA_REF;

class MAPDATA
{
public:
		 MAPDATA();
		 MAPDATA(int i);	// I should clean up pointer...
    explicit	 MAPDATA(int width, int height);
		 MAPDATA(const MAPDATA &data);
    
    virtual	~MAPDATA();

    MAPDATA 	&operator=(const MAPDATA &data);

    // For some reason I had myDistMaps(i)() access the non-const version
    // resulting in some serious performance penalty in getDist calls.
    // Since that is very, very, bad, I've opted for safety and renamed
    // the write accessor
    float	 operator()(int x, int y) const;

    void	 uniquify();
    // Only invoke after you unique!
    float	&data(int x, int y);

    int		 width() const { return myWidth; }
    int		 height() const { return myHeight; }

    // Fill with constant.
    void	 constant(float value);

    // Adds to our value the given distance map, but scaled nonlinearly
    // by value.
    void	 accumulateGradient(MAPDATA dist, float value);

    // Return sum of all cells.
    float	 total() const;

    void	 load(istream &is);
    void	 save(ostream &os) const;

protected:
    const float	*readData() const;
    float	*writeData();

private:
    MAPDATA_REF	*myRef;
    int		 myWidth, myHeight;
};

#endif


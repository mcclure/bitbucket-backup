/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        mapdata.cpp ( Save Sucmmer Library, C++ )
 *
 * COMMENTS:
 */

#include "mapdata.h"
#include "thread.h"
#include "rand.h"

#include <memory.h>
#include <math.h>
#include <assert.h>

class MAPDATA_REF
{
public:
    void	 incRef();
    void	 decRef();
    
    // Ensure I am the only pointer to the data.
    MAPDATA_REF	*uniquify();
    bool	 isUnique() const { return myRef == 1; }

    float	*data() { return myData; }

    // Returns a ref that already has been inced!
    static MAPDATA_REF *create(int nbytes);

private:
		 MAPDATA_REF();
    virtual	~MAPDATA_REF();

    ATOMIC_INT32 myRef;
    int		 mySize;
    float	*myData;
};

MAPDATA_REF::MAPDATA_REF()
{
    myData = 0;
    mySize = 0;
    myRef.set(0);
}

MAPDATA_REF::~MAPDATA_REF()
{
    delete [] myData;
}

void
MAPDATA_REF::incRef()
{
    myRef.add(1);
}

void
MAPDATA_REF::decRef()
{
    int		nval;
    nval = myRef.add(-1);
    if (nval <= 0)
	delete this;
}

MAPDATA_REF *
MAPDATA_REF::uniquify()
{
    // Empty refs are easy to unique :>
    if (!myData)
	return this;

    // if only one ref, it is trivial
    if (myRef <= 1)
	return this;

    // Need to copy for upcoming write!
    MAPDATA_REF	*result;

    result = create(mySize);
    memcpy(result->data(), data(), mySize * sizeof(float));

    // Remove a ref from ourself.
    decRef();

    return result;
}

MAPDATA_REF *
MAPDATA_REF::create(int nbytes)
{
    MAPDATA_REF		*result;

    result = new MAPDATA_REF;
    result->incRef();
    result->myData = new float[nbytes];
    result->mySize = nbytes;

    return result;
}

//
// Mapdata functions
//

MAPDATA::MAPDATA()
{
    myRef = 0;
    myWidth = myHeight = 0;
}

MAPDATA::MAPDATA(int width, int height)
{
    myRef = MAPDATA_REF::create(width * height);
    myWidth = width;
    myHeight = height;

    // What everyone expects anyways.
    constant(0);
}

MAPDATA::MAPDATA(const MAPDATA &data)
{
    myRef = 0;

    *this = data;
}

MAPDATA::~MAPDATA()
{
    if (myRef)
	myRef->decRef();
}


MAPDATA &
MAPDATA::operator=(const MAPDATA &data)
{
    // Trivial assignment.
    if (this == &data)
	return *this;

    if (data.myRef)
	data.myRef->incRef();
    
    if (myRef)
	myRef->decRef();

    myRef = data.myRef;
    myWidth = data.myWidth;
    myHeight = data.myHeight;

    return *this;
}

float
MAPDATA::operator()(int x, int y) const
{
    assert(x >= 0 && x < width());
    assert(y >= 0 && y < height());
    return readData()[y * width() + x];
}

void
MAPDATA::uniquify()
{
    writeData();
}

float &
MAPDATA::data(int x, int y)
{
    assert(x >= 0 && x < myWidth);
    assert(y >= 0 && y < myHeight);
    assert(myRef->isUnique());
    return ((float *)readData())[y * width() + x];
}

void
MAPDATA::constant(float value)
{
    int		i, n;
    if (!value)
	memset(writeData(), 0, myWidth * myHeight * sizeof(float));
    else
    {
	n = myWidth * myHeight;
	uniquify();
	for (i = 0; i < n; i++)
	{
	    writeData()[i] = value;
	}
    }
}

float
MAPDATA::total() const
{
    int		i, n;
    n = myWidth * myHeight;

    const float	*s = readData();
    float	result = 0.0f;

    for (i = 0; i < n; i++)
    {
	result += *s;
	s++;
    }
    return result;
}

void
MAPDATA::accumulateGradient(MAPDATA dist, float value)
{
    assert(width() == dist.width());
    assert(height() == dist.height());

    int		i, n;
    n = myWidth * myHeight;

    float	*d = writeData();
    const float	*s = dist.readData();

    for (i = 0; i < n; i++)
    {
	if (*s < 0)
	    *d = -1.0;
	//*d += value * sqrt(*s);
	*d = MIN(*d, value * *s);
	s++;
	d++;
    }
}

void
MAPDATA::load(istream &is)
{
    is.read((char *) &myWidth, sizeof(int));
    is.read((char *) &myHeight, sizeof(int));

    if (myRef)
	myRef->decRef();
    myRef = MAPDATA_REF::create(width() * height());

    is.read((char *) writeData(), myWidth * myHeight * sizeof(float));
}

void
MAPDATA::save(ostream &os) const
{
    os.write((const char *) &myWidth, sizeof(int));
    os.write((const char *) &myHeight, sizeof(int));
    os.write((const char *) readData(), myWidth * myHeight * sizeof(float));
}

const float *
MAPDATA::readData() const
{
    return myRef->data();
}

float *
MAPDATA::writeData()
{
    myRef = myRef->uniquify();
    return myRef->data();
}

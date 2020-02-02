/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        dpdf.cpp ( Save Scummer Library, C++ )
 *
 * COMMENTS:
 */

#include <assert.h>
#include "dpdf.h"
#include "rand.h"

#include <memory.h>

DPDF::DPDF()
{
    myMin = myMax = 0;
    myProb = new double[1];
    myProb[0] = 1.0;
}

DPDF::DPDF(int constant)
{
    myMin = constant;
    myMax = constant;
    myProb = new double[1];
    myProb[0] = 1.0;
}

DPDF::DPDF(int min, int max)
{
    int		i, range;
    double	prob;

    // Ensure positive.
    if (min > max)
    {
	i = max;
	max = min;
	min = i;
    }

    // Inclusive!
    range = max - min + 1;
    prob = 1.0 / range;
    myMin = min;
    myMax = max;
    
    myProb = new double[range];
    for (i = 0; i < range; i++)
    {
	myProb[i] = prob;
    }

    assert(isValid());
}

DPDF::DPDF(const DPDF &dpdf)
{
    myProb = 0;
    *this = dpdf;
}

DPDF::~DPDF()
{
    delete [] myProb;
}

DPDF &
DPDF::operator=(const DPDF &dpdf)
{
    delete [] myProb;

    myMin = dpdf.myMin;
    myMax = dpdf.myMax;
    myProb = new double [myMax - myMin + 1];
    memcpy(myProb, dpdf.myProb, sizeof(double) * (myMax - myMin + 1));

    return *this;
}

double
DPDF::probability(int num) const
{
    if (num < myMin || num > myMax)
	return 0.0;

    return myProb[num - myMin];
}

double
DPDF::expectedValue() const
{
    double 	total = 0;
    int		i;
    
    for (i = myMin; i <= myMax; i++)
    {
	total += i * probability(i);
    }

    return total;
}

int
DPDF::evaluate() const
{
    double		val, total = 0;
    int			i;

    val = rand_double();

    for (i = myMin; i <= myMax; i++)
    {
	total += probability(i);
	if (val < total)
	    return i;
    }
    // Failed to find, we'll just return max.
    return myMax;
}

double
DPDF::probabilityGreaterThan(int num) const
{
    double		total = 0.0;

    // Note this is strictly greater than, thus the initial
    // ++
    for (num++; num <= myMax; num++)
    {
	total += probability(num);
    }
    
    return total;
}

void
DPDF::applyGivenGreaterThan(int num)
{
    // Trivial case...
    if (myMin > num)
	return;

    double		*prob;
    double		 scale;
    int			 i;

    // This is the amount of probability left.
    scale = probabilityGreaterThan(num);
    // If !scale, we have a problem because all the bases belong
    // to them!  We assert and punt by converting to a constant.
    if (!scale)
    {
	*this = DPDF(num+1);
    }
    else
    {
	scale = 1.0 / scale;

	prob = new double [myMax - (num+1) + 1];
	
	// copy subset...
	for (i = num+1; i <= myMax; i++)
	{
	    prob[i - (num+1)] = probability(i);
	}

	// Write in our new max/min
	myMin = num+1;
	delete [] myProb;
	myProb = prob;

	// Scale by the scale factor to normalize...
	scaleComponentWise(scale);
    }
    assert(isValid());
}

void
DPDF::max(DPDF a, DPDF b)
{
    // We know this is max of rnages...
    *this = DPDF(MAX(a.myMin, b.myMin), MAX(a.myMax, b.myMax));

    // If i > myMax, then max->prob(i) = dpdf->prob(i)
    // By symmetry, if i > dpdf->myMax, max->prob(i) = prob(i)

    // max->prob(i) = a->prob(i) * b->prob(<=i)
    // 		    + b->prob(i) * a->prob(<i)
    // Note use of < rather than <= to avoid double counting the case
    // where the two probabilities are equal.

    int			i;

    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] = a.probability(i) * (1.0 - b.probabilityGreaterThan(i))
			+ b.probability(i) * (1.0 - a.probabilityGreaterThan(i-1));
    }
    assert(isValid());
}

void
DPDF::min(DPDF a, DPDF b)
{
    *this = DPDF(MIN(a.myMin, b.myMin), MIN(a.myMax, b.myMax));

    // min->prob(i) = a->prob(i) * b->prob(>=i)
    // 		    + b->prob(i) * a->prob(>i)
    // Note use of < rather than <= to avoid double counting the case
    // where the two probabilities are equal.

    int			i;

    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] = a.probability(i) * (b.probabilityGreaterThan(i))
			+ b.probability(i) * (a.probabilityGreaterThan(i-1));
    }
    assert(isValid());
}

void
DPDF::negate()
{
    int		i;
    DPDF	dpdf(-myMax, -myMin);

    for (i = myMin; i <= myMax; i++)
    {
	// Merely reverse the array.
	dpdf.myProb[myMax - i] = probability(i);
    }

    // And assign...
    *this = dpdf;
    assert(isValid());
}

void
DPDF::expandRange(int min, int max)
{
    expandRange(min);
    expandRange(max);
}

void
DPDF::expandRange(const DPDF &dpdf)
{
    expandRange(dpdf.myMin, dpdf.myMax);
}

void
DPDF::expandRange(int num)
{
    double		*prob;

    if (num < myMin)
    {
	prob = new double [myMax - num + 1];
	memset(prob, 0, sizeof(double) * (myMax - num + 1));
	memcpy(&prob[myMin - num], myProb, sizeof(double) * (myMax - myMin + 1));

	myMin = num;
	delete [] myProb;
	myProb = prob;
    }
    if (num > myMax)
    {
	prob = new double [num - myMin + 1];
	memset(prob, 0, sizeof(double) * (num - myMin + 1));
	memcpy(prob, myProb, sizeof(double) * (myMax - myMin + 1));

	myMax = num;
	delete [] myProb;
	myProb = prob;
    }
}

void
DPDF::addComponentWise(const DPDF &dpdf)
{
    int			i;
    
    expandRange(dpdf.myMin, dpdf.myMax);
    
    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] += dpdf.probability(i);
    }
}

void
DPDF::scaleComponentWise(double scale)
{
    int			i;
    
    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] *= scale;
    }
}

void
DPDF::zero()
{
    int			i;
    
    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] = 0.0;
    }
}

DPDF &
DPDF::operator*=(double prob)
{
    int		i;
    
    // Need to have zero.
    expandRange(0);

    // Scale everything.
    for (i = myMin; i <= myMax; i++)
    {
	myProb[i-myMin] *= prob;
    }

    // All of the missing probability goes to 0.
    myProb[0 - myMin] += 1.0 - prob;

    assert(isValid());
    return *this;
}

DPDF &
DPDF::operator+=(int constant)
{
    // A simple shift sufficies...
    myMin += constant;
    myMax += constant;
    assert(isValid());
    return *this;
}

DPDF &
DPDF::operator-=(const DPDF &dpdf)
{
    DPDF	neg;

    neg = dpdf;
    neg.negate();
    *this += neg;

    assert(isValid());
    return *this;
}

DPDF &
DPDF::operator+=(const DPDF &dpdf)
{
    int		i;
    DPDF	total(myMin), sum(myMin);
    
    // Zero out the total.
    total.zero();

    for (i = dpdf.myMin; i <= dpdf.myMax; i++)
    {
	// Make a copy of this.
	sum = *this;

	// Add in i.
	sum += i;

	// Scale by the probability of i being chosen in dpdf.
	sum.scaleComponentWise(dpdf.probability(i));

	// Add in to the total.
	total.addComponentWise(sum);
    }

    // Set ourselves to the total
    *this = total;

    assert(isValid());
    return *this;
}

void
DPDF::getQuartile(int &min, int &q1, int &q2, int &q3, int &max) const
{
    min = getPercentile(0.0);
    q1 = getPercentile(0.25);
    q2 = getPercentile(0.5);
    q3 = getPercentile(0.75);
    max = getPercentile(1.0);
}

int
DPDF::getPercentile(double percent) const
{
    double		total = 0;
    int			i;

    for (i = myMin; i <= myMax; i++)
    {
	total += probability(i);
	if (total > percent)
	    return i;
    }
    // Ran off the top, search backwards for first non-zero
    for (i = myMax; i >= myMin; i--)
    {
	if (probability(i))
	    return i;
    }

    // Pretty arbitrary at this point.
    return myMax;
}


double
DPDF::totalprob() const
{
    int		i;
    double	total = 0.0;
    
    for (i = myMin; i <= myMax; i++)
    {
	total += probability(i);
    }

    return total;
}

bool
DPDF::isValid(double tol) const
{
    double	total;

    total = totalprob();

    if (total > 1.0 + tol || total < 1.0 - tol)
    {
	assert(false);
	return false;
    }
    return true;
}

void
DPDF::save(ostream &os) const
{
    int		i;

    os.write((const char *) &myMin, sizeof(int));
    os.write((const char *) &myMax, sizeof(int));
    for (i = myMin; i <= myMax; i++)
    {
	os.write((const char *) &myProb[i-myMin], sizeof(double));
    }
}

void
DPDF::load(istream &is)
{
    delete [] myProb;

    int		i;

    is.read((char *) &myMin, sizeof(int));
    is.read((char *) &myMax, sizeof(int));
    myProb = new double[myMax - myMin + 1];
    for (i = myMin; i <= myMax; i++)
    {
	is.read((char *) &myProb[i-myMin], sizeof(double));
    }
}

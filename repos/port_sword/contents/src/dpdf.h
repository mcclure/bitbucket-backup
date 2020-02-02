/*
 * Licensed under BSD license.  See LICENCE.TXT  
 *
 * Produced by:	Jeff Lait
 *
 *      	7DRL Development
 *
 * NAME:        dpdf.h ( Save Scummer Library, C++ )
 *
 * COMMENTS:
 *	Stores and manipulates a discrete probability function.
 *	Tracks the probability of a function returning certain numbers.
 *	Allows you to compose dpdfs in obvious fashions.
 */

#ifndef __dpdf__
#define __dpdf__

#include <iostream>
using namespace std;

class DPDF
{
public:
    // Constructors...
    DPDF();
    DPDF(int constant);		// Always returns given constant.
				// NOT explicit as we want to be able
				// to cast up!
    DPDF(int min, int max);	// Uniform distribution in [min..max]

    DPDF(const DPDF &dpdf);		// Copy constructor

    virtual ~DPDF();

    void	save(ostream &os) const;
    void	load(istream &is);

    DPDF &operator=(const DPDF &dpdf);	// Assignment operator.

    double	probability(int num) const;

    double	expectedValue() const;

    // Uses the random number generator to pick a random integer inside
    // our DPDF using our DPDF.
    int		evaluate() const;

    // Returns the probability of this function returning something
    // strictly greater than the given number.
    double	probabilityGreaterThan(int num) const;

    // Clips out any values less than or equal to the number.
    void	applyGivenGreaterThan(int num);

    // Creates the DPDF of the sum of two DPDFs.
    DPDF &operator+=(const DPDF &dpdf);
    DPDF &operator-=(const DPDF &dpdf);

    // The maximum operator applied to DPDFs
    // Rewrites this.
    // We force a copy constructor so this can be a parameter.
    void	max(DPDF a, DPDF b);

    // Ditto, but min.
    void	min(DPDF a, DPDF b);

    // Adds a constant value to the DPDF - this is somewhat of a no-op
    // as it just shifts the DPDF.
    // Note that this is equivalent to upcasting the constant to a
    // DPDF and then adding, thus meaning we aren't making an ambiguity
    // with casting operations
    // It is just this is more effecient, and more importantly, the
    // DPDF += uses this as an atomic operation.
    DPDF &operator+=(int constant);

    // Multiplies this DPDF by the given PROBABILITY.  This multiplies
    // all entries by PROBABILITY and adds to the 0 entry 1-PROBABIILITY
    DPDF &operator*=(double prob);

    // Negates the range of the probability [myMin..myMax] -> [-myMax..-myMin]
    // and a similar reversal of the actual values.
    void	negate();

    void	getQuartile(int &min, int &q1, int &q2, int &q3, int &max) const;

    // Returns the smallest i such that prob(<=i) > percent
    int		getPercentile(double percent) const;


protected:
    // Sums our probabilities.  This is supposed to be one.
    double	totalprob() const;

    // Verifies our total is 1.0
    bool	isValid(double tol=1e-4f) const;
    
    // Expands this to encompass the given range, adding zero probability
    // as necessary
    void	expandRange(int min, int max);

    // Expands the range to include the given number.
    void	expandRange(int min);

    // Expands to include range of other dpdf.
    void	expandRange(const DPDF &dpdf);
    
    // The following math operations are not DPDF operations!  They are,
    // however, useful in building them if you don't care about
    // efficiency.
    // (DPDF operations have the property that the resulting DPDF
    // always sums to 1)
    
    // Adds each probability into this dpdf in a component wise manner.
    // Expands this as necessary.
    void	addComponentWise(const DPDF &dpdf);

    // Multiplies all entries by the given scale.
    void	scaleComponentWise(double scale);

    // Zeros the system
    void	zero();
    
private:
    int		myMin, myMax;	// Min & max possible values.
    double	*myProb;	// Probability for each value.
};

#endif


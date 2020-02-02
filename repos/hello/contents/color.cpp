// Note: These functions are sample code provided by a "Eugene Vishnevsky" as part of a tutorial on color
// models which can be found in various places on the internet. Because this code in context is provided
// for educational purposes, and because it is already reproduced on several high-profile sites, I am assuming
// there are no restrictions on its reproduction.

#include "color.h"
#include <math.h>

// r,g,b values are from 0 to 1
// h = [0,360), s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)

// This is ugly; I don't care.
#define MIN2(a, b) ( a < b ? a : b )
#define MAX2(a, b) ( a < b ? b : a )
#define MIN(a, b, c) MIN2( MIN2(a, b), c )
#define MAX(a, b, c) MAX2( MAX2(a, b), c )

void RGBtoHSV( double r, double g, double b, double *h, double *s, double *v )
{
	float min, max, delta;
	
	if (r < 0) r = 0; // Saturate
	if (r > 1.0) r = 1.0; // Saturate
	if (g < 0) g = 0; // Saturate
	if (g > 1.0) g = 1.0; // Saturate
	if (b < 0) b = 0; // Saturate
	if (b > 1.0) b = 1.0; // Saturate

	min = MIN( r, g, b );
	max = MAX( r, g, b );
	*v = max;				// v

	delta = max - min;

	if( max != 0 )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}

	if( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan

	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;

}

void HSVtoRGB( double *r, double *g, double *b, double h, double s, double v )
{
	int i;
	float f, p, q, t;

	if (h < 0) h = 0; // Saturate
	if (h >= 360) h = 0; // Wrap around
	if (s < 0) s = 0; // Saturate
	if (s > 1.0) s = 1.0; // Saturate
	if (v < 0) v = 0; // Saturate
	if (v > 1.0) v = 1.0; // Saturate

	if( s == 0 ) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}
		
	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}

}
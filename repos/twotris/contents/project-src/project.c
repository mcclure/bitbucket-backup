/*
 *  project.c
 *  "c part"
 *
 *  Created by Andi McClure on 8/22/13.  AND ME
 *  Copyright 2013 Run Hello. All rights reserved.
 *
 */

#include "project.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

// General functions

EXPORT void c_test() {
	fprintf(stderr, "ALIVE\n");
}

struct rgb_col
{
	unsigned char b,g,r;//r,g,b;
};

struct rgb_chan
{
	unsigned char x[3];
};

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff
#else
#define rmask 0x000000ff
#define gmask 0x0000ff00
#define bmask 0x00ff0000
#define amask 0xff000000
#endif


// Images

// Level stuff

///haha i'm just going to start writing some code here you can't stop me
///it's gonna be really bad though

#define SEG_W 16//4
#define QUOT_W (surface->w/SEG_W)
#define QUOT_W_MAX (1024/SEG_W)
#define MAX_WORMS 100

int dx_tab[4] = { +1, 0,-1, 0 };
int dy_tab[4] = {  0,+1, 0,-1 };

int tile_file[16][4][16][16];

uint32_t color_make(SDL_Surface *surface, uint32_t r, uint32_t g, uint32_t b) {
	return ( (r&0xFF)<<surface->format->Rshift )
		|  ( (g&0xFF)<<surface->format->Gshift )
		|  ( (b&0xFF)<<surface->format->Bshift );
}

EXPORT void level_init(SDL_Surface *surface, int worms, int worm_length, int worm_start_dist, uint32_t color1, uint32_t color2, uint32_t color3)
{
	FILE *f;
	int i,j,k,l;
	int d;
	int what_segment_to_use[QUOT_W_MAX][QUOT_W_MAX];
	int wx[MAX_WORMS], wy[MAX_WORMS];
	int n;
	char c[2];
	size_t s;
	uint32_t colors[3] = {color3, color1, color2};
	
	f = fopen("resource/tiles.txt", "r");
	if (!f) return;
	for (i=0; i<16; ++i)
		for (j=0; j<4; ++j)
		{
			for (k=0; k<16; ++k)
			{
				for (l=0; l<16; ++l)
				{
					s = fread(c, 1, 1, f);
					switch (c[0])
					{
					case '4':
						tile_file[i][j][k][l] = 1; break;
					case '5':
						tile_file[i][j][k][l] = 2; break;
					default:
						tile_file[i][j][k][l] = 0; break;
					}
				}
				s = fread(c, 1, 1, f); //newline
			}
			s = fread(c, 1, 1, f); //newline
		}
	fclose(f);


	srand(time(0));
	memset(what_segment_to_use, 0, sizeof(int)*QUOT_W*QUOT_W);
	
	//just shove some stuff in randomly, later this will grow outward from a point
	
	//grow out some sections of wall
	wx[0] = QUOT_W/2;
	wy[0] = QUOT_W/2;
	for (i=1; i<worms; ++i)
	{
		wx[i] = wx[0]-worm_start_dist+rand()%(2*worm_start_dist+1);
		wy[i] = wy[0]-worm_start_dist+rand()%(2*worm_start_dist+1);
		if (wx[i]<0) wx[i] = 0;
		if (wy[i]<0) wx[i] = 0;
		if (wx[i]>=QUOT_W) wx[i] = QUOT_W-1;
		if (wy[i]>=QUOT_W) wy[i] = QUOT_W-1;
	}
	what_segment_to_use[QUOT_W/2][QUOT_W/2] = 1;
	for (j=0; j<worm_length; ++j)
		for (i=0; i<worms; ++i)
		{
			d = rand()%4;
			wx[i] += dx_tab[d];
			wy[i] += dy_tab[d];
			if (wx[i]<0) wx[i] = 0;
			if (wy[i]<0) wy[i] = 0;
			if (wx[i]>=QUOT_W) wx[i] = QUOT_W-1;
			if (wy[i]>=QUOT_W) wy[i] = QUOT_W-1;
			what_segment_to_use[wx[i]][wy[i]] = 1;
		}
	
	/******
	 
basically the idea here is that
this is something that will overwrite sections of ANDI GLITCH LEVELS with BROG TILES
currently the tiles are super simple but thats kind of cool too
	 
	 ******/
	
	//figure out which segments to use based on adjacency
	for (i=0; i<QUOT_W; ++i)
		for (j=0; j<QUOT_W; ++j)
		{
			if (what_segment_to_use[i][j]==0) continue;
			n = 0;
			if (i>0 && what_segment_to_use[i-1][j]>0)
				n |= 8;
			if (j>0 && what_segment_to_use[i][j-1]>0)
				n |= 4;
			if (i<QUOT_W-1 && what_segment_to_use[i+1][j]>0)
				n |= 2;
			if (j<QUOT_W-1 && what_segment_to_use[i][j+1]>0)
				n |= 1;
			what_segment_to_use[i][j] = 1+n;
		}
	
	//fill in the level according to the segments we've chosen
	for (i=0; i<QUOT_W; ++i)
		for (j=0; j<QUOT_W; ++j)
		{
			if (what_segment_to_use[i][j]>0) //skip it if it's empty, don't overwrite other generation
			{
				n = rand()%4;
			for (k=0; k<SEG_W; ++k)
				for (l=0; l<SEG_W; ++l)
					set_color(surface, i*SEG_W+k, j*SEG_W+l, colors[ tile_file[what_segment_to_use[i][j]-1][n][k][l] ]);
					//level_segment[what_segment_to_use[i][j]-1][k][l];
			}
		}
	
}

#define SEG_W2 4
#define QUOT_W2 (surface->w/SEG_W2)
#define QUOT_W_MAX2 (1024/SEG_W2)

//good old copy-past codineg
EXPORT void level_init2(SDL_Surface *surface, int worms, int worm_length, int worm_start_dist, uint32_t color1, uint32_t color2)
{
	FILE *f;
	int i,j,k,l;
	int d;
	int what_segment_to_use[QUOT_W_MAX2][QUOT_W_MAX2];
	int wx[MAX_WORMS], wy[MAX_WORMS];
	int n;
	char c[2];
	size_t s;
	uint32_t colors[2] = {color1, color2};
	
	f = fopen("resource/tiles2.txt", "r");
	if (!f) return;
	for (i=0; i<16; ++i)
		for (j=0; j<1; ++j)
		{
			for (k=0; k<4; ++k)
			{
				for (l=0; l<4; ++l)
				{
					s = fread(c, 1, 1, f);
					switch (c[0])
					{
					case '1':
						tile_file[i][j][k][l] = 1; break;
					default:
						tile_file[i][j][k][l] = 0; break;
					}
				}
				s = fread(c, 1, 1, f); //newline
			}
			s = fread(c, 1, 1, f); //newline
		}
	fclose(f);


	srand(time(0));
	memset(what_segment_to_use, 0, sizeof(int)*QUOT_W2*QUOT_W2);
	
	//just shove some stuff in randomly, later this will grow outward from a point
	
	//grow out some sections of wall
	wx[0] = QUOT_W2/2;
	wy[0] = QUOT_W2/2;
	for (i=1; i<worms; ++i)
	{
		wx[i] = wx[0]-worm_start_dist+rand()%(2*worm_start_dist+1);
		wy[i] = wy[0]-worm_start_dist+rand()%(2*worm_start_dist+1);
		if (wx[i]<0) wx[i] = 0;
		if (wy[i]<0) wx[i] = 0;
		if (wx[i]>=QUOT_W2) wx[i] = QUOT_W2-1;
		if (wy[i]>=QUOT_W2) wy[i] = QUOT_W2-1;
	}
	what_segment_to_use[QUOT_W2/2][QUOT_W2/2] = 1;
	for (j=0; j<worm_length; ++j)
		for (i=0; i<worms; ++i)
		{
			d = rand()%4;
			wx[i] += dx_tab[d];
			wy[i] += dy_tab[d];
			if (wx[i]<0) wx[i] = 0;
			if (wy[i]<0) wy[i] = 0;
			if (wx[i]>=QUOT_W2) wx[i] = QUOT_W2-1;
			if (wy[i]>=QUOT_W2) wy[i] = QUOT_W2-1;
			what_segment_to_use[wx[i]][wy[i]] = 1;
		}
		
	//figure out which segments to use based on adjacency
	for (i=0; i<QUOT_W2; ++i)
		for (j=0; j<QUOT_W2; ++j)
		{
			if (what_segment_to_use[i][j]==0) continue;
			n = 0;
			if (i>0 && what_segment_to_use[i-1][j]>0)
				n |= 2;
			if (j>0 && what_segment_to_use[i][j-1]>0)
				n |= 1;
			if (i<QUOT_W2-1 && what_segment_to_use[i+1][j]>0)
				n |= 8;
			if (j<QUOT_W2-1 && what_segment_to_use[i][j+1]>0)
				n |= 4;
			what_segment_to_use[i][j] = 1+n;
		}
	
	//fill in the level according to the segments we've chosen
	for (i=0; i<QUOT_W2; ++i)
		for (j=0; j<QUOT_W2; ++j)
		{
			if (what_segment_to_use[i][j]>0) //skip it if it's empty, don't overwrite other generation
			{
				
			for (k=0; k<SEG_W2; ++k)
				for (l=0; l<SEG_W2; ++l)
					set_color(surface, i*SEG_W2+k, j*SEG_W2+l, colors[ tile_file[what_segment_to_use[i][j]-1][0][k][l] ]);
					//level_segment[what_segment_to_use[i][j]-1][k][l];
			}
		}
	
}

uint32_t oxbox[3] = {
	0xffffff00, 0xffff00ff, 0xff00ffff
};

EXPORT void also_do_something_neat_idk(SDL_Surface *surface, uint32_t color)
{
	int x,y,i;
	uint32_t c1, c2, d, e;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
			for (i=0; i<3; ++i)
		{
			int n;
			n = 0;
			e = 0;
			if (x>0)
			{
				d = ((get_color(surface,x-1,y)>>(i*8)) & 0xff);
				e |= d;
				if (d>0x7f) ++n;
			}
			if (y>0)
			{
				d = ((get_color(surface,x,y-1)>>(i*8)) & 0xff);
				e |= d;
				if (d>0x7f) ++n;
			}
			if (x<surface->w-1)
			{
				d = ((get_color(surface,x+1,y)>>(i*8)) & 0xff);
				e |= d;
				if (d>0x7f) ++n;
			}
			if (y<surface->h-1)
			{
				d = ((get_color(surface,x,y+1)>>(i*8)) & 0xff);
				e |= d;
				if (d>0x7f) ++n;
			}
			c1 = get_color(surface,x,y);
			c2 = c1 | (e << (i*8));
			if (c2>c1) ++c1;
			else if (c2<c1) --c1;
			set_color(surface,x,y,c1);
		}
}

EXPORT void do_a_thing(SDL_Surface *surface)
{
	int x,y,i;
	uint32_t c1, c2, d, e;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
			for (i=0; i<3; ++i)
			{
				int n;
				n = 0;
				e = 0;
				if (x>0)
				{
					d = ((get_color(surface,x-1,y)>>(i*8)) & 0xff);
					e |= d;
					if (d>0x7f) ++n;
				}
				if (y>0)
				{
					d = ((get_color(surface,x,y-1)>>(i*8)) & 0xff);
					e |= d;
					if (d>0x7f) ++n;
				}
				if (x<surface->w-1)
				{
					d = ((get_color(surface,x+1,y)>>(i*8)) & 0xff);
					e |= d;
					if (d>0x7f) ++n;
				}
				if (y<surface->h-1)
				{
					d = ((get_color(surface,x,y+1)>>(i*8)) & 0xff);
					e |= d;
					if (d>0x7f) ++n;
				}
				c1 = get_color(surface,x,y);
				c2 = (c1>>(i*8)) & 0xff;
//				if (n<2)
//					c2 &= oxbox[i];
//				else
//					c2 |= (e << (i*8));
				if (n>2 && c2<0xff) ++c2;
				else if (c2>0) --c2;
				c1 = (c1 & oxbox[i]) | (c2 << (i*8));
				set_color(surface,x,y,c1);
			}

}

EXPORT void do_another_thing(SDL_Surface *surface)
{
	int x,y,nx,ny,i;
	uint32_t c1, c2;
	struct rgb_chan rgb1, rgb2;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			nx = x-1;
			ny = y;
			if (nx<0)
			{
				nx += surface->w;
				++ny;
				if (ny>=surface->h)
					ny = 0;
			}
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_chan*)&c1;
			c2 = get_color(surface,nx,ny);
			rgb2 = *(struct rgb_chan*)&c2;
			for (i=0; i<3; ++i)
			{
				rgb1.x[i] = rgb1.x[i]%64 + 61*(rgb1.x[i]/64) + 2*(rgb2.x[i]/64) + (rgb2.x[(i+1)%3]/64);
			}
			c1 =*(uint32_t*)&rgb1;
			set_color(surface,x,y, c1);
		}
			
}

// from http://www.cs.rit.edu/~ncs/color/t_convert.html
//kind of loses some detail in the float/char conversion but hey who cares
void RGBtoHSV( double r, double g, double b, double *h, double *s, double *v )
{
	double mn, mx, delta;
	mn = min( r, min( g, b) );
	mx = max( r, max( g, b) );
	*v = mx;				// v
	delta = mx - mn;
	if( mx != 0 )
		*s = delta / mx;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if( r == mx )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == mx )
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
	double f, p, q, t;
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

EXPORT void do_a_weird_thing(SDL_Surface *surface) //okay it's just colourcycling
{
	int x,y;
	uint32_t c1;
	double R,G,B,H,S,V;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(surface,x,y);
			R = (c1&0xff)/255.0;
			c1 = c1 >> 8;
			G = (c1&0xff)/255.0;
			c1 = c1 >> 8;
			B = (c1&0xff)/255.0;
			RGBtoHSV(R,G,B,&H,&S,&V);
			H += 1.0;
			if (H>360) H -=360;
			HSVtoRGB(&R,&G,&B,H,S,V);
			c1 = (int)(B*255);
			c1 = c1 << 8;
			c1 += (int)(G*255);
			c1 = c1 << 8;
			c1 += (int)(R*255);
			set_color(surface,x,y, c1);
		}
}

EXPORT void do_a_locational_thing(SDL_Surface *surface, int X, int Y)
{
//	int x;
	int y;
	double total[3];
	uint32_t c1,c2;
	struct rgb_col rgb, rgb2;
	
//	total[0] = 0; total[1] = 0; total[2] = 0;
//	for (x=0; x<surface->w; ++x)
//	{
//		c1 = get_color(surface, x,Y);
//		rgb = *(struct rgb_col*)&c1;
//		total[0] += rgb.r/(double)surface->w;
//		total[1] += rgb.g/(double)surface->w;
//		total[2] += rgb.b/(double)surface->w;
//	}
//	for (x=0; x<surface->w; ++x)
//	{
//		rgb.r = (unsigned char)total[0];
//		rgb.g = (unsigned char)total[1];
//		rgb.b = (unsigned char)total[2];
//		c1 = *(uint32_t*)&rgb;
//		c2 = get_color(surface, x,Y);
//		rgb2 = *(struct rgb_col*)&c2;
//		if (rgb2.r>rgb.r) --rgb2.r;
//		if (rgb2.r<rgb.r) ++rgb2.r;
//		if (rgb2.g>rgb.g) --rgb2.g;
//		if (rgb2.g<rgb.g) ++rgb2.g;
//		if (rgb2.b>rgb.b) --rgb2.b;
//		if (rgb2.b<rgb.b) ++rgb2.b;
//		c2 = *(uint32_t*)&rgb2;
//		set_color(surface,x,Y,c2);
//	}
	
	total[0] = 0; total[1] = 0; total[2] = 0;
	for (y=0; y<surface->h; ++y)
	{
		c1 = get_color(surface, X,y);
		rgb = *(struct rgb_col*)&c1;
		total[0] += rgb.r/(double)surface->w;
		total[1] += rgb.g/(double)surface->w;
		total[2] += rgb.b/(double)surface->w;
	}
	for (y=0; y<surface->h; ++y)
	{
		rgb.r = (unsigned char)total[0];
		rgb.g = (unsigned char)total[1];
		rgb.b = (unsigned char)total[2];
		c1 = *(uint32_t*)&rgb;
		c2 = get_color(surface, X,y);
		rgb2 = *(struct rgb_col*)&c2;
		if (rgb2.r>rgb.r) --rgb2.r;
		if (rgb2.r<rgb.r) ++rgb2.r;
		if (rgb2.g>rgb.g) --rgb2.g;
		if (rgb2.g<rgb.g) ++rgb2.g;
		if (rgb2.b>rgb.b) --rgb2.b;
		if (rgb2.b<rgb.b) ++rgb2.b;
		c2 = *(uint32_t*)&rgb2;
		set_color(surface,X,y,c2);
	}
}

int brush_size = 12;
int brush_type = 1;


void brush_smoosh(SDL_Surface *surface, int X, int Y)
{
	int x,y,i,j;
	uint32_t c1;
	for (i=0; i<brush_size; ++i) //which ring
		for (j=0; j<=i; ++j) //within ring
		{
			//do each of the four sides of the diamond

			x = X-1-i+j;
			y = Y+j;
			if (j==0)
				c1 = get_color(surface, x-1,y);
			else if (j==i)
				c1 = get_color(surface, x,y+1);
			else
			{
				c1 = (get_color(surface, x-1,y) + get_color(surface, x,y+1))/2;
			}
			set_color(surface,x,y,c1);

			y = Y+1+i-j;
			x = X+j;
			if (j==0)
				c1 = get_color(surface, x+1,y);
			else if (j==i)
				c1 = get_color(surface, x,y+1);
			else
			{
				c1 = (get_color(surface, x+1,y) + get_color(surface, x,y+1))/2;
			}
			set_color(surface,x,y,c1);

			x = X+1+i-j;
			y = Y-j;
			if (j==0)
				c1 = get_color(surface, x+1,y);
			else if (j==i)
				c1 = get_color(surface, x,y-1);
			else
			{
				c1 = (get_color(surface, x+1,y) + get_color(surface, x,y-1))/2;
			}
			set_color(surface,x,y,c1);

			y = Y-1-i+j;
			x = X-j;
			if (j==0)
				c1 = get_color(surface, x-1,y);
			else if (j==i)
				c1 = get_color(surface, x,y-1);
			else
			{
				c1 = (get_color(surface, x-1,y) + get_color(surface, x,y-1))/2;
			}
			set_color(surface,x,y,c1);
		}
}


#define dist(a,b) abs(a-b)
void brush_lightning(SDL_Surface *surface, int X, int Y)
{
	int x,y,i,j,k,nx,ny,d;
	uint32_t c1,c2,c3;
	struct rgb_chan rgb1, rgb2, rgb3;
	
	c1 = get_color(surface, X,Y);
	rgb1 = *(struct rgb_chan*)&c1;
	for (k=0; k<3; ++k)
		if (rgb1.x[k]<0x5f)
			rgb1.x[k] += 2;
	c1 = *(uint32_t*)&rgb1;
	set_color(surface,X,Y,c1);

	//use last bit to record "visited"
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c2 = get_color(surface,x,y);
			c2 &= 0xfffffffe;
			set_color(surface,x,y,c2);
		}
	
	for (i=0; i<brush_size*4; ++i)
	{
		//send out a spark
		x = X; y = Y;

		//search within component
		for (j=0;j<brush_size*4+16;++j)
		{
			nx = x; ny = y;
			switch(rand()%4)
			{
			case 0:	--nx; break;
			case 1: --ny; break;
			case 2: ++nx; break;
			case 3: ++ny; break;
			}
			c2 = get_color(surface,nx,ny);
			if (c2&0x01) continue; //visited
			c2 |= 0x01;
			set_color(surface,nx,ny,c2); //record visited
			rgb2 = *(struct rgb_chan*)&c2;
			d = 0;
			for (k=0; k<3; ++k)
				d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
			if (d>128+rgb1.x[0]/2+rgb1.x[1]/2+rgb1.x[2]/2) continue;
			x = nx; y = ny;
			for (k=0; k<3; ++k)
				rgb2.x[k] = ((int)rgb1.x[k]+(int)rgb2.x[k])/2;
			c2 = *(uint32_t*)&rgb2;
			c2 |= 0x01;
			set_color(surface,x,y,c2);
		}
		//move out of component
		switch(rand()%4)
		{
		case 0: --x; break;
		case 1: --y; break;
		case 2: ++x; break;
		case 3: ++y; break;
		}
		c2 = get_color(surface,x,y);
		rgb2 = *(struct rgb_chan*)&c2;
		d = 0;
		for (k=0; k<3; ++k)
			d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
		if (d<4)
		{
			for (k=0; k<3; ++k)
				rgb3.x[k] = min(0xff,rgb2.x[k] + 1);
			c3 = *(uint32_t*)&rgb3;
			set_color(surface,x,y,c3);
		}
		else 
			set_color(surface,x,y,c1);
	}
}

void weird_lightning(SDL_Surface *surface, int X, int Y)
{
	int x,y,i,j,k,nx,ny,d;
	uint32_t c1,c2,c3;
	struct rgb_chan rgb1, rgb2, rgb3;
	
	c1 = get_color(surface, X,Y);
	rgb1 = *(struct rgb_chan*)&c1;
	for (k=0; k<3; ++k)
		++rgb1.x[k];
	c1 = *(uint32_t*)&rgb1;
	set_color(surface,X,Y,c1);

	//use last bit to record "visited"
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c2 = get_color(surface,x,y);
			c2 &= 0xfffffffe;
			set_color(surface,x,y,c2);
		}
	
	//for (i=0; i<brush_size; ++i)
	{
		//send out a spark
		x = X; y = Y;

		//search within component
		for (j=0;j<brush_size;++j)
		{
			nx = x; ny = y;
			switch(rand()%4)
			{
			case 0:	--nx; break;
			case 1: --ny; break;
			case 2: ++nx; break;
			case 3: ++ny; break;
			}
			c2 = get_color(surface,nx,ny);
			if (c2&0x01) continue; //visited
			c2 |= 0x01;
			set_color(surface,nx,ny,c2); //record visited
			rgb2 = *(struct rgb_chan*)&c2;
			d = 0;
			for (k=0; k<3; ++k)
				d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
			if (d<32) continue;
			x = nx; y = ny;
			for (k=0; k<3; ++k)
				rgb2.x[k] += ((int)rgb2.x[k]-(int)rgb1.x[k])/8;
			c2 = *(uint32_t*)&rgb2;
			c2 |= 0x01;
			set_color(surface,x,y,c2);
		}
		//move out of component
		switch(rand()%4)
		{
		case 0: --x; break;
		case 1: --y; break;
		case 2: ++x; break;
		case 3: ++y; break;
		}
		c2 = get_color(surface,x,y);
		rgb2 = *(struct rgb_chan*)&c2;
		for (k=0; k<3; ++k)
			rgb2.x[k] = rgb1.x[k] - rgb2.x[k];
		c2 = *(uint32_t*)&rgb2;
		set_color(surface,x,y,c2);
	}
}

void square_lightning(SDL_Surface *surface, int X, int Y)
{
	int x,y,i,j,k,nx,ny,d;
	uint32_t c1,c2,c3;
	struct rgb_chan rgb1, rgb2, rgb3;
	
	c1 = get_color(surface, X,Y);
	rgb1 = *(struct rgb_chan*)&c1;
	for (k=0; k<3; ++k)
		if (rgb1.x[k]<0x5f)
			rgb1.x[k] += 2;
	c1 = *(uint32_t*)&rgb1;
	set_color(surface,X,Y,c1);

	//use last bit to record "visited"
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c2 = get_color(surface,x,y);
			c2 &= 0xfffffffe;
			set_color(surface,x,y,c2);
		}
	
	for (i=0; i<brush_size; ++i)
	{
		//send out a spark
		x = X; y = Y;

		//search within component
		for (j=0;j<brush_size*4+16;++j)
		{
			nx = x; ny = y;
			switch(rand()%4)
			{
			case 0:	--nx; break;
			case 1: --ny; break;
			case 2: ++nx; break;
			case 3: ++ny; break;
			}
			c2 = get_color(surface,nx,ny);
			if (c2&0x01) continue; //visited
			c2 |= 0x01;
			set_color(surface,nx,ny,c2); //record visited
			rgb2 = *(struct rgb_chan*)&c2;
			d = 0;
			for (k=0; k<3; ++k)
				d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
			if (d>128+rgb1.x[0]/2+rgb1.x[1]/2+rgb1.x[2]/2) continue;
			x = nx; y = ny;
			for (k=0; k<3; ++k)
				rgb2.x[k] = ((int)rgb1.x[k]+(int)rgb2.x[k])/2;
		}
		//move out of component
		switch(rand()%4)
		{
		case 0: --x; break;
		case 1: --y; break;
		case 2: ++x; break;
		case 3: ++y; break;
		}
		nx = x; ny = y;
		for (x=min(nx,X); x<=max(nx,X); ++x)
			for (y=min(ny,Y); y<=max(ny,Y); ++y)
			{
				c2 = get_color(surface,x,y);
				rgb3 = *(struct rgb_chan*)&c2;
				for (k=0; k<3; ++k)
					rgb3.x[k] = (rgb1.x[k]+rgb3.x[k])/2;
				c3 = *(uint32_t*)&rgb3;
				set_color(surface,x,y,c3);
			}
	}
}

void dark_lightning(SDL_Surface *surface, int X, int Y)
{
	int x,y,i,j,k,nx,ny,d;
	uint32_t c1,c2,c3;
	struct rgb_chan rgb1, rgb2, rgb3;
	
	c1 = get_color(surface, X,Y);
	rgb1 = *(struct rgb_chan*)&c1;
	for (k=0; k<3; ++k)
		if (rgb1.x[k]>0)
			--rgb1.x[k];
	c1 = *(uint32_t*)&rgb1;
	set_color(surface,X,Y,c1);

	//use last bit to record "visited"
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c2 = get_color(surface,x,y);
			c2 &= 0xfffffffe;
			set_color(surface,x,y,c2);
		}
	
	for (i=0; i<brush_size*2; ++i)
	{
		//send out a spark
		x = X; y = Y;

		//search within component
		for (j=0;j<brush_size*4+16;++j)
		{
			nx = x; ny = y;
			switch(rand()%4)
			{
			case 0:	--nx; break;
			case 1: --ny; break;
			case 2: ++nx; break;
			case 3: ++ny; break;
			}
			c2 = get_color(surface,nx,ny);
			if (c2&0x01) continue; //visited
			c2 |= 0x01;
			set_color(surface,nx,ny,c2); //record visited
			rgb2 = *(struct rgb_chan*)&c2;
			d = 0;
			for (k=0; k<3; ++k)
				d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
			if (d>128+rgb1.x[0]/2+rgb1.x[1]/2+rgb1.x[2]/2) continue;
			x = nx; y = ny;
			//for (k=0; k<3; ++k)
			//	rgb2.x[k] = ((int)rgb1.x[k]+(int)rgb2.x[k])/2;
			//c2 = *(uint32_t*)&rgb2;
			//c2 |= 0x01;
			//set_color(surface,x,y,c2);
		}
		//move out of component
		switch(rand()%4)
		{
		case 0: --x; break;
		case 1: --y; break;
		case 2: ++x; break;
		case 3: ++y; break;
		}
		c2 = get_color(surface,x,y);
		rgb2 = *(struct rgb_chan*)&c2;
		d = 0;
		for (k=0; k<3; ++k)
			d += abs((int)rgb2.x[k]-(int)rgb1.x[k]);
		if (d<4)
		{
			for (k=0; k<3; ++k)
				if (rgb2.x[k]>0)
					--rgb2.x[k];
			c3 = *(uint32_t*)&rgb2;
			set_color(surface,x,y,c3);
		} else {
			for (k=0; k<3; ++k)
				rgb3.x[k] = (rgb2.x[k]+rgb1.x[k])/2;
			c3 = *(uint32_t*)&rgb3;
			set_color(surface,x,y,c3);
		}
	}
}

void brush_col(SDL_Surface *surface, int x, int y)
{
	int i,j;
	uint32_t c0,c;
	int s;
	for (j=-3; j<=+3; ++j)
	{
		s = 1+brush_size/2;
		c0 = get_color(surface,x+j,y-s);
		for (i=y-s; i<y+s; ++i)
		{
			c = get_color(surface,x+j,i+1);
			set_color(surface,x+j,i,c);
		}
		set_color(surface,x+j,i,c0);
	}
}

void brush_dichotomy(SDL_Surface *surface, int x, int y)
{
	int i,j,k;
	uint32_t c1,c2,c3;
	struct rgb_chan rgb1, rgb2, rgb3;
	int s;

	for (k=0; k<3; ++k)
	{
		rgb2.x[k] = 255;
		rgb3.x[k] = 0;
	}

	s = 1+brush_size/2;
	for (i=-s; i<=+s; ++i)
		for (j=-s; j<=+s; ++j)
		{
			c1 = get_color(surface,x+i,y+j);
			rgb1 = *(struct rgb_chan*)&c1;
			//for (k=0; k<3; ++k)
			//{
			//	rgb2.x[k] = min(rgb1.x[k], rgb2.x[k]);
			//	rgb3.x[k] = max(rgb1.x[k], rgb3.x[k]);
			//}
			if (rgb1.x[0]+rgb1.x[1]+rgb1.x[2]<rgb2.x[0]+rgb2.x[1]+rgb2.x[2])
			{
				rgb2.x[0] = rgb1.x[0]; rgb2.x[1] = rgb1.x[1]; rgb2.x[2] = rgb1.x[2];
			}
			if (rgb1.x[0]+rgb1.x[1]+rgb1.x[2]>rgb3.x[0]+rgb3.x[1]+rgb3.x[2])
			{
				rgb3.x[0] = rgb1.x[0]; rgb3.x[1] = rgb1.x[1]; rgb3.x[2] = rgb1.x[2];
			}
		}
	//for (k=0; k<3; ++k)
	//{
	//	if (rgb2.x[k]>0) --rgb2.x[k];
	//	if (rgb3.x[k]<0xff) ++rgb3.x[k];
	//}
	for (i=-s; i<=+s; ++i)
		for (j=-s; j<=+s; ++j)
		{
			c1 = get_color(surface,x+i,y+j);
			rgb1 = *(struct rgb_chan*)&c1;
			for (k=0; k<3; ++k)
			{
				//if (rgb1.x[k]-rgb2.x[k]<rgb3.x[k]-rgb1.x[k])
				if ((rgb1.x[0]+rgb1.x[1]+rgb1.x[2])-(rgb2.x[0]+rgb2.x[1]+rgb2.x[2])<(rgb3.x[0]+rgb3.x[1]+rgb3.x[2])-(rgb1.x[0]+rgb1.x[1]+rgb1.x[2]))
				{
					if (rgb1.x[k]>rgb2.x[k]) --rgb1.x[k];
				} else {
					if (rgb1.x[k]<rgb3.x[k]) ++rgb1.x[k];
				}
			}
			c1 = *(uint32_t*)&rgb1;
			set_color(surface,x+i,y+j,c1);
		}
}

void brush_control(SDL_Surface *surface, int x, int y)
{
	int i,j,k;
	uint32_t c1,c2,c3,c4,c5;
	struct rgb_chan rgb1, rgb2, rgb3, rgb4, rgb5;
	int s;

	s = 1+brush_size/2;
	for (i=-s; i<=+s; ++i)
		for (j=-s; j<=+s; ++j)
		{
			if (!(i==s || i==-s || j==s || j==-s)) continue; //just do the border
			c1 = get_color(surface,x+i,y+j);
			rgb1 = *(struct rgb_chan*)&c1;

			c2 = get_color(surface,x+i+1,y+j);
			rgb2 = *(struct rgb_chan*)&c2;
			c3 = get_color(surface,x+i-1,y+j);
			rgb3 = *(struct rgb_chan*)&c3;
			c4 = get_color(surface,x+i,y+j+1);
			rgb4 = *(struct rgb_chan*)&c4;
			c5 = get_color(surface,x+i,y+j-1);
			rgb5 = *(struct rgb_chan*)&c5;
			for (k=0; k<3; ++k)
			{
				rgb1.x[k] = rgb2.x[k] ^ rgb3.x[k] ^ rgb4.x[k] ^ rgb5.x[k];
			}
			c1 = *(uint32_t*)&rgb1;
			set_color(surface,x+i,y+j,c1);
		}
}

void brush_untitled(SDL_Surface *surface, int x, int y)
{
	int i,j,k,j_s;
	uint32_t c1,c2;
	int s;

	s = 1+brush_size;
	j=x;
	k=y;
	j_s=+1;
	c1 = get_color(surface,j,k);
	for (i=0; i<=(s+1)*(s+1); ++i)
	{
		j+=j_s;
		if (j>=x+s/2 || j<x-s/2)
		{
			j_s*=-1;
			++k;
			if (k>=y+s/2)
				k = y-s/2;
		}
		c2 = get_color(surface,j,k);
		set_color(surface,j,k,c1);
		c1=c2;
	}
}

EXPORT void set_brush_size(int n)
{
	brush_size = n;
}

EXPORT void set_brush_type(int n)
{
	brush_type = n;
}

EXPORT void circle_thing(SDL_Surface *surface, int X, int Y)
{
	switch(brush_type)
	{
	case 0:
		break;
	case 1:
		brush_smoosh(surface,X,Y);
		break;
	case 2:
		brush_col(surface,X,Y);
		break;
	case 3:
		brush_lightning(surface,X,Y);
		break;
	case 4:
		square_lightning(surface,X,Y);
		break;
	case 5:
		brush_dichotomy(surface,X,Y);
		break;
	case 6:
		weird_lightning(surface,X,Y);
		break;
	case 7:
		brush_control(surface,X,Y);
		break;
	case 8:
		brush_untitled(surface,X,Y);
		break;
	case 9:
		dark_lightning(surface,X,Y);
		break;
	}
}

EXPORT void down_thing(SDL_Surface *surface)
{
	int x,y; //int i,j;
	uint32_t c1, c2;
	struct rgb_col rgb1, rgb2;
	unsigned char tmp;
	for (x=0; x<surface->w; ++x)
		for (y=surface->h-1; y>0; --y)
		{
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_col*)&c1;
			c2 = get_color(surface,x,y-1);
			rgb2 = *(struct rgb_col*)&c2;

			if (rgb2.r>rgb1.r)
			{
				tmp = rgb1.r;
				rgb1.r = rgb2.r;
				rgb2.r = tmp;
			}
			if (rgb2.g>rgb1.g)
			{
				tmp = rgb1.g;
				rgb1.g = rgb2.g;
				rgb2.g = tmp;
			}
			if (rgb2.b>rgb1.b)
			{
				tmp = rgb1.b;
				rgb1.b = rgb2.b;
				rgb2.b = tmp;
			}
			c1 = *(uint32_t*)&rgb1;
			c2 = *(uint32_t*)&rgb2;
			set_color(surface,x,y,c1);
			set_color(surface,x,y-1,c2);
		}
}

EXPORT void find_edges(SDL_Surface *surface, int channel, int level, uint32_t color)
{
	int x,y, n;
	uint32_t c1, c2;
	struct rgb_chan rgb1, rgb2;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_chan*)&c1;
			n = 0;
			if (x>0)
			{
				c2 = get_color(surface,x-1,y);
				rgb2 = *(struct rgb_chan*)&c2;
				if (rgb1.x[channel]>=level && rgb2.x[channel]<level) ++n;
			}
			if (y>0)
			{
				c2 = get_color(surface,x,y-1);
				rgb2 = *(struct rgb_chan*)&c2;
				if (rgb1.x[channel]>=level && rgb2.x[channel]<level) ++n;
			}
			if (x<surface->w-1)
			{
				c2 = get_color(surface,x+1,y);
				rgb2 = *(struct rgb_chan*)&c2;
				if (rgb1.x[channel]>=level && rgb2.x[channel]<level) ++n;
			}
			if (y<surface->h-1)
			{
				c2 = get_color(surface,x,y+1);
				rgb2 = *(struct rgb_chan*)&c2;
				if (rgb1.x[channel]>=level && rgb2.x[channel]<level) ++n;
			}
			if (n>0) set_color(surface,x,y,color);
		}
}

EXPORT void bandit(SDL_Surface *surface)
{
	int x,y, n;
	uint32_t c1;
	struct rgb_chan rgb1;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_chan*)&c1;
			for (n=0; n<3; ++n)
			{
				if ((((int)rgb1.x[n])/64)*64+16<rgb1.x[n])
					--rgb1.x[n];
				else
					++rgb1.x[n];
			}
			c1 = *(uint32_t*)&rgb1;
			set_color(surface,x,y,c1);
		}
}

EXPORT void magic(SDL_Surface *surface)
{
	int x,y;// n;
	uint32_t c1,c2;
	struct rgb_col rgb1,rgb2;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_col*)&c1;
			if (rgb1.r>32)
			{
				c2 = get_color(surface,x-1,y);
				rgb2 = *(struct rgb_col*)&c2;
				rgb2.r = (rgb1.r+rgb2.r)/2; 
				c2 = *(uint32_t*)&rgb2;
				set_color(surface,x-1,y,c2);
				
				c2 = get_color(surface,x+1,y+1);
				rgb2 = *(struct rgb_col*)&c2;
				++rgb2.r;
				c2 = *(uint32_t*)&rgb2;
				set_color(surface,x+1,y+1,c2);

				c2 = get_color(surface,x+1,y-1);
				rgb2 = *(struct rgb_col*)&c2;
				++rgb2.r;
				c2 = *(uint32_t*)&rgb2;
				set_color(surface,x+1,y-1,c2);
			}
			//c1 = *(uint32_t*)&rgb1;
			//set_color(surface,x,y,c1);
		}
}

const double value[] = //how much a given segment's similarity is worth
{
	20.0/(4.0*4.0),		//4*4
	30.0/(8.0*8.0),		//8*8
	20.0/(16.0*16.0),	//16*16
	15.0/(32.0*32.0),	//32*32
	10.0/(64.0*64.0),	//64*64
	 5.0/(128.0*128.0),	//128*128
};

EXPORT double compute_similarity(SDL_Surface *surface1, SDL_Surface *surface2)
{
	int x,y,i,a,b,q;
	double total[3], real_total, quot;
	uint32_t c1, c2;
	struct rgb_chan rgb1, rgb2;
	int start_x, start_y, width, num_segs;
	double h1,s1,v1,h2,s2,v2;
	
	real_total = 0;
	num_segs = 4;

	for (q=0; q<6; ++q)
	{
		for (a=0; a<num_segs; ++a)
			for (b=0; b<num_segs; ++b)
			{
				width = surface1->w/num_segs;
				start_x = a*width;
				start_y = b*width;
				for (i=0; i<3; ++i)
					total[i] = 0;
				for (x=start_x; x<start_x+width; ++x)
					for (y=start_y; y<start_y+width; ++y)
					{
						c1 = get_color(surface1,x,y);
						rgb1 = *(struct rgb_chan*)&c1;
						c2 = get_color(surface2,x,y);
						rgb2 = *(struct rgb_chan*)&c2;
						RGBtoHSV((double)rgb1.x[0],(double)rgb1.x[1]/255.0,(double)rgb1.x[2]/255.0, &h1,&s1,&v1);
						RGBtoHSV((double)rgb2.x[0],(double)rgb2.x[1]/255.0,(double)rgb2.x[2]/255.0, &h2,&s2,&v2);
						//total[0] += (h1-h2)/128.0;
						if (abs(h1-h2)>180)
							total[0] -= 360+h1-h2;
						else
							total[0] += h1-h2;
						total[1] += (s1-s2)/128.0;
						total[2] += (v1-v2)/128.0;
					}
				quot = abs(total[0])+abs(total[1])+abs(total[2]);
				//if (quot>0.01f) printf("%f\n", quot);
				//if (quot<1.0)//0.00001f)
					//real_total += value[q];
				if (quot>=0.0 && quot<32.0)
					real_total += value[q]*((32.0-quot)/32.0);
			}
		num_segs*=2;
	}

	//printf("%f\n", real_total);
	//real_total += 0.4;

	return real_total;
}

SDL_Surface *load_image(const char *filename);

EXPORT void textify(SDL_Surface *surface)
{
	SDL_Surface *temp, *font;
	uint32_t c1;
	struct rgb_chan rgb1;
	int x,y,any,i,j;
	SDL_Rect r1,r2;

	temp = SDL_CreateRGBSurface(0, surface->w, surface->h, 24, rmask, gmask, bmask, 0);
	memcpy(temp->pixels, surface->pixels, 3 * surface->w * surface->h);
	any = 0;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(surface,x,y);
			rgb1 = *(struct rgb_chan*)&c1;
			if (rgb1.x[0]+rgb1.x[1]+rgb1.x[2]>224+224+224)
			{
				set_color(surface,x,y,0);
				set_color(temp,x,y,0xffffffff);
				any = 1;
			} else {
				set_color(temp,x,y,0);
			}
		}
	if (any==0)
	{
		SDL_FreeSurface(temp);
		return;
	}
	font = load_image("resource/font/font8x8.png");
	r1.w = r2.w = 8;
	r1.h = r2.h = 8;
	for (x=0; x<surface->w; ++x)
		for (y=0; y<surface->h; ++y)
		{
			c1 = get_color(temp,x,y);
			if (c1>0)
			{
				r1.x =r1.w*(rand()%(font->w/r1.w));
				r1.y = r1.h*(rand()%(font->h/r1.h));
				r2.x = x; r2.y = y;
				SDL_UpperBlit(font, &r1, surface, &r2);
				for (i=x; i<x+r1.w && i<surface->w; ++i)
					for (j=y; j<y+r1.h && j<surface->h; ++j)
						set_color(temp,i,j,0);
			}
		}

	SDL_FreeSurface(temp);
	SDL_FreeSurface(font);
}

int dir(SDL_Surface *surface1, int x, int y)
{
	//while (y<0)
	//	y += y+surface1->h;
	if (y%2==0)
	{
		if (x==surface1->w-1) return 1;
		else return 0;
	} else {
		if (x==1) return 1;
		else return 2;
	}
}

int dir_maximising_disparity(SDL_Surface *surface1, SDL_Surface *surface2, int x, int y)
{
	int d,nx,ny,i,p;
	uint32_t c1,c2;
	struct rgb_chan rgb1,rgb2;
	int best_d, best_p;

	for (d=0;d<4;++d)
	{
		nx = x+dx_tab[d];
		ny = y+dy_tab[d];
		c1 = get_color(surface1,nx,ny);
		rgb1 = *(struct rgb_chan*)&c1;
		c2 = get_color(surface2,nx,ny);
		rgb2 = *(struct rgb_chan*)&c2;
		p = 0;
		for (i=0;i<3;++i)
			p+=abs(rgb1.x[i]-rgb2.x[i]);
		if (d==0 || p>best_p)
		{
			best_p = p;
			best_d = d;
		}
	}
	return best_d;
}

#define IS_ATTEMPTS 2
int last_weapon=-1;
EXPORT int increase_similarity(SDL_Surface *surface1, SDL_Surface *surface2, int x, int y)
{
	SDL_Surface *newsurf;
	int new_x, new_y, new_d;
	int i,n,m,j;
	double similarity, old_similarity;


	old_similarity = compute_similarity(surface1, surface2);
	//n = (100-old_similarity)/5;
	//for (i=0; i<n; ++i)
	//	old_similarity-=(rand()%8)*0.125f;

	newsurf = SDL_CreateRGBSurface(0, surface1->w, surface1->h, 24, rmask, gmask, bmask, 0);
	for (i=0; i<IS_ATTEMPTS; ++i)
	{
		memcpy(newsurf->pixels, surface1->pixels, 4 * surface1->w * surface1->h);
		
		new_x = x;
		new_y = y;
		if (rand()%4==0)
			new_d = dir_maximising_disparity(surface1,surface2,x,y);
		else
			new_d = dir(surface1,x,y);//min(rand()%4,rand()%4);
		new_x += dx_tab[new_d];
		new_y += dy_tab[new_d];
		if (new_x<0) new_x+=surface1->w;
		if (new_y<0) new_y+=surface1->h;
		if (new_x>=surface1->w) new_x-=surface1->w;
		if (new_y>=surface1->h) new_y-=surface1->h;

		//do a random action
		//these are arranged in order of how drastic they are so it'll pick the more subtle ones as it closes in
		//n = rand()%32;
		//for (j=0;j<4;++j)
		//{
		//	m = rand()%32;
		//	if (abs(m-old_similarity*32/100.0)<abs(n-old_similarity*32/100.0))
		//		n = m;
		
		if (last_weapon>-1)
		{
			n = last_weapon;
			last_weapon = -1;
		}
		//else if (i==1)
		//{
		//	if (old_similarity<10)
		//		n = rand()%8;
		//	else if (old_similarity<20)
		//		n = rand()%8+4;
		//	else if (old_similarity<30)
		//		n = rand()%8+8;
		//	else if (old_similarity<40)
		//		n = rand()%8+16;
		//	else if (old_similarity<50)
		//		n = rand()%8+20;
		//	else
		//		n = rand()%8+24;
		//}
		else n = max(rand()%32,rand()%32);
		switch(n)
		{
		case 0:
			find_edges(newsurf, 0, 128, 0xffffff); break;
		case 1:
			do_another_thing(newsurf); break; //leftward greying blur
		case 2:
			down_thing(newsurf); break; //sort
		case 3:
			find_edges(newsurf, 1, 128, 0xff00ff); break;
		case 4:
			magic(newsurf); break; //red flow
		case 5:
			do_a_thing(newsurf); break; //contrast blur
		case 6:
			find_edges(newsurf, 2, 92, 0xff0000); break;
		case 7: 
			brush_size = 32; brush_smoosh(newsurf,x,y); break;
		case 8:
			do_a_weird_thing(newsurf); break; //colourcycle
		case 9: 
			brush_size = 32; weird_lightning(newsurf,x,y); break;
		case 10: 
			brush_size = 32; brush_lightning(newsurf,x,y); break;
		case 11: 
			brush_size = 16; square_lightning(newsurf,x,y); break;
		case 12:
			bandit(newsurf); break; //banding
		case 13: 
			brush_size = 32; brush_control(newsurf,x,y); break;
		case 14:
			do_a_locational_thing(newsurf,x,y); break; //blur column
		case 15:
			textify(newsurf); break;
		case 16: 
			brush_size = 24; brush_col(newsurf,x,y); break;
		case 17:
			also_do_something_neat_idk(newsurf, 0); break; //blue glow
		case 18: 
			brush_size = 24; brush_untitled(newsurf,x,y); break;
		case 19: 
			brush_size = 64; dark_lightning(newsurf,x,y); break;
		case 20: 
			brush_size = 4; weird_lightning(newsurf,x,y); break;
		case 21: 
			brush_size = 4; brush_smoosh(newsurf,x,y); break;
		case 22: 
			brush_size = 4; brush_control(newsurf,x,y); break;
		case 23: 
			brush_size = 8; brush_untitled(newsurf,x,y); break;
		case 24: 
			brush_size = 8; brush_dichotomy(newsurf,x,y); break;
		case 25: 
			brush_size = 4; brush_col(newsurf,x,y); break;
		case 26: 
			brush_size = 2; dark_lightning(newsurf,x,y); break;
		case 27: 
			brush_size = 2; brush_dichotomy(newsurf,x,y); break;
		case 28: 
			brush_size = 1; square_lightning(newsurf,x,y); break;
		case 29: 
			brush_size = 1; weird_lightning(newsurf,x,y); break;
		case 30: 
			brush_size = 1; brush_lightning(newsurf,x,y); break;
		case 31: 
			brush_size = 1; dark_lightning(newsurf,x,y); break;
		}

		similarity = compute_similarity(newsurf, surface2);
		if (similarity>compute_similarity(surface1, surface2))
		{
			last_weapon = n;
			memcpy(surface1->pixels, newsurf->pixels, 4 * surface1->w * surface1->h);
			SDL_FreeSurface(newsurf);
			return new_d;
		}

		//if (i>IS_ATTEMPTS/2)
		//	old_similarity-=0.0125;//(rand()%8)*0.125f;
    }

	//none found
	SDL_FreeSurface(newsurf);
	return dir(surface1,x,y);//min(rand()%2,rand()%2);
}

SDL_Surface *mosurf;
SDL_Rect mr1,mr2;
#define MOUSE_W 16
EXPORT void preserve_mouse(SDL_Surface *surface, int x, int y)
{
	mr1.x = max(0, x-MOUSE_W/2);
	mr1.w = min(MOUSE_W, surface->w-mr1.x);
	mr1.y = max(0, y-MOUSE_W/2);
	mr1.h = min(MOUSE_W, surface->h-mr1.y);
	mr2.x=0; mr2.y=0;
	mr2.w = mr1.w; mr2.h = mr1.h;
	mosurf = SDL_CreateRGBSurface(0, mr1.w, mr1.h, 24, rmask, gmask, bmask, 0);
	SDL_UpperBlit(surface, &mr1, mosurf, &mr2);
}
EXPORT void restore_mouse(SDL_Surface *surface, int x, int y)
{
	SDL_UpperBlit(mosurf, &mr2, surface, &mr1);
	SDL_FreeSurface(mosurf);
}

EXPORT void flipline(SDL_Surface *surface, int X, int Y)
{
	uint32_t c;
	double d,f,l;
	double x,y,x1,y1;

	c = get_color(surface,X,Y);
	d = sqrt(((double)X-Y)*(X-Y)*2);
	x=X; y = Y;
	x1 = y-x;
	y1 = x-y;
	l = sqrt(x1*x1 + y1*y1);
	if (l<0.001) l = 0.001;
	x1 /= l;
	y1 /= l;
	for (f = 0; f<d; f+=1.0, x+=x1, y+=y1)
	{
		set_color(surface,x,y,c);
	}
}

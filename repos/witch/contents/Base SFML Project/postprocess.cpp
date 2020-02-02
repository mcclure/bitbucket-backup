/*
 *  postprocess.cpp
 *  SFML
 *
 *  Created by Andi McClure on 4/26/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#include "postprocess.h"

Processor *currentProcessor = NULL;
int blurRadius = 1;

void Processor::drawDot(float x, float y, Pixel p)
{
	if ((x >= 0) && (x < worldWidth) && (y >= 0) && (y < worldHeight))
	{
		state[(int)x][(int)y] = p;
	}
}

void Processor::drawLine(float x1, float y1, float x2, float y2, Pixel p)
{
	float x, y;
	for (float t = 0; t <= 1.0; t += 0.1)
	{
		x = x1 + t * (x2 - x1);
		y = y1 + t * (y2 - y1);
		if ((x >= 0) && (x < worldWidth) && (y >= 0) && (y < worldHeight))
		{
			if (state[(int)x][(int)y].color[RED] < 0.5)
				state[(int)x][(int)y].color[RED] += 0.00;
			if (state[(int)x][(int)y].color[GREEN] < 0.5)
				state[(int)x][(int)y].color[GREEN] += 0.004;
			if (state[(int)x][(int)y].color[BLUE] < 0.5)
				state[(int)x][(int)y].color[BLUE] += 0.007;
		}
	}
}

void FadeProcessor::preBuffer()
{
	for (uint iY = 0; iY < worldHeight; iY ++) 
	{
		for (uint iX = 0; iX < worldWidth; iX ++) 
		{
			for (uint i = 0; i < 3; i ++)
			{
				if (state[iX][iY].color[i] > 0.08)
					state[iX][iY].color[i] -= 0.08;
				else
					(state[iX][iY].color[i] = 0);
			}
		}
	}
}

void maybeBetter(Pixel *& prefer, Pixel &other) {
	if (prefer->age < other.age)
		prefer = &other;
}

void BleedProcessor::postBuffer()
{
	swapBoard();
	
#if 1 // Edges?
	for (uint iX = 0; iX < worldWidth; iX ++) {
		stateScratch[iX][0] = stateScratch[iX][worldHeight-1] = Pixel(0.75, 0.75, 0.75); //Pixel().randomNear(0,0,0,0.3);
	}
	for (uint iY = 0; iY < worldHeight; iY ++) {
		stateScratch[0][iY] = stateScratch[worldWidth-1][iY] = Pixel(0.75, 0.75, 0.75); //Pixel().randomNear(0,0,0,0.3);
	}
#endif
	
	for (uint iY = 0; iY < worldHeight; iY ++) 
	{
		for (uint iX = 0; iX < worldWidth; iX ++) 
		{
			Pixel &center = state[iX][iY];
			
			Pixel *prefer = &stateScratch[iX][iY];
			
			if (iX > 0) maybeBetter(prefer, stateScratch[iX-1][iY]);
			if (iY > 0) maybeBetter(prefer, stateScratch[iX][iY-1]);
			if (iX < worldWidth-1) maybeBetter(prefer, stateScratch[iX+1][iY]);
			if (iY < worldHeight-1) maybeBetter(prefer, stateScratch[iX][iY+1]);
			
#if 0
			if (iX > 0            && iY < worldHeight-1) maybeBetter(prefer, stateScratch[iX-1][iY+1]);
			if (iX < worldWidth-1 && iY < worldHeight-1) maybeBetter(prefer, stateScratch[iX+1][iY+1]);
			if (iX < worldWidth-1 && iY > 0)             maybeBetter(prefer, stateScratch[iX+1][iY-1]);
			if (iX > 0            && iY > 0)             maybeBetter(prefer, stateScratch[iX-1][iY-1]);
#endif
			
			center = *prefer;
			if (center.age > 0) center.age--;
		}
	}
}


void LightingProcessor::drawDot(float x, float y, Pixel p)
{
	if ((x >= 0) && (x < worldWidth) && (y >= 0) && (y < worldHeight))
	{
		state[(int)x][(int)y].color[RED] += 0.005 * p.color[RED];
		state[(int)x][(int)y].color[GREEN] += 0.005 * p.color[GREEN];
		state[(int)x][(int)y].color[BLUE] += 0.005 * p.color[BLUE];
	}
}


void LightingProcessor::drawLine(float x1, float y1, float x2, float y2, Pixel p)
{
	float x, y;
	//for (float t = -6.4; t < 0.6; t += 0.1) // fun- draws long quivering spindles
	for (float t = 0; t <= 1.0; t += 0.1)
	{
		x = x1 + t * (x2 - x1);
		y = y1 + t * (y2 - y1);
		if ((x >= 0) && (x < worldWidth) && (y >= 0) && (y < worldHeight))
		{
			state[(int)x][(int)y].color[RED] += 0.005 * p.color[RED];
			state[(int)x][(int)y].color[GREEN] += 0.005 * p.color[GREEN];
			state[(int)x][(int)y].color[BLUE] += 0.005 * p.color[BLUE];
		}
	}
}


void LightingProcessor::preBuffer()
{
	swapBoard();
	Pixel p1(1.0, 0.0, 1.0);
	Pixel p2(0.2, 0.75, 1.0);
	for (uint iY = 0; iY < worldHeight; iY ++) 
	{
		for (uint iX = 0; iX < worldWidth; iX ++) 
		{
			for (uint i = 0; i < 3; i ++)
			{
				// fade the source image
				/*
				if (state[iX][iY].color[i] > 0.1)
					state[iX][iY].color[i] -= 0.1;
				else
					(state[iX][iY].color[i] = 0);
				*/
				
				// clear
				//colorLookup[iX][iY].color[i] = lerp(((float)iY) / ((float) worldHeight), p1.color[i], p2.color[i]);
				//colorLookup[iX][iY].color[i] += 0.1 * ((float) (iY + iY % 2));
				state[iX][iY].color[i] = 0.0;
				//if (state[iX][iY].color[i] > 0.01)
				//	state[iX][iY].color[i] -= 0.01; 
			}
		}
	}
}

// this uses color laid down by the dots and line drawing and pretends it's a heightmap
// then it does a couple of things with that heightmap
// it does something resembling refraction by getting the slope of the heightmap along both x and y axes, and using that to offset the rendering of the background
// it also can lay in some diffuse lighting based on the slopes
// it also can do multiplicative filtering based on the color of the dots and lines drawn
void LightingProcessor::postBuffer()
{
	Processor::postBuffer();

	//swapBoard();
	float xSlope, ySlope;
	int lookupX, lookupY;
	float lighting, multiplicativeColor;
	float bendAmplitude = 1024.0f;
	float height;
	Pixel multiplicativeTint(1.0, 0.5, 0.01);
	for (int iY = 1; iY < worldHeight - 1; iY ++) 
	{
		for (int iX = 1; iX < worldWidth - 1; iX ++) 
		{
			xSlope = state[iX + 1][iY].color[BLUE] - state[iX][iY].color[BLUE];
			ySlope = state[iX][iY + 1].color[BLUE] - state[iX][iY].color[BLUE];
			lookupX = max(0, min(worldWidth - 1, (int) (iX + bendAmplitude * xSlope)));
			lookupY = max(0, min(worldHeight - 1, (int) (iY + bendAmplitude * ySlope)));
			height = state[iX][iY].color[BLUE];
			for (uint i = 0; i < 3; i ++)
			{
				lighting = 1.0 * (0.0 + ySlope);// * state[iX][iY].color[i]; // see, this is wrong because it ends up being brighter at hight points
				multiplicativeColor = pow(multiplicativeTint.color[i], height * 10); //lerp(height, 1.0, state[iX][iY].color[i]);
				state[iX][iY].color[i] = clamp(0.0, lighting + multiplicativeColor * colorLookup[lookupX][lookupY].color[i], 1.0);
			}
		}
	}
}

void BlurProcessor::postBuffer()
{
	Processor::postBuffer();
	swapBoard();
	float value;
	Pixel total;
	int totalOperations = 0;
	float pixelsPerOperationDivisor = 1.0f / ((float) (pow(blurRadius + blurRadius + 1, 2)));
	for (uint iY = 0; iY < worldHeight; iY ++) 
	{
		for (uint iX = 0; iX < worldWidth; iX ++) 
		{
			total.color[RED] = 0; // comment out for neat bands
			total.color[GREEN] = 0; 
			total.color[BLUE] = 0;
			totalOperations ++;
			for (int iY2 = -blurRadius; iY2 <= blurRadius; iY2 ++)
			{
				for (int iX2 = -blurRadius; iX2 <= blurRadius; iX2 ++)
				{
					for (uint i = 0; i < 3; i ++)
					{
						total.color[i] += stateScratch[(iX + iX2 + worldWidth) % worldWidth][(iY + iY2 + worldHeight) % worldHeight].color[i];
					}
				}
			}
			for (uint i = 0; i < 3; i ++)
			{
				state[iX][iY].color[i] = total.color[i] * pixelsPerOperationDivisor;
			}
		}
	}

	int totalPixels = worldWidth * worldHeight;
}
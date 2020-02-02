#include "globals.h"
#include "scene.h"

Pixel** state = NULL;
Pixel** stateScratch = NULL;
Pixel** colorLookup = NULL;
Scene *scene = NULL;
extern int worldWidth, worldHeight;
GroupTuning* groupTuning;

board makeBoard() {
	board result = new Pixel*[worldWidth];
	for (uint i = 0; i < worldWidth; i ++)
	{
		result[i] = new Pixel[worldHeight];
	}

#if 1
	// Uncomment to make new default color
	for (uint iY = 0; iY < worldHeight; iY ++) 
	{
		for (uint iX = 0; iX < worldWidth; iX ++) 
		{
			result[iX][iY] = Pixel(0,0,0);
		}
	}
#endif
	
	return result;
}

void destroyBoard(board b) {
	for(uint i = 0; i < worldWidth; i++)
		delete[] b[i];
	delete[] b;
}

inline void jiggle(float &x, float _t) {
	x = clamp(0, x + _t, 1);
}

Pixel Pixel::randomNear(float _t) const {
	float o = randomfloat(-_t, _t);
	float _r = color[RED], _g = color[GREEN], _b = color[BLUE];
	jiggle(_r, o); jiggle(_g, o); jiggle(_b, o);
	return Pixel(_r,_g,_b);
}

#define FIXED_PICK 1
#define TEST_PICK 0
#define RANDOM_PICK 0

void setupScene() {
	scene = new Scene();
	
#if FIXED_PICK
	int pick = 8, gpick = 0;
#else
#if TEST_PICK
	static int pick_gen = 0, gpick_gen = 0;
	int pick = pick_gen, gpick = gpick_gen; pick_gen++;
	if (pick_gen > 5) { pick_gen = 0; gpick_gen++; }
	if (gpick_gen > 1) { gpick_gen = 0; }
#else /* RANDOM_PICK */
	int pick = random() % 8, gpick = random() % 2;
	//int pick = 5, gpick = 0;
#endif
#endif

	fprintf(stderr, "PICKED %d GFX %d\n", pick, gpick);
	
	switch (pick) {
		case 0: { // "Basic scene" (CONSIDER DELETE?)
			scene->groupImage = RESOURCES "testImage.png";
			scene->colorImage = RESOURCES "gravel.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 16;
			
			scene->songWanderers = 2;
			scene->songFrequency = 110;
		} break;
		case 1: {
			scene->groupImage = RESOURCES "tubeImage.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 8;
			
			scene->setSpringRules(0, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(1, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(2, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 330;
		} break;
		case 2: {
			scene->groupImage = RESOURCES "freeShapes.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 16;
			
			scene->songWanderers = 3;
			scene->songFrequency = 440;
		} break;
		case 3: {
			scene->groupImage = RESOURCES "circularWallRupture.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 6;
			scene->setSpringRules(0, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(1, SPRING_FREE);
			scene->setSpringRules(2, SPRING_GROUPED);
			scene->setSpringRules(3, SPRING_GROUPED);
			scene->setSpringRules(4, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 4: {
			scene->groupImage = RESOURCES "partitionTest.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 16;
			scene->setSpringRules(0, SPRING_GROUPED);
			//scene->setSpringRules(1, SPRING_GROUPED);
			//scene->setSpringRules(2, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 5: {
			scene->groupImage = RESOURCES "envelopes.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 8;
			scene->setSpringRules(0, SPRING_GROUPED);
			scene->setSpringRules(1, SPRING_GROUPED);
			scene->setSpringRules(2, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(3, SPRING_FREE);
			scene->setSpringRules(4, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 6: {
			if (random() % 2 == 0)
				scene->groupImage = RESOURCES "villi0.png";
			else
				scene->groupImage = RESOURCES "villi1.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 8;
			scene->setSpringRules(0, SPRING_LINK_GROUP_PLUS_ANCHORS);
			scene->setSpringRules(1, SPRING_LINK_GROUP_PLUS_ANCHORS);
			scene->setSpringRules(2, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(3, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(4, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 7: {
			scene->groupImage = RESOURCES "internalChannels0.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 8;
			scene->setSpringRules(0, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(1, SPRING_GROUPED);
			scene->setSpringRules(2, SPRING_CENTERED_AND_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 8: {
			scene->groupImage = RESOURCES "blobs2.png";
			scene->backgroundColorImage = RESOURCES "background0.png";
			scene->particleOften = 8;
			scene->setSpringRules(0, SPRING_FREE);
			scene->setSpringRules(1, SPRING_LINK_GROUP_PLUS_ANCHORS);
			scene->setSpringRules(2, SPRING_LINK_GROUP_PLUS_ANCHORS);
			scene->setSpringRules(3, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(4, SPRING_LINK_GROUP_PLUS_ANCHORS);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 9: {
			scene->groupImage = RESOURCES "stringy.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 6;
			scene->setSpringRules(0, SPRING_GROUPED);
			scene->setSpringRules(1, SPRING_GROUPED);
			scene->setSpringRules(2, SPRING_GROUPED);
			scene->setSpringRules(3, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 10: {
			scene->groupImage = RESOURCES "wheels0.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 6;
			scene->setSpringRules(0, SPRING_GROUPED);
			scene->setSpringRules(1, SPRING_GROUPED);
			scene->setSpringRules(2, SPRING_GROUPED);
			scene->setSpringRules(3, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
		case 11: {
			scene->groupImage = RESOURCES "network.png";
			scene->backgroundColorImage = RESOURCES "background1.png";
			scene->particleOften = 4;
			scene->setSpringRules(0, SPRING_CENTERED_AND_GROUPED);
			scene->setSpringRules(1, SPRING_LINK_GROUP_PLUS_ANCHORS);
			scene->setSpringRules(2, SPRING_GROUPED);
			scene->setSpringRules(3, SPRING_GROUPED);
			
			scene->songWanderers = 2;
			scene->songFrequency = 220;
		} break;
	}
	
	switch (gpick) {
		case 0: {
			scene->processorSetup = PROCESS_FADE;
			scene->song = SONG_WANDER;
		} break;
		case 1: {
			int gpick2 = random() % 3;
			fprintf(stderr, "GFX pick2 %d\n", gpick2);
			if (gpick2 == 0) scene->processorSetup = PROCESS_NONE;
			if (gpick2 == 1) scene->processorSetup = PROCESS_BLEED;
			if (gpick2 == 2) scene->processorSetup = PROCESS_BLEEDTHENFADE;
			scene->song = SONG_BUZZER;
		} break;
		case 2: {
			scene->processorSetup = PROCESS_LIGHTING;
			scene->song = SONG_BUZZER;
		} break;
	}
}

void teardownScene() {
	delete scene;
	scene = NULL;
}

void setupBoards() 
{
	state = makeBoard();
	stateScratch = makeBoard();
	colorLookup = makeBoard();
	sf::Image backgroundColorImage;
	// Andi, I'm sorry I put this here!
	// There was an order of operations problem and I couldn't figure out a way to put this in setupGraphicsObjects()
	bool useBackgroundColorImage = scene->backgroundColorImage.size();
	if (!useBackgroundColorImage || backgroundColorImage.loadFromFile(scene->backgroundColorImage))
		std::cout << "images loaded\n";
	else
		std::cout << "ERROR: image file not loaded correctly\n";
	if (useBackgroundColorImage)
	{
		sf::Color sampleColor;
		for (uint iY = 0; iY < backgroundColorImage.getSize().y; iY ++)
		{
			for (uint iX = 0; iX < backgroundColorImage.getSize().x; iX ++)
			{
				sampleColor = backgroundColorImage.getPixel(iX, iY);
				Pixel p(sampleColor.r / 255.0, sampleColor.g / 255.0, sampleColor.b / 255.0);
				colorLookup[iX][iY] = p;
			}
		}
	}
}

void teardownBoards() {
	destroyBoard(state);
	destroyBoard(stateScratch);
	destroyBoard(colorLookup);
}
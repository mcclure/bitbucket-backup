#include "graphicsObjects.h"
#include "scene.h"

sf::Uint8* pixel;
sf::Sprite sprite;
// These two are causing exceptions when they're global! Making them into pointers seems to fix it. 
// Hmm. Why doesn't it work to have them be objects?
sf::Texture* texture;
sf::RenderWindow* window;

bool fullscreen;
uint zoom;
uint renderWidth;
uint renderHeight;
uint worldDrawSize;

int worldWidth, worldHeight;
std::vector<Particle*> particle;
std::vector<Emitter*> emitter;
// for partitioning
float binSize;
int binsAcross, binsTall;
Bin** bin; // grid of square bins for partitioning

#define DEFAULT_COLORS 3
Pixel defaultColor[DEFAULT_COLORS] = { Pixel(191.0 / 255.0, 62.0 / 255.0, 100.0 / 255.0), 
									   Pixel(255.0 / 255.0, 0.0 / 255.0, 64.0 / 255.0),
									   Pixel(64.0 / 255.0, 92.0 / 255.0, 255.0 / 255.0) };

void setupGraphicsObjects() {
	switch(scene->processorSetup) {
		case PROCESS_NONE: currentProcessor = new Processor(); break;
		case PROCESS_FADE: currentProcessor = new FadeProcessor(); break;
		case PROCESS_BLEED: currentProcessor = new BleedProcessor(); break;
		case PROCESS_BLEEDTHENFADE: currentProcessor = new BleedProcessor( new FadeProcessor() ); break;
		case PROCESS_LIGHTING: currentProcessor = new LightingProcessor( new BlurProcessor() ); break;
	}

	// Initialize draw variables
	fullscreen = false; // ADD: restore settings from file
	worldDrawSize = 144;
	// * 4 because pixels have 4 components (RGBA)
	pixel = new sf::Uint8[renderWidth * renderHeight * 4]; 
	for (uint iY = 0; iY < renderHeight; iY ++) 
	{
		for (uint iX = 0; iX < renderWidth; iX ++) 
		{
			int idx = (iX + renderWidth * iY) * 4;
			pixel[idx] = 0;
			pixel[idx + 1] = 8;
			pixel[idx + 2] = 8;
			pixel[idx + 3] = 0xFF;
		}
	}
	texture = new sf::Texture();
	if (!texture->create(renderWidth, renderHeight)) {
		std::cout << "Error creating texture\n";
	}
	sprite.setTexture(*texture);

	worldWidth = worldHeight = 144;

	// loading test
	// probably contain this somehow
	sf::Image testImage, colorImage, backgroundColorImage;
	bool useColorImage = scene->colorImage.size();
	bool useBackgroundColorImage = scene->backgroundColorImage.size();
	if (testImage.loadFromFile(scene->groupImage) && (!useColorImage || colorImage.loadFromFile(scene->colorImage)))
	{
		std::cout << "images loaded\n";
	}
	else
		std::cout << "ERROR: image file not loaded correctly\n";

	Particle* newParticle;
	/*
	for (uint i = 0; i < 300; i ++)
	{
		newParticle = new Particle(randomfloat(0, worldWidth), randomfloat(0, worldHeight), random() % 3);
		particle.push_back(newParticle);
	}
	*/
	// put the background color image into our buffer
	sf::Color sampleColor;
	for (uint iY = 0; iY < backgroundColorImage.getSize().y; iY ++)
	{
		for (uint iX = 0; iX < backgroundColorImage.getSize().x; iX ++)
		{
			sampleColor = backgroundColorImage.getPixel(iX, iY);
			Pixel p(sampleColor.r, sampleColor.g, sampleColor.b);
			colorLookup[iX][iY] = p;
		}
	}

	// set up partitions
	binSize = 4;
	binsAcross = worldWidth / binSize;
	binsTall = worldHeight / binSize;
	bin = new Bin*[binsAcross];
	for (uint iX = 0; iX < binsAcross; iX ++)
	{
		bin[iX] = new Bin[binsTall];
	}
	for (uint iY = 0; iY < binsTall; iY ++)
	{
		for (uint iX = 0; iX < binsAcross; iX ++)
		{
			float left = iX * binSize;
			float top = iY * binSize;
			bin[iX][iY].setSize(left, top, binSize, binSize);
		}
	}

	// inspect all the bins to see if they're intact
	if (0)
	{
		for (uint iY = 0; iY < binsTall; iY ++)
		{
			for (uint iX = 0; iX < binsAcross; iX ++)
			{
				Bin* currentBin = &bin[iX][iY];
				cout << "Bin " << iX << ", " << iY << ": left = " << currentBin->left << ", top = " << currentBin->top << ", right = " << currentBin->right << ", bottom = " << currentBin->bottom << "\n";
			}
		}
	}

	// create particles based on the group image
	int group;
	if (scene->particleOften) {
		for (uint iY = 0; iY < worldHeight; iY ++)
		{
			for (uint iX = 0; iX < worldWidth; iX ++)
			{
				group = -1;
				sampleColor = testImage.getPixel(iX, iY);
				float r; float g; float b;
				r = sampleColor.r; g = sampleColor.g; b = sampleColor.b;
				
				// materials
				if ((r == 255) &&	(g == 255) && (b == 0))
					group = 0;
				else if ((r == 255) && (g == 127) && (b == 0))
					group = 1;
				else if ((r == 255) && (g == 0) && (b == 0))
					group = 2;
				else if ((r == 255) && (g == 0) && (b == 255))
					group = 3;
				else if ((r == 127) && (g == 0) && (b == 255))
					group = 4;
				else if ((r == 0) && (g == 0) && (b == 255))
					group = 5;

				// emitters
				// these emit, respectively, group 0, group 1 and group 2
				else if ((r == 255) && (g == 255) && (b == 127))
					emitter.push_back(new Emitter(iX + 0.5, iY + 0.5, 0));
				else if ((r == 255) && (g == 191) && (b == 127))
					emitter.push_back(new Emitter(iX + 0.5, iY + 0.5, 1));
				else if ((r == 255) && (g == 127) && (b == 127))
					emitter.push_back(new Emitter(iX + 0.5, iY + 0.5, 2));

				if ((group != -1)
				&& (random() % scene->particleOften == 0))
				{
					Pixel p;
					if (useColorImage) {
						sf::Color colorColor = colorImage.getPixel(iX, iY);
						p = Pixel(colorColor.r, colorColor.g, colorColor.b);
					} else {
						p = defaultColor[group % DEFAULT_COLORS];
					}
					
					if (scene->colorVary)
						p = p.randomNear();
				
					newParticle = new Particle(iX + 0.5, iY + 0.5, group, p);
					// look up the following in the Scene object:
					newParticle->applyStartPositionSpring = scene->springRule(group) & SPRING_CENTERED;
					newParticle->applySpringAttachmentForces = scene->springRule(group) & SPRING_GROUPED;
					newParticle->applySpringAttachToAnchored = scene->springRule(group) & SPRING_AUTOLINK_ANCHORS;
					particle.push_back(newParticle);
				}
			}
		}
	}

	// after we've created all particles, run through them and see which need spring connections to one another
	connectSprings();
}

void teardownGraphicsObjects() {
	FOREACH( Particle *p, particle )
		delete p;
	}
	particle.clear();
	
	FOREACH( Emitter *e, emitter )
		delete e;
	}
	emitter.clear();
}

void connectSprings()
{
	// go through all particles
	// do an against all check for each
	// connect those in certain groups to their neighbors with springs
	Particle* currentParticle;
	Particle* targetParticle;
	float distance, distanceX, distanceY;
	float springDistanceThreshold = 8.0;
	int springCount;
	for (uint i = 0; i < particle.size(); i ++)
	{
		currentParticle = particle[i];
		springCount = 0;
		currentParticle->springAttachment.clear(); // wipe current connections
		for (uint i2 = 0; i2 < particle.size(); i2 ++)
		{
			targetParticle = particle[i2];
			if ((i2 != i)
			//&& (targetParticle->group == 0)
			&& ((targetParticle->group == currentParticle->group)
			|| (currentParticle->applySpringAttachToAnchored && targetParticle->applyStartPositionSpring)))
			{
				distanceX = particle[i2]->x - currentParticle->x;
				distanceY = particle[i2]->y - currentParticle->y;
				distance = sqrt(pow(distanceX, 2) + pow(distanceY, 2));
				if (distance < springDistanceThreshold)
				{
					// attach these two particles with a spring
					oneWaySpringLink(currentParticle, targetParticle);
					oneWaySpringLink(targetParticle, currentParticle);
					springCount ++;
				}
			}
		}
	}
}


void oneWaySpringLink(Particle* particle1, Particle* particle2)
{
	bool alreadyAttached = false;
	for (uint i = 0; i < particle1->springAttachment.size(); i ++)
	{
		if (particle1->springAttachment[i] == particle2)
			alreadyAttached = true;
	}
	if (!alreadyAttached)
		particle1->springAttachment.push_back(particle2);
}


// makes a particle at the specified location. This adds it to the manager.
// if we want to set additional parameters (like color) we can optionally use the pointer this returns.
Particle* spawnParticle(float x, float y, int group)
{
	Pixel p;
	p = defaultColor[group % DEFAULT_COLORS];
	Particle* newParticle = new Particle(x, y, group, p);
	newParticle->applyStartPositionSpring = scene->springRule(group) & SPRING_CENTERED;
	newParticle->applySpringAttachmentForces = scene->springRule(group) & SPRING_GROUPED;
	newParticle->applySpringAttachToAnchored = scene->springRule(group) & SPRING_AUTOLINK_ANCHORS;
	particle.push_back(newParticle);
	return newParticle;
}


void setupWindow(bool fullScreen) {
	renderWidth = 320;
	renderHeight = 180;
	zoom = 4;
	window = new sf::RenderWindow();

	if (fullScreen)
	{
		// get a list of all available fullscreen modes
		std::vector<sf::VideoMode> mode = sf::VideoMode::getFullscreenModes();
		// list modes
		for (uint i = 0; i < mode.size(); i ++)
		{
			float aspectRatio = ((float) mode[i].width) / ((float) mode[i].height);
			std::cout << "Mode " << i << ": " << mode[i].width << "x" << mode[i].height << "(" << aspectRatio << ")\n";
		}
		// ADD: try to find one with the correct aspect ratio
		sf::VideoMode selectedMode = mode[0];
		float multiplier = selectedMode.width / renderWidth;
		window->create(mode[0], "The Witch's Hut", sf::Style::Fullscreen);
		sprite.setScale(multiplier, multiplier);
		//window.create(sf::VideoMode(320, 240, 32), "Space Exploration Game", sf::Style::Fullscreen);
	}
	else
	{
		window->create(sf::VideoMode(zoom * renderWidth, zoom * renderHeight, 32), "SFML window");
		sprite.setScale(zoom, zoom);
	}
}


// cleans up all bins
// it would probably be faster to remove pointers individually as particles exit bins, 
// but that's WAY more likely to break
// do later for OCD points
void resetBins()
{
	for (uint iY = 0; iY < binsTall; iY ++)
	{
		for (uint iX = 0; iX < binsAcross; iX ++)
		{
			bin[iX][iY].memberParticleCount = 0;
			// reset the vector of pointers to particles
			bin[iX][iY].memberParticle.clear();
		}
	}	
}


// figure out which bin the particle should be in, and file it
void sortIntoBin(Particle* inputParticle)
{
	inputParticle->binX = -1; 
	inputParticle->binY = -1;
	// Note on bounds safety:
	// there are no bins outside of the world. Hmm.
	// for now if something leaves the world, it exits the system and doesn't get filed
	// this means we don't let it interact until it comes back into the grid
	// (the boundary forces should push it back in)
	if ((inputParticle->x >= 0) && (inputParticle->x < worldWidth)
	&& (inputParticle->y >= 0) && (inputParticle->y < worldHeight))
	{ // okay, we're in bounds. file the particle
		inputParticle->binX = inputParticle->x / binSize;
		inputParticle->binY = inputParticle->y / binSize;
		bin[inputParticle->binX][inputParticle->binY].memberParticle.push_back(inputParticle);
		bin[inputParticle->binX][inputParticle->binY].memberParticleCount ++;
	}
}
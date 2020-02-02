#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

#include <SFML/Graphics.hpp>
#include <time.h>
#include "audioObjects.h"
#include "graphicsObjects.h"
#include "globals.h"
#include "postprocess.h"
#include "scene.h"

extern std::vector<Particle*> particle;
extern std::vector<Emitter*> emitter;
extern Bin** bin;
extern int binsAcross, binsTall;
extern float binSize;
extern GroupTuning* groupTuning;
bool reloadLevel = true;

string mac_resources_path;

int main()
{
	sf::Clock clock;
	srandom(time(NULL)); for(int i = 0; i < 10; i++) random();
	setupWindow(false); // load from disk?

#ifdef __APPLE__
	{
		// I cannot BELIEVE SFML does not handle this for us.

		// Interrogate executable path from bundle
		CFBundleRef mainBundle = CFBundleGetMainBundle();

		CFStringRef exePath = CFURLCopyFileSystemPath( CFBundleCopyExecutableURL(mainBundle), kCFURLPOSIXPathStyle);

		CFStringEncoding encodingMethod = CFStringGetSystemEncoding();

		// Convert CFStringRef into C string
		mac_resources_path = CFStringGetCStringPtr(exePath, encodingMethod);
		
		// Slice off last four path elements
		for(int c = 0; c < 4; c++) {
			mac_resources_path.resize( mac_resources_path.rfind("/") );
		}
	}
#endif

	{ // TITLE SCREEN
		sf::Sprite sprite2;
		sf::Texture texture2;
		if (texture2.loadFromFile(RESOURCES "title.png"))
		{
			sprite2.setTexture(texture2);
			sprite2.setScale(zoom, zoom);
			window->draw(sprite2); // draw the texture into the actual window
			window->display();
		}
		sf::Event event;
		while (1) {
			window->pollEvent(event);
			if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) {
                window->close();
				break;
			}
			if (event.type == sf::Event::KeyPressed || event.type == sf::Event::MouseButtonPressed)
				break;
		}
	}

	window->clear();

	// set up group properties
	groupTuning = new GroupTuning[10];
	for (uint i = 0; i < 10; i ++)
	{
		groupTuning[i] = GroupTuning();
	}

	// for giving the particles a little motion, so they aren't completely static
	int perturbParticleCount = 32;
	int* perturbIndex;
	perturbIndex = new int[perturbParticleCount];
	int* perturbTimer;
	perturbTimer = new int[perturbParticleCount];
	float* perturbAccelerationX;
	perturbAccelerationX = new float[perturbParticleCount];
	float* perturbAccelerationY;
	perturbAccelerationY = new float[perturbParticleCount];

    while (window->isOpen())
    {
		if (reloadLevel) {
			setupScene();
			setupGraphicsObjects();
			setupBoards();
			setupAudio();
			reloadLevel = false;
		}
	
		// get elapsed time
		sf::Time elapsedTime = clock.getElapsedTime();
		
        sf::Event event;
        while (window->pollEvent(event))
        {
			// get elapsed time
			sf::Time elapsedTime = clock.getElapsedTime();
		
			if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
                window->close();
			if (event.type == sf::Event::KeyPressed)
			{
				// update list of key states
				//keyState[event.key.code] = true;

				switch(event.key.code) {
					case sf::Keyboard::Left:   debug_audioparam_ptr(-1); break;
					case sf::Keyboard::Right:  debug_audioparam_ptr(1); break;
					case sf::Keyboard::Up:     debug_audioparam_incr(1); break;
					case sf::Keyboard::Down:   debug_audioparam_incr(-1); break;
					case sf::Keyboard::Space:  reloadLevel = true; break;
				
					// for testing emitters
					case sf::Keyboard::Z: 
						FOREACH (Emitter* currentEmitter, emitter)
							currentEmitter->trigger(0);
						}
						break;
					case sf::Keyboard::X: 
						FOREACH (Emitter* currentEmitter, emitter)
							currentEmitter->trigger(1);
						}
						break;
					case sf::Keyboard::C: 
						FOREACH (Emitter* currentEmitter, emitter)
							currentEmitter->trigger(2);
						}
						break;
				}

				//uiManager->keyPress(event.key.code);
				if (event.key.code == sf::Keyboard::T)
				{
					/*
					if (!testUIVisible)
					{
						uiManager->addEntity(new TestUI());
						testUIVisible = true;
					}
					*/
				}
			}
			if (event.type == sf::Event::MouseButtonPressed) // On each click, zxc 5 times
			{
				FOREACH (Emitter* currentEmitter, emitter)
					for(int c = 0; c < 5; c++) {
						currentEmitter->trigger(0);
						currentEmitter->trigger(1);
						currentEmitter->trigger(2);
					}
				}
			}
        }


		// TIMING
		// right now it's fixed frame rate- but we could use a variable delta instead
		if (elapsedTime.asSeconds() >= (1.0 / 60.0))
		{
			clock.restart();

			updateAudio(); // Call however often we want but every frame is a good time.		
//			window->clear(); // if we're redrawing the entire window every frame, can we remove this?

			// update here
			//uiManager->update(); // the world and space interfaces are updated here
			//uiManager->cleanUp(); // remove marked interfaces from the manager
		
			uint offsetX = renderWidth / 2 - worldDrawSize / 2;
			uint offsetY = renderHeight / 2 - worldDrawSize / 2;
		
			// mouse input
			sf::Vector2i mousePosition = sf::Mouse::getPosition(*window);
			float mouseX = mousePosition.x / zoom - offsetX;
			float mouseY = mousePosition.y / zoom - offsetY;
			bool applyMouseInput = sf::Mouse::isButtonPressed(sf::Mouse::Left);

			// perturb things gently, so they never quite hold still
			for (int i = 0; i < perturbParticleCount; i ++)
			{
				if (perturbTimer[i] > 0)
					perturbTimer[i] --;
				if (perturbTimer[i] <= 0)
				{
					perturbTimer[i] = 240 + random() % 120;
					perturbIndex[i] = random() % particle.size();
					float strength = 0.01;
					perturbAccelerationX[i] = randomfloat(-strength, strength);
					perturbAccelerationY[i] = randomfloat(-strength, strength);
				}
				if (perturbIndex[i] < particle.size())
				{
					Particle* target = particle[perturbIndex[i]];
					target->velocityX += perturbAccelerationX[i];
					target->velocityY += perturbAccelerationY[i];
				}
			}

			
			// affect group properties with key input here
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
			{
				groupTuning[0].sameGroupRepulsionDistance = 5.5;
				groupTuning[0].sameGroupRepulsionAmount = 0.002;
			}
			else
			{
				groupTuning[0].sameGroupRepulsionDistance = 4.0;
				groupTuning[0].sameGroupRepulsionAmount = 0.002;
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
			{
				groupTuning[1].sameGroupRepulsionDistance = 2.5;
				groupTuning[1].sameGroupRepulsionAmount = 0.002;
			}
			else
			{
				groupTuning[1].sameGroupRepulsionDistance = 5.0;
				groupTuning[1].sameGroupRepulsionAmount = 0.002;
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
			{
				groupTuning[2].sameGroupRepulsionDistance = 2.5;
				groupTuning[2].sameGroupRepulsionAmount = 0.002;
			}
			else
			{
				groupTuning[2].sameGroupRepulsionDistance = 4.0;
				groupTuning[2].sameGroupRepulsionAmount = 0.002;
			}

			// update particles
			float attraction, repulsion;
			// for per-group differences in interaction, there are two possible values for some of these
			// for instance, a particle will be attracted to its own group 0.01 and to the other group 0.0
			float attractionAmount, repulsionDistance, repulsionAmount;
			/*
			float sameGroupRepulsionDistance = 4;
			float otherGroupRepulsionDistance = 5;
			float sameGroupRepulsionAmount = 0.002;
			float otherGroupRepulsionAmount = 0.001;
			float attractionDistance = 20.0;
			float sameGroupAttractionAmount = 0.01;
			float otherGroupAttractionAmount = 0.0;
			*/
			float sameGroupRepulsionDistance = groupTuning[0].sameGroupRepulsionDistance;
			float otherGroupRepulsionDistance = groupTuning[0].otherGroupRepulsionDistance;
			float sameGroupRepulsionAmount = groupTuning[0].sameGroupRepulsionAmount;
			float otherGroupRepulsionAmount = groupTuning[0].otherGroupRepulsionAmount;
			float attractionDistance = groupTuning[0].attractionDistance;
			float sameGroupAttractionAmount = groupTuning[0].sameGroupAttractionAmount;
			float otherGroupAttractionAmount = groupTuning[0].otherGroupAttractionAmount;
			
			//if (scene->alwaysRunAttraction)
				//otherGroupAttractionAmount = 0.002;
			// for behavior at edges of world
			float margin = 0;
			float leftEdge = -margin;
			float rightEdge = worldWidth + margin;
			float topEdge = -margin;
			float bottomEdge = worldHeight + margin;
			float edgePushAmount = 0.001;
			// for spatial partitioning
			// these are used when each particle decides in which bins it needs to check for neighbors
			int leftBin, topBin, rightBin, bottomBin;
			int binRadius = ceil(attractionDistance / binSize); // the maximum distance a particle will need to look is the attraction distance
			int currentGroup;

			// main loop, once per particle
			// NOTE: all the velocities are affected here by attraction and repulsion, but the particles don't actually move yet.
			// this avoids weird order-dependent asymmetries
			FOREACH (Particle* currentParticle, particle)
				currentGroup = currentParticle->group;
				// attractive / repulsive forces
				
				// partitioning
				// these are the bins which this particle needs to look for neighbors in
				leftBin = max(0, min(binsAcross - 1, currentParticle->binX - binRadius));
				rightBin = max(0, min(binsAcross - 1, currentParticle->binX + binRadius));
				topBin = max(0, min(binsTall - 1, currentParticle->binY - binRadius));
				bottomBin = max(0, min(binsTall - 1, currentParticle->binY + binRadius));
	
				// OLD SLOW CHECK FROM BEFORE PARTIONING
				// CHECKS AGAINST ALL PARTICLES INSTEAD OF USING BINS
				// replace with iterating over only those particles which are nearby
				/*
				for (uint i2 = 0; i2 < particle.size(); i2 ++)
				{
					Particle* targetParticle = particle[i2];
				*/

				// inner loop- once per neighbor of the current particle
				// go through all the relevant bins and do repulsion and attraction on neighbors
				for (uint binY = topBin; binY <= bottomBin; binY ++)
				{
				for (uint binX = leftBin; binX <= rightBin; binX ++)
				{
				Bin* currentBin = &bin[binX][binY];
				for (uint i2 = 0; i2 < currentBin->memberParticle.size(); i2 ++) 
				{
					Particle* targetParticle = currentBin->memberParticle[i2];
					if ((currentParticle == targetParticle) == false) // add check to skip self - self checks
					{
						float distanceX = targetParticle->x - currentParticle->x;
						float distanceY = targetParticle->y - currentParticle->y;
						float distance = sqrt(square(distanceX) + square(distanceY));

						// per-group rules check
						// switch between the two tuning sets
						if (currentParticle->group == targetParticle->group)
						{
							repulsionAmount = groupTuning[currentGroup].sameGroupRepulsionAmount;
							repulsionDistance = groupTuning[currentGroup].sameGroupRepulsionDistance;
							attractionAmount = groupTuning[currentGroup].sameGroupAttractionAmount;
						}
						else
						{
							repulsionAmount = groupTuning[currentGroup].otherGroupRepulsionAmount;
							repulsionDistance = groupTuning[currentGroup].otherGroupRepulsionDistance;
							attractionAmount = groupTuning[currentGroup].otherGroupAttractionAmount;
						}
					
						// attraction
						if ((groupTuning[currentGroup].applySpringAttachmentForces == false)
						&& (currentParticle->applyStartPositionSpring == false)) // possibly turn off attraction entirely for some situations
						{
							if (distance < attractionDistance)
							{
								//attraction = attractionAmount * (1.0 - distance / attractionDistance); // linear, falls off to zero completely at attractionDistance
								attraction = attractionAmount / distance;
								currentParticle->velocityX += attraction * distanceX / distance;
								currentParticle->velocityY += attraction * distanceY / distance;	
							}
						}

						// repulsion
						if (distance < repulsionDistance)
						{
							repulsion = -0.5 * (1.0 - distance / repulsionDistance);
							currentParticle->velocityX += repulsion * distanceX / distance;
							currentParticle->velocityY += repulsion * distanceY / distance;
						}
					}
				}
				} // x bin loop
				} // y bin loop

				// start position spring
				// a simple way to do fixed structures
				//if (false)
				if (currentParticle->applyStartPositionSpring)
				{
					float distanceX = currentParticle->startX - currentParticle->x; // Shadow
					float distanceY = currentParticle->startY - currentParticle->y;
					float distance = sqrt(square(distanceX) + square(distanceY));
					if (distance > 0.01)
					{
						attraction = 0.03; //0.0002;
						currentParticle->velocityX += attraction * distanceX / distance;
						currentParticle->velocityY += attraction * distanceY / distance;
					}
				}
					
				// permanent spring connections
				// pull toward all the particles we're spring-linked to
				if ((groupTuning[currentGroup].applySpringAttachmentForces)
				&& (currentParticle->springAttachment.size() > 0))
				{
					attraction = 0.006;
					for (int i3 = 0; i3 < currentParticle->springAttachment.size(); i3 ++)
					{
						Particle *springParticle = currentParticle->springAttachment[i3];
						float distanceX = springParticle->x - currentParticle->x;
						float distanceY = springParticle->y - currentParticle->y;
						// Note: I fixed that loop depth typo, now these happen exponentially less
						float distancesq = square(distanceX) + square(distanceY);
						if (distancesq > square(sameGroupRepulsionDistance))
						{
							currentParticle->velocityX += attraction * distanceX;
							currentParticle->velocityY += attraction * distanceY;
						}
					}
				}

				// mouse interaction
				// this isn't very good
				// NOTE: selectively repelling some groups could be fun
				if (applyMouseInput)
				{
					float distanceX = mouseX - currentParticle->x;
					float distanceY = mouseY - currentParticle->y;
					float distance = sqrt(square(distanceX) + square(distanceY));
					if ((distance > 0.01) && (distance < 32))
					{
						attraction = 0.05;
						if (currentParticle->applyStartPositionSpring == true)
							attraction *= 0.5;
						currentParticle->velocityX += attraction * distanceX / distance;
						currentParticle->velocityY += attraction * distanceY / distance;
					}
				}

				// friction
				currentParticle->velocityX *= 0.99;
				currentParticle->velocityY *= 0.99;
			}

			// NOTE: now that all the attraction and repulsion has been calculated,
			// we can move the particles (all at once)
			resetBins(); // reset the spatial partitioning 
			FOREACH (Particle* currentParticle, particle)
				currentParticle->x += currentParticle->velocityX;
				currentParticle->y += currentParticle->velocityY;

				// edge behavior
				if (currentParticle->x < leftEdge)
				{
					currentParticle->x = leftEdge;
					currentParticle->velocityX = 0;
					//currentParticle->velocityX += edgePushAmount;
				}
				else if (currentParticle->x > rightEdge)
				{
					currentParticle->x = rightEdge;
					currentParticle->velocityX = 0;
					//currentParticle->velocityX -= edgePushAmount;
				}
				if (currentParticle->y < topEdge)
				{
					currentParticle->y = topEdge;
					currentParticle->velocityY = 0;
					//currentParticle->velocityY += edgePushAmount;
				}
				else if (currentParticle->y > bottomEdge)
				{
					currentParticle->y = bottomEdge;
					currentParticle->velocityY = 0;
					//currentParticle->velocityY -= edgePushAmount;
				}

				// now file all the the particles into new bins
				// now that we've moved everything
				sortIntoBin(currentParticle);
			}
			

			// update emitters
			FOREACH (Emitter* currentEmitter, emitter)
				currentEmitter->update();
			}

			// draw things here
			//uiManager->draw();
			currentProcessor->preBuffer();
			FOREACH (Particle* currentParticle, particle)
				for (uint i2 = 0; i2 < currentParticle->springAttachment.size(); i2 ++)
					currentProcessor->drawLine(currentParticle->x, currentParticle->y, currentParticle->springAttachment[i2]->x, currentParticle->springAttachment[i2]->y, currentParticle->p);
			}
			FOREACH (Particle* currentParticle, particle)
				currentProcessor->drawDot(currentParticle->x, currentParticle->y, currentParticle->p);
			}
			// draw calls for testing partitioning
			if (0)
			{
				Pixel p(1.0, 0.5, 0.0);
				for (uint iY = 0; iY < binsTall; iY ++)
				{
					for (uint iX = 0; iX < binsAcross; iX ++)
					{
						Bin* currentBin = &bin[iX][iY];
						currentProcessor->drawLine(currentBin->left, currentBin->top, currentBin->right, currentBin->top, p);
						currentProcessor->drawLine(currentBin->left, currentBin->top, currentBin->left, currentBin->bottom, p);
						if (currentBin->memberParticle.size() > 0)
							currentProcessor->drawDot(currentBin->left + 2, currentBin->top + 2, p);
					}
				}
			}
			currentProcessor->postBuffer();

			uint pixelIndex;
			uint r, g, b;
			for (uint iY = 0; iY < worldDrawSize; iY ++) 
			{
				for (uint iX = 0; iX < worldDrawSize; iX ++) 
				{
					r = (int) (state[iX][iY].color[RED] * 255.0);
					g = (int) (state[iX][iY].color[GREEN] * 255.0);
					b = (int) (state[iX][iY].color[BLUE] * 255.0);
					pixelIndex = 4 * ((offsetX + iX) + renderWidth * (offsetY + iY));
					pixel[pixelIndex] = r;
					pixel[pixelIndex + 1] = g;
					pixel[pixelIndex + 2] = b;
				}
			}
			// show render time
			/*
			if (showFrameRate)
			{
				// the first line is the time for the current frame
				// the second line is the average time (over the last 60 frames)
				// .0166666 repeating is 60 frames per second
				float average = 0;
				frameTime[frameTimeIndex] = elapsedTime.asSeconds();
				frameTimeIndex = (frameTimeIndex + 1) % 60;
				for (uint i = 0; i < 60; i ++)
				{
					average += frameTime[i];
				}
				average /= 60;
				std::stringstream stringStream;
				stringStream << elapsedTime.asSeconds() << "\n" << average << std::endl;
				FontManager::drawString(128, 128, pixel, 0, stringStream.str());
			}
			*/

			texture->update(pixel); // put the contents of the raw pixel data into the texture
			window->draw(sprite); // draw the texture into the actual window

			window->display();
		}
		
		if (reloadLevel) {
			teardownAudio();
			teardownBoards();
			teardownGraphicsObjects();
			teardownScene();
		}
    }

    return 0;
}
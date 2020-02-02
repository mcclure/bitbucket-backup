
#include <glfw.h>
#include <sstream>

#include "Game.h"

using namespace ld;

namespace SFX { enum { Train, Bip, Money, Siren, Attention, Jingle, Timeout, Count }; }

// Camera scaling (resolution independent)
const float CameraScale = windowSize / 2.f * 1.0f / 144.f;

// Minimum zoom
const float MinimumZoom = 0.4f;

// Maximum zoom
const float MaximumZoom = 1.f;

// Camera zoom-in and out speed
const float CameraZoomSpeed = 2.f;

// Maximum time between packages.
const float CountdownTotal = 60.f;

// Arrested time limit.
const float ArrestedLimit = 0.2f;

// End scene's duration.
const float EndDuration = 1.f;

//---------------------------------------------------------------------
Game::Game() : ambianceMusic("ambiance.ogg"), backgroundMusic("bgm.ogg")
{
	// initialize
	receivingInput = false;

	// set window title
	glfwSetWindowTitle("Rive Gauche | Left button: Set next stop; Right button: Zoom-out; F2: Restart | by @lucasncv for LD25");

	// set background color
	glClearColor(0.f, 0.17f, 0.21f, 1.f);

	// fonts
#ifdef __APPLE__
#define VERDANA "Verdana.ttf"
#else
#define VERDANA "verdana.ttf"
#endif
	bigFont.loadFile(VERDANA, windowSize * 2 / 100);
	smallFont.loadFile(VERDANA, windowSize / 60);

	// sounds
	sfx = allocate_unique<Sound>(SFX::Count);
	sfx[SFX::Train].loadFile("train.ogg");
	sfx[SFX::Bip].loadFile("bip.ogg");
	sfx[SFX::Money].loadFile("money.ogg");
	sfx[SFX::Siren].loadFile("siren.ogg");
	sfx[SFX::Attention].loadFile("attention.ogg");
	sfx[SFX::Jingle].loadFile("jingle.ogg");
	sfx[SFX::Timeout].loadFile("timeout.ogg");

	// effects
	moneyPackage.variation = Pi;
	moneyPackage.duration = 1.f;
	moneyPackage.velocity = 30.f;
	moneyPackage.interval = 0.02f;
	moneyPackage.size = 1.5f;
	moneyPackage.color = Yellow;

	// begin
	restart();
}

//---------------------------------------------------------------------
Game::~Game()
{
	// delete voyageurs
	for (auto e : voyageurs) delete e;
}

//---------------------------------------------------------------------
void Game::restart()
{
	// delete voyageurs
	for (auto e : voyageurs) delete e;
	voyageurs.clear();

	// create player
	voyageurs.push_back(player = new Player());
	player->placeAt(leMetro.getStation(L"La Motte-Picquet - Grenelle"));

	// spawn money
	moneyPackage.position = leMetro.getRandomStation()->position;
	countdown = CountdownTotal;

	// score
	score = 0U;
	bonusScore = 0.f;

	// prepare
	prepareWave();

	// camera
	camera = player->getPosition();
	cameraZoom = 1.f;

	// reset
	effectTimer = 0.f;
	stationNameTimer = 0.f;
	lastStation = nullptr;
	arrested = 0.f;
}

//---------------------------------------------------------------------
bool Game::finished() const
{
	return arrested >= ArrestedLimit;
}

//---------------------------------------------------------------------
void Game::update()
{
	// quit game if esc is pressed
	if (key::isDown(GLFW_KEY_ESC))
	{
		quit();
		return;
	}

	// restart game
	if (key::isDown(GLFW_KEY_F2))
	{
		restart();
		return;
	}

	// skip frames
	if (key::isDown(GLFW_KEY_LALT))
		return;
	
	// game is running
	if (!finished())
	{
		// handle input
		if (!receivingInput && mouse::isDown(0))
		{
			Vec2 cursor = mouse::cursor();

			cursor.x -= windowSize / 2.f;
			cursor.y = windowSize / 2.f - cursor.y;

			cursor /= CameraScale * cameraZoom;
			cursor += camera;

			// store old direction
			Direction direction = player->getDirection();
			player->stop();

			// for each connection
			for (auto &stretch : player->getStation()->stretches)
			{
				bool forward = stretch->a == player->getStation();

				// find adjacent station
				Station *target = forward ? stretch->b : stretch->a;

				// clicked?
				if ((target->position - cursor).lengthSquared() <= 64.f)
				{
					player->move(Direction(stretch->ligne, forward));
					break;
				}
			}

			// sfx
			if (direction != player->getDirection())
				sfx[SFX::Bip].play();
		}

		// changed station
		if (lastStation != player->getStation())
		{
			// update
			lastStation = player->getStation();

			// show name
			stationNameTimer = 1.f;

			// sfx
			sfx[SFX::Train].play();
		}

		// check station
		if (!player->isMoving())
		{
			// get package
			if ((player->getPosition() - moneyPackage.position).lengthSquared() <= 1.f)
			{
				// score
				++score;
				bonusScore += countdown;

				// respawn money
				moneyPackage.position = leMetro.getRandomStation()->position;
				countdown = CountdownTotal;

				// sfx
				sfx[SFX::Money].play();

				// stop
				player->stop();

				// prepare
				prepareWave();
			}

			// cops around
			for (auto cop : voyageurs)
			{
				if (cop != player && !cop->isMoving() && cop->getStation() == lastStation)
				{
					arrested += timeStep;
					if (finished()) sfx[SFX::Siren].play();
					break;
				}
			}
		}

		// countdown
		if ((countdown -= timeStep) <= 0.f)
		{
			countdown = 0.f;
			arrested = ArrestedLimit;
			sfx[SFX::Timeout].play();
		}

		// update metro
		leMetro.update();

		bool moving = player->isMoving();

		// voyageurs
		for (auto e : voyageurs)
			e->update();

		if (moving && !player->isMoving())
		{
			if (player->getStation()->name == L"Montparnasse Bienvenüe (S)" && random::unif() < 0.5f)
				sfx[SFX::Jingle].play();
			else if (random::unif() < 0.1f)
				sfx[SFX::Attention].play();
		}
	}

	// game finished
	else
	{
		// end scene
		if (arrested < ArrestedLimit + EndDuration)
			arrested += timeStep;

		// restart
		else if (!receivingInput && mouse::isDown(0))
		{
			restart();
			return;
		}
	}

	// station name
	if (stationNameTimer > 0.f)
		stationNameTimer -= timeStep / 3.f;

	// camera
	if (mouse::isDown(1))
	{
		// zoom-in
		float delta = cameraZoom - MinimumZoom;
		cameraZoom -= std::min(delta, timeStep * CameraZoomSpeed);

		// move camera
		Vec2 cameraDelta = Vec2(814.516f, 702.847f) - camera;
		camera += cameraDelta * (delta == 0 ? 1.f : timeStep * CameraZoomSpeed / delta);
	}
	else
	{
		// zoom-out
		float delta = MaximumZoom - cameraZoom;
		cameraZoom += std::min(delta, timeStep * CameraZoomSpeed);

		// move camera
		Vec2 cameraDelta = player->getPosition() - camera;
		camera += cameraDelta * (delta == 0 ? 1.f : timeStep * CameraZoomSpeed / delta);
	}

	// effects
	effectTimer += timeStep * Pi;

	// money effect
	moneyPackage.update();

	// store input state
	receivingInput = mouse::isDown(0);
}

//---------------------------------------------------------------------
void Game::draw()
{
	// apply camera transformations
	matrix::push();
	matrix::mult(Vec2::one * windowSize / 2.f);
	matrix::mult(CameraScale * cameraZoom, CameraScale * cameraZoom);
	matrix::mult(-camera);

	// cops
	for (auto cop : voyageurs)
	{
		if (cop == player) continue;
		Vec2 p = cop->getPosition();

		float radius = cop->getDirection().isValid() || cop->isMoving() ? 12.f : 8.f;

		// draw red arc
		uiShapes.setColor(Red);
		uiShapes.drawArc(p, radius, effectTimer + 0.5f * Pi, Pi / 2.f);

		// draw blue arc
		uiShapes.setColor(Color(0.f, 0.5f, 1.f));
		uiShapes.drawArc(p, radius, effectTimer + 1.5f * Pi, Pi / 2.f); 
	}
	
	// highlight possible directions
	for (auto &stretch : player->getStation()->stretches)
	{
		// find adjacent station
		Station *target = stretch->b;
		if (target == player->getStation()) target = stretch->a;

		// set color
		if (player->getStation()->stretchTowards(player->getDirection()) == stretch)
			uiShapes.setColor(stretch->ligne->color);
		else
			uiShapes.setColor(Color(1.f, 1.f, 1.f, 0.1f));

		// draw
		uiShapes.drawArc(target->position, 8.f, effectTimer, Pi / 2.f);
		uiShapes.drawArc(target->position, 8.f, effectTimer + Pi, Pi / 2.f);
	}

	// draw the map
	leMetro.draw();

	// money
	moneyPackage.draw();

	// voyageurs
	player->draw();

	// UI
	matrix::pop();
	
	// show station name
	if (stationNameTimer > 0.f)
	{
		// transition effect
		float a = std::min(1.f, 10.f * std::sin(stationNameTimer * Pi));

		// draw background
		uiShapes.setColor(Color(0.f, 0.f, 0.f, a * 0.8f));
		uiShapes.drawRectangle(Rect(windowSize * (1.f - 0.65f), 0.035f * windowSize, windowSize * 0.65f, windowSize * 0.05f));

		// draw name
		glColor4f(1.f, 1.f, 1.f, a);
		bigFont.draw(Vec2(windowSize * 0.4f, 0.05f * windowSize), lastStation->name);
	}

	// score
	wchar_t buffer[32];
	swprintf(buffer, 32, L"$ %.2f", bonusScore + score * 100.f);

	std::wstring scoreString(buffer);
	float w = smallFont.measure(scoreString).width();

	// end scene?
	if (finished())
	{
		float t = std::min(arrested - ArrestedLimit, EndDuration) / EndDuration;
		float x = std::sin(t * Pi / 2.f);

		// background
		uiShapes.setColor(Color(0.f, 0.f, 0.f, 0.8f));
		uiShapes.drawRectangle(Rect(Vec2(1.5f - x, 0.5f) * windowSize, (float)windowSize, windowSize * 0.05f));

		glColor4f(1.f, 1.f, 1.f, x);
		bigFont.draw(Vec2(windowSize * 0.5f - w / 2.f, windowSize * 0.492f), scoreString);
	}
	else
	{
		// draw background
		uiShapes.setColor(Color(0.f, 0.f, 0.f, 0.8f));
		uiShapes.drawRectangle(Rect(0.f, windowSize * 0.95f, w + windowSize * 0.01f, windowSize * 0.03f));

		// draw score
		glColor4f(1.f, 1.f, 1.f, 1.f);
		smallFont.draw(Vec2(0.005f, 0.955f) * windowSize, scoreString);
	}

	// countdown
	std::wostringstream ss;
	ss.str(std::wstring());
	ss.clear();
	ss << L":" << (unsigned)countdown;

	std::wstring countdownString(ss.str());
	w = bigFont.measure(countdownString).width();

	// background
	uiShapes.setColor(Color(0.f, 0.f, 0.f, 0.8f));
	uiShapes.drawRectangle(Rect(Vec2(0.5f, 0.9f) * windowSize, w + windowSize * 0.02f, windowSize * 0.05f));

	glColor4f(1.f, 1.f, 1.f, 1.f);
	bigFont.draw(Vec2(std::floor(windowSize * 0.5f - w / 2.f), std::floor(windowSize * 0.892f)), countdownString);
}

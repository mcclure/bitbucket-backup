
#include "Voyageur.h"

using namespace ld;

//---------------------------------------------------------------------
Voyageur::Voyageur()
{
	placeAt(nullptr);
}

//---------------------------------------------------------------------
Station *Voyageur::getStation() const
{
	return nextStop;
}

//---------------------------------------------------------------------
Vec2 Voyageur::getPosition() const
{
	// stationary
	if (!movement) return nextStop->position;

	// get train position
	float instant = movement->ligne->trainPosition();

	// going forward?
	if (nextStop == movement->b)
		return movement->getCurve().evaluate(instant, 1.f);
	
	// going backwards
	return movement->getCurve().evaluate(1.f - instant, -1.f);
}

//---------------------------------------------------------------------
Direction Voyageur::getDirection() const
{
	return direction;
}

//---------------------------------------------------------------------
bool Voyageur::isMoving() const
{
	return movement != nullptr;
}

//---------------------------------------------------------------------
void Voyageur::placeAt(Station *station)
{
	nextStop = station;
	movement = nullptr;
	direction.ligne = nullptr;
	insideTrain = false;
}

//---------------------------------------------------------------------
void Voyageur::stop()
{
	direction.ligne = nullptr;

	// exit train
	if (!movement) insideTrain = false;
}

//---------------------------------------------------------------------
bool Voyageur::move(const Direction &dir)
{
	if (!nextStop || !nextStop->stretchTowards(dir))
		return false;

	// exit train
	if (!movement && direction != dir)
		insideTrain = false;

	direction = dir;
	return true;
}

//---------------------------------------------------------------------
void Voyageur::update()
{
	// handle movement
	if (movement)
	{
		// arrived?
		if (!movement->ligne->trainsEnRoute())
		{
			// exit train?
			if (direction.ligne != movement->ligne)
				insideTrain = false;

			// movement ended
			movement = nullptr;

			// end of line?
			if (insideTrain && !nextStop->stretchTowards(direction))
			{
				direction.ligne = nullptr;
				insideTrain = false;
			}
		}
	}

	// wait for the train
	else if (direction.ligne)
	{
		// train leaving?
		if (!direction.ligne->trainsEnRoute())
			insideTrain = true;

		// train left?
		else if (insideTrain)
		{
			// get path
			movement = nextStop->stretchTowards(direction);

			// valid movement?
			if (!movement)
			{
				direction.ligne = nullptr;
				insideTrain = false;
			}
			else
			{
				nextStop = direction.forward ? movement->b : movement->a;
			}
		}
	}
}

//---------------------------------------------------------------------
Player::Player()
{
	// set up particles
	for (auto &emitter : emitters)
	{
		emitter.variation = Pi * 0.75f;
		emitter.direction = 0.f;
		emitter.duration = 1.f;
		emitter.interval = 0.1f;
		emitter.velocity = 20.f;
		emitter.size = 2.f;
		emitter.color = White;
	}

	emitters[1].color = Black;
	emitters[1].direction = Pi;
}

//---------------------------------------------------------------------
void Player::update()
{
	// particles
	for (auto &emitter : emitters)
	{
		emitter.position = getPosition();
		emitter.update();
	}

	// base
	Voyageur::update();
}

//---------------------------------------------------------------------
void Player::draw()
{
	// particles
	for (auto &emitter : emitters)
		emitter.draw();
}

//---------------------------------------------------------------------
Cop::Cop(const Direction &patrol, Station *station) : patrol(patrol)
{	// start patrol
	placeAt(station);
	cooldown = 0.f;
}

//---------------------------------------------------------------------
void Cop::update()
{
	// if moving
	if (isMoving())
	{
		// stop at the next station
		stop();
	}

	// waiting next train
	else if (!getDirection().ligne)
	{
		// just stopped
		if (cooldown <= 0.f)
		{
			// reset timer
			cooldown = Metro::timeFactor * 3.9f;
		}

		// take next train
		else if ((cooldown -= timeStep) <= 0.f)
		{
			// continue patrol
			if (!move(patrol))
				move(patrol = Direction(patrol.ligne, !patrol.forward));
		}
	}

	// base
	Voyageur::update();
}

#pragma once

#include "Resource.h"
#include "RMath.h"

namespace ld
{
	// The time step.
	const float timeStep = 1.f / 60.f;

	// The window size. (window is a square)
	extern unsigned windowSize;

	//
	// Manages the internals of a game.
	//
	class BaseGame
	{
	public:
		//
		// Default constructor.
		//
		BaseGame();

		//
		// Destructor.
		//
		~BaseGame();

		//
		// Starts the game. Returns after the game closes itself.
		//
		void run();

		//
		// Sets a flag that instructs the game to quit the main loop.
		//
		void quit();

		//
		// Gets the running instance.
		//
		// @return null if the game's not running, else the instance.
		//
		static BaseGame *runningInstance();

	protected:
		//
		// Updates the game.
		//
		virtual void update() = 0;

		//
		// Draws the game.
		//
		virtual void draw() = 0;

	private:
		//
		// Queries if the engine may begin a new step.
		//
		// @return true if it allows, false if it should wait.
		//
		bool beginStep();

		// true to running.
		bool running;

		// The time consumed until now.
		double consumedTime;

		// The single instance.
		static BaseGame *instance;
	};

	//
	// Random.
	//
	namespace random
	{
		//
		// Gets a random single-precision floating point number between 0 and 1 following an uniform
		// distribution.
		//
		float unif();
	}

	//
	// Keyboard.
	//
	namespace key
	{
		//
		// Query if the given key is begin pressed.
		//
		bool isDown(int);
	}

	//
	// Mouse.
	//
	namespace mouse
	{
		//
		// Query if the given button is begin pressed.
		//
		bool isDown(int);

		//
		// Gets the cursor position.
		//
		Vec2 cursor();
	}

	namespace matrix
	{
		//
		// Multiplies the current matrix by a translation matrix.
		//
		void mult(Vec2 translation);

		//
		// Multiplies the current matrix by a rotation matrix.
		//
		void mult(float rotation);

		//
		// Multiplies the current matrix by a translation-rotation matrix.
		//
		void mult(Vec2 translation, float rotation);

		//
		// Multiplies the current matrix by a scaling matrix.
		//
		void mult(float scaleX, float scaleY);

		//
		// glPushMatrix.
		//
		void push();

		//
		// glPopMatrix.
		//
		void pop();
	}
}

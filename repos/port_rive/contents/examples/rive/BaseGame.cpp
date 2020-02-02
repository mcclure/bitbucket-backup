
#include <assert.h>
#include <glfw.h>
#include <random>

#include "BaseGame.h"

using namespace ld;

// Static initializations
BaseGame *BaseGame::instance = nullptr;
static std::mt19937 rndEngine;
static float _matrix[16];

unsigned ld::windowSize = 800U;

//---------------------------------------------------------------------
static unsigned randomSeed()
{
#if defined(_MSC_VER)
	return (unsigned)__rdtsc();
#elif defined(__GCCE__)
	unsigned lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return (unsigned)(((unsigned long long)hi << 32) | lo);
#else
	return (unsigned)(glfwGetTime() * 100000000.0);
#endif
}

//---------------------------------------------------------------------
static int GLFWAPI windowClosed()
{
	if (BaseGame::runningInstance())
		BaseGame::runningInstance()->quit();

	return GL_FALSE;
};

//---------------------------------------------------------------------
BaseGame::BaseGame()
{
	// keep a reference to this instance
	assert(!instance);
	instance = this;

	// initialize GLFW
	glfwInit();

	// open default window
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 8);
	glfwOpenWindow(windowSize, windowSize, 0, 0, 0, 0, 0, 0, GLFW_WINDOW);
	glfwSetWindowCloseCallback(windowClosed);

	// get desktop resolution
	GLFWvidmode mode;
	glfwGetDesktopMode(&mode);

	// move window to the center of the screen
	mode.Width -= windowSize;
	mode.Height -= windowSize;
	glfwSetWindowPos(mode.Width / 2,
#ifdef __APPLE__
					 0
#else
					 mode.Height / 2
#endif
	 );

	// reset states
	glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// reset projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// set viewport
	glOrtho(0.f, (float)windowSize, 0.f, (float)windowSize, 1.f, -1.f);

	// random
	rndEngine.seed(randomSeed());
}

//---------------------------------------------------------------------
BaseGame::~BaseGame()
{
	// nullify running instance
	instance = nullptr;

	// destroy GLFW
	glfwTerminate();
}

//---------------------------------------------------------------------
void BaseGame::run()
{
	// running
	running = true;

	// time
	consumedTime = glfwGetTime();

	// main loop
	while (running)
	{
		// update stuff
		if (beginStep()) update();

		// clear
		glClear(GL_COLOR_BUFFER_BIT);

		// clear transformations
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// draw stuff
		draw();

		// end scene
		glfwSwapBuffers();
	}
}

//---------------------------------------------------------------------
void BaseGame::quit()
{
	running = false;
}

//---------------------------------------------------------------------
BaseGame *BaseGame::runningInstance()
{
	return instance;
}

//---------------------------------------------------------------------
bool BaseGame::beginStep()
{
	double delta = std::floor((glfwGetTime() - consumedTime) / timeStep);

	consumedTime += delta * timeStep;

	return delta > 0.;
}

//---------------------------------------------------------------------
float random::unif()
{
	std::uniform_real_distribution<float> unif;
	return unif(rndEngine);
}

//---------------------------------------------------------------------
bool key::isDown(int key)
{
	return glfwGetKey(key) == GLFW_PRESS;
}

//---------------------------------------------------------------------
bool mouse::isDown(int button)
{
	return glfwGetMouseButton(button) == GLFW_PRESS;
}

//---------------------------------------------------------------------
Vec2 mouse::cursor()
{
	int x = 0, y = 0;
	glfwGetMousePos(&x, &y);
	return Vec2((float)x, (float)y);
}

//---------------------------------------------------------------------
void matrix::mult(Vec2 translation)
{
	// zero
    memset(_matrix, 0, sizeof(float) * 16);

	// identity
	_matrix[0] = _matrix[5] = _matrix[10] = _matrix[15] = 1.f;

	// translation
	_matrix[12] = translation.x;
	_matrix[13] = translation.y;

	glMultMatrixf(_matrix);
}

//---------------------------------------------------------------------
void matrix::mult(float rotation)
{
	// zero
	memset(_matrix, 0, sizeof(float) * 16);

	// rotation
	_matrix[0] = _matrix[5] = cos(-rotation);
    _matrix[1] = -(_matrix[4] = sin(-rotation));

	// identity
    _matrix[10] = _matrix[15] = 1.f;

	glMultMatrixf(_matrix);
}

//---------------------------------------------------------------------
void matrix::mult(Vec2 translation, float rotation)
{
	// zero
    memset(_matrix, 0, sizeof(float) * 16);

	// translation
	_matrix[12] = translation.x;
	_matrix[13] = translation.y;

	// rotation
	_matrix[0] = _matrix[5] = cos(-rotation);
    _matrix[1] = -(_matrix[4] = sin(-rotation));

	// identity
    _matrix[10] = _matrix[15] = 1.f;

	glMultMatrixf(_matrix);
}

//---------------------------------------------------------------------
void matrix::mult(float scaleX, float scaleY)
{
	// zero
    memset(_matrix, 0, sizeof(float) * 16);

	// identity
    _matrix[10] = _matrix[15] = 1.f;

	// scale
	_matrix[0] = scaleX;
	_matrix[5] = scaleY;

	glMultMatrixf(_matrix);
}

//---------------------------------------------------------------------
void matrix::push()
{
	glPushMatrix();
}

//---------------------------------------------------------------------
void matrix::pop()
{
	glPopMatrix();
}

#include "Particle.h"


Particle::Particle(float x, float y, int group, Pixel p) :
x(x), y(y), startX(x), startY(y), binX(-1), binY(-1), applyStartPositionSpring(false), applySpringAttachmentForces(false), applySpringAttachToAnchored(false), group(group), p(p)
{
	velocityX = randomfloat(-0.1, 0.1);
	velocityY = randomfloat(-0.1, 0.1);
}


Particle::~Particle(void)
{
}

#include "Emitter.h"


Emitter::Emitter(float x, float y, int group) :
x(x), y(y), group(group), timer(0)
{
}


Emitter::~Emitter(void)
{
}


void Emitter::update()
{
	if (timer > 0)
		timer --;
}


void Emitter::trigger(int triggerGroup)
{
	if ((triggerGroup == group)
	&& (timer == 0))
	{
		timer = 4;
		float offset = 6.0;
		spawnParticle(x + randomfloat(-offset, offset), y + randomfloat(-offset, offset), group);
	}
}
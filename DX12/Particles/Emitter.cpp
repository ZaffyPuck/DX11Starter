#include "Emitter.h"

Emitter::Emitter()
	:
	firstLiveParticleIndex(0),
	firstDeadParticleIndex(0),
	liveParticleCount(0)
{
}

Emitter::~Emitter()
{
	delete[] particles;
}

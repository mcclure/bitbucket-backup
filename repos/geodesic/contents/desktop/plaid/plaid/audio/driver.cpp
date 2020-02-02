#include "implementation.h"


using namespace plaid;


//Driver registry
typedef std::multimap<float, AudioDriver*> Drivers;
typedef Drivers::value_type Driver;
static Drivers &driverRegistry() {static Drivers c; return c;}
static bool &lockDrivers() {static bool b = false; return b;}

AudioDriver::AudioDriver(String _name, float preference) :
	name(_name)
{
	if (lockDrivers()) return;
	Drivers &drivers = driverRegistry();
	drivers.insert(Driver(preference, this));
}

void AudioDriver::LockRegistry()
{
	lockDrivers() = true;
}

AudioDriver *AudioDriver::Default()
{
	Drivers &drivers = driverRegistry();
	if (drivers.size()) return drivers.rbegin()->second;
	return NULL;
}

AudioDriver *AudioDriver::Find(const String &name)
{
	Drivers &drivers = driverRegistry();
	for (Drivers::const_iterator i=drivers.begin(), e=drivers.end(); i!=e; ++i)
	{
		if (i->second->name.find(name) == 0) return i->second;
	}
	return NULL;
}

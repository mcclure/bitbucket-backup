#include <cassert>
#include <ctime>
#include <cstring>

#include "audio.h"
#include "implementation.h"
#include "../thread/lockfree.h"

#include "util.h"
#include "effects.h"

#if PLAIDGADGET
	#include "../console/console.h"
	#include "../graphics/graphics.h"
	#include "../geometry/2d.h"
	#include "../player/player.h"
	#include "../storage/storage.h"

	#include "../script/bindings.h"
#else //PLAIDGADGET
	#define PI 3.14159f
#endif


using namespace plaid;

using namespace std;


#if PLAIDGADGET
static void SoundConD(void *p)                     {new (p) Sound();}
static void SoundCopy(void *p, const Sound &o)     {new (p) Sound(o);}
static void SoundDest(void *p)                     {((Sound*)p)->~Sound();}
static bool SoundEquals(Sound *s, void *other, int typeID)
{
	if (typeID == 0) return s->null();
	if (typeID == asGetActiveContext()->GetEngine()->
		GetObjectTypeByName("Sound")->GetTypeId())
	{
		return (*s == *((Sound*)other));
	}
	else
	{
		//TODO comparison with audiostream types
		return false;
	}
}


static int audioBindings(Scripts &scripts, asIScriptEngine *engine)
{
	//Sound type
	AS_CLASS("Sound", sizeof(Sound),
		asOBJ_VALUE | asOBJ_APP_CLASS_CDAK | asOBJ_ASHANDLE);
	{
		//Construct/destroy
		AS_CONSTRUCT("Sound", "void f()",                asFUNCTION(SoundConD));
		AS_CONSTRUCT("Sound", "void f(const Sound &in)", asFUNCTION(SoundCopy));
		AS_DESTRUCT("Sound",                             asFUNCTION(SoundDest));

		//Handle compare
		AS_METHOD("Sound", "bool opEquals(?&in) const",
			asFUNCTION(SoundEquals), PGC_METHOD_OBJFIRST);

		//Assign/compare
		AS_METH("Sound", "bool opEquals(const Sound &in)",
			asMETHODPR(Sound, operator==, (const Pointer<AudioStream>&) const,
				bool));
		AS_METH("Sound", "Sound &opAssign(const Sound &in)",
			asMETHODPR(Sound, operator=, (const Sound&), Sound&));
	}


	//Audio type
	AS_CLASS("AUDIO", 0, asOBJ_REF|asOBJ_NOHANDLE);
	{
		//Format
		//AS_METH("AUDIO", "AudioFormat format()", asMETHOD(Audio, format));

		//Mixer controls
		AS_METH("AUDIO", "float get_volume()",
			asMETHODPR(Audio, volume, (), float));
		AS_METH("AUDIO", "void set_volume(float)",
			asMETHODPR(Audio, volume, (float), void));

		AS_METH("AUDIO", "void play(Sound)",
			asMETHODPR(Audio, play, (Sound), void));
		AS_METH("AUDIO", "void play(Sound, float)",
			asMETHODPR(Audio, play, (Sound, float), void));
		AS_METH("AUDIO", "float volume(Sound)",
			asMETHODPR(Audio, volume, (Sound), float));
		AS_METH("AUDIO", "void volume(Sound, float)",
			asMETHODPR(Audio, volume, (Sound, float), void));
		AS_METH("AUDIO", "void pause(Sound)",   asMETHOD(Audio, pause));
		AS_METH("AUDIO", "void stop(Sound)",    asMETHOD(Audio, stop));
		AS_METH("AUDIO", "bool playing(Sound)", asMETHOD(Audio, playing));
		AS_METH("AUDIO", "bool has(Sound)",     asMETHOD(Audio, has));

		//Stream files
		AS_METH("AUDIO", "Sound stream(String, bool loop=false)",
			asMETHODPR(Audio, stream, (String, bool), Sound));

		//Stream microphone
		AS_METH("AUDIO", "Sound microphone()", asMETHOD(Audio, microphone));
	}

	//Singleton-style global
	AS_GLOBAL("AUDIO Audio", scripts.audio);

	return 0;
}
static Bindings bindings(L"Audio", audioBindings);

#endif // PLAIDGADGET


namespace plaid
{
	class AudioImp_Dummy : public AudioImp
	{
	public:
		AudioImp_Dummy(Audio &audio, AudioScheduler &scheduler) :
			AudioImp(audio, scheduler), output(2, 120)
		{
			started = false;
			t = 0.0f;
		}
		virtual ~AudioImp_Dummy() {}

		virtual void startStream() {started = true;}

		virtual AudioFormat format() {return output;}
		virtual double time() {return t;}
		virtual float load() {return 0.0f;}

		virtual void update()
		{
			if (!started) return;

#if PLAIDGADGET
			float tn = t + audio.clock.delta();
#else
			float tn = t + 1.0f;
#endif

			//Pull a trivial chunk of audio
			Sint32 b[4], *out[2] = {b,b+2}, in[2];
			scheduler.render(out, in, 2, t, tn);

			t = tn;
		}

	private:
		AudioFormat output;
		bool started;
		float t;
	};

	static class Driver_Dummy : public AudioDriver
	{
	public:
		Driver_Dummy() : AudioDriver(L"dummy", -1.0f) {}
		virtual ~Driver_Dummy() {}

		virtual AudioImp *create(const DriverParams &params)
		{
			return new AudioImp_Dummy(params.audio, params.scheduler);
		}
	} driver;
}

class ExampleGen : public AudioStream
{
public:
	ExampleGen()
	{
		output.channels = 1;
		output.rate = 48000;
		fm = 0.0f;
		wave = 0.0f;
		circ = 2.0f*PI;
		settings.waveFreq = waveBase = 440.0f;
		modRange = 110.0f;
		settings.modFreq = modBase = 110.0f;
		settings.loud = false;
	}

	virtual AudioFormat format()
	{
		return output;
	}

	virtual void tick(Uint64 frame)
	{
		queue.push(settings);
	}

	virtual bool exhausted()
	{
		return false;
	}

	virtual void pull(AudioChunk &chunk)
	{
		if (chunk.first()) queue.pull(cur);

		Sint32 val;
		Sint32 *pos = chunk.start(0), *end = chunk.end(0);

		float step = circ / output.rate;
		float amp = cur.loud ? 5000000.0f : 0.0f;
		while (pos < end)
		{
			//Frequency oscillator
			fm += step * cur.modFreq;
			if (fm > circ) fm -= circ;

			//Output oscillator
			wave += step * (cur.waveFreq + sin(fm) * modRange);
			if (wave > circ) wave -= circ;

			//Output
			val = Sint32(amp * sin(wave));
			*(pos++) = val;
		}
	}

	void control(float mx, float my, bool _loud)
	{
		settings.waveFreq = waveBase * (1.0f + my);
		settings.modFreq = modBase * (1.0f + mx);
		settings.loud = _loud;
	}

private:
	AudioFormat output;

	//Static settings
	float circ;
	float modBase, waveBase;
	float modRange;

	//Dynamic settings
	struct Settings
	{
		bool loud;
		float modFreq, waveFreq;
	};
	Settings settings;
	LockFreeQueue<Settings> queue;

	//Mixerside state
	Settings cur;
	float fm, wave;
};


#if PLAIDGADGET
	Audio::Audio(const ModuleSet &modules, bool headless) :
		Module(modules)
#else
	Audio::Audio(bool headless)
#endif //PLAIDGADGET
{
	scheduler = new AudioScheduler(*this);

	//headless = true;

	if (headless)
	{
		imp = new AudioImp_Dummy(*this, *scheduler);
	}
	else
	{
		AudioDriver::DriverParams params = {*this, *scheduler};
		imp = AudioDriver::Default()->create(params);
	}

	//Set up the scheduler
	scheduler->setupFront(imp->format(), imp->time());
	master = scheduler->master;
	vol = 1.0f;
	master->volume(1.0f);

	//Start the audio stream
	imp->startStream();
}

void Audio::update()
{
	/*static ExampleGen *eGen = NULL;
	if (!eGen) {eGen = new ExampleGen(); play(Sound(eGen));}
	eGen->control(mouse().x, mouse().y, mouse.left);*/

#if PLAIDGADGET
	console.monitor() << L"audio: " << int(imp->load()*100.0f+.9f) << std::endl;

	if (keyboard.f6.pressed())
	{
		if (vol < .01f) console(L"audio volume .1");
		else if (vol < .2f) console(L"audio volume 1");
		else console(L"audio volume 0");
	}
#endif

	scheduler->tick(imp->time());
	imp->update();
}

Audio::~Audio()
{
	delete imp;
	delete scheduler;
}

float Audio::load()
{
	return imp->load();
}

#if PLAIDGADGET
void Audio::handle(Command &command)
{
	if (command.is(L"hush"))
	{
		if (command.help())
		{
			command.acknowledge();
			console << L"\n'hush' or 'audio hush' silences all audio.\n\n";
		}
		else
		{
			volume(0.0f);
			command.respond(L"Mum's the word!");
		}
	}
	else if (command.is(L"volume"))
	{
		if (command.help())
		{
			command.acknowledge();
			console << L"\n'volume <vol>' sets the volume.\n"
				L"    0.0 is silence, 1.0 is normal.  2.0 is double!\n\n";
		}
		else if (command.parameters() != 2)
		{
			command.respond(L"ERROR: usage is 'volume <vol>'");
		}
		else
		{
			float v;
			if (!Parse(command[1], v))
			{
				command.respond(L"ERROR: 'volume' expects a decimal number.");
			}
			else
			{
				volume(v);
				command.acknowledge();
				console << L"Master volume changed to " << v << L"\n";
			}
		}
	}
	else if (command.is(L"audiolog"))
	{
		if (command.help())
		{
			command.acknowledge();
			console << L"\n'audiolog' enables audio scheduler logging.\n\n";
		}
		else
		{
			scheduler->monitor = !scheduler->monitor;
		}
	}
}
#endif PLAIDGADGET

AudioFormat Audio::format()
{
	return imp->format();
}

float Audio::volume()
{
	return vol;
}

void Audio::volume(float v)
{
	vol = v;
	master->volume(v);
}

#if PLAIDGADGET
Sound Audio::stream(const File &file, bool loop)
{
	return stream(file.name(), loop);
}
#endif

Sound Audio::stream(String fname, bool loop)
{
	AudioCodec *codec = AudioCodec::Find(fname);
	if (!codec)
	{
		std::wcout << L"Could not find codec for: " << fname << std::endl;
		return Sound::Null();
	}

	AudioCodec::StreamParams params = {fname, loop};
	return codec->stream(params);
}

Sound Audio::microphone()
{
	return scheduler->microphone();
}

void Audio::play(Sound stream)
{
	master->play(stream);
}
void Audio::play(Sound stream, float volume)
{
	master->play(stream);
	master->volume(stream, volume);
}
void Audio::pause(Sound stream)
{
	master->pause(stream);
}
void Audio::stop(Sound stream)
{
	master->drop(stream);
}
void Audio::volume(Sound stream, float volume)
{
	master->volume(stream, volume);
}

bool Audio::playing(Sound stream)
{
	return master->playing(stream);
}
bool Audio::has(Sound stream)
{
	return master->has(stream);
}
float Audio::volume(Sound stream)
{
	return master->volume(stream);
}


//=============================================================================
//========    AND THE REST OF THE STUFF   =====================================
//=============================================================================

static const float dbLogMult = 10.0f / 2.30258509f;
static const float multSemitone = 1.0594630943f;
static const float semiLogMult = 1.0f / 0.0577622650f;

float plaid::Decibels(float db)
{
	return std::pow(10.0f, db/10.0f);
}
float plaid::ToDecibels(float mult)
{
	return std::log(mult) * dbLogMult;
}
float plaid::Semitones(float steps)
{
	return std::pow(multSemitone, steps);
}
float plaid::ToSemitones(float mult)
{
	return std::log(mult) * semiLogMult;
}

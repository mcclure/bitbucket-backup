#include <iomanip>
#include <cstring>
#include <fstream>
#include <vector>
#include <deque>
#include <iostream>

#include "audio.h"
#include "implementation.h"

#if PLAIDGADGET
	#include "../console/console.h"
#endif //PLAIDGADGET

#include "../thread/lockfree.h"

/*
	Change this to 1 to duplicate all audio output into a PCM file.
		This may cause performance issues with audio rendering.
*/
#define PCM_OUT 0


using namespace std;


namespace plaid
{
	struct AudioScheduler::Front
	{
		//Frame timing
		Uint64 prepFrame;

		//Feed to mix thread (shared)
		LockFreeQueue<double> frames;
	};

	struct AudioScheduler::Back
	{
		//Queues from the audio mixer back to the game thread (shared)
		struct MixReport {char rep[120];};
		LockFreeQueue<MixReport> feedback;

		//Scratch data
		AudioScratch::Stack scratch;

		//Frame timing
		std::deque<double> queued;
		Uint64 mixFrame, ackFrame;
		Uint32 pullCount;
		double firstFrame, lastFrame;
		Uint64 sampleCount;
		Uint64 overflow;

		//Frame splitting
		Uint32 frameSamples, leftover;
		float frameCut;

		//Microphone data
		const Sint32 *mikeData;
		Uint32 mikeLength;
		Uint32 mikeFrame;

#if PCM_OUT
		std::ofstream pcmOut;
#endif
	};


	class MikeStream : public AudioStream
	{
	public:
		MikeStream(AudioFormat &fmt, AudioScheduler::Back* &_back) :
			output(fmt), back(_back)
		{
			readFrame = back->mikeFrame;
			prog = 0;
		}

		virtual AudioFormat format()
		{
			return output;
		}

		virtual void tick(Uint64 frame)
		{
		}

		virtual bool exhausted()
		{
			return false;  // Speak with passion.
		}

		virtual void pull(AudioChunk &chunk)
		{
			//This is possible at startup; race conditions are bad.
			if (!back) chunk.silence();

			//Shorthand.
			Uint32 frame = back->mikeFrame, length = back->mikeLength;
			const Sint32 *data = back->mikeData;

			//How much information is available?
			if (readFrame != frame) {readFrame = frame; prog = 0;}
			Uint32 want = chunk.length(), get = std::min(length-prog, want);

			//Read what we can from the buffer
			std::memcpy((void*)chunk.start(0), (const void*)(data+prog), 4*get);
			prog += get;

			//Fill your cup too full and it will spill...
			if (get < want)
				std::memset((void*)(chunk.start(0)+get), 0, 4*(want-get));
		}

	private:
		AudioFormat output;
		AudioScheduler::Back* &back;
		Uint32 prog, readFrame;
	};
}


using namespace plaid;


AudioScheduler::AudioScheduler(Audio &_audio) :
	audio(_audio)
{
	front = NULL;
	back = NULL;
	master = NULL;

	monitor = false;
}

AudioScheduler::~AudioScheduler()
{
#if PCM_OUT
	pcmOut.close();
#endif

	delete front;
	delete back;
}

void AudioScheduler::setupFront(AudioFormat _format, double time)
{
	format = _format;
	if (format.channels > PG_MAX_CHANNELS)
		reportError("Number of audio output channels exceeds maximum...");

	//Create master mixer
	master = new Mixer(format);
	signal = Signal(master);
	signal.tick(0);

	//Create front
	front = new Front;
	front->prepFrame = 0;

	//First tick
	//tick(time);
}

void AudioScheduler::setupBack(double time)
{
	back = new Back;

	//Timing
	back->firstFrame = back->lastFrame = time;
	back->mixFrame = back->ackFrame = 0;
	back->pullCount = 0;
	back->sampleCount = 0;
	back->overflow = 0;
	back->frameSamples = back->leftover = 0;
	back->frameCut = 0.0f;

#if PCM_OUT
	data->pcmOut.open("pg.pcm", ios_base::out|ios_base::binary|ios_base::trunc);
#endif

	//Microphone buffers
	back->mikeData = NULL;
	back->mikeLength = 0;
	back->mikeFrame = 0;
}

Sound AudioScheduler::microphone()
{
	AudioFormat form = format;
	form.channels = 1;
	return Sound(new MikeStream(form, back));
}

void AudioScheduler::tick(double time)
{
	if (!front) reportError(L"AudioScheduler was not set up!");

	//Queue frame's data for mixer thread
	++front->prepFrame;
	signal.tick(front->prepFrame);
	front->frames.push(time);

	//Feedback from mixer
	if (back)
	{
		Back::MixReport rep;
		while (back->feedback.pull(rep))
		{
#if PLAIDGADGET
			if (monitor) std::cout << "audio: " << rep.rep << std::endl;
#endif // PLAIDGADGET
		}
	}
}

void AudioScheduler::render(Sint32 **speaker, const Sint32 *microphone,
	Uint32 moments, double time, double capTime)
{
	//Debug tone (highlights underflows)
	/*{
		for (Uint16 *i = (Uint16*) _speaker, *e = i+frameCount*2; i<e;)
		{
			*i++ = +20000; *i++ = +20000; *i++ = 0; *i++ = 0;
			*i++ = -20000; *i++ = -20000; *i++ = 0; *i++ = 0;
		}
	}*/


	//Possibly setup backend data
	if (!back)
	{
		setupBack(time);
	}


	//Microphone stuff
	back->mikeData = microphone;
	back->mikeLength = moments;
	++back->mikeFrame;


	//LET'S GET READY TO RENDERRRRRRR
	Uint32 require = moments;
	Sint32 *pos[PG_MAX_CHANNELS];
	for (unsigned i = 0; i < format.channels; ++i)
	{
		//std::cout << i << ": " << speaker[i] << std::endl;
		pos[i] = speaker[i];
	}

	//A little output
	std::stringstream mixReport;
	mixReport << std::setw(5) << require << ' ';


	//Main rendering loop
	while (require)
	{
		//Chunk parameters
		Uint32 length;
		float a, b;

		if (back->leftover)
		{
			//Finish rendering incomplete frame
			a = back->frameCut;
			length = back->leftover;
		}
		else
		{
			//Ascertain length of new frame
			double curFrame;
			if (back->queued.size())
			{
				//Get queued frames if any...
				curFrame = back->queued.front();
				back->queued.pop_front();
			}
			else
			{
				//...Otherwise pull new ones
				if (!front->frames.pull(curFrame)) break;
			}

			Uint64 samp = Uint64(format.rate * (curFrame - back->firstFrame));
			if (curFrame < back->firstFrame) samp = 0;
			if (samp <= back->sampleCount)
			{
				//Degenerate case: minimal render
				back->frameSamples = 1;
				samp = ++back->sampleCount;
			}
			else
			{
				//Normal case: calculate difference
				back->frameSamples = samp - back->sampleCount;
				back->sampleCount = samp;
			}

			// Limit frame length
			back->frameSamples = std::min(back->frameSamples, format.rate/4);

			//Overflow countermeasure
			if (back->overflow)
			{
				int reduc = std::min(
					int(back->frameSamples-1), //Very aggressive
					int(back->overflow));
				back->frameSamples -= reduc;
				back->overflow -= reduc;
			}

			//And prep...
			length = back->frameSamples;
			a = 0.0f;
			back->lastFrame = curFrame;
			++back->mixFrame;
		}

		//Decide whether to cut off the end
		if (length > require)
		{
			//We can't fit this whole frame in...
			mixReport << (back->leftover?'-':'[');
			back->leftover = length - require;
			b = back->frameCut = 1.0f-back->leftover/float(back->frameSamples);
			length = require;
		}
		else
		{
			//Smooth sailing.
			mixReport << (back->leftover?']':'#');
			b = 1.0f;
			back->leftover = 0;
		}

		//RENDARR
		if (length)
		{
			AudioChunk chunk(back->scratch, format, pos, length,
				AudioSync(back->pullCount++, back->mixFrame, a, b));
			signal.pull(chunk);
			//chunk.silence();
			for (unsigned i = 0; i < format.channels; ++i) pos[i] += length;
			require -= length;
		}
	}

	if (require)
	{
		//EXTRA RENDARR
		mixReport << "+" << require;
		if (back->leftover) std::cout <<
			"FRAME LEFTOVER DETECTED DURING EXTRA-MIX" << std::endl;
		AudioChunk chunk(back->scratch, format, pos, require,
			AudioSync(back->pullCount++, back->mixFrame, 1.0f, 1.0f));
		signal.pull(chunk);
		//chunk.silence();
		for (unsigned i = 0; i < format.channels; ++i) pos[i] += require;
	}

	//Gather up extra frames
	{
		double hold;
		while (front->frames.pull(hold)) back->queued.push_back(hold);
	}

	mixReport << " // ";

	//Calculate overflow (tolerance of one frame-length)
	if (back->queued.size() > 1)
	{
		back->overflow = format.rate*(*(back->queued.end()-2)-back->lastFrame);
		mixReport << " -" << std::setw(5) << back->overflow;
	}
	/*mixReport << " LATENCY > " << fixed << setprecision(2)
		<< (capTime-back->lastFrame);*/
	if (back->queued.size())
	{
		double waste = back->queued.back()-back->lastFrame;
		mixReport << ", WASTE = " << waste;
	}


#if PCM_OUT
	//DEBUG OUTPUT
	pcmOut.write((const char*) outBuffer, obSize);
#endif


	/*if (Pa_GetStreamTime(stream) > criticalTime)
		mixReport << " (LATE FOR CAPTURE!)";
	if (obFill != obSize) mixReport << " (INCOMPLETE RENDER)";*/


	//Compose report and send back
	std::string s = mixReport.str();
	Back::MixReport rep;
	std::memcpy(rep.rep, s.c_str(), std::min(s.length()+1,size_t(120)));
	back->feedback.push(rep);
}

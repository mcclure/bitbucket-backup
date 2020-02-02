#include "audio.h"

#include "util.h"
#include "effects.h"



namespace plaid
{
	class DummyStream : public AudioStream
	{
	public:
		DummyStream(const AudioFormat &format) :
			fmt(format) {}

		virtual AudioFormat format()            {return fmt;}
		virtual void tick(Uint64 frame)         {}
		virtual bool exhausted()                {return false;}
		virtual void pull(AudioChunk &chunk)    {/* NOTHING HAPPENS */}

	private:
		AudioFormat fmt;
	};
}


using namespace plaid;


Signal Signal::Dummy(const AudioFormat &format)
{
	return Signal(new DummyStream(format));
}


Signal::Signal(Sound sound)
{
	if (sound) bind(sound, AudioFormat(), false);
}

Signal::Signal(Sound sound, AudioFormat format, bool transcode)
{
	if (sound) bind(sound, format, transcode);
}

Signal::Signal(AudioStream *stream)
{
	if (stream)
	{
		bind(Sound(stream), AudioFormat(), false);
	}
}

Signal::Signal(AudioStream *stream, AudioFormat format, bool transcode)
{
	if (stream)
	{
		bind(Ref<AudioStream>(stream), format, transcode);
	}
}

void Signal::bind(Ref<AudioStream> stream, AudioFormat form,
	bool transcode)
{
	AudioFormat src = stream->format();

	if (!form.rate) form.rate = src.rate;

	/*std::cout << "Binding signal at " << src.rate << " hz";
	if (form.type != AudioFormat::NONE)
		std::cout << " to receptor at " << form.rate << " hz";*/

	//Check format compliance
	if (form.channels == 0 || src == form)
	{
		//std::cout << " ...trivial" << std::endl;
		if (!stream->attach()) return;
		data = new Data;
		data->stream = stream;
		data->trans = NULL;
	}
	else if (transcode)
	{
		//std::cout << " ...transcoding";

		//Sub-attachment to current format
		Signal subsig;
		subsig.bind(stream, AudioFormat(), false);
		if (!subsig) return;

		//Bound...
		data = new Data;
		data->stream = stream;

		//Attach transcoder
		subsig = new Transcoder(subsig, form);

		//Resample?
		if (form.rate != src.rate)
		{
			//std::cout << " with pitch shift";
			subsig = new Pitch(subsig, float(src.rate)/float(form.rate));
		}

		data->trans = new Signal(subsig);
		//std::cout << std::endl;
	}
	//else std::cout << " ...failed..." << std::endl;
}

Signal::Data::~Data()
{
	if (trans) delete trans;
	else stream->detach();
}

void Signal::tick(Uint64 frame) const
{
	if (!data) return;

	if (data->trans) data->trans->tick(frame);
	else data->stream->tick(frame);
}

bool Signal::exhausted() const
{
	return data ? data->stream->exhausted() : true;
}

AudioFormat Signal::format() const
{
	if (!data) return AudioFormat();

	if (data->trans) return data->trans->format();
	return data->stream->format();
}

void Signal::pull(AudioChunk &chunk) const
{
	if (!data) {chunk.silence(); return;}

	if (data->trans) data->trans->pull(chunk);
	else data->stream->pull(chunk);
}

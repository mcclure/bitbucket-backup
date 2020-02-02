#include <iostream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <cstdio>


#include <plaid/audio/implementation.h>

//#include <plaid/storage.h>


//stb_vorbis
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"


using namespace std;


namespace plaid
{
	namespace codec_ogg_stb
	{
		class OGGStream : public AudioStream
		{
		public:
			OGGStream(String file, bool _loop)
			{
				loop = _loop;
				finished = true;
				valid = false;

				std::string name = ToStdString(file);

				//cout << "Load OGG " << name << endl;

				//OGG file
				int error = 0;
				ogg = stb_vorbis_open_filename(&name[0], &error, NULL);
				if (!ogg) {cout << " VORBIS FAIL" << endl; return;}
				info = stb_vorbis_get_info(ogg);

				//Info
				output.channels = info.channels;
				output.rate = info.sample_rate;

				//Woot!
				valid = true;
				finished = false;
			}
			virtual ~OGGStream()
			{
			/*#if XIPH_OGG
				if (valid) ov_clear(&ogg);
			#else*/
				if (valid) stb_vorbis_close(ogg);
				//cout << "OGG ENDED" << endl;
			//#endif
			}

			bool success()
			{
				return valid;
			}

			virtual AudioFormat format()
			{
				return output;
			}

			virtual bool exhausted()
			{
				return finished;
			}

			virtual void tick(Uint64 frame)
			{
			}

			virtual void pull(AudioChunk &chunk)
			{
				if (!chunk.length())   return;
				if (!valid || finished) {chunk.silence(); return;}

				int samples, have = 0, need = chunk.length();

				//Create pointers to 16-bit data
				short *d16[PG_MAX_CHANNELS];
				for (Uint32 i = 0; i < chunk.format().channels; ++i)
					d16[i] = (short*) chunk.start(i);

				while (true)
				{
					samples = stb_vorbis_get_samples_short(ogg,
						chunk.format().channels, d16, (need-have));

					if (samples < 0)
					{
						finished = true;
						cout << " VORBIS ERROR" << endl;
						break;
					}
					if (samples == 0)
					{
						//File's end
						if (loop)
						{
							stb_vorbis_seek_start(ogg);
							continue;
						}
						else
						{
							finished = true;
							break;
						}
					}

					for (Uint32 i=0; i < chunk.format().channels; ++i)
						d16[i] += samples;
					have += samples;
					if (have > need) cout << "VORBIS OVERDRAW" << endl;

					//std::cout << "OGG pull: " << have << "/" << need << std::endl;

					if (have >= need) break;
				}

				//Cutoff marker if necessary
				if (have < need) chunk.cutoff(have);

				//Upsample data to 24-bit Sint32s
				for (Uint32 i=0; i < chunk.format().channels; ++i)
				{
					Sint32 *start = chunk.start(i), *op = start + have;
					short *ip = d16[i];
					while (op!=start) {*(--op) = 256 * Sint32(*(--ip));}
				}
			}

		private:
			bool loop, finished;
			bool valid;
			AudioFormat output;

			FILE *file;
			stb_vorbis *ogg;
			stb_vorbis_info info;
		};


		//Register the codec by instantiating it in static time
		static class Codec : public AudioCodec
		{
		public:
			Codec() : AudioCodec(L"ogg,ogv") {}
			virtual ~Codec() {}

			virtual Sound stream(const StreamParams &p)
			{
				OGGStream *ogg = new OGGStream(p.file, p.loop);
				if (!ogg->success()) {delete ogg; return Sound();}
				return Sound(ogg);
			}
		} codec;
	}
}

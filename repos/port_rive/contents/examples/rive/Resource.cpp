
#include <list>
#include <vector>
#include <chrono>

#include <glfw.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRIGONOMETRY_H
#include FT_OUTLINE_H
#include <al.h>
#include <alc.h>

#include "stb/stb_image.h"
#include "stb/stb_vorbis.h"

#include "Resource.h"
#include <float.h>

// Number of buffers on the queue
#define bufferCount 3

// Buffer length in seconds
#define bufferLength 1

using namespace ld;

//---------------------------------------------------------------------
Color::Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
{
}

//---------------------------------------------------------------------
Color::Color(unsigned rgba)
{
	r = ((rgba >> 24) & 0xFF) / 255.f;
	g = ((rgba >> 16) & 0xFF) / 255.f;
	b = ((rgba >> 8) & 0xFF) / 255.f;
	a = (rgba & 0xFF) / 255.f;
}

//---------------------------------------------------------------------
Texture::Texture() : handle(0)
{
}

//---------------------------------------------------------------------
Texture::~Texture()
{
	glDeleteTextures(1, (GLuint*)&handle);
}

//---------------------------------------------------------------------
void Texture::loadFile(const std::string &path)
{
	unsigned char *img = nullptr;
	int w = 2, h = 2;

	// blank texture
	if (!path.empty())
	{
		int imgChannels;

		// load the img
		img = stbi_load(path.c_str(), &w, &h, &imgChannels, STBI_rgb_alpha);

		// validate
		if (!img || !w || !h)
		{
			if (img) stbi_image_free(img);
			w = h = 2;
			img = nullptr;
		}
	}

	// generate texture
	glGenTextures(1, (GLuint*)&handle);
	glBindTexture(GL_TEXTURE_2D, (GLuint)handle);

	// load it and set it up
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);// 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);// GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);// GL_NEAREST);

	// release the image
	if (img) stbi_image_free(img);
}

//---------------------------------------------------------------------
void Texture::draw(const Rect &src, const Rect &dest) const
{
    // bind the texture
	glBindTexture(GL_TEXTURE_2D, handle);

    // draw the texture's geometry
	glBegin(GL_QUADS);
	{
		glTexCoord2f(src.left, src.top);	glVertex2f(dest.left, dest.top);
		glTexCoord2f(src.left, src.bottom);	glVertex2f(dest.left, dest.bottom);
		glTexCoord2f(src.right, src.bottom);glVertex2f(dest.right, dest.bottom);
		glTexCoord2f(src.right, src.top);	glVertex2f(dest.right, dest.top);
	}
    glEnd();
}

//---------------------------------------------------------------------
struct Font::Glyph
{
	//
	// Loads this glyph given the font and the character.
	//
	bool load(FT_Face, wchar_t);

	// The amount the cursor should be moved after this glyph.
	Vec2 advance;

	// The size of the texture quad used to draw this glyph.
	Vec2 size;
		
	// The texture coordinates of this glyph.
	Vec2 texCoords;

	// The texture handle of this glyph.
	unsigned texture;
};

//---------------------------------------------------------------------
struct FreeTypeLibrary
{
public:
	//
	// Destructor.
	//
	~FreeTypeLibrary()
	{
		FT_Done_FreeType(library);
	}

	//
	// Gets the global instance.
	//
	static FreeTypeLibrary &instance()
	{
		static FreeTypeLibrary instance;
		return instance;
	}

	// The freetype library object.
	FT_Library library;

private:
	//
	// Default constructor.
	//
	FreeTypeLibrary()
	{
		FT_Init_FreeType(&library);
	}
};

//---------------------------------------------------------------------
Font::Font() : face(nullptr)
{
}

//--------------------------------------------------------------------
Font::~Font()
{
	// destroy cached glyphs
	for (auto glyph : glyphs)
	{
		glDeleteTextures(1, &glyph.second->texture);
		delete glyph.second;
	}

	// destroy freetype resources
	if (face) FT_Done_Face(face);
}

//---------------------------------------------------------------------
void Font::loadFile(const std::string &fontname, unsigned height)
{
	auto lib = FreeTypeLibrary::instance().library;

	// destroy old font
	if (face)
	{
		// destroy cached glyphs
		for (auto glyph : glyphs)
		{
			glDeleteTextures(1, &glyph.second->texture);
			delete glyph.second;
		}

		// destroy freetype resources
		glyphs.clear();
		FT_Done_Face(face);
	}

	// load the font from the file
#if defined(__APPLE__)
#define FONTDIR "/Library/Fonts/"
#elif defined(_WINDOWS)
#define FONTDIR "C:\\Windows\\Fonts\\"
#else
#define FONTDIR ""
#endif
	if (FT_New_Face(lib, fontname.c_str(), 0, &face) && FT_New_Face(lib, (FONTDIR + fontname).c_str(), 0, &face))
	{
		face = nullptr;
		return;
	}

	// select charmap and font size
	if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) ||	FT_Set_Char_Size(face, height << 6, height << 6, 96, 96))
	{
		FT_Done_Face(face);
		face = nullptr;
		return;
	}

	// cache
	fontHeight = height;
	lineSpacing = face->size->metrics.height >> 6;
}

//---------------------------------------------------------------------
void Font::draw(const Vec2 &position, const std::wstring &text) const
{
	Vec2 v(position);

	// for each character
	for (auto c : text)
	{
		// check special characters
		if (c == '\n')
		{
			v.x = position.x;
			v.y -= lineSpacing;
			continue;
		}

		// get glyph
		const Glyph *glyph = getGlyph(c);
		if (!glyph) continue;

		// set position
		float oy = glyph->advance.y;

		// bing texture
		glBindTexture(GL_TEXTURE_2D, glyph->texture);
		
		// render
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0, 0);
			glVertex2f(v.x, v.y + oy + glyph->size.y);

			glTexCoord2f(0, glyph->texCoords.y);
			glVertex2f(v.x, v.y + oy);

			glTexCoord2f(glyph->texCoords.x, glyph->texCoords.y);
			glVertex2f(v.x + glyph->size.x, v.y + oy);

			glTexCoord2f(glyph->texCoords.x, 0);
			glVertex2f(v.x + glyph->size.x, v.y + oy + glyph->size.y);
		}
		glEnd();

		// next
		v.x += glyph->advance.x;
	}

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0U);
}

//---------------------------------------------------------------------
void Font::draw(const Rect &area, const std::wstring &text, int alignment) const
{
	const Glyph *glyph;
	std::list<float> lines;
	Vec2 v(0.f, (float)lineSpacing);

	// for each character
	for (size_t i = 0; i < text.length() && v.y < area.height(); ++i)
	{
		wchar_t c = text[i];
		glyph = nullptr;

		// search for line ending
		if (c == '\n' || ((glyph = getGlyph(c)) && v.x + glyph->advance.x > area.width()))
		{
			lines.push_back(v.x);
			v.x = 0;
			v.y += lineSpacing;
		}

		// advance one character
		if (glyph) v.x += glyph->advance.x;
	}

	// adjust ending
	if (v.x > 0.f)
		lines.push_back(v.x);
	else
		v.y -= lineSpacing;
	
	// apply vertical alignment
	if (alignment & VCenter)
		v.y = (area.height() - v.y) / 2 + area.top;
	else if (alignment & Bottom)
		v.y = area.height() - v.y + area.top;
	else
		v.y = area.top;

	// for each line
	unsigned i = 0;
	for (auto &line : lines)
	{
		// apply horizontal alignment
		if (alignment & Center)
			v.x = (area.width() - line) / 2 + area.left;
		else if (alignment & Right)
			v.x = area.width() - line + area.left;
		else
			v.x = area.left;

		// for each character
		while (line > FLT_EPSILON)
		{
			wchar_t c = text[i++];

			// get glyph
			if (c != L'\n' && (glyph = getGlyph(c)))
			{
				// set position
				float oy = glyph->advance.y;

				// bing texture
				glBindTexture(GL_TEXTURE_2D, glyph->texture);
		
				// render
				glBegin(GL_QUADS);
				{
					glTexCoord2f(0, 0);
					glVertex2f(v.x, v.y + oy + glyph->size.y);

					glTexCoord2f(0, glyph->texCoords.y);
					glVertex2f(v.x, v.y + oy);

					glTexCoord2f(glyph->texCoords.x, glyph->texCoords.y);
					glVertex2f(v.x + glyph->size.x, v.y + oy);

					glTexCoord2f(glyph->texCoords.x, 0);
					glVertex2f(v.x + glyph->size.x, v.y + oy + glyph->size.y);
				}
				glEnd();

				// next
				v.x += glyph->advance.x;
				line -= glyph->advance.x;
			}
		}

		// next line
		v.y -= lineSpacing;
	}

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0U);
}

//---------------------------------------------------------------------
Rect Font::measure(const std::wstring &text) const
{
	const Glyph *glyph;
	Rect rect = Rect::zero;
	float x = 0;

	// for each character
	for (auto c : text)
	{
		// check special characters
		if (c == '\n')
		{
			if (x > rect.right) rect.right = x;
			x = 0;

			rect.bottom += lineSpacing;
		}

		// regular character
		else if (glyph = getGlyph(c))
			x += glyph->advance.x;
	}

	// last line
	if (x > 0.f) rect.bottom += lineSpacing;
	if (x > rect.right) rect.right = x;

	// return
	return rect;
}

//---------------------------------------------------------------------
const Font::Glyph *Font::getGlyph(wchar_t c) const
{
	// verify if it's already loaded
	auto it = glyphs.find(c);
	if (it != glyphs.end()) return it->second;

	// load it
	Glyph glyph;

	if (!glyph.load(face, c))
		return nullptr;

	// push it into the cache
	auto p = glyphs.insert(std::pair<wchar_t, Glyph*>(c, new Glyph(glyph)));

	// return it
	return p.first->second;
}

//---------------------------------------------------------------------
inline unsigned _pow2(unsigned i)
{
	unsigned p2;
	for (p2 = 1; p2 < i; p2 <<= 1);
	return p2;
}

//---------------------------------------------------------------------
bool Font::Glyph::load(FT_Face face, wchar_t c)
{
	// try to load
	if (FT_Load_Char(face, c, FT_LOAD_TARGET_NORMAL))
		return false;

	FT_Glyph glyph;

	// get glyph description
    if (FT_Get_Glyph(face->glyph, &glyph))
		return false;

	// convert to bitmap data
	if (glyph->format != FT_GLYPH_FORMAT_BITMAP && FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1))
	{
		FT_Done_Glyph(glyph);
		return false;
	}

	// get bitmap
    FT_Bitmap &bitmap = ((FT_BitmapGlyph)glyph)->bitmap;

	// get power of 2 size
	int texSize = std::max(_pow2(bitmap.width), _pow2(bitmap.rows));

	// load pixels
	GLubyte *data = new GLubyte[2 * texSize * texSize];

	// copy pixels
	for (int j = 0; j < texSize; j++)
	{
		for (int i = 0; i < texSize; i++)
		{
			if (i >= bitmap.width || j >= bitmap.rows)
				data[2 * (i + j * texSize)] = data[2 * (i + j * texSize) + 1] = 0;
			else
				data[2 * (i + j * texSize)] = data[2 * (i + j * texSize) + 1] = bitmap.buffer[i + bitmap.width * j];
		}
	}

	// create its texture
	glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// fill the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA16, texSize, texSize, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);

	// delete pixel buffer
	delete data;

	// cache properties
	advance.x = (float)(face->glyph->advance.x >> 6);
	advance.y = (float)((face->glyph->metrics.horiBearingY - face->glyph->metrics.height) >> 6);

	size.x = (float)bitmap.width;
	size.y = (float)bitmap.rows;

	texCoords.x = (float)bitmap.width / texSize;
	texCoords.y = (float)bitmap.rows / texSize;

	// finish
	FT_Done_Glyph(glyph);
	return true;
}

//---------------------------------------------------------------------
class OpenALLibrary
{
public:
	//
	// Destructor.
	//
	~OpenALLibrary()
	{
		// unset
		alcMakeContextCurrent(0);

		// destroy
		if (context) alcDestroyContext(context);
		if (device) alcCloseDevice(device);
	}

	//
	// Gets the global instance.
	//
	static OpenALLibrary &instance()
	{
		static OpenALLibrary instance;
		return instance;
	}

	// OpenAL device handle.
	ALCdevice *device;

	// OpenAL context handle.
	ALCcontext *context;

private:
	//
	// Default constructor.
	//
	OpenALLibrary()
	{
		// create
		device = alcOpenDevice(0);
		context = alcCreateContext(device, 0);

		// set
		alcMakeContextCurrent(context);
	}
};

//---------------------------------------------------------------------
Sound::Sound() : alBuffer(0)
{
	OpenALLibrary::instance();

	// create source
	alGenSources(1, &alSource);
	alSourcei(alSource, AL_BUFFER, 0);
}

//---------------------------------------------------------------------
Sound::~Sound()
{
	// delete source
	alSourcei(alSource, AL_BUFFER, 0);
    alDeleteSources(1, &alSource);

	// delete buffer
	alDeleteBuffers(1, (ALuint*)&alBuffer);
}

//---------------------------------------------------------------------
void Sound::loadFile(const std::string &path)
{
	int format = 0;

	// open file
	stb_vorbis *ogg = stb_vorbis_open_filename(path.c_str(), nullptr, nullptr);
	if (!ogg) return;

	// get information
	stb_vorbis_info info = stb_vorbis_get_info(ogg);

	// calculate sample count
	unsigned nSamples = stb_vorbis_stream_length_in_samples(ogg) * (unsigned)info.channels;
	short *samples = new short[nSamples];

	// read the samples from the provided file
	if (stb_vorbis_get_samples_short_interleaved(ogg, info.channels, samples, (int)nSamples) * info.channels != nSamples)
	{
		delete samples;
		stb_vorbis_close(ogg);
		return;
	}

	// close file
	stb_vorbis_close(ogg);

	// check format
	if (info.channels == 1)
		format = AL_FORMAT_MONO16;
	else if (info.channels == 2)
		format = AL_FORMAT_STEREO16;
	else if (info.channels == 4)
		format = alGetEnumValue("AL_FORMAT_QUAD16");
	else if (info.channels == 6)
		format = alGetEnumValue("AL_FORMAT_51CHN16");
	else if (info.channels == 7)
		format = alGetEnumValue("AL_FORMAT_61CHN16");
	else if (info.channels == 8)
		format = alGetEnumValue("AL_FORMAT_71CHN16");

	// generate OpenAL buffer
	alGenBuffers(1, &alBuffer);

	// copy buffer
	ALsizei size = static_cast<ALsizei>(nSamples) * sizeof(short);
	alBufferData(alBuffer, format, samples, size, info.sample_rate);

	// delete stuff
	delete samples;

	// assign
	alSourcei(alSource, AL_BUFFER, (ALint)alBuffer);
}

//---------------------------------------------------------------------
void Sound::play()
{
	alSourcePlay(alSource);
}

//---------------------------------------------------------------------
BackgroundMusic::BackgroundMusic(const std::string &path) : abortThread(false)
{
	OpenALLibrary::instance();

	// open file
	stb_vorbis *ogg = stb_vorbis_open_filename(path.c_str(), nullptr, nullptr);

	// start streaming
	if (ogg) thread = std::thread([this, ogg] () { stream(ogg); });
}

//---------------------------------------------------------------------
BackgroundMusic::~BackgroundMusic()
{
	// signals destruction
	{
		std::lock_guard<std::mutex> guard(mutex);
		abortThread = true;
	}

	// wait for thread
	thread.join();
}

//---------------------------------------------------------------------
void BackgroundMusic::stream(void *file)
{
	stb_vorbis *ogg = (stb_vorbis*)file;
	ALint nBuffers, state;
	ALuint alSource;

	// create source
	alGenSources(1, &alSource);
	alSourcei(alSource, AL_BUFFER, 0);

	// get information
	stb_vorbis_info info = stb_vorbis_get_info(ogg);

	// cache properties
	unsigned channelCount = info.channels;
	unsigned sampleRate = info.sample_rate;
	unsigned sampleCount = stb_vorbis_stream_length_in_samples(ogg) * (unsigned)info.channels;
	unsigned soundFormat =  AL_FORMAT_MONO16;

	// check format
	if (info.channels == 2)
		soundFormat = AL_FORMAT_STEREO16;
	else if (info.channels == 4)
		soundFormat = alGetEnumValue("AL_FORMAT_QUAD16");
	else if (info.channels == 6)
		soundFormat = alGetEnumValue("AL_FORMAT_51CHN16");
	else if (info.channels == 7)
		soundFormat = alGetEnumValue("AL_FORMAT_61CHN16");
	else if (info.channels == 8)
		soundFormat = alGetEnumValue("AL_FORMAT_71CHN16");

	// initialize state
	unsigned queuedSamples = 0, sampleOffset = 0;

	// file buffer
	std::vector<short> fileBuffer;
	fileBuffer.resize(sampleRate * channelCount * bufferLength);

	// generate sound buffers
	ALuint alBuffers[bufferCount], alBuffer;
	alGenBuffers(bufferCount, alBuffers);

	// keep streaming
	while (!abortThread)
	{
		// lock
		mutex.lock();

		// check
		alGetSourcei(alSource, AL_SOURCE_STATE, &state);

		// playing
		if (state == AL_PLAYING && queuedSamples < sampleCount)
		{
			// get the number of buffers that have been processed (ie. ready for reuse)
			alGetSourcei(alSource, AL_BUFFERS_PROCESSED, &nBuffers);

			// while there're empty buffers
			while (nBuffers--)
			{
				// pop the first unused buffer from the queue
				alSourceUnqueueBuffers(alSource, 1, &alBuffer);

				// update sample offset
				ALint sizeInBytes, bits;

				alGetBufferi(alBuffer, AL_SIZE, &sizeInBytes);
				alGetBufferi(alBuffer, AL_BITS, &bits);

				sampleOffset += (sizeInBytes * 8) / bits;

				// read data
				unsigned samples = (unsigned)stb_vorbis_get_samples_short_interleaved(ogg, channelCount, &fileBuffer[0], (int)fileBuffer.size());

				samples *= channelCount;
				queuedSamples += samples;

				// fill the buffer
				if (samples > 0)
				{
					alBufferData(alBuffer, soundFormat, &fileBuffer[0], (ALsizei)samples * sizeof(short), sampleRate);

					// push into the buffer queue
					alSourceQueueBuffers(alSource, 1, &alBuffer);
				}

				// end of file?
				if (samples < (int)fileBuffer.size())
					break;
			}

			// end of file
			if (queuedSamples >= sampleCount)
			{
				// reset count
				queuedSamples = 0;

				// reset file
				stb_vorbis_seek_start(ogg);
			}

			// reset offset
			if (sampleOffset >= sampleCount)
				sampleOffset -= sampleCount;
		}

		// stopped
		else if (state == AL_STOPPED || state == AL_INITIAL)
		{
			// unqueue all buffers
			alSourcei(alSource, AL_BUFFER, 0);

			// prepare buffers
			for (nBuffers = 0; nBuffers < bufferCount;)
			{
				// read data
				unsigned samples = (unsigned)stb_vorbis_get_samples_short_interleaved(ogg, channelCount, &fileBuffer[0], (int)fileBuffer.size());

				samples *= channelCount;
				queuedSamples += samples;

				// fill the buffer
				if (samples > 0) alBufferData(alBuffers[nBuffers++], soundFormat, &fileBuffer[0], (ALsizei)samples * sizeof(short), sampleRate);

				// end of file?
				if (samples < (int)fileBuffer.size())
					break;
			}

			if (nBuffers > 0)
			{
				// set-up the queue
				alSourceQueueBuffers(alSource, nBuffers, alBuffers);

				// start playing
				alSourcePlay(alSource);
			}

			// end of file
			if (queuedSamples >= sampleCount)
			{
				// reset count
				queuedSamples = 0;

				// reset file
				stb_vorbis_seek_start(ogg);
			}
		}

		// unlock
		mutex.unlock();

		// sleep
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// lock
	std::lock_guard<std::mutex> guard(mutex);

	// stop the playback
	alSourceStop(alSource);

	// unqueue any buffer left in the queue
	alSourcei(alSource, AL_BUFFER, 0);

	// delete the buffers
	alDeleteBuffers(bufferCount, alBuffers);

	// close file
	stb_vorbis_close(ogg);

	// delete source
	alSourcei(alSource, AL_BUFFER, 0);
    alDeleteSources(1, &alSource);
}

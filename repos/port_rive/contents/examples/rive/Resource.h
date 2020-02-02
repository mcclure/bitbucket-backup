#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <memory>

#include "RMath.h"

namespace ld
{
	//
	// Represents a RGBA color.
	//
	struct Color
	{
		//
		// Default Constructor.
		//
		Color(float r = 0.f, float g = 0.f, float b = 0.f, float a = 1.f);

		//
		// Constructor from rgba value.
		//
		Color(unsigned rgba);

		// The channels.
		float r, g, b, a;
	};

	// Some useful colors
	const Color White(1.f, 1.f, 1.f);
	const Color Black(0.f, 0.f, 0.f);
	const Color Red(1.f, 0.f, 0.f);
	const Color Green(0.f, 1.f, 0.f);
	const Color Blue(0.f, 0.f, 1.f);
	const Color Magenta(1.f, 0.f, 1.f);
	const Color Yellow(1.f, 1.f, 0.f);
	const Color Cyan(0.f, 1.f, 1.f);

	//
	// Encapsulates an OpenGL texture.
	//
	class Texture
	{
	public:
		//
		// Default Constructor.
		//
		Texture();

		//
		// Destructor.
		//
		~Texture();

		//
		// Load an image file.
		//
		virtual void loadFile(const std::string &path);

		//
		// Draws a portion of this texture at a given region.
		//
		virtual void draw(const Rect &source, const Rect &destination) const;

	private:
		// OpenGL handle.
		unsigned handle;
	};

	//
	// Font.
	//
	class Font
	{
	public:
		//
		// Default constructor.
		//
		Font();

		//
		// Destructor.
		//
		~Font();

		//
		// Loads a font from a file.
		//
		void loadFile(const std::string &path, unsigned fontHeight);

		//
		// Draws the given text at the specified position.
		//
		void draw(const Vec2 &topLeft, const std::wstring &text) const;

		//
		// Draws the given text aligned inside the specified rectangle.
		//
		void draw(const Rect &destination, const std::wstring &text, int alignment = Top | Left) const;

		//
		// Measures the area taken by drawing a text.
		//
		Rect measure(const std::wstring &text) const;

		// Aligns text horizontally to the left.
		static const int Left = 0;

		// Aligns text horizontally to the center.
		static const int Center = 1;

		// Aligns text horizontally to the right.
		static const int Right = 2;

		// Aligns text vertically to the top.
		static const int Top = 0;

		// Aligns text vertically to the center.
		static const int VCenter = 4;

		// Aligns text vertically to the bottom.
		static const int Bottom = 8;

		// Centers text horizontally and vertically.
		static const int Middle = 5;

	private:
		// Glyph data.
		struct Glyph;

		//
		// Gets a character's glyph. Can be nullptr.
		//
		const Glyph *getGlyph(wchar_t) const;

		// The glyphs.
		mutable std::map<wchar_t, Glyph*> glyphs;

		// Height of the font.
		unsigned fontHeight;

		// The line spacing.
		unsigned lineSpacing;

		// The freetype face object.
		struct FT_FaceRec_ *face;
	};

	//
	// Sound.
	//
	class Sound
	{
	public:
		//
		// Default constructor.
		//
		Sound();

		//
		// Destructor.
		//
		~Sound();

		//
		// Loads a sound from a file.
		//
		void loadFile(const std::string&);

		//
		// Plays this object.
		//
		void play();

	private:
		// OpenAL source handle.
		unsigned alSource;

		// OpenAL buffer handle.
		unsigned alBuffer;		
	};

	//
	// Background Music.
	//
	class BackgroundMusic
	{
	public:
		//
		// Default constructor.
		//
		BackgroundMusic(const std::string&);

		//
		// Destructor.
		//
		~BackgroundMusic();

	private:
		//
		// Streams an ogg file.
		//
		void stream(void*);

		// Signals object destruction.
		bool abortThread;

		// The thread.
		std::thread thread;

		// The mutex.
		std::mutex mutex;
	};

	//---------------------------------------------------------------------
	template <class T> inline std::unique_ptr<T[]> allocate_unique(std::size_t n)
	{
		return std::unique_ptr<T[]>(new T[n]);
	}
}

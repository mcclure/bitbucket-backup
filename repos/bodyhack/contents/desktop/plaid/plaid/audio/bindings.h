#ifndef PLAIDGADGET_AUDIO_BINDINGS_H
#define PLAIDGADGET_AUDIO_BINDINGS_H

#include "../script/bindings.h"
#include "stream.h"


/*
	A header for registering the basic behaviors of all AudioStream classes;
		most notably casting to and from Sound handles, and reference counting.

	Factory functions and class-specific behaviors will need to be registered
		as usual.  Try to use property accessors where possible.
*/

namespace plaid
{
	//Easy macro:  EG.  AS_AUDIO_CLASS("Amp", Amp)
	#define AS_AUDIO_CLASS(TYPE, CPPTYPE) \
		AS_CHECK(plaid::registerAudioStream<CPPTYPE>(scripts,engine,TYPE))

	//The magical autocode for all this silliness...
	template <class T>
	int registerAudioStream(Scripts &scripts, asIScriptEngine *engine,
		const char *name)
	{
		scripts.include(L"Audio");

		//Sanity check
		{
			T *t = NULL;
			AudioStream *typeCheck = t;
		}

		//Internal code
		struct Functions
		{
			static Sound toSound(T *t) {return Sound(t);}
			static T *fromSound(const Sound &sound)
			{
				AudioStream *stream = sound;
				T *t = dynamic_cast<T*>(stream);
				return t;
			}
		};

		//Reference type basics
		AS_REFCOUNTED(name, T);

		//Implicit cast to Sound
		AS_IMPLICIT_CAST(name, "Sound f()",
			asFUNCTION(Functions::toSound), PGC_METHOD_OBJFIRST);

		//Construct from Sound
		/*std::string decl(name);
		decl.append("@ f(const Sound &in)");
		AS_FACTORY(name, decl.c_str(),
			asFUNCTION(Functions::fromSound), PGC_FUNCTION);*/

		return 0;
	}
}


#endif // PLAIDGADGET_AUDIO_BINDINGS_H

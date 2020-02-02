/**
    "Plaidgadget" game engine copyright (C) 2008-2009 Evan Balster

    This header file contains features globally significant to the game engine.

    - Memory Watch, which can assist in the tracking of memory leaks.
    - Error Reporting which can be expected to stop the engine.
	- Active Modules, which form the main architecture of plaidgadget.
	- Smart Pointers, which are generally useful and comply with memory watch.
	- Reflection, which provides lots of useful generic functionality.
	- Serialization, which is also generally useful and supports smart pointers.
*/

#ifndef PLAIDGADGET_MAIN_H
#define PLAIDGADGET_MAIN_H


#if PLAIDGADGET

#include <vector>
#include <list>
#include <map>
#include "util/types.h"
#include "util/memory.h"
#include "util/ref.h"

//Debug control
#define reportError(X) plaid::_reportError(X, __FILE__, __LINE__)
#define reportWarning(X) plaid::_reportWarning(X, __FILE__, __LINE__)
#define pgNote(X) plaid::_reportNote(X)


//Program error reporting

namespace plaid
{
	/*
		Various error reporting mechanisms -- reportError creates a "redscreen".
	*/
    void _reportError(std::string message, const char *fname, int line);
    void _reportWarning(std::string message, const char *fname, int line);
    void _reportNote(std::string message);
    void _reportError(String message, const char *fname, int line);
    void _reportWarning(String message, const char *fname, int line);
    void _reportNote(String message);
}



//Game engine code proper begins here!
//Anything included after this point gets memory-tracked if memwatch is active.


//Forward declare type_info class...
namespace std {class type_info;}

namespace plaid
{
	//Some classes Reflected needs to know about.
	class Examine;
	class Read;
	class Write;
	class SerialData;


	/*
		An object can extend Reflected and register a Type at static time to
			enable reflection and serialization at runtime.  Very useful!

		For classes that can't afford the 2 words of memory overhead,
			there will eventually be a means of static reflection.
	*/
	class Reflected : public RefCounted
	{
	public:
		/*
			Member objects represent member variables of a class.
				Declared in <plaid/reflect/reflect.h>.
		*/
		class Member;

		/*
			Describes a reflected type.
		*/
		class Type
		{
		public:
			//A cheap non-alphabetic comparator for STL maps and sets
			struct CheapCompare
			{
				bool operator()(const Type &l, const Type &r) const
					{return l.cheapCompare(r);}
			};

			typedef std::vector<Type*> Parents;
			typedef std::list<Member*> Members;
			typedef std::vector<Member*> FastMembers;

		protected:
			typedef std::map<String, Member*> MemberMap;
			typedef std::pair<String, Member*> MemberPair;
			typedef Reflected* (*CreatorFunction)(Read &read);

			class Data;

		public:
			//Default constructor for NULL type.
			Type() :   data(NULL) {}
			~Type() {}

			//Null type objects
			bool null() const      {return !data;}
			operator bool() const  {return data;}
			static Type Null()     {return Type();}

			/*
				Create a "blank" instance of the type.
					This function is mainly used by the serialization system.
					Some Types do not support this and will return NULL.
			*/
			Reflected *create(Read &read) const;

			/*
				Get some descriptive information about the type.
					version() relates to serialization.
					hash() is a direct hash of the value of name().
			*/
			String name() const;
			String description() const;
			Uint32 version() const;
			Uint64 hash() const;

			/*
				Get C++ RTTI information for this Type.
			*/
			const std::type_info &rtti() const;

			/*
				Get this Type's parent or a list thereof.
					parent() returns an arbitrary parent if there are multiple.
			*/
			Type parent() const;
			const Parents &parents() const;

			/*
				Returns whether the given type is this type or an acestor of it.
			*/
			bool is(Type _type) const;

			/*
				Look up members by name.
			*/
			Member *operator[](String name) const;

			/*
				Advanced member lookups used by the serialization system.
			*/
			const FastMembers &members() const;
			const FastMembers &membersStored() const;
			const FastMembers &membersSynced() const;
			Members membersStored(Uint32 version) const;
			Members membersSynced(Uint32 version) const;
			Members membersNaive(Uint32 version) const;

			/*
				Get a detailed description of a runtime object.
			*/
			String detail() const;

			/*
				Alphabetical and "cheap" (hash) ordering for Types.
			*/
			bool operator<(const Type &other) const;
			bool cheapCompare(const Type &o) const;

			/*
				Simple equality checking.
			*/
			bool operator==(const Type &o) const   {return o.data == data;}
			bool operator!=(const Type &o) const   {return o.data != data;}

			/*
				Look up a type by its name, hash or typeid value.
			*/
			static Type lookUp(const String &name);
			static Type lookUp(Uint64 hash);
			static Type lookUp(const std::type_info &info);

			/*
				Diagnostic function; prints all registed types to stdout.
			*/
			static void printTypes();

		protected:
			Type(Data *_data) : data(_data) {}

			static void add(Type type);

		protected:
			Data *data;
		};

	public:
		//Straightforward enough.
		Reflected() {}
		virtual ~Reflected() {}

		//For backwards compatibility with old saved objects of this type.
		virtual void serialConvert(SerialData &data) {}

		//Called after all members have been read by deserializer.
		virtual void deserialized() {}

		//Get Type object describing this object.
		virtual Type type() const = 0;
		bool is(Type _type) const;
	};

	typedef Reflected::Member Member;
	typedef Reflected::Type Type;


	/*
		Serializes to a size of zero.  Good for plugging holes.  Used in Shape.
	*/
	struct Nothing {};


	/*
		Represents runtime cache data attached to another object.  Used for
			things like OpenGL objects attached to shapes, et cetera.
	*/
	class CacheData : public RefCounted
	{
	public:
		virtual ~CacheData() {}
		virtual void invalidate() {}
	};


	/*
		A base class for Module which makes communication with the most
			useful built-in modules very easy.

		For more info on the modules (than the terse descriptions here) see the
			corresponding header files.
	*/
	class ModuleSet
	{
	public:
		class Program        &program;   // Controls startup/shutdown, main loop
		class Universe       &universe;  // Generic object manager/messager
		class Clock          &clock;     // Tracks various time metrics
		class Randomizer     &random;    // Generates random numbers

		class Graphics       &graphics;  // Rasterizes and displays visuals
		                                 //     See submodule: graphics.csg
		                                 //     See submodule: graphics.text
		class Audio          &audio;     // Used for audio processing/output

		class Player         &player;    // Handles most sources of user input
		                                 //     UI/console focus can block this
		const class Mouse    &mouse;     // Shorthand for player.mouse
		const class Keyboard &keyboard;  // Shorthand for player.keyboard
		const class Gamepad  &gamepad(int); // S-hand for player.gamepad(x)

		class Storage        &storage;   // Handles filesystem I/O
		class Network        &network;   // Online game and HTTP functionality

		class Console        &console;   // An interactive debug terminal
		class GUI            &gui;       // An extensible, flexible UI system

		class Scripts        &scripts;   // Runtime-compiled scripting.

	private:
		ModuleSet(class Program *source, bool headless);
		void cleanup();

		friend class Program;
	};


	/*
		Messages are used as a means of sending text commands to modules.
			More information available in <plaid/console/console.h>
	*/
	class Command;


	/*
		Represents a module which is updated at a fixed framerate.
	*/
	class Module : public ModuleSet
	{
	public:
		virtual ~Module() {}

		virtual void update() {}
		virtual void handle(Command &command) {}

	protected:
		/*
			You'll need to call this super constructor from your Module's.
		*/
		Module(const ModuleSet &modules) : ModuleSet(modules) {}

	private:
		friend class Program;
		friend class ModuleSet;
	};
}

#else //PLAIDGADGET

#include "util/types.h"
#include "util/memory.h"
#include "util/ref.h"

//No graphical error handling
#define reportError(X) throw(X)
#define reportWarning(X)
#define pgNote(X)

namespace plaid
{
	class Reflected {};
}

#endif //PLAIDGADGET


#endif //PLAIDGADGET_MAIN_H

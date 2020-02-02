#ifndef PLAIDGADGET_REF_H
#define PLAIDGADGET_REF_H


#ifndef NO_PLAIDGADGET
	#include "memory.h"
#endif


#include "traits.h"


/*
	Plaidgadget's Ref<T> class is the basis of a lightweight reference
		counting system used for memory management in the engine and any
		other applications which wish to utilize it.


	This header provides the classes Ref<T> and AutoRef<T>, which fulfill the
		same function with some slight differences.
		They each function as reference-counted pointers, binding to an object
		which has been dynamically allocated and keeping it alive until all Refs
		which refer to it are destroyed, at which point deletion occurs.

	Ref<T> only functions for T which derive from class RefCounted.
		It is safer, more efficient and more convenient to use.
		Try to use it wherever extending RefCounted is possible.

	AutoRef<T> is for other values of T, and uses an external reference count.
		There is a risk of multiple AutoRefs being constructed from a single T*,
		causing multiple independent reference counts and early deletion.
		In order to reduce this risk, AutoRefs must be explicitly constructed.


	A common trick is to code classes with private inner classes holding any
		member variables, and a smart pointer to the inner class as the sole
		member of the enclosing class.  The outer class becomes a reference
		to the inner one, and can be copied and destroyed at low cost.
		(This allows for C#-like semantics)

	Notably it's not portably possible to use a Ref as an "opaque pointer",
		as the Ref template is instantiated with instructions to delete an
		incomplete type when the last one is destroyed.  This might cause
		link errors or undefined behavior if allowed to compile.
		(You should get a warning, though...)
*/


namespace plaid
{
	//RefCount should be an efficient integer type.
	typedef unsigned int RefCount;


	/*
		This class is more efficient and safer, though by no means necessary,
			to use in combination with the Ref<T> class below.
			Multiple-inheritance can be used if another base is desired.
			(But beware diamond-inheritance of RefCounted itself!)

		Unlike other types, multiple Ref<T> can be instantiated from a pointer
			to a RefCounted subclass without issues of safety.
			Because the reference-count is inside the RefCounted, it is not
			possible for multiple reference counts to arise for one target.
			Such targets can even dispense Refs to themselves!

		A Ref with no template parameter is a Ref< RefCounted > and useful when
			a forward-declared class needs to be shared.
			Simply extend RefCounted from that class and optionally keep a raw
			pointer or reference in the holding object for convenience.
	*/
	class RefCounted
	{
	public:
		RefCounted() : _refs(0) {}

		//Use these only with great care; usually Ref<T> is safer and easier.
		void     _retain()         {++_refs;}
		void     _release()        {if (_refs) if (!--_refs) delete this;}
		RefCount _refCount() const {return _refs;}

	protected:
		// It's best practice to make destructors protected in these types.
		// But, if you don't, I doubt anyone will make a fuss about it.
		virtual ~RefCounted() {}

	private:
		template<typename T> friend class Ref;
		template<typename T> friend class AutoRef;
		friend class RefHelper;
		//virtual bool _surrogate() {return false;}
		RefCount _refs;
		enum {ImplementsRefCounting = 1};
	};


	/*
		A "surrogate" RefCounted that does the counting for objects that don't
			extend the RefCounted class.  DO NOT EXTEND.
	*/
	class RefHelper : public RefCounted
		{public: virtual ~RefHelper() {}
		private: template<typename T> friend class AutoRef;
		RefHelper() {} /*virtual bool _surrogate() {return true;}*/};


	/*
		The Pointer class emulates some behaviors of C++ pointers.
			It is the base class of AutoRef<T> and Ref<T>.

		It is not meant to be particularly useful itself.
	*/
	template<typename T>
	class Pointer
	{
	public:
		//Construct a Ptr.
		Pointer(T *ptr)                      : p(ptr) {}

		//Construct a null Ptr.
		Pointer()                            {}
		static Pointer<T> Null()             {return Pointer<T>();}

		//Explicitly check if this Ptr is null.
		bool null() const                    {return !p;}

		//Emulate behavior of pointers.
		operator T*  () const                {return p;}
		T *operator->() const                {return p;}
		T &operator* () const                {return *p;}

		//Pointer comparison.
		bool operator< (const Pointer<T> &o) const  {return (p <  o.p);}
		bool operator> (const Pointer<T> &o) const  {return (p >  o.p);}
		bool operator<=(const Pointer<T> &o) const  {return (p <= o.p);}
		bool operator>=(const Pointer<T> &o) const  {return (p >= o.p);}
		bool operator==(const Pointer<T> &o) const  {return (p == o.p);}
		bool operator!=(const Pointer<T> &o) const  {return (p != o.p);}
		bool operator< (const T*          o) const  {return (p <  o);}
		bool operator> (const T*          o) const  {return (p >  o);}
		bool operator<=(const T*          o) const  {return (p <= o);}
		bool operator>=(const T*          o) const  {return (p >= o);}
		bool operator==(const T*          o) const  {return (p == o);}
		bool operator!=(const T*          o) const  {return (p != o);}

	protected:
		//Assignment is non-public mainly to avoid possible errors with Refs.
		Pointer operator=(const Pointer &o)  {p = o.p; return *this;}

	protected:
		T *p;
	};


	/*
		This class implements Ref-counting where T extends RefCounted.
	*/
	template<typename T = RefCounted>
	class Ref : public Pointer<T>
	{
	private:
		//Ensure T inherits from RefCounted.
		enum {SanityCheck = T::ImplementsRefCounting};

	public:
		//Constructor, allowing implicit conversion
		Ref(T *ptr)                          : Pointer<T>(ptr) {retain();}

		//Construct null Refs
		Ref()                                : Pointer<T>(NULL) {}
		static Ref<T> Null()                 {return Ref<T>();}

		//Destructor
		~Ref()                               {release();}

		//Copying
		Ref(const Ref &o)                    : Pointer<T>(o.p) {retain();}
		template<typename D>
		Ref(const Ref<D> &o)                 : Pointer<T>(o.p) {retain();}
		Ref &operator=(const Ref &o)
			{o.retain(); release(); this->p=o.p; return *this;}


		//Use this in lieu of dynamic_cast to keep Refs coherent.
		//  Like a dynamic_cast, it might produce a NULL Ref.
		template<typename D>
		Ref<D> dynamicCast() const
			{D *d = dynamic_cast<D*>(this->p);
			return d ? Ref<D>(d) : Ref<D>();}

		//And a static cast, for friendly compile errors when misused.
		template<typename D>
		Ref<D> staticCast() const
			{return Ref<D>(static_cast<D*>(this->p));}

		//Query remaining references
		RefCount count() const            {return this->p ? this->p->_refs : 0;}
		bool     last()  const            {return this->p && this->p->_refs==1;}

	private:
		template<typename D> friend class Ref;

		void retain() const       {if (this->p) ++this->p->_refs;}
		void release()            {if (!this->p || --this->p->_refs) return;
		/*  */                     delete this->p; this->p=NULL;}
	};


	/*
		This class implements Ref-counting where T doesn't extend RefCounted.
	*/
	template<typename T>
	class AutoRef : public Pointer<T>
	{
	private:
		//Ensure T doesn't extend RefCounted.
		//  If it does, why in the heck would you use an AutoRef?
		PG_RULE( (Relation<T, RefCounted>::Derives == 0) );

	public:
		//This constructor is explicit, so you know when you use it.
		//  If you bind to a T* twice, expect to have some serious errors.
		explicit AutoRef(T *ptr)             : Pointer<T>(ptr),  r(NULL)
			{if (this->p) {r=new RefHelper; retain();} else r=NULL;}

		//Construct null AutoRefs
		AutoRef()                            : Pointer<T>(NULL), r(NULL) {}
		static AutoRef<T> Null()             {return AutoRef<T>();}

		//Destructors
		~AutoRef()                           {release();}

		//Copying
		AutoRef(const AutoRef &o)            : Pointer<T>(o.p),r(o.r){retain();}
		template<typename D>
		AutoRef(const AutoRef<D> &o)         : Pointer<T>(o.p),r(o.r){retain();}
		AutoRef &operator=(const AutoRef &o)
			{o.retain(); release(); this->p=o.p; r=o.r; return *this;}

		//Use this in lieu of dynamic_cast to keep AutoRefs coherent.
		//  Like a dynamic_cast, it might produce a NULL AutoRef.
		template<typename D>
		AutoRef<D> dynamicCast() const
			{D *d = dynamic_cast<D*>(this->p);
			return d ? AutoRef<D>(d, r) : AutoRef<D>();}

		//And a static cast, for friendly compile errors when misused.
		template<typename D>
		AutoRef<D> staticCast() const
			{return AutoRef<D>(static_cast<D*>(this->p), r);}

		//Query remaining references
		RefCount count() const               {return r ? r->_refs : 0;}
		bool     last()  const               {return r && r->_refs==1;}

		//Comparisons
		/*bool operator< (const AutoRef<T> &o) const  {return (p <  o.p);}
		bool operator> (const AutoRef<T> &o) const  {return (p >  o.p);}
		bool operator<=(const AutoRef<T> &o) const  {return (p <= o.p);}
		bool operator>=(const AutoRef<T> &o) const  {return (p >= o.p);}
		bool operator==(const AutoRef<T> &o) const  {return (p == o.p);}
		bool operator!=(const AutoRef<T> &o) const  {return (p != o.p);}*/

	private:
		template<typename D> friend class AutoRef;

		AutoRef(T *_p, RefCounted *_r)       : Pointer<T>(_p), r(_r) {retain();}

		void retain() const                  {if (r) ++r->_refs;}
		void release()                   {if (!r || --r->_refs) return;
		/* ... */                         delete this->p; delete r;
		/* ... */                         this->p=NULL; r=NULL;}

		RefCounted *r;
	};
}


#endif // PLAIDGADGET_REF_H

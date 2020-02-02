#ifndef PLAIDGADGET_ATTRIBUTE_H
#define PLAIDGADGET_ATTRIBUTE_H


/*
	These macros provide a primitive means of specifying 'attribute' members of
		a class.  Placed in a union with an object of type INSIDE, they expose
		indirectly an object of type OUTSIDE  by calling an EXPRESSION which
		deduces the latter from the former with the former called 'X'.
*/
namespace plaid // (for decoration)
{
	/*
		This attribute acts like a pointer to an object of type OUTSIDE,
			or a parameterless function which retreives a reference.
	*/
	#define PGAttribute_Pointer(INSIDE, OUTSIDE, EXPRESSION) \
		struct {INSIDE X;\
		OUTSIDE* operator->() {return &(EXPRESSION);} \
		OUTSIDE& operator*() {return (EXPRESSION);} \
		operator OUTSIDE*() {return &(EXPRESSION);} \
		operator OUTSIDE&() {return (EXPRESSION);} }

	/*
		This type is like the last one but lets you add extra functionality.
	*/
	#define PGAttribute_PointerExt(INSIDE, OUTSIDE, EXPRESSION, EXTRA) \
		struct {INSIDE X;\
		OUTSIDE* operator->() {return &(EXPRESSION);} \
		OUTSIDE& operator*() {return (EXPRESSION);} \
		operator OUTSIDE*() {return &(EXPRESSION);} \
		operator OUTSIDE&() {return (EXPRESSION);} \
		EXTRA }

	//OUTSIDE& operator()() {return (EXPRESSION);}
}

#endif // PLAIDGADGET_ATTRIBUTE_H

#ifndef PG_HASHMAP_H_INCLUDED
#define PG_HASHMAP_H_INCLUDED


#include "../core.h"

/*
	A templated map class which uses plaidgadget (wstring) Strings as keys.
*/

namespace plaid
{
	template<class _Value>
	class StringMap
	{
	public:
		class Iterator
		{
		};

	public:
		StringMap()

	private:
		struct Node
		{

			//Node *next;
		};

	}
}


#endif // PG_HASHMAP_H_INCLUDED

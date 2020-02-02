#ifndef PLAIDGADGET_TRAITS_H
#define PLAIDGADGET_TRAITS_H

/*
	A witch's brew of strange and arcane template magicks.
*/

#define PG_JOIN(X,Y) PG_JOIN2(X,Y)
#define PG_JOIN2(X,Y) X##Y

//We call compile-time asserts 'rules', because it's
namespace pg_rules
{
	template <bool> struct STATIC_ASSERT;
	template <> struct STATIC_ASSERT<true> { enum { value = 1 }; };
	template<int x> struct CHECK{};
}

#define PG_RULE(x) \
	typedef ::pg_rules::CHECK<\
		sizeof(::pg_rules::STATIC_ASSERT< (bool)( x ) >)>\
			PG_JOIN(_pg_rule_, __LINE__)

namespace plaid
{
	//  Relation < A, B >
	//  Facilitates examining of the relationship between two types.

	//  Properties:
	//  Relation<A,B>::Derives     -  A is equal to or derived from B.
	//  Relation<A,B>::IsDerivedBy -  B is equal to or derived from A.
	//  Relation<A,B>::Equal       -  A is the same type as B.
	//  Relation<A,B>::Child       -  A derives B, but is not B.
	//  Relation<A,B>::Parent      -  B derives A, but is not A.

	template<class A, class B>
	class Relation
	{
	private:
		//The mysterious hacky guts
		class Yes {char a[1];}; class No {char a[10];};
		static Yes TestB(B*); static No TestB(...);
		static Yes TestA(A*); static No TestA(...);

	public:
		enum
		{
			Derives     = (sizeof(TestB(static_cast<A*>(0)))==sizeof(Yes))?1:0,
			IsDerivedBy = (sizeof(TestA(static_cast<B*>(0)))==sizeof(Yes))?1:0,
			Equal = (Derives & IsDerivedBy)?1:0,
			Child = (Derives & !IsDerivedBy)?1:0,
			Parent = (!Derives & IsDerivedBy)?1:0,
		};
	};


	//  IsSame < A, B >
	//  Compares two types
}

#endif //PLAIDGADGET_TRAITS_H

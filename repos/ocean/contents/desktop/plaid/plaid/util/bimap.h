#ifndef PG_BIMAP_H_INCLUDED
#define PG_BIMAP_H_INCLUDED


#include <map>


template<typename _A, typename _B,
	typename _AC = std::less<_A>, typename _BC = std::less<_B> >
class BiMap
{
public:
	typedef std::map<_A, _B, _AC> Forward;
	typedef std::pair<_A, _B> ForwardPair;
	typedef std::map<_B, _A, _BC> Backward;
	typedef std::pair<_B, _A> BackwardPair;

	typedef typename Forward::iterator iterator;
	typedef typename Forward::const_iterator const_iterator;
	typedef typename Backward::iterator anti_iterator;
	typedef typename Backward::const_iterator const_anti_iterator;

public:
	BiMap() {}

	void insert(const _A &a, const _B &b)
		{forward.insert(ForwardPair(a, b));
		backward.insert(BackwardPair(b, a));}

	_B &operator[](const _A &a)
		{return forward[a];}
	const _B &operator[](const _A &a) const
		{return forward[a];}

	_A &operator[](const _B &b)
		{return backward[b];}
	const _A &operator[](const _B &b) const
		{return backward[b];}

	iterator begin()
		{return forward.begin();}
	const_iterator begin() const
		{return forward.begin();}
	anti_iterator anti_begin()
		{return backward.begin();}
	const_anti_iterator anti_begin() const
		{return backward.begin();}

	iterator end()
		{return forward.end();}
	const_iterator end() const
		{return forward.end();}
	anti_iterator anti_end()
		{return backward.end();}
	const_anti_iterator anti_end() const
		{return backward.end();}

	size_t size() const
		{return forward.size();}

	iterator find(const _A &a)
		{return forward.find(a);}
	anti_iterator find(const _B &b)
		{return backward.find(b);}

	iterator findA(const _A &a)
		{return forward.find(a);}
	anti_iterator findB(const _B &b)
		{return backward.find(b);}

private:
	Forward forward;
	Backward backward;
};



#endif // PG_BIMAP_H_INCLUDED

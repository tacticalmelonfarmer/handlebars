#pragma once

#include "utility.hpp"
#include "typelist.hpp"
#include <exception>

namespace utility
{

template <size_t Index, typename T, typename ... Ts>
struct variant_deleter
{
	variant_deleter<Index + 1, Ts...> next;
	inline constexpr void operator()(size_t HeldIndex, void* HeldPtr) const
	{
		if(Index == HeldIndex)
			delete static_cast<T*>(HeldPtr);
		else
			next(HeldIndex, HeldPtr);
	}
};

template <size_t Index, typename T>
struct variant_deleter<Index, T>
{
	inline constexpr void operator()(size_t HeldIndex, void* HeldPtr) const
	{
		if(Index == HeldIndex)
			delete static_cast<T*>(HeldPtr);
		// TODO: possibly throw on error, but i dont think such error is likely
		else
			throw bad_variant_access("warning/non-fatal: attempted deletion of empty variant");
	}
};

struct bad_variant_access : public std::runtime_error {};

template <typename T, typename ... Ts>
struct variant
{
	typedef typelist<T, Ts...> types;

    variant()
        : held_(sizeof...(Ts) + 1)
    {}
    template <typename I>
    variant(I Init)
        : union_(new typename tl_type_at< tl_index_of<I, types>::index, types >::type())
        , held_(tl_index_of<I, types>::index)
	{}
	~variant()
	{ variant<T, Ts...>::deleter_(held_, union_); }

	bool empty() const
	{ return held_ == sizeof...(Ts) + 1 }
		
	template <typename P>
	variant<T, Ts...>& operator=(const P& rhs)
	{
		if(!empty())
			variant<T, Ts...>::deleter_(held_, union_);
		union_ = new(union_) P(rhs);
		held_ = tl_index_of<P, types>::index;
		return *this;
	}

    template <typename G>
    G& get()
    {
		if(!empty())
		{
			if(held_ == tl_index_of<G, types>::index)
				return *static_cast<G*>(union_);
			else
				throw bad_variant_access("variant does not hold the requested type");
		}
		else
			throw bad_variant_access("variant is empty");
    }
private:
    void* union_;
	size_t held_;
	static const variant_deleter<0, T, Ts...> deleter_;
};

}
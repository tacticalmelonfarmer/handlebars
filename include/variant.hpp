#pragma once

#include "utility.hpp"
#include "typelist.hpp"
#include <exception>
#include <iostream>

namespace utility
{

struct bad_variant_access : public std::runtime_error
{
	bad_variant_access(const char* what) : std::runtime_error(what) {}
};

template <size_t Index, typename T, typename ... Ts>
struct variant_deleter
{
	variant_deleter<Index + 1, Ts...> next;
	inline void operator()(size_t HeldIndex, void* HeldPtr) const
	{
		if(Index == HeldIndex)
		{
			if constexpr(std::is_reference<T>::value)
				delete static_cast<uncvref_t<T>*>(HeldPtr);
			else
				delete static_cast<T*>(HeldPtr);
		}
		else
			next(HeldIndex, HeldPtr);
	}
};

template <size_t Index, typename T>
struct variant_deleter<Index, T>
{
	inline void operator()(size_t HeldIndex, void* HeldPtr) const
	{
		if(Index == HeldIndex)
		{
			if constexpr(std::is_reference<T>::value)
				delete static_cast<uncvref_t<T>*>(HeldPtr);
			else
				delete static_cast<T*>(HeldPtr);
		}
		else
			throw bad_variant_access("[warning/non-fatal]: attempted deletion of empty variant");
	}
};

template <typename T, typename ... Ts>
struct variant
{
	typedef typelist<T, Ts...> types;
	typedef variant_deleter<0, T, Ts...> deleter_type;
	static constexpr size_t invalid_index = sizeof...(Ts) + 1; // one past the last type

    variant()
		: held_(invalid_index)
    {}
    template <typename I>
    variant(I Init)
        : union_(new typename tl_type_at< tl_index_of<I, types>::index, types >::type())
        , held_(tl_index_of<I, types>::index)
	{}
	~variant()
	{ if(!empty()) variant<T, Ts...>::deleter_(held_, union_); }

	bool empty() const
	{ return held_ == invalid_index; }

	void clear()
	{
		if(!empty())
		{
			deleter_(held_, union_);
			held_ = invalid_index;
		}
	}

	template <typename C>
	bool is_type() const
	{ return tl_index_of<C, types>::index == held_; }
		
	template <typename P>
	variant<T, Ts...>& operator=(P&& rhs)
	{
		clear();
		if constexpr(tl_has_type<P, types>::value)
		{
			if constexpr(std::is_reference<P>::value)
			{
				union_ = new uncvref_t<P>*(&rhs);
				held_ = tl_index_of<P, types>::index;
			}
			else
			{
				union_ = new P(rhs);
				held_ = tl_index_of<P, types>::index;
			}
		}
		else if constexpr(tl_has_conversion<P, types>::value)
		{
			union_ = new typename tl_find_conversion<P, types>::type(rhs);
			held_ = tl_index_of<typename tl_find_conversion<P, types>::type, types>::index;
		}
		else
		{
			static_assert(tl_has_type<P, types>::value || tl_has_conversion<P, types>::value, "could not find suitable conversion in variant assignment");
		}
		return *this;
	}

    template <typename G>
    G& get()
    {
		if(!empty())
		{
			if(held_ == tl_index_of<G, types>::index)
			{
				if constexpr(std::is_reference<G>::value)
					return **static_cast<uncvref_t<G>**>(union_);
				else
					return *static_cast<G*>(union_);
			}
			else
				throw bad_variant_access("variant does not hold the requested type");
				
		}
		else
			throw bad_variant_access("variant is empty");
	}
	
	template <typename G>
	const G& get() const
	{ return get<G>(); }
private:
    void* union_;
	size_t held_;
	static constexpr deleter_type deleter_ = deleter_type();
};

}
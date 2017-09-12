#pragma once

#include "utility.hpp"

namespace utility
{

template <typename P, size_t Index, typename T, typename ... Ts>
struct tuple_impl // body
{
	typedef T type;
	typedef tuple_impl<tuple_impl<P, Index, T, Ts...>, Index + 1, Ts...> next_type;
	typedef P previous_type;
	enum { index = Index, begin = 0, end = 0 };
	T value;
	next_type *next;
	previous_type *previous;

	tuple_impl(previous_type* Previous, T Value, Ts... Others) : value(Value), next(new next_type(this, Others...)), previous(Previous) {}
};

template <typename P, size_t Index, typename T>
struct tuple_impl<P, Index, T>  // tail
{
	typedef T type;
	typedef P previous_type;
	enum { index = Index, begin = 0, end = 1 };
	T value;
	previous_type *previous;

	tuple_impl(previous_type* Previous, T Value) : value(Value), previous(Previous) {}
};

template <typename ... Ts>
struct tuple; // head

template <typename T>
struct tuple<T> // single type, head body and tail
{
	typedef T type;
	enum { index = 0, begin = 1, end = 1, size = 1 };
	T  value;

	tuple(T Value) : value(Value) {}
	tuple() : value(T()) {}
};

template <typename T, typename ... Ts>
struct tuple<T, Ts...>
{
	typedef T type;
	typedef tuple_impl<tuple<T, Ts...>, 1, Ts...> next_type;
	enum { index = 0, begin = 1, end = 0, size = sizeof...(Ts) + 1 };
	T value;
	next_type *next;

	tuple(T Value, Ts... Others) : value(Value), next(new next_type(this, Others...)) {}
	tuple() : value(T()), next(new next_type(this, Ts()...)) {}
};

struct tuple_index_out_of_bounds {} tuple_index_out_of_bounds;

template <size_t Index, typename Tuple>
auto& tuple_get(Tuple& From)
{
	if constexpr(Tuple::index == Index)
		return From.value;
	else if constexpr(!Tuple::end)
		return tuple_get<Index/*, typename Tuple::next_type*/>(*From.next);
	else 
		return tuple_index_out_of_bounds;
}

template <typename F, typename Tuple, size_t ...S >
decltype(auto) tuple_apply_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
	return std::forward<F>(fn)(tuple_get<S>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
decltype(auto) tuple_apply(F&& fn, Tuple&& t)
{
	std::size_t constexpr tSize = uncvref_t<Tuple>::size;

	return tuple_apply_impl(std::forward<F>(fn),
	                        std::forward<Tuple>(t),
	                        std::make_index_sequence<tSize>());
}

template <typename F, typename Tuple, typename ReturnTuple, size_t Index>
void tuple_foreach_impl(F&& func, Tuple&& tup, ReturnTuple& result)
{
	size_t constexpr tSize = uncvref_t<Tuple>::size;
	if constexpr(Index == tSize - 1)
	{
		tuple_get<Index>(result) = func(tuple_get<Index>(tup));
	}
	else
	{
		tuple_get<Index>(result) = func(tuple_get<Index>(tup));
		tuple_foreach_impl<F, Tuple, ReturnTuple, Index + 1>(std::forward<F>(func), std::forward<Tuple>(tup), result);
	}
}

template <typename F, typename Tuple, typename ReturnTuple = Tuple>
uncvref_t<ReturnTuple> tuple_foreach(F&& func, Tuple&& tup)
{
	size_t constexpr tSize = uncvref_t<Tuple>::size;
	size_t constexpr rtSize = uncvref_t<ReturnTuple>::size;
	static_assert(tSize == rtSize, "Tuple must be same size as ReturnTuple");

	uncvref_t<ReturnTuple> result;
	tuple_get<0>(result) = func(tuple_get<0>(tup));

	tuple_foreach_impl<F, Tuple, uncvref_t<ReturnTuple>, 1>(std::forward<F>(func), std::forward<Tuple>(tup), result);

	return result;
}

template <typename F, typename Tuple, size_t Index>
void tuple_foreach_noreturn_impl(F&& func, Tuple&& tup)
{
	size_t constexpr tSize = uncvref_t<Tuple>::size;
	if constexpr(Index == tSize - 1)
	{
		func(tuple_get<Index>(tup));
	}
	else
	{
		func(tuple_get<Index>(tup));
		tuple_foreach_noreturn_impl<F, Tuple, Index + 1>(std::forward<F>(func), std::forward<Tuple>(tup));
	}
}

template <typename F, typename Tuple>
void tuple_foreach_noreturn(F&& func, Tuple&& tup)
{
	func(tuple_get<0>(tup));
	tuple_foreach_noreturn_impl<F, Tuple, 1>(std::forward<F>(func), std::forward<Tuple>(tup));
}

}
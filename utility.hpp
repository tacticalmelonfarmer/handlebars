#pragma once
#include <cstddef>
#include <type_traits>
#include <utility>
#include <functional>
#include <tuple>

namespace utility
{

template <typename T>
using uncvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename R, typename ... ArgTs>
size_t get_address(std::function<R(ArgTs...)> f) {
    typedef R (fnType)(ArgTs...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

template <bool E>
struct ct_require;

template <>
struct ct_require<true>
{};

template <bool E, typename T, typename F>
struct ct_if_else;

template <typename T, typename F>
struct ct_if_else<false, T, F>
{
	typedef F type;
};

template <typename T, typename F>
struct ct_if_else<true, T, F>
{
	typedef T type;
};

template <size_t Index, typename ... Ts>
struct ct_select;

template <typename T, typename ... Ts>
struct ct_select<0, T, Ts...>
{
	typedef T type;
};

template <size_t Index, typename T, typename ... Ts>
struct ct_select<Index, T, Ts...>
{
	typedef typename ct_select<Index - 1, Ts...>::type type;
};

template <size_t ... Indices>
struct ct_index_range { enum{ size = sizeof...(Indices) }; };

template <size_t Begin, size_t End, size_t Position = 0, bool AddIndex = Begin == 0, bool AtEnd = Position == End, size_t ... Result>
struct ct_make_index_range;

template <size_t Begin, size_t End, size_t Position, size_t ... Result>
struct ct_make_index_range<Begin, End, Position, /*AddIndex=*/false, /*AtEnd*/true, Result...>
{
	typedef ct_index_range<Result...> type;
};

template <size_t Begin, size_t End, size_t Position, size_t ... Result>
struct ct_make_index_range<Begin, End, Position, /*AddIndex=*/true, /*AtEnd*/true, Result...>
{
	typedef ct_index_range<Result..., Position> type;
};

template <size_t Begin, size_t End, size_t Position, size_t ... Result>
struct ct_make_index_range<Begin, End, Position, /*AddIndex=*/false, /*AtEnd*/false, Result...>
{
	static constexpr size_t add_next_index = Position + 1 >= Begin && Position + 1 <= End;
	static constexpr size_t next_is_last = Position + 1 == End;
	typedef typename ct_make_index_range<Begin, End, Position + 1, add_next_index, next_is_last, Result...>::type type;
};

template <size_t Begin, size_t End, size_t Position, size_t ... Result>
struct ct_make_index_range<Begin, End, Position, /*AddIndex=*/true, /*AtEnd*/false, Result...>
{
	static constexpr size_t add_next_index = Position + 1 >= Begin && Position + 1 <= End;
	static constexpr size_t next_is_last = Position + 1 == End;
	typedef typename ct_make_index_range<Begin, End, Position + 1, add_next_index, next_is_last, Result..., Position>::type type;
};

template <size_t Begin, size_t End, size_t ... Result>
struct ct_make_index_range<Begin, End, /*Position=*/0, /*AddIndex=*/false, /*AtEnd*/false, Result...>
{
	static constexpr size_t add_next_index = 1 >= Begin && 1 <= End;
	static constexpr size_t next_is_last = 1 == End;
	typedef typename ct_make_index_range<Begin, End, 1, add_next_index, next_is_last, Result...>::type type;
};

template <size_t Begin, size_t End, size_t ... Result>
struct ct_make_index_range<Begin, End, /*Position=*/0, /*AddIndex=*/true, /*AtEnd*/false, Result...>
{
	static constexpr size_t add_next_index = 1 >= Begin && 1 <= End;
	static constexpr size_t next_is_last = 1 == End;
	typedef typename ct_make_index_range<Begin, End, 1, add_next_index, next_is_last, 0>::type type;
};

template <typename TL, size_t Size = 0>
struct tl_min_size;

template <template <typename...> typename TL, typename T_deduce, typename ... TL_deduce, size_t Size>
struct tl_min_size<TL<T_deduce, TL_deduce...>, Size>
{
	static constexpr size_t current_size = sizeof(T_deduce);
	static constexpr bool comparison = current_size < Size;
	static constexpr size_t size = (TL<T_deduce, TL_deduce...>::end ? (comparison ? current_size : Size) : tl_min_size<TL<TL_deduce...>, (comparison ? current_size : Size)>::size);
};

template <typename TL, size_t Size = 0>
struct tl_max_size;

template <template <typename...> typename TL, typename T_deduce, typename ... TL_deduce, size_t Size>
struct tl_max_size<TL<T_deduce, TL_deduce...>, Size>
{
	static constexpr size_t current_size = sizeof(T_deduce);
	static constexpr bool comparison = current_size > Size;
	static constexpr size_t size = (TL<T_deduce, TL_deduce...>::end ? (comparison ? current_size : Size) : tl_max_size<TL<TL_deduce...>, (comparison ? current_size : Size)>::size);
};

template <typename ... Ts>
struct typelist { enum{ size = sizeof...(Ts) }; };

template <template <typename...> typename ApplyTo, typename TL>
struct tl_apply;

template <template <typename...> typename ApplyTo, template <typename...> typename TL, typename ... TL_deduce>
struct tl_apply<ApplyTo, TL<TL_deduce...>>
{
	typedef ApplyTo<TL_deduce...> type;
};

template <template <typename...> typename ApplyTo, typename TL, typename ... Post>
struct tl_apply_before;

template <template <typename...> typename ApplyTo, template <typename...> typename TL, typename ... TL_deduce, typename ... Post>
struct tl_apply_before<ApplyTo, TL<TL_deduce...>, Post...>
{
	typedef ApplyTo<TL_deduce..., Post...> type;
};

template <template <typename...> typename ApplyTo, typename TL, typename ... Pre>
struct tl_apply_after;

template <template <typename...> typename ApplyTo, template <typename...> typename TL, typename ... TL_deduce, typename ... Pre>
struct tl_apply_after<ApplyTo, TL<TL_deduce...>, Pre...>
{
	typedef ApplyTo<Pre..., TL_deduce...> type;
};

template <size_t Index, typename TL>
struct tl_type_at;

template <size_t Index, template <typename...> typename TL, typename ... TL_deduce>
struct tl_type_at<Index, TL<TL_deduce...>>
{
	static_assert(Index < TL<TL_deduce...>::size, "typelist index out of bounds");
	typedef typename ct_select<Index, TL_deduce...>::type type;
};

template <typename TL, typename ... NewTypes>
struct tl_push_back;

template <template <typename...> typename TL, typename ... TL_deduce, typename ... NewTypes>
struct tl_push_back<TL<TL_deduce...>, NewTypes...>
{
	typedef typename tl_apply_before<typelist, TL<TL_deduce...>, NewTypes...>::type type;
};

template <typename TL, typename ... NewTypes>
struct tl_push_front;

template <template <typename...> typename TL, typename ... TL_deduce, typename ... NewTypes>
struct tl_push_front<TL<TL_deduce...>, NewTypes...>
{
	typedef typename tl_apply_after<typelist, TL<TL_deduce...>, NewTypes...>::type type;
};

template <typename TL>
struct tl_back
{
	typedef typename tl_type_at<TL::size - 1, TL>::type type;
};

template <typename TL>
struct tl_front
{
	typedef typename tl_type_at<0, TL>::type type;
};

template <typename TL, size_t Begin, size_t End, size_t Position = 0, bool AtEnd = false, bool AddType = (Begin == 0), typename ... Result>
struct tl_subrange;

template <typename TL, size_t Begin, size_t End, size_t Position, typename ... Result>
struct tl_subrange<TL, Begin, End, Position, true, true, Result...> // end
{
	typedef typelist<Result..., typename tl_type_at<Position, TL>::type> type;
};

template <typename TL, size_t Begin, size_t End, size_t Position, typename ... Result>
struct tl_subrange<TL, Begin, End, Position, true, false, Result...> // end
{
	typedef typelist<Result...> type;
};

template <typename TL, size_t Begin, size_t End, size_t Position, typename ... Result>
struct tl_subrange<TL, Begin, End, Position, false, true, Result...>
{
	static constexpr bool add_next_type = Position + 1 >= Begin && Position + 1 <= End;
	static constexpr bool next_is_last = Position + 1 == End;
	typedef typename tl_subrange<TL, Begin, End, Position + 1, next_is_last, add_next_type, Result..., typename tl_type_at<Position, TL>::type>::type type;
};

template <typename TL, size_t Begin, size_t End, size_t Position, typename ... Result>
struct tl_subrange<TL, Begin, End, Position, false, false, Result...>
{
	static constexpr bool add_next_type = Position + 1 >= Begin && Position + 1 <= End;
	static constexpr bool next_is_last = Position + 1 == End;
	typedef typename tl_subrange<TL, Begin, End, Position + 1, next_is_last, add_next_type, Result...>::type type;
};

template <typename TL, size_t Begin, size_t End, typename ... Result>
struct tl_subrange<TL, Begin, End, 0, false, true, Result...> // entry add type
{
	static constexpr bool Position = 0;
	static constexpr bool add_next_type = Begin <= 1;
	static constexpr bool next_is_last = End == 1;
	typedef typename tl_subrange<TL, Begin, End, Position + 1, next_is_last, add_next_type, typename tl_type_at<Position, TL>::type>::type type;
};

template <typename TL, size_t Begin, size_t End, typename ... Result>
struct tl_subrange<TL, Begin, End, 0, false, false, Result...> // entry without adding type
{
	static constexpr bool Position = 0;
	static constexpr bool add_next_type = Begin == 1;
	static constexpr bool next_is_last = End == 1;
	typedef typename tl_subrange<TL, Begin, End, Position + 1, next_is_last, add_next_type>::type type;
};

template <typename TL1, typename TL2>
struct tl_join;

template <template <typename> typename TL1, typename ... TL1_deduce, template <typename> typename TL2, typename ... TL2_deduce>
struct tl_join<TL1<TL1_deduce...>, TL2<TL2_deduce...>>
{
	typedef typelist<TL1_deduce..., TL2_deduce...> type;
};

template <typename TLDest, size_t Index, typename TLSource>
struct tl_insert
{
	typedef typename tl_subrange<TLDest, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TLDest, Index, TLDest::size - first_partition::size - 1>::type last_partition;
	typedef typename tl_join<first_partition, typename tl_join<TLSource, last_partition>::type>::type type;
};

template <typename TLDest, size_t Index, typename ... NewTypes>
struct tl_insert_t
{
	typedef typename tl_subrange<TLDest, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TLDest, Index, TLDest::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, typename tl_push_front<last_partition, NewTypes...>::type>::type type;
};

template <typename TL, size_t Index>
struct tl_remove
{
	typedef typename tl_subrange<TL, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TL, Index + 1, TL::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, last_partition>::type type;
};

template <typename TL, size_t Index, size_t Size>
struct tl_remove_subrange
{
	typedef typename tl_subrange<TL, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TL, Index + Size, TL::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, last_partition>::type type;
};

template <typename TL, size_t Size = 1>
struct tl_pop_back
{
	typedef typename tl_subrange<TL, Size, TL::size - Size>::type type;
};

template <typename TL, size_t Size = 1>
struct tl_pop_front
{
	typedef typename tl_subrange<TL, (TL::size - 1) - Size, Size>::type type;
};

template <template <typename> typename ApplyTo, typename TL>
struct tl_callsign;

template <template <typename> typename ApplyTo, template <typename...> typename TL, typename R, typename ... Ps>
struct tl_callsign<ApplyTo, TL<R, Ps...>>
{
	typedef ApplyTo<R(Ps...)> type;
};

template <typename S>
struct callsign; //< callsign means function signature

template <typename R, typename ... Ps>
struct callsign<R(Ps...)>
{
	typedef R return_type;
	typedef typelist<Ps...> param_types;
	typedef R (*pointer_type)(Ps...);
};

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

struct _tuple_index_out_of_bounds_ {} tuple_index_out_of_bounds;

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

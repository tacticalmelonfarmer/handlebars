#pragma once
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>


namespace utility
{

template <typename R, typename ... ArgTs>
size_t get_address(std::function<R(ArgTs...)> f) {
    typedef R (fnType)(ArgTs...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

template <typename F, typename Tuple, size_t ...S >
decltype(auto) apply_tuple_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
	return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
decltype(auto) apply_tuple(F&& fn, Tuple&& t)
{
	std::size_t constexpr tSize
		= std::tuple_size<typename std::remove_reference<Tuple>::type>::value;

	return apply_tuple_impl(std::forward<F>(fn),
	                        std::forward<Tuple>(t),
	                        std::make_index_sequence<tSize>());
}

template <bool E, typename T, typename F>
struct ct_if_else
{
	typedef F type;
};

template <typename T, typename F>
struct ct_if_else<true, T, F>
{
	typedef T type;
};

template <unsigned Index, typename ... Ts>
struct ct_select;

template <typename T, typename ... Ts>
struct ct_select<0, T, Ts...>
{
	typedef T type;
};


template <unsigned Index, typename T, typename ... Ts>
struct ct_select<Index, T, Ts...>
{
	typedef typename ct_select<Index - 1, Ts...>::type type;
};

/*template <unsigned Index, typename T, typename ... Ts>
struct typelist_impl
{
	typedef T type;
	enum { index = Index, begin = 0, end = 0 };
	typedef typelist_impl<Index + 1, Ts...> next;
};

template <unsigned I, typename T>
struct typelist_impl<I, T>
{
	typedef T type;
	enum { index = I, begin = 0, end = 1 };
};

template <typename ... Ts>
struct typelist;

template <>
struct typelist<>
{
	typedef void type;
	enum { index = -1, begin = -1, end = -1, size = 0 };
};

template <typename T>
struct typelist<T>
{
	typedef T type;
	enum { index = 0, begin = 1, end = 1, size = 1 };
};

template <typename T, typename ... Ts>
struct typelist<T, Ts...>
{
	typedef T type;
	enum { index = 0, begin = 1, end = 0, size = sizeof...(Ts) + 1 };
	typedef typelist_impl<1, Ts...> next;
};*/

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

template <unsigned Index, typename TL>
struct tl_get;

template <unsigned Index, template <typename...> typename TL, typename ... TL_deduce>
struct tl_get<Index, TL<TL_deduce...>>
{
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
	typedef typename tl_get<TL::size - 1, TL>::type type;
};

template <typename TL>
struct tl_front
{
	typedef typename TL::type type;
};

template <typename TL, unsigned Index, unsigned Size, /*-->ignore these parameters-->*/ bool Begin = true, typename ... Result>
struct tl_subrange
{
	typedef typename
	ct_if_else<
		Begin,
		typename tl_get<Index, TL>::new_typelist, // start with a zero index
		TL
	>::type iterator;
	typedef typename
	ct_if_else<
		iterator::index == Size-1,
		typelist<Result..., typename iterator::type>,
		typename tl_subrange<typename iterator::next, Index, Size, false, Result..., typename iterator::type>::type
	>::type type;
};

template <typename TL1, typename TL2>
struct tl_join;

template <template <typename> typename TL1, typename ... TL1_deduce, template <typename> typename TL2, typename ... TL2_deduce>
struct tl_join<TL1<TL1_deduce...>, TL2<TL2_deduce...>>
{
	typedef typelist<TL1_deduce..., TL2_deduce...> type;
};

template <typename TLDest, unsigned Index, typename TLSource>
struct tl_insert
{
	typedef typename tl_subrange<TLDest, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TLDest, Index, TLDest::size - first_partition::size - 1>::type last_partition;
	typedef typename tl_join<first_partition, typename tl_join<TLSource, last_partition>::type>::type type;
};

template <typename TLDest, unsigned Index, typename ... NewTypes>
struct tl_insert_t
{
	typedef typename tl_subrange<TLDest, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TLDest, Index, TLDest::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, typename tl_push_front<last_partition, NewTypes...>::type>::type type;
};

template <typename TL, unsigned Index>
struct tl_remove
{
	typedef typename tl_subrange<TL, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TL, Index + 1, TL::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, last_partition>::type type;
};

template <typename TL, unsigned Index, unsigned Size>
struct tl_remove_subrange
{
	typedef typename tl_subrange<TL, 0, Index - 1>::type first_partition;
	typedef typename tl_subrange<TL, Index + Size, TL::size - first_partition::size>::type last_partition;
	typedef typename tl_join<first_partition, last_partition>::type type;
};

template <typename TL, unsigned Size = 1>
struct tl_pop_back
{
	typedef typename tl_subrange<TL, Size, TL::size - Size>::type type;
};

template <typename TL, unsigned Size = 1>
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

}


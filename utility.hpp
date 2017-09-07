#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

template <typename R, typename ... ArgTs>
size_t getAddress(std::function<R(ArgTs...)> f) {
    typedef R (fnType)(ArgTs...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

template <typename F, typename Tuple, size_t ...S >
decltype(auto) apply_tuple_(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
	return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
decltype(auto) apply_tuple(F&& fn, Tuple&& t)
{
	std::size_t constexpr tSize
		= std::tuple_size<typename std::remove_reference<Tuple>::type>::value;

	return apply_tuple_(std::forward<F>(fn),
	                        std::forward<Tuple>(t),
	                        std::make_index_sequence<tSize>());
}

template <bool E, typename T, typename F>
struct ct_if;

template <typename T, typename F>
struct ct_if<0, T, F>
{
	typedef F type;
};

template <typename T, typename F>
struct ct_if<1, T, F>
{
	typedef T type;
};

template <unsigned I, typename P, typename T>
struct tl_tail_
{
	typedef T type;
	enum { index = I, begin = 0, end = 1 };
	typedef P prev;
};

template <unsigned I, typename P, typename T, typename ... Ts>
struct tl_body_
{
	typedef T type;
	enum { index = I, begin = 0, end = 0 };
	typedef typename ct_if<(sizeof...(Ts) > 1),
						   tl_body_<I + 1, tl_body_<I, P, T, Ts...>, Ts...>,
						   tl_tail_<I + 1, tl_body_<I, P, T, Ts...>, Ts...>>::type next;
	typedef P prev;
};

template <typename T, typename ... Ts>
struct typelist
{
	typedef T type;
	enum { index = 0, begin = 1, end = 0 };
	typedef typename ct_if<(sizeof...(Ts) > 1),
						   tl_body_<1, typelist<T, Ts...>, Ts...>,
						   tl_tail_<1, typelist<T, Ts...>, Ts...>>::type next;
};
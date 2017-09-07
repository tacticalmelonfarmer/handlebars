#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

template<class R, class ... ArgTs>
size_t getAddress(std::function<R(ArgTs...)> f) {
    typedef R (fnType)(ArgTs...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

template<typename F, typename Tuple, size_t ...S >
decltype(auto) apply_tuple_(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
	return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
}

template<typename F, typename Tuple>
decltype(auto) apply_tuple(F&& fn, Tuple&& t)
{
	std::size_t constexpr tSize
		= std::tuple_size<typename std::remove_reference<Tuple>::type>::value;

	return apply_tuple_(std::forward<F>(fn),
	                        std::forward<Tuple>(t),
	                        std::make_index_sequence<tSize>());
}

#include <type_traits>

namespace impl {
    template <typename T> struct fn_arg_type { using type = void; };
    template <typename R, typename X> struct fn_arg_type<R (X)> { using type = X; };
    template <typename R, typename X> struct fn_arg_type<R (*)(X)> { using type = X; };
    template <typename R, typename C, typename X> struct fn_arg_type<R (C::*)(X)> { using type = X; };

    template <typename...> struct void_type { using type = void; };

    template <typename T, typename = void>
    struct unary_argument_type { using type = typename fn_arg_type<T>::type; };

    template <typename T>
    struct unary_argument_type<T, typename void_type<decltype(&T::operator())>::type> {
        using type = typename fn_arg_type<decltype(&T::operator())>::type;
    };
}

template <typename T>
using unary_argument_type_t = typename impl::unary_argument_type<T>::type;


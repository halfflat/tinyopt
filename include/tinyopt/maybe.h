#pragma once

#include <stdexcept>
#include <utility>
#include <type_traits>

#include <iostream>

namespace to {

constexpr struct nothing_t {} nothing;

template <typename T>
struct maybe {
    bool ok = false;
    alignas(T) char data[sizeof(T)];

    maybe() noexcept: ok(false) {}
    maybe(nothing_t) noexcept: ok(false) {}
    maybe(const T& v): ok(true) { construct(v); }
    maybe(T&& v): ok(true) { construct(std::move(v)); }
    maybe(const maybe& m): ok(m.ok) { if (ok) construct(*m); }
    maybe(maybe&& m): ok(m.ok) { if (ok) construct(std::move(*m)); }

    ~maybe() { destroy(); }

    template <typename U>
    maybe(const maybe<U>& m): ok(m.ok) { if (ok) construct(*m); }

    template <typename U>
    maybe(maybe<U>&& m): ok(m.ok) { if (ok) construct(std::move(*m)); }

    maybe& operator=(nothing_t) { return destroy(), *this; }
    maybe& operator=(const T& v) { return assign(v), *this; }
    maybe& operator=(T&& v) { return assign(std::move(v)), *this; }
    maybe& operator=(const maybe& m) { return m.ok? assign(*m): destroy(), *this; }
    maybe& operator=(maybe&& m) { return m.ok? assign(std::move(*m)): destroy(), *this; }

    const T& value() const & { return assert_ok(), *vptr(); }
    T&& value() && { return assert_ok(), std::move(*vptr()); }

    const T& operator*() const & noexcept { return *vptr(); }
    T&& operator*() && { return std::move(*vptr()); }
    operator bool() const noexcept { return ok; }

private:
    T* vptr() noexcept { return reinterpret_cast<T*>(data); }
    const T* vptr() const noexcept { return reinterpret_cast<const T*>(data); }

    void construct(const T& v) { new (data) T(v); ok = true; }
    void construct(T&& v) { new (data) T(std::move(v)); ok = true; }

    void assign(const T& v) { if (ok) *vptr()=v; else construct(v); }
    void assign(T&& v) { if (ok) *vptr()=std::move(v); else construct(std::move(v)); }

    void destroy() { if (ok) (**this).~T(); ok = false; }
    void assert_ok() const { if (!ok) throw std::invalid_argument("is nothing"); }
};

namespace impl {
    template <typename T>
    struct is_maybe_: std::false_type {};

    template <typename T>
    struct is_maybe_<maybe<T>>: std::true_type {};
}

template <typename T>
using is_maybe = impl::is_maybe_<std::remove_cv_t<std::remove_reference_t<T>>>;

template <>
struct maybe<void> {
    bool ok = false;

    constexpr maybe(): ok(false) {}
    constexpr maybe(nothing_t&): ok(false) {}
    constexpr maybe(const nothing_t&): ok(false) {}
    constexpr maybe(nothing_t&&): ok(false) {}

    template <typename X, typename = std::enable_if_t<!is_maybe<X>::value>>
    constexpr maybe(X&&): ok(true) {}

    template <typename U>
    constexpr maybe(const maybe<U>& m): ok(m.ok) {}

    maybe& operator=(nothing_t) noexcept { return ok = false, *this; }
    maybe& operator=(const maybe& m) noexcept { return ok = m.ok, *this; }
    template <typename U>
    maybe& operator=(U&& v) noexcept { return ok = true, *this; }

    constexpr operator bool() const noexcept { return ok; }
};

constexpr maybe<void> something(true);

template <typename X>
auto just(X&& x) { return maybe<std::decay_t<X>>(std::forward<X>(x)); }

template <
    typename F,
    typename T,
    typename R = std::decay_t<decltype(std::declval<F>()(std::declval<const T&>()))>,
    typename = std::enable_if_t<std::is_same<R, void>::value>
>
maybe<void> operator<<(F&& f, const maybe<T>& m) {
    if (m) return f(*m), something; else return nothing;
}

template <
    typename F,
    typename T,
    typename R = std::decay_t<decltype(std::declval<F>()(std::declval<const T&>()))>,
    typename = std::enable_if_t<!std::is_same<R, void>::value>
>
maybe<R> operator<<(F&& f, const maybe<T>& m) {
    if (m) return f(*m); else return nothing;
}

template <typename F>
auto operator<<(F&& f, const maybe<void>& m) -> maybe<std::decay_t<decltype(f())>> {
    return m? (f(), something): nothing;
}

template <typename T, typename U>
auto operator<<(T& x, const maybe<U>& m) -> maybe<std::decay_t<decltype(x=*m)>> {
    if (m) return x=*m; else return nothing;
}

} // namespace to

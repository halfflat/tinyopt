#pragma once

#include <stdexcept>
#include <utility>
#include <type_traits>

namespace to {

constexpr struct nothing_t {} nothing;

template <typename T>
struct maybe {
    bool ok = false;
    alignas(T) char data[sizeof(T)];

    maybe(): ok(false) {}
    maybe(nothing_t): ok(false) {}
    maybe(const T& v): ok(true) { construct(v); }
    maybe(T&& v): ok(true) { construct(std::move(v)); }
    maybe(const maybe& m): ok(m.ok) { if (ok) construct(*m); }
    maybe(maybe&& m): ok(m.ok) { if (ok) construct(std::move(*m)); }

    template <typename U>
    maybe(const maybe<U>& m): ok(m.ok) { if (ok) construct(*m); }

    template <typename U>
    maybe(maybe<U>&& m): ok(m.ok) { if (ok) construct(std::move(*m)); }

    maybe& operator=(nothing_t) { destroy(); return *this; }
    maybe& operator=(const T& v) { assign(v); return *this; }
    maybe& operator=(T&& v) { assign(std::move(v)); return *this; }
    maybe& operator=(const maybe& m) { m.ok? assign(*m): destroy(); return *this; }
    maybe& operator=(maybe&& m) { m.ok? assign(std::move(*m)): destroy(); return *this; }

    const T& value() const & { assert_ok(); return *vptr(); }
    T&& value() && { assert_ok(); return std::move(*vptr()); }

    const T& operator*() const & { return *vptr(); }
    T&& operator*() && { return std::move(*vptr()); }
    explicit operator bool() const { return ok; }

private:
    T* vptr() { return reinterpret_cast<T*>(data); }
    const T* vptr() const { return reinterpret_cast<const T*>(data); }

    void construct(const T& v) { new (data) T(v); ok = true; }
    void construct(T&& v) { new (data) T(std::move(v)); ok = true; }

    void assign(const T& v) { if (ok) *vptr()=v; else construct(v); }
    void assign(T&& v) { if (ok) *vptr()=std::move(v); else construct(std::move(v)); }

    void destroy() { if (ok) (**this).~T(); ok = false; }

    void assert_ok() const { if (!ok) throw std::invalid_argument("is nothing"); }
};

template <>
struct maybe<void> {
    bool ok = false;

    maybe(): ok(false) {}
    template <typename U>
    maybe(const maybe<U>& m): ok(m.ok) {}
    template <typename U>
    maybe(maybe<U>&& m): ok(m.ok) {}
    maybe(nothing_t): ok(false) {}
    template <typename X> maybe(X&&): ok(true) {}

    maybe& operator=(nothing_t) { ok = false; return *this; }
    maybe& operator=(const maybe& m) { ok = m.ok; return *this; }
    template <typename U>
    maybe& operator=(U&& v) { ok = true; return *this; }

    explicit operator bool() const { return ok; }
};

template <typename F, typename T>
auto operator<<(F&& f, const maybe<T>& m) -> maybe<std::decay_t<decltype(f(*m))>> {
    if (m) return f(*m); else return nothing;
}

template <typename F>
auto operator<<(F&& f, const maybe<void>& m) -> maybe<std::decay_t<decltype(f())>> {
    if (m) return f(), maybe<void>(1); else return nothing;
}

template <typename T, typename U>
auto operator<<(T& x, const maybe<U>& m) -> maybe<std::decay_t<decltype(x=*m)>> {
    if (m) return x=*m; else return nothing;
}

} // namespace to

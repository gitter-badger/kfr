/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "abs.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "round.hpp"
#include "select.hpp"
#include "shuffle.hpp"

#if CID_HAS_WARNING("-Wc99-extensions")
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

namespace kfr
{

namespace internal
{

template <typename T>
constexpr static T fold_constant_div = choose_const<T>(0x1.921fb6p-1f, 0x1.921fb54442d18p-1);

template <typename T>
constexpr static T fold_constant_hi = choose_const<T>(0x1.922000p-1f, 0x1.921fb40000000p-1);
template <typename T>
constexpr static T fold_constant_rem1 = choose_const<T>(-0x1.2ae000p-19f, 0x1.4442d00000000p-25);
template <typename T>
constexpr static T fold_constant_rem2 = choose_const<T>(-0x1.de973ep-32f, 0x1.8469898cc5170p-49);

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> trig_horner(vec<T, N>, mask<T, N> msk, T a0, T b0)
{
    return select(msk, a0, b0);
}

template <typename T, size_t N, typename... Ts>
KFR_SINTRIN vec<T, N> trig_horner(vec<T, N> x, mask<T, N> msk, T a0, T b0, T a1, T b1, Ts... values)
{
    return fmadd(trig_horner(x, msk, a1, b1, values...), x, select(msk, a0, b0));
}

template <typename T, size_t N, typename Tprecise = f64>
KFR_SINTRIN vec<T, N> trig_fold(vec<T, N> x, vec<itype<T>, N>& quadrant)
{
    const vec<T, N> xabs    = abs(x);
    constexpr vec<T, N> div = fold_constant_div<T>;
    vec<T, N> y             = floor(xabs / div);
    quadrant = cast<itype<T>>(y - floor(y * T(1.0 / 16.0)) * T(16.0));

    const mask<T, N> msk = bitcast<T>((quadrant & 1) != 0);
    quadrant = kfr::select(msk, quadrant + 1, quadrant);
    y        = select(msk, y + T(1.0), y);
    quadrant = quadrant & 7;

    constexpr vec<Tprecise, N> hi = cast<Tprecise>(fold_constant_hi<T>);
    constexpr vec<T, N> rem1      = fold_constant_rem1<T>;
    constexpr vec<T, N> rem2      = fold_constant_rem2<T>;
    return cast<T>(cast<Tprecise>(xabs) - cast<Tprecise>(y) * hi) - y * rem1 - y * rem2;
}

template <size_t N>
KFR_SINTRIN vec<f32, N> trig_sincos(vec<f32, N> folded, mask<f32, N> cosmask)
{
    constexpr f32 sin_c2  = -0x2.aaaaacp-4f;
    constexpr f32 sin_c4  = 0x2.222334p-8f;
    constexpr f32 sin_c6  = -0xd.0566ep-16f;
    constexpr f32 sin_c8  = 0x3.64cc1cp-20f;
    constexpr f32 sin_c10 = -0x5.6c4a4p-24f;
    constexpr f32 cos_c2  = -0x8.p-4f;
    constexpr f32 cos_c4  = 0xa.aaaabp-8f;
    constexpr f32 cos_c6  = -0x5.b05d48p-12f;
    constexpr f32 cos_c8  = 0x1.a065f8p-16f;
    constexpr f32 cos_c10 = -0x4.cd156p-24f;

    const vec<f32, N> x2 = folded * folded;

    vec<f32, N> formula = trig_horner(x2, cosmask, 1.0f, 1.0f, cos_c2, sin_c2, cos_c4, sin_c4, cos_c6, sin_c6,
                                      cos_c8, sin_c8, cos_c10, sin_c10);

    formula = select(cosmask, formula, formula * folded);
    return formula;
}

template <size_t N>
KFR_SINTRIN vec<f64, N> trig_sincos(vec<f64, N> folded, mask<f64, N> cosmask)
{
    constexpr f64 sin_c2  = -0x2.aaaaaaaaaaaaap-4;
    constexpr f64 sin_c4  = 0x2.22222222220cep-8;
    constexpr f64 sin_c6  = -0xd.00d00cffd6618p-16;
    constexpr f64 sin_c8  = 0x2.e3bc744fb879ep-20;
    constexpr f64 sin_c10 = -0x6.b99034c1467a4p-28;
    constexpr f64 sin_c12 = 0xb.0711ea8fe8ee8p-36;
    constexpr f64 sin_c14 = -0xb.7e010897e55dp-44;
    constexpr f64 sin_c16 = -0xb.64eac07f1d6bp-48;
    constexpr f64 cos_c2  = -0x8.p-4;
    constexpr f64 cos_c4  = 0xa.aaaaaaaaaaaa8p-8;
    constexpr f64 cos_c6  = -0x5.b05b05b05ad28p-12;
    constexpr f64 cos_c8  = 0x1.a01a01a0022e6p-16;
    constexpr f64 cos_c10 = -0x4.9f93ed845de2cp-24;
    constexpr f64 cos_c12 = 0x8.f76bc015abe48p-32;
    constexpr f64 cos_c14 = -0xc.9bf2dbe00379p-40;
    constexpr f64 cos_c16 = 0xd.1232ac32f7258p-48;

    vec<f64, N> x2 = folded * folded;
    vec<f64, N> formula =
        trig_horner(x2, cosmask, 1.0, 1.0, cos_c2, sin_c2, cos_c4, sin_c4, cos_c6, sin_c6, cos_c8, sin_c8,
                    cos_c10, sin_c10, cos_c12, sin_c12, cos_c14, sin_c14, cos_c16, sin_c16);

    formula = select(cosmask, formula, formula * folded);
    return formula;
}

template <typename T, size_t N, typename = u8[N > 1]>
KFR_SINTRIN vec<T, N> sincos_mask(vec<T, N> x_full, mask<T, N> cosmask)
{
    vec<itype<T>, N> quadrant;
    vec<T, N> folded = trig_fold(x_full, quadrant);

    mask<T, N> flip_sign = select(cosmask, (quadrant == 2) || (quadrant == 4), quadrant >= 4);

    mask<T, N> usecos = (quadrant == 2) || (quadrant == 6);
    usecos = usecos ^ cosmask;

    vec<T, N> formula = trig_sincos(folded, usecos);

    mask<T, N> negmask = x_full < 0;

    flip_sign = flip_sign ^ (negmask & ~cosmask);

    formula = select(flip_sign, -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> sin(vec<T, N> x)
{
    vec<itype<T>, N> quadrant;
    vec<T, N> folded = trig_fold(x, quadrant);

    mask<T, N> flip_sign = quadrant >= 4;
    mask<T, N> usecos    = (quadrant == 2) || (quadrant == 6);

    vec<T, N> formula = trig_sincos(folded, usecos);

    formula = select(flip_sign ^ x.asmask(), -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> cos(vec<T, N> x)
{
    vec<itype<T>, N> quadrant;
    vec<T, N> folded = trig_fold(x, quadrant);

    mask<T, N> eq4       = (quadrant == 4);
    mask<T, N> flip_sign = (quadrant == 2) || eq4;
    mask<T, N> usecos    = (quadrant == 0) || eq4;

    vec<T, N> formula = trig_sincos(folded, usecos);

    formula = select(flip_sign, -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> fastsin(vec<T, N> x)
{
    constexpr vec<T, N> msk = broadcast<N>(internal::highbitmask<T>);

    constexpr static T c2 = -0.16665853559970855712890625;
    constexpr static T c4 = +8.31427983939647674560546875e-3;
    constexpr static T c6 = -1.85423981747590005397796630859375e-4;

    const vec<T, N> pi = c_pi<T>;

    x -= pi;
    vec<T, N> y = abs(x);
    y = select(y > c_pi<T, 1, 2>, pi - y, y);
    y = y ^ (msk & ~x);

    vec<T, N> y2      = y * y;
    vec<T, N> formula = c6;
    vec<T, N> y3      = y2 * y;
    formula = fmadd(formula, y2, c4);
    formula = fmadd(formula, y2, c2);
    formula = formula * y3 + y;
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> fastcos(vec<T, N> x)
{
    x += c_pi<T, 1, 2>;
    x = select(x >= c_pi<T, 2>, x - c_pi<T, 2>, x);
    return fastsin(x);
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1 && is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> sincos(vec<T, N> x)
{
    return sincos_mask(x, internal::oddmask<T, N>());
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1 && is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> cossin(vec<T, N> x)
{
    return sincos_mask(x, internal::evenmask<T, N>());
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> sinc(vec<T, N> x)
{
    return select(abs(x) <= c_epsilon<T>, T(1), sin(x) / x);
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> sin(vec<T, N> x)
{
    return sin(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> cos(vec<T, N> x)
{
    return cos(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> fastsin(vec<T, N> x)
{
    return fastsin(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> fastcos(vec<T, N> x)
{
    return fastcos(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> sincos(vec<T, N> x)
{
    return sincos(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> cossin(vec<T, N> x)
{
    return cossin(cast<Tout>(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> sinc(vec<T, N> x)
{
    return sinc(cast<Tout>(x));
}

template <typename T>
KFR_SINTRIN T sindeg(const T& x)
{
    return sin(x * c_degtorad<T>);
}

template <typename T>
KFR_SINTRIN T cosdeg(const T& x)
{
    return cos(x * c_degtorad<T>);
}

template <typename T>
KFR_SINTRIN T fastsindeg(const T& x)
{
    return fastsin(x * c_degtorad<T>);
}

template <typename T>
KFR_SINTRIN T fastcosdeg(const T& x)
{
    return fastcos(x * c_degtorad<T>);
}

template <typename T>
KFR_SINTRIN T sincosdeg(const T& x)
{
    return sincos(x * c_degtorad<T>);
}

template <typename T>
KFR_SINTRIN T cossindeg(const T& x)
{
    return cossin(x * c_degtorad<T>);
}

KFR_HANDLE_SCALAR_1(sin)
KFR_HANDLE_SCALAR_1(cos)
KFR_HANDLE_SCALAR_1(fastsin)
KFR_HANDLE_SCALAR_1(fastcos)
KFR_HANDLE_SCALAR_1(sincos)
KFR_HANDLE_SCALAR_1(cossin)
KFR_HANDLE_SCALAR_1(sinc)

KFR_FN(sin)
KFR_FN(cos)
KFR_FN(fastsin)
KFR_FN(fastcos)
KFR_FN(sincos)
KFR_FN(cossin)
KFR_FN(sinc)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sin(const T1& x)
{
    return internal::sin(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_sin, E1> sin(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cos(const T1& x)
{
    return internal::cos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_cos, E1> cos(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> fastsin(const T1& x)
{
    return internal::fastsin(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_fastsin, E1> fastsin(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> fastcos(const T1& x)
{
    return internal::fastcos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_fastcos, E1> fastcos(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sincos(const T1& x)
{
    return internal::sincos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_sincos, E1> sincos(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cossin(const T1& x)
{
    return internal::cossin(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_cossin, E1> cossin(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sinc(const T1& x)
{
    return internal::sinc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_sinc, E1> sinc(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T>
KFR_SINTRIN T sin2x(const T& sinx, const T& cosx)
{
    return 2 * sinx * cosx;
}

template <typename T>
KFR_SINTRIN T sin3x(const T& sinx, const T& cosx)
{
    return sinx * (-1 + 4 * sqr(cosx));
}

template <typename T>
KFR_SINTRIN T cos2x(const T& sinx, const T& cosx)
{
    return sqr(cosx) - sqr(sinx);
}

template <typename T>
KFR_SINTRIN T cos3x(const T& sinx, const T& cosx)
{
    return cosx * (1 - 4 * sqr(sinx));
}
}

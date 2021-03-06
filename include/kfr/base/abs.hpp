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

#include "function.hpp"
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace internal
{
// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> x)
{
    return x & internal::invhighbitmask<T>;
}

#if defined CID_ARCH_SSSE3

template <typename T, size_t N>
KFR_SINTRIN vec<i64, N> abs(vec<i64, N> x)
{
    return select(x >= T(), x, -x);
}
KFR_SINTRIN i32sse abs(i32sse value) { return _mm_abs_epi32(*value); }
KFR_SINTRIN i16sse abs(i16sse value) { return _mm_abs_epi16(*value); }
KFR_SINTRIN i8sse abs(i8sse value) { return _mm_abs_epi8(*value); }

#if defined CID_ARCH_AVX2
KFR_SINTRIN i32avx abs(i32avx value) { return _mm256_abs_epi32(*value); }
KFR_SINTRIN i16avx abs(i16avx value) { return _mm256_abs_epi16(*value); }
KFR_SINTRIN i8avx abs(i8avx value) { return _mm256_abs_epi8(*value); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native> && !is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> a)
{
    return slice<0, N>(abs(expand_simd(a)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native> && !is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> a)
{
    return concat(abs(low(a)), abs(high(a)));
}

#else

// fallback
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> x)
{
    return select(x >= T(), x, -x);
}
#endif
KFR_HANDLE_SCALAR_1(abs)
KFR_FN(abs)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 abs(const T1& x)
{
    return internal::abs(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_abs, E1> abs(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}

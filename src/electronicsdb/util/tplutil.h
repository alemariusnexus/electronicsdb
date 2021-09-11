/*
    Copyright 2010-2021 David "Alemarius Nexus" Lerch

    This file is part of electronicsdb.

    electronicsdb is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    electronicsdb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with electronicsdb.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../global.h"

#include <cstdlib>
#include <type_traits>


// Adapted from: https://stackoverflow.com/a/31514214/1566841
#define EDB_UTIL_REPEAT_2(M, N) M(N) M(N+1)
#define EDB_UTIL_REPEAT_4(M, N) EDB_UTIL_REPEAT_2(M,N) EDB_UTIL_REPEAT_2(M,N+2)
#define EDB_UTIL_REPEAT_8(M, N) EDB_UTIL_REPEAT_4(M,N) EDB_UTIL_REPEAT_4(M,N+4)
#define EDB_UTIL_REPEAT_16(M, N) EDB_UTIL_REPEAT_8(M,N) EDB_UTIL_REPEAT_8(M,N+8)
#define EDB_UTIL_REPEAT_32(M, N) EDB_UTIL_REPEAT_16(M,N) EDB_UTIL_REPEAT_16(M,N+16)
#define EDB_UTIL_REPEAT_64(M, N) EDB_UTIL_REPEAT_32(M,N) EDB_UTIL_REPEAT_32(M,N+32)

#define EDB_UTIL_REPEAT_2_1(M, N, A) M(A,N) M(A,N+1)
#define EDB_UTIL_REPEAT_4_1(M, N, A) EDB_UTIL_REPEAT_2_1(M,N,A) EDB_UTIL_REPEAT_2_1(M,N+2,A)
#define EDB_UTIL_REPEAT_8_1(M, N, A) EDB_UTIL_REPEAT_4_1(M,N,A) EDB_UTIL_REPEAT_4_1(M,N+4,A)
#define EDB_UTIL_REPEAT_16_1(M, N, A) EDB_UTIL_REPEAT_8_1(M,N,A) EDB_UTIL_REPEAT_8_1(M,N+8,A)
#define EDB_UTIL_REPEAT_32_1(M, N, A) EDB_UTIL_REPEAT_16_1(M,N,A) EDB_UTIL_REPEAT_16_1(M,N+16,A)
#define EDB_UTIL_REPEAT_64_1(M, N, A) EDB_UTIL_REPEAT_32_1(M,N,A) EDB_UTIL_REPEAT_32_1(M,N+32,A)

#define EDB_UTIL_REPEAT_2_2(M, N, A, B) M(A,B,N) M(A,B,N+1)
#define EDB_UTIL_REPEAT_4_2(M, N, A, B) EDB_UTIL_REPEAT_2_2(M,N,A,B) EDB_UTIL_REPEAT_2_2(M,N+2,A,B)
#define EDB_UTIL_REPEAT_8_2(M, N, A, B) EDB_UTIL_REPEAT_4_2(M,N,A,B) EDB_UTIL_REPEAT_4_2(M,N+4,A,B)
#define EDB_UTIL_REPEAT_16_2(M, N, A, B) EDB_UTIL_REPEAT_8_2(M,N,A,B) EDB_UTIL_REPEAT_8_2(M,N+8,A,B)
#define EDB_UTIL_REPEAT_32_2(M, N, A, B) EDB_UTIL_REPEAT_16_2(M,N,A,B) EDB_UTIL_REPEAT_16_2(M,N+16,A,B)
#define EDB_UTIL_REPEAT_64_2(M, N, A, B) EDB_UTIL_REPEAT_32_2(M,N,A,B) EDB_UTIL_REPEAT_32_2(M,N+32,A,B)

#define EDB_UTIL_DECLARE_PACK_FRIEND(PackName, N) \
friend std::tuple_element_t < \
std::min(static_cast<size_t>(N+1), sizeof...(PackName)), \
std::tuple<void, PackName...> \
>;
#define EDB_UTIL_DECLARE_PACK_FRIEND_ADVANCED(BasePack, ExtPack, N) \
friend std::tuple_element_t < \
std::min(static_cast<size_t>(N+1), sizeof...(BasePack)), \
std::tuple<void, ExtPack...> \
>;
#define EDB_UTIL_DECLARE_PACK_FRIENDS(PackName) EDB_UTIL_REPEAT_64_1(EDB_UTIL_DECLARE_PACK_FRIEND, 0, PackName)
#define EDB_UTIL_DECLARE_PACK_FRIENDS_ADVANCED(BasePack, ExtPack) EDB_UTIL_REPEAT_64_2(EDB_UTIL_DECLARE_PACK_FRIEND_ADVANCED, 0, BasePack, ExtPack)

namespace electronicsdb
{


template <typename T, typename... Ts>
struct varpack_index;
template <typename T, typename... Ts>
struct varpack_index<T, T, Ts...> : std::integral_constant<size_t, 0> {};
template <typename T, typename U, typename... Ts>
struct varpack_index<T, U, Ts...> : std::integral_constant<size_t, 1 + varpack_index<T, Ts...>::value> {};
template <typename T, typename... Ts>
constexpr size_t varpack_index_v = varpack_index<T, Ts...>::value;


}

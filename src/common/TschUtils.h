/*
 * Simulation model for IEEE 802.15.4 Time Slotted Channel Hopping (TSCH)
 *
 * Copyright (C) 2022  Institute of Communication Networks (ComNets),
 *                     Hamburg University of Technology (TUHH)
 *           (C) 2022  Yevhenii Shudrenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMON_TSCHUTILS_H_
#define COMMON_TSCHUTILS_H_

#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>

namespace tsch {

template<typename ... Args>
static inline std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

/**
 * Remove intersection elements from vector a
 */
template<typename T>
static inline void remove_intersection(std::vector<T>& a, std::vector<T>& b) {
    std::unordered_multiset<T> st;
    st.insert(a.begin(), a.end());
    st.insert(b.begin(), b.end());
    auto predicate = [&st](const T& k){ return st.count(k) > 1; };
    a.erase(std::remove_if(a.begin(), a.end(), predicate), a.end());
}


}


#endif /* COMMON_TSCHUTILS_H_ */

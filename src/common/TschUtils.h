/*
 * TschUtils.h
 *
 *  Created on: Jan 18, 2021
 *      Author: yevhenii
 */

#ifndef COMMON_TSCHUTILS_H_
#define COMMON_TSCHUTILS_H_

#include <string>
#include <memory>
#include <stdexcept>

namespace tsch {

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args );

}

#endif /* COMMON_TSCHUTILS_H_ */

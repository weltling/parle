// enum_operator.hpp
// Copyright (c) 2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_ENUM_OPERATOR_HPP
#define PARSERTL_ENUM_OPERATOR_HPP

#include <type_traits>

namespace parsertl
{
	// Operator to convert enum class to underlying type (usually int)
	// Example:
	// enum class number { one = 1, two, three };
	// int num = *number::two;
	template <typename T>
	auto operator*(T e) noexcept ->
		std::enable_if_t<std::is_enum<T>::value, std::size_t>
	{
		return static_cast<std::size_t>(e);
	}

	// This is the compile time version of the above operator
	// (e.g. Setting a C style array size using an enum)
	template <typename T>
	constexpr auto operator+(T e) noexcept ->
		std::enable_if_t<std::is_enum<T>::value, std::size_t>
	{
		return static_cast<std::size_t>(e);
	}
}

#endif

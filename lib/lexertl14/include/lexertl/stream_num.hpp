// stream_num.hpp
// Copyright (c) 2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_STREAM_NUM_HPP
#define LEXERTL_STREAM_NUM_HPP

#include <sstream>

namespace lexertl
{
	template<typename T>
	void stream_num(const T num_, std::ostream& ss_)
	{
		ss_ << num_;
	}

	template<typename T>
	void stream_num(const T num_, std::wostream& ss_)
	{
		ss_ << num_;
	}

	// MSVC doesn't support streaming integers etc to
	// std::basic_ostringstream<char32_t>.
	template<typename T>
	void stream_num(const T num_, std::basic_ostream<char32_t>& ss_)
	{
		std::stringstream css_;
		std::string count_;

		css_ << num_;
		count_ = css_.str();

		for (char c_ : count_)
		{
			ss_ << static_cast<char32_t>(c_);
		}
	}
}

#endif

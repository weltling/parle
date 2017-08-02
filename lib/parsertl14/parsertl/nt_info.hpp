// nt_info.hpp
// Copyright (c) 2016-2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_NT_INFO_HPP
#define PARSERTL_NT_INFO_HPP

#include <vector>

namespace parsertl
{
using char_vector = std::vector<char>;

struct nt_info
{
    bool _nullable;
    char_vector _first_set;
    char_vector _follow_set;

    nt_info(const std::size_t terminals_) :
        _nullable(false),
        _first_set(terminals_, 0),
        _follow_set(terminals_, 0)
    {
    }
};

using nt_info_vector = std::vector<nt_info>;
}

#endif

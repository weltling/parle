// token.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_TOKEN_HPP
#define PARSERTL_TOKEN_HPP

#include <string>
#include <vector>

namespace parsertl
{
template<typename iterator>
struct token
{
    using char_type = typename iterator::value_type::char_type;
    using iter_type = typename iterator::value_type::iter_type;
    using string = std::basic_string<char_type>;
    using token_vector = std::vector<token<iterator>>;
    std::size_t id;
    iter_type first;
    iter_type second;

    token() :
        id(static_cast<std::size_t>(~0)),
        first(),
        second()
    {
    }

    token(const std::size_t id_, const iter_type &first_,
        const iter_type &second_) :
        id(id_),
        first(first_),
        second(second_)
    {
    }

    string str() const
    {
        return string(first, second);
    }
};
}

#endif

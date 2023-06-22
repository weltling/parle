// capture.hpp
// Copyright (c) 2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_CAPTURE_HPP
#define PARSERTL_CAPTURE_HPP

#include <iterator>
#include <string>

namespace parsertl
{
    template<typename iterator>
    struct capture
    {
        using iter_type = iterator;
        using char_type = typename std::iterator_traits<iter_type>::value_type;
        using string = std::basic_string<char_type>;

        iterator first = iterator();
        iterator second = iterator();

        capture() = default;

        capture(const iterator& first_,
            const iterator& second_) :
            first(first_),
            second(second_)
        {
        }

        bool operator==(const capture& rhs_) const
        {
            return first == rhs_.first &&
                second == rhs_.second;
        }

        bool empty() const
        {
            return first == second;
        }

        string str() const
        {
            return string(first, second);
        }

        string substr(const std::size_t soffset_,
            const std::size_t eoffset_) const
        {
            return string(first + soffset_, second - eoffset_);
        }

        std::size_t length() const
        {
            return second - first;
        }
    };
}

#endif

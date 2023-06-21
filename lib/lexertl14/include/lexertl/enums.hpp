// enums.hpp
// Copyright (c) 2005-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LEXERTL_ENUMS_HPP
#define LEXERTL_ENUMS_HPP

namespace lexertl
{
    enum class regex_flags
    {
        icase = 1, dot_not_newline = 2, dot_not_cr_lf = 4,
        skip_ws = 8, match_zero_len = 16
    };
    // 0 = end_state, 1 = id, 2 = user_id, 3 = push_dfa
    // 4 = next_dfa, 5 = dead_state, 6 = dfa start
    enum class state_index
    {
        end_state, id, user_id, push_dfa,
        next_dfa, eol, dead_state, transitions
    };
    // Rule flags:
    enum class feature_bit
    {
        bol = 1, eol = 2, skip = 4, again = 8,
        multi_state = 16, recursive = 32, advance = 64
    };
    // End state flags:
    enum class state_bit
    {
        end_state = 1, greedy = 2, pop_dfa = 4
    };
}

#endif

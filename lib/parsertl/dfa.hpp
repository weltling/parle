// dfa.hpp
// Copyright (c) 2014-2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DFA_HPP
#define PARSERTL_DFA_HPP

#include <deque>
#include <vector>

namespace parsertl
{
using size_t_pair = std::pair<std::size_t, std::size_t>;
using size_t_pair_vector = std::vector<size_t_pair>;

struct dfa_state
{
    size_t_pair_vector _basis;
    size_t_pair_vector _closure;
    size_t_pair_vector _transitions;
};

using dfa = std::deque<dfa_state>;
}

#endif

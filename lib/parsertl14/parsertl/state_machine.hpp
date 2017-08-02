// state_machine.hpp
// Copyright (c) 2014-2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_STATE_MACHINE_HPP
#define PARSERTL_STATE_MACHINE_HPP

#include "enums.hpp"
#include <deque>
#include <map>
#include <vector>

namespace parsertl
{
struct state_machine
{
    struct entry
    {
        eaction action;
        std::size_t param;

        entry() :
            action(error),
            param(syntax_error)
        {
        }

        entry(const eaction action_, const std::size_t param_) :
            action(action_),
            param(param_)
        {
        }

        void clear()
        {
            action = error;
            param = syntax_error;
        }
    };

    using table = std::vector<entry>;
    using size_t_vector = std::vector<std::size_t>;
    using size_t_size_t_pair = std::pair<std::size_t, size_t_vector>;
    using rules = std::deque<size_t_size_t_pair>;

    table _table;
    std::size_t _columns;
    std::size_t _rows;
    rules _rules;

    state_machine() :
        _columns(0),
        _rows(0)
    {
    }

    void clear()
    {
        _table.clear();
        _columns = _rows = 0;
        _rules.clear();
    }

    bool empty() const
    {
        return _table.empty();
    }
};
}

#endif

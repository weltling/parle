// state_machine.hpp
// Copyright (c) 2014-2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_STATE_MACHINE_HPP
#define PARSERTL_STATE_MACHINE_HPP

#include "enums.hpp"
#include <map>
#include <vector>

namespace parsertl
{
template<typename id_type>
struct basic_state_machine
{
    // If you get a compile error here you have
    // failed to define an unsigned id type.
    static_assert(std::is_unsigned<id_type>::value, "Your id type is signed");

    struct entry
    {
        eaction action;
        id_type param;

        entry() :
            action(error),
            param(syntax_error)
        {
        }

        entry(const eaction action_, const id_type param_) :
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
    using id_type_vector = std::vector<id_type>;
    using id_type_pair = std::pair<id_type, id_type_vector>;
    using rules = std::vector<id_type_pair>;

    table _table;
    std::size_t _columns;
    std::size_t _rows;
    rules _rules;

    basic_state_machine() :
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

using state_machine = basic_state_machine<uint16_t>;
}

#endif

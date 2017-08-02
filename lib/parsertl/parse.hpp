// parse.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_PARSE_HPP
#define PARSERTL_PARSE_HPP

#include "match_results.hpp"
#include "state_machine.hpp"
#include <vector>

namespace parsertl
{
// Parse entire sequence and return boolean
template<typename iterator>
bool parse(const state_machine &sm_, iterator &iter_, match_results &results_)
{
    while (results_.entry.action != error)
    {
        switch (results_.entry.action)
        {
        case error:
            break;
        case shift:
            results_.stack.push_back(results_.entry.param);

            if (results_.token_id != 0)
            {
                ++iter_;
            }

            results_.token_id = iter_->id;

            if (results_.token_id == iterator::value_type::npos())
            {
                results_.entry.action = error;
                results_.entry.param = unknown_token;
            }
            else
            {
                results_.entry = sm_._table[results_.stack.back() *
                    sm_._columns + results_.token_id];
            }

            break;
        case reduce:
        {
            const std::size_t size_ =
                sm_._rules[results_.entry.param].second.size();

            if (size_)
            {
                results_.stack.resize(results_.stack.size() - size_);
            }

            results_.token_id = sm_._rules[results_.entry.param].first;
            results_.entry = sm_._table[results_.stack.back() * sm_._columns +
                results_.token_id];
            break;
        }
        case go_to:
            results_.stack.push_back(results_.entry.param);
            results_.token_id = iter_->id;
            results_.entry = sm_._table[results_.stack.back() * sm_._columns +
                results_.token_id];
            break;
        }

        if (results_.entry.action == accept)
        {
            const std::size_t size_ =
                sm_._rules[results_.entry.param].second.size();

            if (size_)
            {
                results_.stack.resize(results_.stack.size() - size_);
            }

            break;
        }
    }

    return results_.entry.action == accept;
}
}

#endif

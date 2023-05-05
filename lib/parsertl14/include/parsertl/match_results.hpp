// match_results.hpp
// Copyright (c) 2017-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_MATCH_RESULTS_HPP
#define PARSERTL_MATCH_RESULTS_HPP

#include "runtime_error.hpp"
#include "state_machine.hpp"
#include <vector>

namespace parsertl
{
    template<typename sm_type>
    struct basic_match_results
    {
        using id_type = typename sm_type::id_type;
        std::vector<id_type> stack;
        id_type token_id = static_cast<id_type>(~0);
        typename sm_type::entry entry;

        basic_match_results()
        {
            stack.push_back(0);
            entry.action = action::error;
            entry.param = static_cast<id_type>(error_type::unknown_token);
        }

        basic_match_results(const std::size_t reserved_) :
            stack(reserved_)
        {
            basic_match_results();
        }

        basic_match_results(const id_type token_id_, const sm_type& sm_)
        {
            reset(token_id_, sm_);
        }

        basic_match_results(const id_type token_id_, const sm_type& sm_,
            const std::size_t reserved_) :
            stack(reserved_)
        {
            basic_match_results(token_id_, sm_);
        }

        void clear()
        {
            stack.clear();
            stack.push_back(0);
            token_id = static_cast<id_type>(~0);
            entry.clear();
        }

        void reset(const id_type token_id_, const sm_type& sm_)
        {
            stack.clear();
            stack.push_back(0);
            token_id = token_id_;

            if (token_id == static_cast<id_type>(~0))
            {
                entry.action = action::error;
                entry.param = static_cast<id_type>(error_type::unknown_token);
            }
            else
            {
                entry = sm_.at(stack.back(), token_id);
            }
        }

        id_type reduce_id() const
        {
            if (entry.action != action::reduce)
            {
                throw runtime_error("Not in a reduce state!");
            }

            return entry.param;
        }

        template<typename token_vector>
        typename token_vector::value_type& dollar(const std::size_t index_,
            const sm_type& sm_, token_vector& productions) const
        {
            if (entry.action != action::reduce)
            {
                throw runtime_error("Not in a reduce state!");
            }

            return productions[productions.size() -
                production_size(sm_, entry.param) + index_];
        }

        template<typename token_vector>
        const typename token_vector::value_type& dollar(const std::size_t index_,
            const sm_type& sm_, const token_vector& productions) const
        {
            if (entry.action != action::reduce)
            {
                throw runtime_error("Not in a reduce state!");
            }

            return productions[productions.size() -
                production_size(sm_, entry.param) + index_];
        }

        std::size_t production_size(const sm_type& sm,
            const std::size_t index_) const
        {
            return sm._rules[index_].second.size();
        }

        bool operator ==(const basic_match_results& rhs_) const
        {
            return stack == rhs_.stack &&
                token_id == rhs_.token_id &&
                entry == rhs_.entry;
        }
    };

    using match_results = basic_match_results<state_machine>;
    using uncompressed_match_results =
        basic_match_results<uncompressed_state_machine>;
}

#endif

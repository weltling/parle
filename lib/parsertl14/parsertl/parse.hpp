// parse.hpp
// Copyright (c) 2017-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_PARSE_HPP
#define PARSERTL_PARSE_HPP

#include "match_results.hpp"
#include <vector>

namespace parsertl
{
    // Parse entire sequence and return boolean
    template<typename lexer_iterator, typename sm_type>
    bool parse(lexer_iterator& iter_, const sm_type& sm_,
        basic_match_results<sm_type>& results_)
    {
        while (results_.entry.action != action::error)
        {
            switch (results_.entry.action)
            {
            case action::shift:
                results_.stack.push_back(results_.entry.param);

                if (iter_->id != 0)
                    ++iter_;

                results_.token_id = iter_->id;

                if (results_.token_id == lexer_iterator::value_type::npos())
                {
                    results_.entry.action = action::error;
                    results_.entry.param =
                        static_cast<typename sm_type::id_type>
                        (error_type::unknown_token);
                }
                else
                {
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                }

                break;
            case action::reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();

                if (size_)
                {
                    results_.stack.resize(results_.stack.size() - size_);
                }

                results_.token_id = sm_._rules[results_.entry.param].first;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                break;
            }
            case action::go_to:
                results_.stack.push_back(results_.entry.param);
                results_.token_id = iter_->id;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                break;
            default:
                // action::accept
                // action::error
                break;
            }

            if (results_.entry.action == action::accept)
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

        return results_.entry.action == action::accept;
    }
}

#endif

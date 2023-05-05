// match.hpp
// Copyright (c) 2018-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_MATCH_HPP
#define PARSERTL_MATCH_HPP

#include "../../../lexertl14/include/lexertl/iterator.hpp"
#include "lookup.hpp"
#include "parse.hpp"

namespace parsertl
{
    // Parse entire sequence and return boolean
    template<typename lexer_iterator, typename sm_type>
    bool match(lexer_iterator iter_, const sm_type& sm_)
    {
        basic_match_results<sm_type> results_(iter_->id, sm_);

        return parse(iter_, sm_, results_);
    }

    template<typename lexer_iterator, typename sm_type, typename captures>
    bool match(lexer_iterator iter_, const sm_type& sm_, captures& captures_)
    {
        basic_match_results<sm_type> results_(iter_->id, sm_);
        // Qualify token to prevent arg dependant lookup
        using token = parsertl::token<lexer_iterator>;
        typename token::token_vector productions_;

        captures_.clear();
        captures_.resize(sm_._captures.back().first +
            sm_._captures.back().second.size() + 1);
        captures_[0].emplace_back(iter_->first, iter_->second);

        while (results_.entry.action != action::error &&
            results_.entry.action != action::accept)
        {
            if (results_.entry.action == action::reduce)
            {
                const auto& row_ = sm_._captures[results_.entry.param];

                if (!row_.second.empty())
                {
                    std::size_t index_ = 0;

                    for (const auto& pair_ : row_.second)
                    {
                        const auto& token1_ = results_.
                            dollar(pair_.first, sm_, productions_);
                        const auto& token2_ = results_.
                            dollar(pair_.second, sm_, productions_);
                        auto& entry_ = captures_[row_.first + index_ + 1];

                        entry_.emplace_back(token1_.first, token2_.second);
                        ++index_;
                    }
                }
            }

            lookup(iter_, sm_, results_, productions_);
        }

        return results_.entry.action == action::accept;
    }
}

#endif

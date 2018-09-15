// match.hpp
// Copyright (c) 2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_MATCH_HPP
#define PARSERTL_MATCH_HPP

#include "../../lexertl14/lexertl/iterator.hpp"
#include "lookup.hpp"
#include "parse.hpp"

namespace parsertl
{
    // Parse entire sequence and return boolean
    template<typename iterator, typename sm_type, typename lsm>
    bool match(iterator begin_, iterator end_,
        const lsm &lsm_, const sm_type &gsm_)
    {
        using lex_iterator = lexertl::iterator<iterator, lsm,
            lexertl::match_results<iterator>>;
        lex_iterator iter_(begin_, end_, lsm_);
        basic_match_results<sm_type> results_(iter_->id, gsm_);

        return parse(gsm_, iter_, results_);
    }

    template<typename iterator, typename captures, typename sm_type,
        typename lsm>
    bool match(iterator begin_, iterator end_, captures &captures_,
        lsm &lsm_, const sm_type &gsm_)
    {
        using lex_iterator = lexertl::iterator<iterator, lsm,
            lexertl::match_results<iterator>>;
        lex_iterator iter_(begin_, end_, lsm_);
        basic_match_results<sm_type> results_(iter_->id, gsm_);
        using token = parsertl::token<lex_iterator>;
        typename token::token_vector productions_;

        captures_.clear();
        captures_.resize(gsm_._captures.back().first +
            gsm_._captures.back().second.size() + 1);
        captures_[0].push_back(std::make_pair(begin_, end_));

        while (results_.entry.action != parsertl::error &&
            results_.entry.action != parsertl::accept)
        {
            if (results_.entry.action == parsertl::reduce)
            {
                const auto &row_ = gsm_._captures[results_.entry.param];

                if (!row_.second.empty())
                {
                    std::size_t index_ = 0;

                    for (const auto &pair_ : row_.second)
                    {
                        const auto &token1_ = results_.
                            dollar(gsm_, pair_.first, productions_);
                        const auto &token2_ = results_.
                            dollar(gsm_, pair_.second, productions_);
                        auto &entry_ = captures_[row_.first + index_ + 1];

                        entry_.push_back(std::make_pair(token1_.first,
                            token2_.second));
                        ++index_;
                    }
                }
            }

            parsertl::lookup(gsm_, iter_, results_, productions_);
        }

        return results_.entry.action == parsertl::accept;
    }
}

#endif

// lookup.hpp
// Copyright (c) 2017-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_LOOKUP_HPP
#define PARSERTL_LOOKUP_HPP

#include "match_results.hpp"
#include "token.hpp"

namespace parsertl
{
    // parse sequence but do not keep track of productions
    template<typename lexer_iterator, typename sm_type>
    void lookup(lexer_iterator& iter_, const sm_type& sm_,
        basic_match_results<sm_type>& results_)
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
                results_.entry.param = static_cast<typename sm_type::id_type>
                    (error_type::unknown_token);
            }
            else
            {
                results_.entry =
                    sm_.at(results_.entry.param, results_.token_id);
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
            results_.entry = sm_.at(results_.stack.back(), results_.token_id);
            break;
        }
        case action::go_to:
            results_.stack.push_back(results_.entry.param);
            results_.token_id = iter_->id;
            results_.entry = sm_.at(results_.stack.back(), results_.token_id);
            break;
        case action::accept:
        {
            const std::size_t size_ =
                sm_._rules[results_.entry.param].second.size();

            if (size_)
            {
                results_.stack.resize(results_.stack.size() - size_);
            }

            break;
        }
        default:
            // action::error
            break;
        }
    }

    // Parse sequence and maintain production vector
    template<typename lexer_iterator, typename sm_type, typename token_vector>
    void lookup(lexer_iterator& iter_, const sm_type& sm_,
        basic_match_results<sm_type>& results_, token_vector& productions_)
    {
        switch (results_.entry.action)
        {
        case action::shift:
            results_.stack.push_back(results_.entry.param);
            productions_.emplace_back(iter_->id, iter_->first, iter_->second);

            if (iter_->id != 0)
                ++iter_;

            results_.token_id = iter_->id;

            if (results_.token_id == lexer_iterator::value_type::npos())
            {
                results_.entry.action = action::error;
                results_.entry.param = static_cast<typename sm_type::id_type>
                    (error_type::unknown_token);
            }
            else
            {
                results_.entry =
                    sm_.at(results_.entry.param, results_.token_id);
            }

            break;
        case action::reduce:
        {
            const std::size_t size_ =
                sm_._rules[results_.entry.param].second.size();
            typename token_vector::value_type token_;

            if (size_)
            {
                results_.stack.resize(results_.stack.size() - size_);
                token_.first = (productions_.end() - size_)->first;
                token_.second = productions_.back().second;
                productions_.resize(productions_.size() - size_);
            }
            else
            {
                if (productions_.empty())
                {
                    token_.first = token_.second = iter_->first;
                }
                else
                {
                    token_.first = token_.second = productions_.back().second;
                }
            }

            results_.token_id = sm_._rules[results_.entry.param].first;
            results_.entry = sm_.at(results_.stack.back(), results_.token_id);
            token_.id = results_.token_id;
            productions_.push_back(token_);
            break;
        }
        case action::go_to:
            results_.stack.push_back(results_.entry.param);
            results_.token_id = iter_->id;
            results_.entry = sm_.at(results_.stack.back(), results_.token_id);
            break;
        case action::accept:
        {
            const std::size_t size_ =
                sm_._rules[results_.entry.param].second.size();

            if (size_)
            {
                results_.stack.resize(results_.stack.size() - size_);
            }

            break;
        }
        default:
            // action::error
            break;
        }
    }
}

#endif

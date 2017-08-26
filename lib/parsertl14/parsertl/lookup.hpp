// lookup.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
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
template<typename iterator>
void lookup(const state_machine &sm_, iterator &iter_, match_results &results_)
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
            results_.entry = sm_._table[results_.stack.back() * sm_._columns +
                results_.token_id];
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
    case accept:
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
}

// Parse sequence and maintain production vector
template<typename iterator, typename token_vector>
void lookup(const state_machine &sm_, iterator &iter_, match_results &results_,
    token_vector &productions_)
{
    switch (results_.entry.action)
    {
    case error:
        break;
    case shift:
        results_.stack.push_back(results_.entry.param);
        productions_.push_back(typename token_vector::value_type(iter_->id,
            iter_->first, iter_->second));

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
        typename token_vector::value_type token_;

        if (size_)
        {
            token_.first = (productions_.end() - size_)->first;
            token_.second = productions_.back().second;
            results_.stack.resize(results_.stack.size() - size_);
            productions_.resize(productions_.size() - size_);
        }
        else
        {
            token_.first = token_.second = iter_->first;
        }

        results_.token_id = sm_._rules[results_.entry.param].first;
        results_.entry = sm_._table[results_.stack.back() *
            sm_._columns + results_.token_id];
        token_.id = results_.token_id;
        productions_.push_back(token_);
        break;
    }
    case go_to:
        results_.stack.push_back(results_.entry.param);
        results_.token_id = iter_->id;
        results_.entry = sm_._table[results_.stack.back() * sm_._columns +
            results_.token_id];
        break;
    case accept:
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
}
}

#endif

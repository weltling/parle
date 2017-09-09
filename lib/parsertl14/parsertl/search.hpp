// search.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

#include "match_results.hpp"
#include "parse.hpp"
#include <set>
#include "token.hpp"

namespace parsertl
{
// Forward declarations:
namespace details
{
template<typename id_type, typename iterator>
void next(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_, std::set<id_type> *prod_set_,
    iterator &last_eoi_, basic_match_results<id_type> &last_results_);
template<typename id_type, typename iterator, typename token_vector>
void next(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_,
    std::multimap<id_type, token_vector> *prod_map_, iterator &last_eoi_,
    basic_match_results<id_type> &last_results_, token_vector &productions_);
template<typename id_type, typename iterator>
bool parse(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_, std::set<id_type> *prod_set_);
template<typename id_type, typename iterator, typename token_vector>
bool parse(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_,
    std::multimap<id_type, token_vector> *prod_map_,
    const token_vector &productions_);
}

// Equivalent of std::search().
template<typename id_type, typename iterator>
bool search(const basic_state_machine<id_type> &sm_, iterator &iter_,
    iterator &end_, std::set<id_type> *prod_set_ = nullptr)
{
    bool hit_ = false;
    iterator curr_ = iter_;
    iterator last_eoi_;
    basic_match_results<id_type> results_;
    basic_match_results<id_type> last_results_;

    end_ = iterator();

    while (curr_ != end_)
    {
        if (prod_set_)
        {
            prod_set_->clear();
        }

        results_.reset(curr_->id, sm_);

        while (results_.entry.action != accept &&
            results_.entry.action != error)
        {
            details::next(sm_, curr_, results_, prod_set_, last_eoi_,
                last_results_);
        }

        hit_ = results_.entry.action == accept;

        if (hit_)
        {
            end_ = curr_;
            break;
        }
        else if (last_eoi_->id != 0)
        {
            iterator eoi_;

            hit_ = details::parse(sm_, eoi_, last_results_, prod_set_);

            if (hit_)
            {
                end_ = last_eoi_;
                break;
            }
        }

        ++iter_;
        curr_ = iter_;
    }

    return hit_;
}

template<typename id_type, typename iterator, typename token_vector>
bool search(const basic_state_machine<id_type> &sm_, iterator &iter_,
    iterator &end_, std::multimap<id_type, token_vector> *prod_map_ = nullptr)
{
    bool hit_ = false;
    iterator curr_ = iter_;
    iterator last_eoi_;
    basic_match_results<id_type> results_;
    basic_match_results<id_type> last_results_;
    token_vector productions_;

    end_ = iterator();

    while (curr_ != end_)
    {
        if (prod_map_)
        {
            prod_map_->clear();
        }

        results_.reset(curr_->id, sm_);

        while (results_.entry.action != accept &&
            results_.entry.action != error)
        {
            details::next(sm_, curr_, results_, prod_map_, last_eoi_,
                last_results_, productions_);
        }

        hit_ = results_.entry.action == accept;

        if (hit_)
        {
            end_ = curr_;
            break;
        }
        else if (last_eoi_->id != 0)
        {
            iterator eoi_;

            hit_ = details::parse(sm_, eoi_, last_results_, prod_map_,
                productions_);

            if (hit_)
            {
                end_ = last_eoi_;
                break;
            }
        }

        ++iter_;
        curr_ = iter_;
    }

    return hit_;
}

namespace details
{
template<typename id_type, typename iterator>
void next(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_, std::set<id_type> *prod_set_,
    iterator &last_eoi_, basic_match_results<id_type> &last_results_)
{
    switch (results_.entry.action)
    {
    case error:
        break;
    case shift:
    {
        const auto *ptr_ = &sm_._table[results_.entry.param * sm_._columns];

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
            results_.entry = ptr_[results_.token_id];
        }

        if (ptr_->action != error)
        {
            last_eoi_ = iter_;
            last_results_.stack = results_.stack;
            last_results_.token_id = 0;
            last_results_.entry = *ptr_;
        }

        break;
    }
    case reduce:
    {
        const std::size_t size_ =
            sm_._rules[results_.entry.param].second.size();
        token<iterator> token_;

        if (prod_set_)
        {
            prod_set_->insert(results_.entry.param);
        }

        if (size_)
        {
            results_.stack.resize(results_.stack.size() - size_);
        }
        else
        {
            token_.first = token_.second = iter_->first;
        }

        results_.token_id = sm_._rules[results_.entry.param].first;
        results_.entry = sm_._table[results_.stack.back() * sm_._columns +
            results_.token_id];
        token_.id = results_.token_id;
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

template<typename id_type, typename iterator, typename token_vector>
void next(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_,
    std::multimap<id_type, token_vector> *prod_map_,
    iterator &last_eoi_, basic_match_results<id_type> &last_results_,
    token_vector &productions_)
{
    switch (results_.entry.action)
    {
    case error:
        break;
    case shift:
    {
        const auto *ptr_ = &sm_._table[results_.entry.param * sm_._columns];

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
            results_.entry = ptr_[results_.token_id];
        }

        if (ptr_->action != error)
        {
            last_eoi_ = iter_;
            last_results_.stack = results_.stack;
            last_results_.token_id = 0;
            last_results_.entry = *ptr_;
        }

        break;
    }
    case reduce:
    {
        const std::size_t size_ =
            sm_._rules[results_.entry.param].second.size();
        token<iterator> token_;

        if (prod_map_)
        {
            prod_map_->insert(std::make_pair(results_.entry.param,
                token_vector(productions_.end() - size_, productions_.end())));
        }

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
        results_.entry = sm_._table[results_.stack.back() * sm_._columns +
            results_.token_id];
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

template<typename id_type, typename iterator>
bool parse(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_, std::set<id_type> *prod_set_)
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

            if (prod_set_)
            {
                prod_set_->insert(results_.entry.param);
            }

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

template<typename id_type, typename iterator, typename token_vector>
bool parse(const basic_state_machine<id_type> &sm_, iterator &iter_,
    basic_match_results<id_type> &results_,
    std::multimap<id_type, token_vector> *prod_map_,
    const token_vector &productions_)
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

            if (prod_map_)
            {
                prod_map_->insert(std::make_pair(results_.entry.param,
                    token_vector(productions_.end() - size_,
                        productions_.end())));
            }

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
}

#endif

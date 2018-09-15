// search.hpp
// Copyright (c) 2017-2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

#include "../../lexertl14/lexertl/iterator.hpp"
#include <map>
#include "match_results.hpp"
#include "parse.hpp"
#include <set>
#include "token.hpp"

namespace parsertl
{
// Forward declarations:
namespace details
{
template<typename sm_type, typename iterator>
void next(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_,
    std::set<typename sm_type::id_type> *prod_set_, iterator &last_eoi_,
    basic_match_results<sm_type> &last_results_);
template<typename sm_type, typename iterator, typename token_vector>
void next(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_, iterator &last_eoi_,
    token_vector &productions_);
template<typename sm_type, typename iterator>
bool parse(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_,
    std::set<typename sm_type::id_type> *prod_set_);
template<typename sm_type, typename iterator, typename token_vector>
bool parse(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_, token_vector &productions_,
    std::multimap<typename sm_type::id_type, token_vector> *prod_map_);
}

template<typename iterator, typename sm_type, typename lsm>
bool search(iterator first_, iterator second_, const lsm &lsm_,
    const sm_type &gsm_)
{
    using lex_iterator = lexertl::iterator<iterator, lsm,
        lexertl::match_results<iterator>>;
    lex_iterator iter_(first_, second_, lsm_);
    lex_iterator end_;

    return search(gsm_, iter_, end_);
}

template<typename iterator, typename captures, typename sm_type,
    typename lsm>
bool search(iterator first_, iterator second_, captures &captures_,
    lsm &lsm_, const sm_type &gsm_)
{
    using lex_iterator = lexertl::iterator<iterator, lsm,
        lexertl::match_results<iterator>>;
    lex_iterator iter_(first_, second_, lsm_);
    lex_iterator end_;
    basic_match_results<sm_type> results_(iter_->id, gsm_);
    using token = parsertl::token<lex_iterator>;
    using token_vector = typename token::token_vector;
    std::multimap<typename sm_type::id_type, token_vector> prod_map_;
    bool success_ = search(gsm_, iter_, end_, &prod_map_);

    captures_.clear();

    if (success_)
    {
        iterator last_ = iter_->first;

        captures_.resize((gsm_._captures.empty() ? 0 :
            gsm_._captures.back().first +
            gsm_._captures.back().second.size()) + 1);
        captures_[0].push_back(std::make_pair(iter_->first, iter_->first));

        for (const auto &pair_ : prod_map_)
        {
            if (gsm_._captures.size() > pair_.first)
            {
                const auto &row_ = gsm_._captures[pair_.first];

                if (!row_.second.empty())
                {
                    std::size_t index_ = 0;

                    for (const auto &token_ : row_.second)
                    {
                        const auto &token1_ = pair_.second[token_.first];
                        const auto &token2_ = pair_.second[token_.second];
                        auto &entry_ = captures_[row_.first + index_ + 1];

                        entry_.push_back(std::make_pair(token1_.first,
                            token2_.second));
                        ++index_;
                    }
                }
            }
        }

        for (const auto &pair_ : prod_map_)
        {
            typename token::iter_type second_ =
                pair_.second.back().second;

            if (second_ > last_)
            {
                last_ = second_;
            }
        }

        captures_.front().back().second = last_;
    }

    return success_;
}

// Equivalent of std::search().
template<typename sm_type, typename iterator>
bool search(const sm_type &sm_, iterator &iter_, iterator &end_,
    std::set<typename sm_type::id_type> *prod_set_ = nullptr)
{
    bool hit_ = false;
    iterator curr_ = iter_;
    iterator last_eoi_;
    // results_ defined here so that allocated memory can be reused.
    basic_match_results<sm_type> results_;
    basic_match_results<sm_type> last_results_;

    end_ = iterator();

    while (curr_ != end_)
    {
        if (prod_set_)
        {
            prod_set_->clear();
        }

        results_.reset(curr_->id, sm_);
        last_results_.clear();

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

template<typename sm_type, typename iterator, typename token_vector>
bool search(const sm_type &sm_, iterator &iter_, iterator &end_,
    std::multimap<typename sm_type::id_type, token_vector> *prod_map_ = nullptr)
{
    bool hit_ = false;
    iterator curr_ = iter_;
    iterator last_eoi_;
    // results_ and productions_ defined here so that
    // allocated memory can be reused.
    basic_match_results<sm_type> results_;
    token_vector productions_;

    end_ = iterator();

    while (curr_ != end_)
    {
        if (prod_map_)
        {
            prod_map_->clear();
        }

        results_.reset(curr_->id, sm_);
        productions_.clear();

        while (results_.entry.action != accept &&
            results_.entry.action != error)
        {
            details::next(sm_, curr_, results_, last_eoi_, productions_);
        }

        hit_ = results_.entry.action == accept;

        if (hit_)
        {
            if (prod_map_)
            {
                iterator again_(iter_->first, last_eoi_->first, iter_.sm());

                results_.reset(iter_->id, sm_);
                productions_.clear();
                details::parse(sm_, again_, results_, productions_, prod_map_);
            }

            end_ = curr_;
            break;
        }
        else if (last_eoi_->id != 0)
        {
            iterator again_(iter_->first, last_eoi_->first, iter_.sm());

            results_.reset(iter_->id, sm_);
            productions_.clear();
            hit_ = details::parse(sm_, again_, results_, productions_,
                prod_map_);

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
template<typename sm_type, typename iterator>
void next(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_,
    std::set<typename sm_type::id_type> *prod_set_, iterator &last_eoi_,
    basic_match_results<sm_type> &last_results_)
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

template<typename sm_type, typename iterator, typename token_vector>
void next(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_, iterator &last_eoi_,
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
        }

        break;
    }
    case reduce:
    {
        const std::size_t size_ =
            sm_._rules[results_.entry.param].second.size();
        token<iterator> token_;

        if (size_)
        {
            token_.first = (productions_.end() - size_)->first;
            token_.second = productions_.back().second;
            results_.stack.resize(results_.stack.size() - size_);
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

template<typename sm_type, typename iterator>
bool parse(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_,
    std::set<typename sm_type::id_type> *prod_set_)
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

template<typename sm_type, typename iterator, typename token_vector>
bool parse(const sm_type &sm_, iterator &iter_,
    basic_match_results<sm_type> &results_, token_vector &productions_,
    std::multimap<typename sm_type::id_type, token_vector> *prod_map_)
{
    while (results_.entry.action != error)
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
            token<iterator> token_;

            if (size_)
            {
                if (prod_map_)
                {
                    prod_map_->insert(std::make_pair(results_.entry.param,
                        token_vector(productions_.end() - size_,
                            productions_.end())));
                }

                token_.first = (productions_.end() - size_)->first;
                token_.second = productions_.back().second;
                results_.stack.resize(results_.stack.size() - size_);
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

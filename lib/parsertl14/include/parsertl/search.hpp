// search.hpp
// Copyright (c) 2017-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

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
        template<typename lexer_iterator, typename sm_type>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            lexer_iterator& last_eoi_,
            basic_match_results<sm_type>& last_results_);
        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, lexer_iterator& last_eoi_,
            token_vector& productions_);
        template<typename lexer_iterator, typename sm_type>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_);
        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_);
    }

    template<typename lexer_iterator, typename sm_type, typename captures>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        captures& captures_)
    {
        basic_match_results<sm_type> results_(iter_->id, sm_);
        // Qualify token to prevent arg dependant lookup
        using token = parsertl::token<lexer_iterator>;
        using token_vector = typename token::token_vector;
        std::multimap<typename sm_type::id_type, token_vector> prod_map_;
        bool success_ = search(iter_, end_, sm_, &prod_map_);

        captures_.clear();

        if (success_)
        {
            auto last_ = iter_->first;

            captures_.resize((sm_._captures.empty() ? 0 :
                sm_._captures.back().first +
                sm_._captures.back().second.size()) + 1);
            captures_[0].emplace_back(iter_->first, iter_->first);

            for (const auto& pair_ : prod_map_)
            {
                if (sm_._captures.size() > pair_.first)
                {
                    const auto& row_ = sm_._captures[pair_.first];

                    if (!row_.second.empty())
                    {
                        std::size_t index_ = 0;

                        for (const auto& token_ : row_.second)
                        {
                            const auto& token1_ = pair_.second[token_.first];
                            const auto& token2_ = pair_.second[token_.second];
                            auto& entry_ = captures_[row_.first + index_ + 1];

                            entry_.emplace_back(token1_.first, token2_.second);
                            ++index_;
                        }
                    }
                }
            }

            for (const auto& pair_ : prod_map_)
            {
                auto sec_ = pair_.second.back().second;

                if (sec_ > last_)
                {
                    last_ = sec_;
                }
            }

            captures_.front().back().second = last_;
        }

        return success_;
    }

    // Equivalent of std::search().
    template<typename lexer_iterator, typename sm_type>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        std::set<typename sm_type::id_type>* prod_set_ = nullptr)
    {
        bool hit_ = false;
        lexer_iterator curr_ = iter_;
        lexer_iterator last_eoi_;
        // results_ defined here so that allocated memory can be reused.
        basic_match_results<sm_type> results_;
        basic_match_results<sm_type> last_results_;

        end_ = lexer_iterator();

        while (curr_ != end_)
        {
            if (prod_set_)
            {
                prod_set_->clear();
            }

            results_.reset(curr_->id, sm_);
            last_results_.clear();

            while (results_.entry.action != action::accept &&
                results_.entry.action != action::error)
            {
                details::next(curr_, sm_, results_, prod_set_, last_eoi_,
                    last_results_);
            }

            hit_ = results_.entry.action == action::accept;

            if (hit_)
            {
                end_ = curr_;
                break;
            }
            else if (last_eoi_->id != 0)
            {
                lexer_iterator eoi_;

                hit_ = details::parse(eoi_, sm_, last_results_, prod_set_);

                if (hit_)
                {
                    end_ = last_eoi_;
                    break;
                }
            }

            if (iter_->id != 0)
                ++iter_;

            curr_ = iter_;
        }

        return hit_;
    }

    template<typename lexer_iterator, typename sm_type, typename token_vector>
    bool search(lexer_iterator& iter_, lexer_iterator& end_, const sm_type& sm_,
        std::multimap<typename sm_type::id_type, token_vector>*
        prod_map_ = nullptr)
    {
        bool hit_ = false;
        lexer_iterator curr_ = iter_;
        lexer_iterator last_eoi_;
        // results_ and productions_ defined here so that
        // allocated memory can be reused.
        basic_match_results<sm_type> results_;
        token_vector productions_;

        end_ = lexer_iterator();

        while (curr_ != end_)
        {
            if (prod_map_)
            {
                prod_map_->clear();
            }

            results_.reset(curr_->id, sm_);
            productions_.clear();

            while (results_.entry.action != action::accept &&
                results_.entry.action != action::error)
            {
                details::next(curr_, sm_, results_, last_eoi_, productions_);
            }

            hit_ = results_.entry.action == action::accept;

            if (hit_)
            {
                if (prod_map_)
                {
                    lexer_iterator again_(iter_->first, last_eoi_->first,
                        iter_.sm());

                    results_.reset(iter_->id, sm_);
                    productions_.clear();
                    details::parse(again_, sm_, results_, productions_,
                        prod_map_);
                }

                end_ = curr_;
                break;
            }
            else if (last_eoi_->id != 0)
            {
                lexer_iterator again_(iter_->first, last_eoi_->first,
                    iter_.sm());

                results_.reset(iter_->id, sm_);
                productions_.clear();
                hit_ = details::parse(again_, sm_, results_, productions_,
                    prod_map_);

                if (hit_)
                {
                    end_ = last_eoi_;
                    break;
                }
            }

            if (iter_->id != 0)
                ++iter_;

            curr_ = iter_;
        }

        return hit_;
    }

    namespace details
    {
        template<typename lexer_iterator, typename sm_type>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_,
            lexer_iterator& last_eoi_,
            basic_match_results<sm_type>& last_results_)
        {
            switch (results_.entry.action)
            {
            case action::shift:
            {
                const auto eoi_ = sm_.at(results_.entry.param);

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
                        sm_.at(results_.entry.param, results_.token_id);
                }

                if (eoi_.action != action::error)
                {
                    last_eoi_ = iter_;
                    last_results_.stack = results_.stack;
                    last_results_.token_id = 0;
                    last_results_.entry = eoi_;
                }

                break;
            }
            case action::reduce:
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

        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        void next(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, lexer_iterator& last_eoi_,
            token_vector& productions_)
        {
            switch (results_.entry.action)
            {
            case action::shift:
            {
                const auto eoi_ = sm_.at(results_.entry.param);

                results_.stack.push_back(results_.entry.param);
                productions_.emplace_back(iter_->id, iter_->first,
                    iter_->second);

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
                        sm_.at(results_.entry.param, results_.token_id);
                }

                if (eoi_.action != action::error)
                {
                    last_eoi_ = iter_;
                }

                break;
            }
            case action::reduce:
            {
                const std::size_t size_ =
                    sm_._rules[results_.entry.param].second.size();
                token<lexer_iterator> token_;

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
                        token_.first = token_.second =
                            productions_.back().second;
                    }
                }

                results_.token_id = sm_._rules[results_.entry.param].first;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
                token_.id = results_.token_id;
                productions_.push_back(token_);
                break;
            }
            case action::go_to:
                results_.stack.push_back(results_.entry.param);
                results_.token_id = iter_->id;
                results_.entry =
                    sm_.at(results_.stack.back(), results_.token_id);
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

        template<typename lexer_iterator, typename sm_type>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_,
            std::set<typename sm_type::id_type>* prod_set_)
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

                    if (prod_set_)
                    {
                        prod_set_->insert(results_.entry.param);
                    }

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

        template<typename lexer_iterator, typename sm_type,
            typename token_vector>
        bool parse(lexer_iterator& iter_, const sm_type& sm_,
            basic_match_results<sm_type>& results_, token_vector& productions_,
            std::multimap<typename sm_type::id_type, token_vector>* prod_map_)
        {
            while (results_.entry.action != action::error)
            {
                switch (results_.entry.action)
                {
                case action::shift:
                    results_.stack.push_back(results_.entry.param);
                    productions_.emplace_back(iter_->id, iter_->first,
                        iter_->second);

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
                    token<lexer_iterator> token_;

                    if (size_)
                    {
                        if (prod_map_)
                        {
                            prod_map_->insert(std::make_pair(results_.entry.
                                param, token_vector(productions_.end() - size_,
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
                            token_.first = token_.second =
                                productions_.back().second;
                        }
                    }

                    results_.token_id = sm_._rules[results_.entry.param].first;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    token_.id = results_.token_id;
                    productions_.push_back(token_);
                    break;
                }
                case action::go_to:
                    results_.stack.push_back(results_.entry.param);
                    results_.token_id = iter_->id;
                    results_.entry =
                        sm_.at(results_.stack.back(), results_.token_id);
                    break;
                default:
                    // accept
                    // error
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
}

#endif

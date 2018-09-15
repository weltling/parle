// bison_lookup.hpp
// Copyright (c) 2017-2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_BISON_LOOKUP_HPP
#define PARSERTL_BISON_LOOKUP_HPP

#include "match_results.hpp"

namespace parsertl
{
// cut down yyparse():
template<typename iterator, typename tables_struct>
void bison_next(iterator &iter_, match_results &results_,
    const tables_struct &tables_)
{
    if (iter_->id == ~0)
    {
        results_.entry.action = error;
        results_.entry.param = unknown_token;
        return;
    }

    // Refer to what yypact is saying about the current state
    int yyn_ = tables_.yypact[results_.stack.back()];

    if (yyn_ == tables_struct::YYPACT_NINF)
        goto yydefault;

    results_.token_id = tables_.yytranslate[iter_->id];
    yyn_ += results_.token_id;

    if (yyn_ < 0 || tables_struct::YYLAST < yyn_ ||
        tables_.yycheck[yyn_] != results_.token_id)
        goto yydefault;

    yyn_ = tables_.yytable[yyn_];

    if (yyn_ <= 0)
    {
        if (yyn_ == 0 || yyn_ == tables_struct::YYTABLE_NINF)
        {
            results_.entry.action = error;
            results_.entry.param = syntax_error;
            return;
        }

        yyn_ *= -1;
        goto yyreduce;
    }

    // ACCEPT
    if (yyn_ == tables_struct::YYFINAL)
    {
        results_.entry.action = accept;
        results_.entry.param = 0;
        return;
    }

    // SHIFT
    results_.entry.action = shift;
    results_.entry.param = yyn_;
    return;

yydefault:
    yyn_ = tables_.yydefact[results_.stack.back()];

    if (yyn_ == 0)
    {
        results_.entry.action = error;
        results_.entry.param = syntax_error;
        return;
    }

yyreduce:
    results_.entry.action = reduce;
    results_.entry.param = yyn_;
}

template<typename iterator, typename tables_struct>
void bison_lookup(iterator &iter_, match_results &results_,
    const tables_struct &tables_)
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

        break;
    case reduce:
    {
        int size_ = tables_.yyr2[results_.entry.param];

        if (size_)
        {
            results_.stack.resize(results_.stack.size() - size_);
        }

        results_.token_id = tables_.yyr1[results_.entry.param];
        results_.entry.action = go_to;
        results_.entry.param = tables_.yypgoto[results_.token_id -
            tables_struct::YYNTOKENS] + results_.stack.back();
        // Drop through to go_to:
    }
    case go_to:
        if (0 <= results_.entry.param &&
            results_.entry.param <= tables_struct::YYLAST &&
            tables_.yycheck[results_.entry.param] == results_.stack.back())
        {
            results_.entry.param = tables_.yytable[results_.entry.param];
        }
        else
        {
            results_.entry.param = tables_.yydefgoto[results_.token_id -
                tables_struct::YYNTOKENS];
        }

        results_.stack.push_back(results_.entry.param);
        break;
    case accept:
        return;
    }
}

template<typename iterator, typename token_vector, typename tables_struct>
void bison_lookup(iterator &iter_, match_results &results_,
    token_vector &productions_, const tables_struct &tables_)
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

        break;
    case reduce:
    {
        int size_ = tables_.yyr2[results_.entry.param];
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

        results_.token_id = tables_.yyr1[results_.entry.param];
        productions_.push_back(token_);
        results_.entry.action = go_to;
        results_.entry.param = tables_.yypgoto[results_.token_id -
            tables_struct::YYNTOKENS] + results_.stack.back();
        // Drop through to go_to:
    }
    case go_to:
        if (0 <= results_.entry.param &&
            results_.entry.param <= tables_struct::YYLAST &&
            tables_.yycheck[results_.entry.param] == results_.stack.back())
        {
            results_.entry.param = tables_.yytable[results_.entry.param];
        }
        else
        {
            results_.entry.param = tables_.yydefgoto[results_.token_id -
                tables_struct::YYNTOKENS];
        }

        results_.stack.push_back(results_.entry.param);
        break;
    case accept:
        return;
    }
}
}

#endif

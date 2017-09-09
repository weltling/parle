// bison_lookup.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_LOOKUP_HPP
#define PARSERTL_LOOKUP_HPP

#include "match_results.hpp"

namespace parsertl
{
// cut down yyparse():
template<typename iterator>
void bison_next(iterator &iter, parsertl::match_results &results)
{
    if (iter->id == ~0)
    {
        results.entry.action = parsertl::error;
        results.entry.param = parsertl::unknown_token;
        return;
    }

    // Refer to what yypact is saying about the current state
    int yyn = yypact[results.stack.back()];

    if (yyn == YYPACT_NINF)
        goto yydefault;

    results.token_id = yytranslate[iter->id];
    yyn += results.token_id;

    if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != results.token_id)
        goto yydefault;

    yyn = yytable[yyn];

    if (yyn <= 0)
    {
        if (yyn == 0 || yyn == YYTABLE_NINF)
        {
            results.entry.action = parsertl::error;
            results.entry.param = parsertl::syntax_error;
            return;
        }

        yyn *= -1;
        goto yyreduce;
    }

    // ACCEPT
    if (yyn == YYFINAL)
    {
        results.entry.action = parsertl::accept;
        results.entry.param = 0;
        return;
    }

    // SHIFT
    results.entry.action = parsertl::shift;
    results.entry.param = yyn;
    return;

yydefault:
    yyn = yydefact[results.stack.back()];

    if (yyn == 0)
    {
        results.entry.action = parsertl::error;
        results.entry.param = parsertl::syntax_error;
        return;
    }

yyreduce:
    results.entry.action = parsertl::reduce;
    results.entry.param = yyn;
}

template<typename iterator, typename token_vector>
void bison_lookup(iterator &iter_, parsertl::match_results &results_)
{
    switch (results_.entry.action)
    {
    case parsertl::error:
        break;
    case parsertl::shift:
        results_.stack.push_back(results_.entry.param);

        if (results_.token_id != 0)
        {
            ++iter_;
        }

        results_.token_id = iter_->id;

        if (results_.token_id == iterator::value_type::npos())
        {
            results_.entry.action = parsertl::error;
            results_.entry.param = parsertl::unknown_token;
        }

        break;
    case parsertl::reduce:
    {
        int size_ = yyr2[results_.entry.param];

        if (size_)
        {
            results_.stack.resize(results_.stack.size() - size_);
        }

        results_.token_id = yyr1[results_.entry.param];
        results_.entry.action = parsertl::go_to;
        results_.entry.param = yypgoto[results_.token_id - YYNTOKENS] +
            results_.stack.back();
        // Drop through to go_to:
    }
    case parsertl::go_to:
        if (0 <= results_.entry.param && results_.entry.param <= YYLAST &&
            yycheck[results_.entry.param] == results_.stack.back())
        {
            results_.entry.param = yytable[results_.entry.param];
        }
        else
        {
            results_.entry.param = yydefgoto[results_.token_id - YYNTOKENS];
        }

        results_.stack.push_back(results_.entry.param);
        break;
    case parsertl::accept:
        return;
    }
}

template<typename iterator, typename token_vector>
void bison_lookup(iterator &iter_, parsertl::match_results &results_,
    token_vector &productions_)
{
    switch (results_.entry.action)
    {
    case parsertl::error:
        break;
    case parsertl::shift:
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
            results_.entry.action = parsertl::error;
            results_.entry.param = parsertl::unknown_token;
        }

        break;
    case parsertl::reduce:
    {
        int size_ = yyr2[results_.entry.param];
        typename token_vector::value_type token_;

        if (size_)
        {
            token_.first = (productions_.end() - size_)->first;
            token_.second = productions_.back().second;
            results_.stack.resize(results_.stack.size() - size_);
            productions_.resize(productions_.size() - size_);
        }

        results_.token_id = yyr1[results_.entry.param];
        productions_.push_back(token_);
        results_.entry.action = parsertl::go_to;
        results_.entry.param = yypgoto[results_.token_id - YYNTOKENS] +
            results_.stack.back();
        // Drop through to go_to:
    }
    case parsertl::go_to:
        if (0 <= results_.entry.param && results_.entry.param <= YYLAST &&
            yycheck[results_.entry.param] == results_.stack.back())
        {
            results_.entry.param = yytable[results_.entry.param];
        }
        else
        {
            results_.entry.param = yydefgoto[results_.token_id - YYNTOKENS];
        }

        results_.stack.push_back(results_.entry.param);
        break;
    case parsertl::accept:
        return;
    }
}
}

#endif

// ebnf_tables.hpp
// Copyright (c) 2018 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_EBNF_TABLES_HPP
#define PARSERTL_EBNF_TABLES_HPP

#include <vector>

namespace parsertl
{
    struct ebnf_tables
    {
        enum
        {
            YYFINAL = 15,
            YYLAST = 31,
            YYNTOKENS = 18,
            YYPACT_NINF = -7,
            YYTABLE_NINF = -1
        };

        enum yytokentype
        {
            EMPTY = 258,
            IDENTIFIER = 259,
            PREC = 260,
            TERMINAL = 261
        };

        const std::vector<unsigned char> yytranslate =
        {
               0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
              16,    17,    13,    15,     2,    14,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,    10,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     8,     2,     9,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,    11,     7,    12,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
               2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
               5,     6
        };
        const std::vector<unsigned char> yyr1 =
        {
               0,    18,    19,    20,    20,    21,    21,    21,    22,    22,
              23,    23,    23,    23,    23,    23,    23,    23,    23,    24,
              24,    24
        };
        const std::vector<unsigned char> yyr2
        {
               0,     2,     1,     1,     3,     0,     1,     2,     1,     2,
               1,     1,     3,     2,     3,     2,     4,     2,     3,     0,
               2,     2
        };
        const std::vector<unsigned char> yydefact =
        {
               5,     6,    10,    11,     5,     5,     5,     0,     2,     3,
              19,     8,     0,     0,     0,     1,     5,     0,     9,     7,
              13,    15,    17,    12,    14,    18,     4,    20,    21,    16
        };
        const std::vector<char> yydefgoto =
        {
              -1,     7,     8,     9,    10,    11,    19
        };
        const std::vector<char> yypact =
        {
              -3,    -7,    -7,    -7,    -3,    -3,    -3,     2,    11,    -7,
               6,    -6,    12,     8,    -1,    -7,    -3,    22,    -6,    -7,
              -7,    -7,    -7,    -7,    13,    -7,    -7,    -7,    -7,    -7
        };
        const std::vector<char> yypgoto =
        {
              -7,    -7,    19,    14,    -7,    21,    -7
        };
        const std::vector<unsigned char> yytable =
        {
               1,     2,    15,     3,    20,     4,    16,    21,     5,    22,
               2,    17,     3,     6,     4,    16,    25,     5,    16,    16,
              24,    23,     6,    12,    13,    14,    27,    29,    28,     0,
              26,    18
        };
        const std::vector<char> yycheck =
        {
               3,     4,     0,     6,    10,     8,     7,    13,    11,    15,
               4,     5,     6,    16,     8,     7,    17,    11,     7,     7,
              12,     9,    16,     4,     5,     6,     4,    14,     6,    -1,
              16,    10
        };
    };
}

#endif

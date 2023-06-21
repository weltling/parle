// replace.hpp
// Copyright (c) 2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_REPLACE_HPP
#define LEXERTL_REPLACE_HPP

#include "lookup.hpp"
#include "state_machine.hpp"
#include "match_results.hpp"

namespace lexertl
{
    template<class out_iter, class fwd_iter,
        class id_type, class char_type,
        class traits, class alloc>
    out_iter replace(out_iter out, fwd_iter first, fwd_iter second,
        const basic_state_machine<char_type, id_type>& sm,
        const std::basic_string<char_type, traits, alloc>& fmt)
    {
        return replace(out, first, second, sm, fmt.c_str());
    }

    template<class out_iter, class fwd_iter,
        class id_type, class char_type>
    out_iter replace(out_iter out, fwd_iter first, fwd_iter second,
        const basic_state_machine<char_type, id_type>& sm,
        const char_type* fmt)
    {
        const char_type* end_fmt = fmt;
        fwd_iter last = first;
        lexertl::match_results<fwd_iter> results(first, second);

        while (*end_fmt)
            ++end_fmt;

        // Lookahead
        lexertl::lookup(sm, results);

        while (results.id != 0)
        {
            std::copy(last, results.first, out);
            std::copy(fmt, end_fmt, out);
            last = results.second;
            lexertl::lookup(sm, results);
        }

        std::copy(last, results.first, out);
        return out;
    }

    template<class id_type, class char_type,
        class straits, class salloc,
        class ftraits, class falloc>
    std::basic_string<char_type, straits, salloc>
        replace(const std::basic_string<char_type, straits, salloc>& s,
        const basic_state_machine<char_type, id_type>& sm,
        const std::basic_string<char_type, ftraits, falloc>& fmt)
    {
        std::basic_string<char_type, straits, salloc> ret;

        replace(std::back_inserter(ret), s.cbegin(), s.cend(), sm, fmt);
        return ret;
    }

    template<class id_type, class char_type,
        class straits, class salloc>
    std::basic_string<char_type, straits, salloc>
        replace(const std::basic_string<char_type, straits, salloc>& s,
            const basic_state_machine<char_type, id_type>& sm,
            const char_type* fmt)
    {
        std::basic_string<char_type, straits, salloc> ret;

        replace(std::back_inserter(ret), s.cbegin(), s.cend(), sm, fmt);
        return ret;
    }

    template<class id_type, class char_type,
        class straits, class salloc>
    std::basic_string<char_type, straits, salloc>
        replace(const char_type* s,
            const basic_state_machine<char_type, id_type>& sm,
            const std::basic_string<char_type, straits, salloc>& fmt)
    {
        std::basic_string<char_type, straits, salloc> ret;
        const char_type* end_s = s;
        
        while (*end_s)
            ++end_s;

        replace(std::back_inserter(ret), s, end_s, sm, fmt);
        return ret;
    }

    template<class id_type, class char_type>
    std::basic_string<char_type> replace(const char_type* s,
        const basic_state_machine<char_type, id_type>& sm,
        const char_type* fmt)
    {
        std::basic_string<char_type> ret;
        const char_type* end_s = s; while (*end_s) ++end_s;
        const char_type* last = s;
        lexertl::match_results<const char_type*> results(s, end_s);

        // Lookahead
        lexertl::lookup(sm, results);

        while (results.id != 0)
        {
            ret.append(last, results.first);
            ret.append(fmt);
            last = results.second;
            lexertl::lookup(sm, results);
        }

        ret.append(last, results.first);
        return ret;
    }
}

#endif

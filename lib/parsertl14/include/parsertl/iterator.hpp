// iterator.hpp
// Copyright (c) 2022-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_ITERATOR_HPP
#define PARSERTL_ITERATOR_HPP

#include "../../../lexertl14/include/lexertl/iterator.hpp"
#include "lookup.hpp"
#include "match_results.hpp"
#include "token.hpp"

namespace parsertl
{
    template<typename lexer_iterator, typename sm_type,
        typename id_type = std::uint16_t>
    class iterator
    {
    public:
        using results = basic_match_results<sm_type>;
        using value_type = results;
        using difference_type = ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::forward_iterator_tag;

        // Qualify token to prevent arg dependant lookup
        using token = parsertl::token<lexer_iterator>;
        using token_vector = typename token::token_vector;

        iterator() = default;

        iterator(const lexer_iterator& iter_, const sm_type& sm_) :
            _iter(iter_),
            _results(_iter->id, sm_),
            _sm(&sm_)
        {
            // The first action can only ever be reduce
            // if the grammar treats no input as valid.
            if (_results.entry.action != action::reduce)
                lookup();
        }

        iterator(const lexer_iterator& iter_, const sm_type& sm_,
            const std::size_t reserved_) :
            _iter(iter_),
            _results(_iter->id, sm_, reserved_),
            _productions(reserved_),
            _sm(&sm_)
        {
            // The first action can only ever be reduce
            // if the grammar treats no input as valid.
            if (_results.entry.action != action::reduce)
                lookup();
        }

        typename token_vector::value_type dollar(const std::size_t index_) const
        {
            return _results.dollar(index_, *_sm, _productions);
        }

        iterator& operator ++()
        {
            lookup();
            return *this;
        }

        iterator operator ++(int)
        {
            iterator iter_ = *this;

            lookup();
            return iter_;
        }

        const value_type& operator *() const
        {
            return _results;
        }

        const value_type* operator ->() const
        {
            return &_results;
        }

        bool operator ==(const iterator& rhs_) const
        {
            return _sm == rhs_._sm &&
                (_sm == nullptr ? true :
                    _results == rhs_._results);
        }

        bool operator !=(const iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        lexer_iterator _iter;
        basic_match_results<sm_type> _results;
        token_vector _productions;
        const sm_type* _sm = nullptr;

        void lookup()
        {
            // do while because we need to move past the current reduce action
            do
            {
                parsertl::lookup(_iter, *_sm, _results, _productions);
            } while (_results.entry.action == action::shift ||
                _results.entry.action == action::go_to);

            switch (_results.entry.action)
            {
            case action::accept:
            case action::error:
                _sm = nullptr;
                break;
            default:
                break;
            }
        }
    };

    using siterator = iterator<lexertl::siterator, state_machine>;
    using citerator = iterator<lexertl::citerator, state_machine>;
    using wsiterator = iterator<lexertl::wsiterator, state_machine>;
    using wciterator = iterator<lexertl::wciterator, state_machine>;
}

#endif

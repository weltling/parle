// iterator.hpp
// Copyright (c) 2018-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_ITERATOR_HPP
#define PARSERTL_SEARCH_ITERATOR_HPP

#include "capture.hpp"
#include "../../../lexertl14/include/lexertl/iterator.hpp"
#include "match_results.hpp"
#include "search.hpp"

namespace parsertl
{
    template<typename lexer_iterator, typename sm_type,
        typename id_type = std::uint16_t>
    class search_iterator
    {
    public:
        using iter_type = typename lexer_iterator::value_type::iter_type;
        using results = std::vector<std::vector<capture<iter_type>>>;
        using value_type = results;
        using difference_type = ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::forward_iterator_tag;

        search_iterator() = default;

        search_iterator(const lexer_iterator& iter_, const sm_type& sm_) :
            _iter(iter_),
            _sm(&sm_)
        {
            _captures.emplace_back();
            _captures.back().emplace_back(iter_->first, iter_->first);
            lookup();
        }

        search_iterator& operator ++()
        {
            lookup();
            return *this;
        }

        search_iterator operator ++(int)
        {
            search_iterator iter_ = *this;

            lookup();
            return iter_;
        }

        const value_type& operator *() const
        {
            return _captures;
        }

        const value_type* operator ->() const
        {
            return &_captures;
        }

        bool operator ==(const search_iterator& rhs_) const
        {
            return _sm == rhs_._sm &&
                (_sm == nullptr ?
                true :
                _captures == rhs_._captures);
        }

        bool operator !=(const search_iterator& rhs_) const
        {
            return !(*this == rhs_);
        }

    private:
        lexer_iterator _iter;
        results _captures;
        const sm_type* _sm = nullptr;

        void lookup()
        {
            lexer_iterator end;

            _captures.clear();

            if (search(_iter, end, *_sm, _captures))
            {
                _iter = end;
            }
            else
            {
                _sm = nullptr;
            }
        }
    };

    using ssearch_iterator =
        search_iterator<lexertl::siterator, state_machine>;
    using csearch_iterator =
        search_iterator<lexertl::citerator, state_machine>;
    using wssearch_iterator =
        search_iterator<lexertl::wsiterator, state_machine>;
    using wcsearch_iterator =
        search_iterator<lexertl::wciterator, state_machine>;
}

#endif

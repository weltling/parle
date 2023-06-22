// internals.hpp
// Copyright (c) 2009-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_INTERNALS_HPP
#define LEXERTL_INTERNALS_HPP

#include "enum_operator.hpp"
#include "enums.hpp"
#include <memory>
#include <vector>

namespace lexertl
{
    namespace detail
    {
        template<typename id_type>
        struct basic_internals
        {
            using id_type_vector = std::vector<id_type>;
            using id_type_vector_vector = std::vector<id_type_vector>;

            id_type _eoi = 0;
            id_type_vector_vector _lookup;
            id_type_vector _dfa_alphabet;
            id_type _features = 0;
            id_type_vector_vector _dfa;

            void clear()
            {
                _eoi = 0;
                _lookup.clear();
                _dfa_alphabet.clear();
                _features = 0;
                _dfa.clear();
            }

            bool empty() const
            {
                return _dfa.empty();
            }

            void add_states(const std::size_t num_)
            {
                for (std::size_t index_ = 0; index_ < num_; ++index_)
                {
                    // lookup *always* has a size 256 now.
                    _lookup.push_back(id_type_vector(256,
                        static_cast<id_type>(*state_index::dead_state)));
                    _dfa_alphabet.push_back(0);
                    _dfa.emplace_back();
                }
            }

            void swap(basic_internals& internals_) noexcept
            {
                std::swap(_eoi, internals_._eoi);
                _lookup.swap(internals_._lookup);
                _dfa_alphabet.swap(internals_._dfa_alphabet);
                std::swap(_features, internals_._features);
                _dfa.swap(internals_._dfa);
            }
        };
    }
}

#endif

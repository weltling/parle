// serialise.hpp
// Copyright (c) 2007-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LEXERTL_SERIALISE_HPP
#define LEXERTL_SERIALISE_HPP

#include "runtime_error.hpp"
#include "state_machine.hpp"

namespace lexertl
{
    namespace detail
    {
        template<typename char_type, typename id_type, class stream>
        void output_vec(const std::vector<id_type>& vec_, stream& stream_)
        {
            std::basic_ostringstream<char_type> ss_;
            std::basic_string<char_type> str_;
            std::size_t line_len_ = 0;

            stream_ << vec_.size() << '\n';

            for (const id_type l_ : vec_)
            {
                ss_ << l_;
                str_ = ss_.str();

                if (line_len_ + str_.size() + 1 > 80)
                {
                    stream_ << '\n' << str_ << ' ';
                    line_len_ = str_.size() + 1;
                }
                else
                {
                    stream_ << str_ << ' ';
                    line_len_ += str_.size() + 1;
                }

                ss_.str("");
            }

            stream_ << '\n';
        }

        template<typename char_type, class stream, typename id_type>
        void input_vec(stream& stream_, std::vector<id_type>& vec_)
        {
            std::size_t num_ = 0;

            stream_>> num_;
            vec_.reserve(num_);

            for (std::size_t idx_ = 0; idx_ < num_; ++idx_)
            {
                std::size_t id_ = 0;

                stream_ >> id_;
                vec_.push_back(static_cast<id_type>(id_));
            }
        }
    }

    template<typename char_type, typename id_type, class stream>
    void save(const basic_state_machine<char_type, id_type>& sm_,
        stream& stream_)
    {
        using internals = detail::basic_internals<id_type>;
        const internals& internals_ = sm_.data();

        // Version number
        stream_ << 1 << '\n';
        stream_ << sizeof(char_type) << '\n';
        stream_ << sizeof(id_type) << '\n';
        stream_ << internals_._eoi << '\n';
        stream_ << internals_._lookup.size() << '\n';

        for (const auto& vec_ : internals_._lookup)
        {
            detail::output_vec<char_type>(vec_, stream_);
        }

        detail::output_vec<char_type>(internals_._dfa_alphabet, stream_);
        stream_ << internals_._features << '\n';
        stream_ << internals_._dfa.size() << '\n';

        for (const auto& vec_ : internals_._dfa)
        {
            detail::output_vec<char_type>(vec_, stream_);
        }
    }

    template<typename char_type, typename id_type, class stream>
    void load(stream& stream_, basic_state_machine<char_type, id_type>& sm_)
    {
        using internals = detail::basic_internals<id_type>;
        internals& internals_ = sm_.data();
        std::size_t num_ = 0;

        internals_.clear();
        // Version
        stream_ >> num_;
        // sizeof(char_type)
        stream_ >> num_;

        if (num_ != sizeof(char_type))
            throw runtime_error("char_type mismatch in lexertl::load()");

        // sizeof(id_type)
        stream_ >> num_;

        if (num_ != sizeof(id_type))
            throw runtime_error("id_type mismatch in lexertl::load()");

        stream_ >> internals_._eoi;
        stream_ >> num_;
        internals_._lookup.reserve(num_);

        for (std::size_t idx_ = 0; idx_ < num_; ++idx_)
        {
            internals_._lookup.emplace_back();
            detail::input_vec<char_type>(stream_, internals_._lookup.back());
        }

        detail::input_vec<char_type>(stream_, internals_._dfa_alphabet);
        stream_ >> internals_._features;
        stream_ >> num_;
        internals_._dfa.reserve(num_);

        for (std::size_t idx_ = 0; idx_ < num_; ++idx_)
        {
            internals_._dfa.emplace_back();
            detail::input_vec<char_type>(stream_, internals_._dfa.back());
        }
    }
}

#endif

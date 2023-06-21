// serialise.hpp
// Copyright (c) 2007-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SERIALISE_HPP
#define PARSERTL_SERIALISE_HPP

#include "runtime_error.hpp"
#include "../../../lexertl14/include/lexertl/serialise.hpp"
#include "state_machine.hpp"

namespace parsertl
{
    template <typename id_type, class stream>
    void save(const basic_state_machine<id_type>& sm_, stream& stream_)
    {
        // Version number
        stream_ << 1 << '\n';
        stream_ << sizeof(id_type) << '\n';
        stream_ << sm_._columns << '\n';
        stream_ << sm_._rows << '\n';
        stream_ << sm_._rules.size() << '\n';

        for (const auto& rule_ : sm_._rules)
        {
            stream_ << rule_.first << '\n';
            lexertl::detail::output_vec<char>(rule_.second, stream_);
        }

        stream_ << sm_._captures.size() << '\n';

        for (const auto& capture_ : sm_._captures)
        {
            stream_ << capture_.first << '\n';
            stream_ << capture_.second.size() << '\n';

            for (const auto& pair_ : capture_.second)
            {
                stream_ << pair_.first << ' ' << pair_.second << '\n';
            }
        }

        stream_ << sm_._table.size() << '\n';

        for (const auto& vec_ : sm_._table)
        {
            stream_ << vec_.size() << '\n';

            for (const auto& pair_ : vec_)
            {
                stream_ << pair_.first << ' ';
                stream_ << static_cast<std::size_t>(pair_.second.action) << ' ';
                stream_ << pair_.second.param << '\n';
            }
        }
    }

    template <class stream, typename id_type>
    void load(stream& stream_, basic_state_machine<id_type>& sm_)
    {
        std::size_t num_ = 0;

        sm_.clear();
        // Version
        stream_ >> num_;
        // sizeof(id_type)
        stream_ >> num_;

        if (num_ != sizeof(id_type))
            throw runtime_error("id_type mismatch in parsertl::load()");

        stream_ >> sm_._columns;
        stream_ >> sm_._rows;
        stream_ >> num_;
        sm_._rules.reserve(num_);

        for (std::size_t idx_ = 0; idx_ < num_; ++idx_)
        {
            sm_._rules.emplace_back();

            auto& rule_ = sm_._rules.back();

            stream_ >> rule_.first;
            lexertl::detail::input_vec<char>(stream_, rule_.second);
        }

        stream_ >> num_;
        sm_._captures.reserve(num_);

        for (std::size_t idx_ = 0, rows_ = num_; idx_ < rows_; ++idx_)
        {
            sm_._captures.emplace_back();

            auto& capture_ = sm_._captures.back();

            stream_ >> capture_.first;
            stream_ >> num_;
            capture_.second.reserve(num_);

            for (std::size_t idx2_ = 0, entries_ = num_;
                idx2_ < entries_; ++idx2_)
            {
                capture_.second.emplace_back();

                auto& pair_ = capture_.second.back();

                stream_ >> num_;
                pair_.first = static_cast<id_type>(num_);
                stream_ >> num_;
                pair_.second = static_cast<id_type>(num_);
            }
        }

        stream_ >> num_;
        sm_._table.reserve(num_);

        for (std::size_t idx_ = 0, rows_ = num_; idx_ < rows_; ++idx_)
        {
            sm_._table.emplace_back();

            auto& vec_ = sm_._table.back();

            stream_ >> num_;
            vec_.reserve(num_);

            for (std::size_t idx2_ = 0, entries_ = num_;
                idx2_ < entries_; ++idx2_)
            {
                vec_.emplace_back();

                auto& pair_ = vec_.back();

                stream_ >> num_;
                pair_.first = static_cast<id_type>(num_);
                stream_ >> num_;
                pair_.second.action = static_cast<action>(num_);
                stream_ >> num_;
                pair_.second.param = static_cast<id_type>(num_);
            }
        }
    }
}

#endif

// state_machine.hpp
// Copyright (c) 2014-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_STATE_MACHINE_HPP
#define PARSERTL_STATE_MACHINE_HPP

#include <algorithm>
#include <cstdint>
#include "enums.hpp"
#include <vector>

namespace parsertl
{
    template<typename id_ty>
    struct base_state_machine
    {
        using id_type = id_ty;
        using capture_vector = std::vector<std::pair<id_type, id_type>>;
        using captures_vector =
            std::vector<std::pair<std::size_t, capture_vector>>;
        using id_type_vector = std::vector<id_type>;
        using id_type_pair = std::pair<id_type, id_type_vector>;
        using rules = std::vector<id_type_pair>;

        std::size_t _columns = 0;
        std::size_t _rows = 0;
        rules _rules;
        captures_vector _captures;

        // If you get a compile error here you have
        // failed to define an unsigned id type.
        static_assert(std::is_unsigned<id_type>::value,
            "Your id type is signed");

        struct entry
        {
            // Qualify action to prevent compilation error
            parsertl::action action;
            id_type param;

            entry() :
                // Qualify action to prevent compilation error
                action(parsertl::action::error),
                param(static_cast<id_type>(error_type::syntax_error))
            {
            }

            // Qualify action to prevent compilation error
            entry(const parsertl::action action_, const id_type param_) :
                action(action_),
                param(param_)
            {
            }

            void clear()
            {
                // Qualify action to prevent compilation error
                action = parsertl::action::error;
                param = static_cast<id_type>(error_type::syntax_error);
            }

            bool operator ==(const entry& rhs_) const
            {
                return action == rhs_.action && param == rhs_.param;
            }
        };

        // No need to specify constructor.
        // Just in case someone wants to use a pointer to the base
        virtual ~base_state_machine() = default;

        virtual void clear()
        {
            _columns = _rows = 0;
            _rules.clear();
            _captures.clear();
        }
    };

    // Uses a vector of vectors for the state machine
    template<typename id_ty>
    class basic_state_machine : public base_state_machine<id_ty>
    {
    public:
        using base_sm = base_state_machine<id_ty>;
        using id_type = id_ty;
        using entry = typename base_sm::entry;
        using table = std::vector<std::vector<std::pair<id_type, entry>>>;

        // No need to specify constructor.
        ~basic_state_machine() override = default;

        void clear() override
        {
            base_sm::clear();
            _table.clear();
        }

        bool empty() const
        {
            return _table.empty();
        }

        entry at(const std::size_t state_) const
        {
            const auto& s_ = _table[state_];
            auto iter_ = std::find_if(s_.begin(), s_.end(),
                [](const auto& pair)
                {
                    return pair.first == 0;
                });

            if (iter_ == s_.end())
                return entry();
            else
                return iter_->second;
        }

        entry at(const std::size_t state_, const std::size_t token_id_) const
        {
            const auto& s_ = _table[state_];
            auto iter_ = std::find_if(s_.begin(), s_.end(),
                [token_id_](const auto& pair)
                {
                    return pair.first == token_id_;
                });

            if (iter_ == s_.end())
                return entry();
            else
                return iter_->second;
        }

        void set(const std::size_t state_, const std::size_t token_id_,
            const entry& entry_)
        {
            auto& s_ = _table[state_];
            auto iter_ = std::find_if(s_.begin(), s_.end(),
                [token_id_](const auto& pair)
                {
                    return pair.first == token_id_;
                });

            if (iter_ == s_.end())
                s_.emplace_back(static_cast<id_type>(token_id_), entry_);
            else
                iter_->second = entry_;
        }

        void push()
        {
            _table.resize(base_sm::_rows);
        }

    private:
        table _table;
    };

    // Uses uncompressed 2d array for state machine
    template<typename id_ty>
    class basic_uncompressed_state_machine : public base_state_machine<id_ty>
    {
    public:
        using base_sm = base_state_machine<id_ty>;
        using id_type = id_ty;
        using entry = typename base_sm::entry;
        using table = std::vector<entry>;

        // No need to specify constructor.
        ~basic_uncompressed_state_machine() override = default;

        void clear() override
        {
            base_sm::clear();
            _table.clear();
        }

        bool empty() const
        {
            return _table.empty();
        }

        entry at(const std::size_t state_) const
        {
            return _table[state_ * base_sm::_columns];
        }

        entry at(const std::size_t state_, const std::size_t token_id_) const
        {
            return _table[state_ * base_sm::_columns + token_id_];
        }

        void set(const std::size_t state_, const std::size_t token_id_,
            const entry& entry_)
        {
            _table[state_ * base_sm::_columns + token_id_] = entry_;
        }

        void push()
        {
            _table.resize(base_sm::_columns * base_sm::_rows);
        }

    private:
        table _table;
    };

    using state_machine = basic_state_machine<uint16_t>;
    using uncompressed_state_machine =
        basic_uncompressed_state_machine<uint16_t>;
}

#endif

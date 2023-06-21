// debug.hpp
// Copyright (c) 2014-2023 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_DEBUG_HPP
#define PARSERTL_DEBUG_HPP

#include "dfa.hpp"
#include <ostream>
#include "rules.hpp"

namespace parsertl
{
    template<typename char_type>
    class basic_debug
    {
    public:
        using rules = basic_rules<char_type>;
        using ostream = std::basic_ostream<char_type>;

        static void dump(const rules& rules_, ostream& stream_)
        {
            const std::size_t start_ = rules_.start();
            const production_vector& grammar_ = rules_.grammar();
            const token_info_vector& tokens_info_ = rules_.tokens_info();
            const std::size_t terminals_ = tokens_info_.size();
            string_vector symbols_;
            std::set<std::size_t> seen_;
            token_map map_;

            rules_.symbols(symbols_);

            // Skip EOI token
            for (std::size_t idx_ = 1, size_ = tokens_info_.size();
                idx_ < size_; ++idx_)
            {
                const token_info& token_info_ = tokens_info_[idx_];
                token_prec_assoc info_(token_info_._precedence,
                    token_info_._associativity);
                auto map_iter_ = map_.find(info_);

                if (map_iter_ == map_.end())
                {
                    map_.insert(token_pair(info_, symbols_[idx_]));
                }
                else
                {
                    map_iter_->second += static_cast<char_type>(' ');
                    map_iter_->second += symbols_[idx_];
                }
            }

            for (const auto& pair_ : map_)
            {
                switch (pair_.first.second)
                {
                case rules::associativity::token_assoc:
                    token(stream_);
                    break;
                case rules::associativity::precedence_assoc:
                    precedence(stream_);
                    break;
                case rules::associativity::non_assoc:
                    nonassoc(stream_);
                    break;
                case rules::associativity::left_assoc:
                    left(stream_);
                    break;
                case rules::associativity::right_assoc:
                    right(stream_);
                    break;
                default:
                    break;
                }

                stream_ << pair_.second << static_cast<char_type>('\n');
            }

            if (start_ != static_cast<std::size_t>(~0))
            {
                stream_ << static_cast<char_type>('\n');
                start(stream_);
                stream_ << symbols_[terminals_ + start_] <<
                    static_cast<char_type>('\n') <<
                    static_cast<char_type>('\n');
            }

            stream_ << static_cast<char_type>('%') <<
                static_cast<char_type>('%') <<
                static_cast<char_type>('\n') <<
                static_cast<char_type>('\n');
            dump_grammar(grammar_, seen_, symbols_, terminals_, stream_);
            stream_ << static_cast<char_type>('%') <<
                static_cast<char_type>('%') <<
                static_cast<char_type>('\n');
        }

        static void dump(const rules& rules_, const dfa& dfa_, ostream& stream_)
        {
            const production_vector& grammar_ = rules_.grammar();
            const std::size_t terminals_ = rules_.tokens_info().size();
            string_vector symbols_;

            rules_.symbols(symbols_);

            for (std::size_t idx_ = 0, dfa_size_ = dfa_.size();
                idx_ < dfa_size_; ++idx_)
            {
                const dfa_state& state_ = dfa_[idx_];
                const size_t_pair_vector& config_ = state_._closure;

                state(idx_, stream_);

                for (const auto& pair_ : config_)
                {
                    const production& p_ = grammar_[pair_.first];
                    std::size_t j_ = 0;

                    stream_ << static_cast<char_type>(' ') <<
                        static_cast<char_type>(' ') <<
                        symbols_[terminals_ + p_._lhs] <<
                        static_cast<char_type>(' ') <<
                        static_cast<char_type>('-') <<
                        static_cast<char_type>('>');
                    dump_rhs(j_, p_, terminals_, pair_, symbols_, stream_);

                    if (j_ == pair_.second)
                    {
                        stream_ << static_cast<char_type>(' ') <<
                            static_cast<char_type>('.');
                    }

                    stream_ << static_cast<char_type>('\n');
                }

                if (!state_._transitions.empty())
                    stream_ << static_cast<char_type>('\n');

                for (const auto& pair_ : state_._transitions)
                {
                    stream_ << static_cast<char_type>(' ') <<
                        static_cast<char_type>(' ') <<
                        symbols_[pair_.first] <<
                        static_cast<char_type>(' ') <<
                        static_cast<char_type>('-') <<
                        static_cast<char_type>('>') <<
                        static_cast<char_type>(' ') << pair_.second <<
                        static_cast<char_type>('\n');
                }

                stream_ << static_cast<char_type>('\n');
            }
        }

    private:
        using production = typename rules::production;
        using production_vector = typename rules::production_vector;
        using string = std::basic_string<char_type>;
        using string_vector = typename rules::string_vector;
        using symbol = typename rules::symbol;
        using token_prec_assoc =
            std::pair<std::size_t, typename rules::associativity>;
        using token_info = typename rules::token_info;
        using token_info_vector = typename rules::token_info_vector;
        using token_map = std::map<token_prec_assoc, string>;
        using token_pair = std::pair<token_prec_assoc, string>;

        static void dump_grammar(const production_vector& grammar_,
            std::set<std::size_t>& seen_, const string_vector& symbols_,
            const std::size_t terminals_, ostream& stream_)
        {
            for (auto iter_ = grammar_.cbegin(), end_ = grammar_.cend();
                iter_ != end_; ++iter_)
            {
                if (seen_.find(iter_->_lhs) == seen_.end())
                {
                    auto lhs_iter_ = iter_;
                    std::size_t index_ = lhs_iter_ - grammar_.begin();

                    stream_ << symbols_[terminals_ + lhs_iter_->_lhs];
                    stream_ << static_cast<char_type>(':');

                    while (index_ != static_cast<std::size_t>(~0))
                        dump_production(grammar_, lhs_iter_, symbols_,
                            terminals_, index_, stream_);

                    seen_.insert(iter_->_lhs);
                    stream_ << static_cast<char_type>(';') <<
                        static_cast<char_type>('\n') <<
                        static_cast<char_type>('\n');
                }
            }
        }

        static void dump_production(const production_vector& grammar_,
            typename production_vector::const_iterator& lhs_iter_,
            const string_vector& symbols_, const std::size_t terminals_,
            std::size_t& index_, ostream& stream_)
        {
            if (lhs_iter_->_rhs.first.empty())
            {
                stream_ << static_cast<char_type>(' ');
                empty(stream_);
            }
            else
            {
                auto rhs_iter_ = lhs_iter_->_rhs.first.cbegin();
                auto rhs_end_ = lhs_iter_->_rhs.first.cend();

                for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
                {
                    const std::size_t id_ =
                        rhs_iter_->_type == symbol::type::TERMINAL ?
                        rhs_iter_->_id :
                        terminals_ + rhs_iter_->_id;

                    // Don't dump '$'
                    if (id_ > 0)
                    {
                        stream_ << static_cast<char_type>(' ') <<
                            symbols_[id_];
                    }
                }
            }

            if (!lhs_iter_->_rhs.second.empty())
            {
                stream_ << static_cast<char_type>(' ');
                prec(stream_);
                stream_ << lhs_iter_->_rhs.second;
            }

            index_ = lhs_iter_->_next_lhs;

            if (index_ != static_cast<std::size_t>(~0))
            {
                const string& lhs_ =
                    symbols_[terminals_ + lhs_iter_->_lhs];

                lhs_iter_ = grammar_.cbegin() + index_;
                stream_ << static_cast<char_type>('\n');

                for (std::size_t i_ = 0, size_ = lhs_.size();
                    i_ < size_; ++i_)
                {
                    stream_ << static_cast<char_type>(' ');
                }

                stream_ << static_cast<char_type>('|');
            }
        }

        static void dump_rhs(std::size_t& j_, const production& p_,
            const std::size_t terminals_, const size_t_pair& pair_,
            const string_vector& symbols_, ostream& stream_)
        {
            for (; j_ < p_._rhs.first.size(); ++j_)
            {
                const symbol& symbol_ = p_._rhs.first[j_];
                const std::size_t id_ = symbol_._type ==
                    symbol::type::TERMINAL ? symbol_._id :
                    terminals_ + symbol_._id;

                if (j_ == pair_.second)
                {
                    stream_ << static_cast<char_type>(' ') <<
                        static_cast<char_type>('.');
                }

                stream_ << static_cast<char_type>(' ') <<
                    symbols_[id_];
            }
        }

        static void empty(std::ostream& stream_)
        {
            stream_ << "%empty";
        }

        static void empty(std::wostream& stream_)
        {
            stream_ << L"%empty";
        }

        static void empty(std::basic_ostream<char32_t> & stream_)
        {
            stream_ << U"%empty";
        }

        static void left(std::ostream& stream_)
        {
            stream_ << "%left ";
        }

        static void left(std::wostream& stream_)
        {
            stream_ << L"%left ";
        }

        static void left(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%left ";
        }

        static void nonassoc(std::ostream& stream_)
        {
            stream_ << "%nonassoc ";
        }

        static void nonassoc(std::wostream& stream_)
        {
            stream_ << L"%nonassoc ";
        }

        static void nonassoc(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%nonassoc ";
        }

        static void prec(std::ostream& stream_)
        {
            stream_ << "%prec ";
        }

        static void prec(std::wostream& stream_)
        {
            stream_ << L"%prec ";
        }

        static void prec(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%prec ";
        }

        static void precedence(std::ostream& stream_)
        {
            stream_ << "%precedence ";
        }

        static void precedence(std::wostream& stream_)
        {
            stream_ << L"%precedence ";
        }

        static void precedence(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%precedence ";
        }

        static void right(std::ostream& stream_)
        {
            stream_ << "%right ";
        }

        static void right(std::wostream& stream_)
        {
            stream_ << L"%right ";
        }

        static void right(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%right ";
        }

        static void start(std::ostream& stream_)
        {
            stream_ << "%start ";
        }

        static void start(std::wostream& stream_)
        {
            stream_ << L"%start ";
        }

        static void start(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%start ";
        }

        static void state(const std::size_t row_, std::ostream& stream_)
        {
            stream_ << "state " << row_ << '\n' << '\n';
        }

        static void state(const std::size_t row_, std::wostream& stream_)
        {
            stream_ << L"state " << row_ << L'\n' << L'\n';
        }

        static void state(const std::size_t row_,
            std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"state " << row_ << U'\n' << U'\n';
        }

        static void token(std::ostream& stream_)
        {
            stream_ << "%token ";
        }

        static void token(std::wostream& stream_)
        {
            stream_ << L"%token ";
        }

        static void token(std::basic_ostream<char32_t>& stream_)
        {
            stream_ << U"%token ";
        }
    };

    using debug = basic_debug<char>;
    using wdebug = basic_debug<wchar_t>;
    using u32debug = basic_debug<char32_t>;
}

#endif

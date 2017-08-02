// rules.hpp
// Copyright (c) 2014-2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_RULES_HPP
#define PARSERTL_RULES_HPP

#include "../../lexertl14/lexertl/generator.hpp"
#include "../../lexertl14/lexertl/iterator.hpp"
#include "narrow.hpp"
#include "runtime_error.hpp"

namespace parsertl
{
template<typename T>
class basic_rules
{
public:
    using char_type = T;

    struct nt_location
    {
        std::size_t _first_production;
        std::size_t _last_production;

        nt_location() :
            _first_production(static_cast<std::size_t>(~0)),
            _last_production(static_cast<std::size_t>(~0))
        {
        }
    };

    using nt_location_vector = std::vector<nt_location>;
    using string = std::basic_string<char_type>;
    using string_size_t_map = std::map<string, std::size_t>;
    using string_size_t_pair = std::pair<string, std::size_t>;
    using string_vector = std::vector<string>;

    struct symbol
    {
        enum type { TERMINAL, NON_TERMINAL };

        type _type;
        std::size_t _id;

        symbol(const type type_, const std::size_t id_) :
            _type(type_),
            _id(id_)
        {
        }

        bool operator<(const symbol &rhs_) const
        {
            return _type < rhs_._type ||
                _type == rhs_._type && _id < rhs_._id;
        }

        bool operator==(const symbol &rhs_) const
        {
            return _type == rhs_._type && _id == rhs_._id;
        }
    };

    using symbol_vector = std::vector<symbol>;

    struct production
    {
        std::size_t _lhs;
        symbol_vector _rhs;
        std::size_t _precedence;
        std::size_t _index;
        std::size_t _next_lhs;

        production(const std::size_t index_) :
            _lhs(static_cast<std::size_t>(~0)),
            _precedence(0),
            _index(index_),
            _next_lhs(static_cast<std::size_t>(~0))
        {
        }

        void clear()
        {
            _lhs = static_cast<std::size_t>(~0);
            _rhs.clear();
            _precedence = 0;
            _index = static_cast<std::size_t>(~0);
            _next_lhs = static_cast<std::size_t>(~0);
        }
    };

    using production_vector = std::vector<production>;

    struct token_info
    {
        enum associativity { token, precedence, nonassoc, left, right };
        std::size_t _precedence;
        associativity _associativity;

        token_info() :
            _precedence(0),
            _associativity(token)
        {
        }

        token_info(const std::size_t precedence_,
            const associativity associativity_) :
            _precedence(precedence_),
            _associativity(associativity_)
        {
        }
    };

    using token_info_vector = std::vector<token_info>;

    basic_rules() :
        _next_precedence(1)
    {
        lexer_rules rules_;

        rules_.insert_macro("LITERAL",
            "'(\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|[^'])+'|"
            "[\"](\\\\([^0-9cx]|[0-9]{1,3}|c[@a-zA-Z]|x\\d+)|[^\"])+[\"]");
        rules_.insert_macro("SYMBOL", "[A-Za-z_.][-A-Za-z_.0-9]*");
        rules_.push("{LITERAL}", LITERAL);
        rules_.push("{SYMBOL}", SYMBOL);
        rules_.push("\\s+", rules_.skip());
        lexer_generator::build(rules_, _token_lexer);

        rules_.push_state("CODE");
        rules_.push_state("EMPTY");
        rules_.push_state("PREC");

        rules_.push("INITIAL,CODE", "[{]", ">CODE");
        rules_.push("CODE", "'(\\\\.|[^'])*'", ".");
        rules_.push("CODE", "[\"](\\\\.|[^\"])*[\"]", ".");
        rules_.push("CODE", "<%", ">CODE");
        rules_.push("CODE", "%>", "<");
        rules_.push("CODE", "[^}]", ".");
        rules_.push("CODE", "[}]", rules_.skip(), "<");

        rules_.push("INITIAL", "%empty", rules_.skip(), "EMPTY");
        rules_.push("INITIAL", "%prec", rules_.skip(), "PREC");
        rules_.push("PREC", "{LITERAL}|{SYMBOL}", PREC, "INITIAL");
        rules_.push("INITIAL,EMPTY", "[|]", OR, "INITIAL");
        rules_.push("INITIAL,CODE,EMPTY,PREC", "[/][*](.|\n)*?[*][/]|[/][/].*",
            rules_.skip(), ".");
        rules_.push("EMPTY,PREC", "\\s+", rules_.skip(), ".");
        lexer_generator::build(rules_, _rule_lexer);

        const std::size_t id_ = insert_terminal(string(1, '$'));

        info(id_);
    }

    void token(const char_type *names_)
    {
        token(names_, 0, token_info::token, "token");
    }

    void left(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::left, "left");
        ++_next_precedence;
    }

    void right(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::right, "right");
        ++_next_precedence;
    }

    void nonassoc(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::nonassoc, "nonassoc");
        ++_next_precedence;
    }

    void precedence(const char_type *names_)
    {
        token(names_, _next_precedence, token_info::precedence, "precedence");
        ++_next_precedence;
    }

    std::size_t push(const char_type *lhs_, const char_type *rhs_)
    {
        const string lhs_str_ = lhs_;

        validate(lhs_);

        if (_terminals.find(lhs_str_) != _terminals.end())
        {
            std::ostringstream ss_;

            ss_ << "Rule ";
            narrow(lhs_, ss_);
            ss_ << " is already defined as a TERMINAL.";
            throw runtime_error(ss_.str());
        }

        push_production(lhs_str_, rhs_);
        return _grammar.size() - 1;
    }

    std::size_t token_id(const char_type *name_) const
    {
        typename string_size_t_map::const_iterator iter_ =
            _terminals.find(name_);

        if (iter_ == _terminals.end())
        {
            std::ostringstream ss_;

            ss_ << "Unknown token '";
            narrow(name_, ss_);
            ss_ << "'.";
            throw runtime_error(ss_.str());
        }

        return iter_->second;
    }

    void start(const char_type *start_)
    {
        validate(start_);
        _start = start_;
    }

    const size_t start() const
    {
        return _start.empty() ?
            npos() :
            _non_terminals.find(_start)->second;
    }

    void validate()
    {
        if (_grammar.empty())
        {
            throw runtime_error("No productions are defined.");
        }

        std::size_t start_ = npos();

        if (_start.empty())
        {
            const std::size_t id_ = _grammar[0]._lhs;

            _start = name_from_id(id_);

            if (!_start.empty())
                start_ = id_;
        }
        else
        {
            typename string_size_t_map::const_iterator iter_ =
                _non_terminals.find(_start);

            if (iter_ != _non_terminals.end())
                start_ = iter_->second;
        }

        if (start_ == npos())
        {
            throw runtime_error("Specified start rule does not exist.");
        }

        // Validate start rule
        if (start_ >= _nt_locations.size() ||
            _grammar[_nt_locations[start_]._first_production].
                _rhs.size() != 1)
        {
            static char_type accept_ [] =
                { '$', 'a', 'c', 'c', 'e', 'p', 't', 0 };
            string rhs_ = _start;

            push_production(accept_, rhs_);
            _grammar.back()._rhs.push_back
                (symbol(symbol::TERMINAL, insert_terminal(string(1, '$'))));
            _start = accept_;
        }
        else
        {
            _grammar[_nt_locations[start_]._first_production]._rhs.
                push_back(symbol(symbol::TERMINAL,
                    insert_terminal(string(1, '$'))));

            for (const auto &p_ : _grammar)
            {
                for (const auto &s_ : p_._rhs)
                {
                    if (s_._type == symbol::NON_TERMINAL && s_._id == start_)
                    {
                        std::ostringstream ss_;
                        const string name_ = name_from_id(p_._lhs);

                        ss_ << "The start symbol occurs on the RHS of rule '";
                        narrow(name_.c_str(), ss_);
                        ss_ << "'.";
                        throw runtime_error(ss_.str());
                    }
                }
            }
        }

        // Validate all non-terminals.
        for (std::size_t i_ = 0, size_ = _nt_locations.size();
            i_ < size_; ++i_)
        {
            if (_nt_locations[i_]._first_production == npos())
            {
                std::ostringstream ss_;
                const string name_ = name_from_id(i_);

                ss_ << "Non-terminal '";
                narrow(name_.c_str(), ss_);
                ss_ << "' does not have any productions.";
                throw runtime_error(ss_.str());
            }
        }
    }

    const production_vector &grammar() const
    {
        return _grammar;
    }

    const token_info_vector &tokens_info() const
    {
        return _tokens_info;
    }

    const nt_location_vector &nt_locations() const
    {
        return _nt_locations;
    }

    void terminals(string_vector &vec_) const
    {
        vec_.resize(_terminals.size());

        for (const auto &pair_ : _terminals)
        {
            vec_[pair_.second] = pair_.first;
        }
    }

    void non_terminals(string_vector &vec_) const
    {
        const std::size_t size_ = vec_.size();

        vec_.resize(size_ + _non_terminals.size());

        for (const auto &pair_ : _non_terminals)
        {
            vec_[size_ + pair_.second] = pair_.first;
        }
    }

    void symbols(string_vector &vec_) const
    {
        terminals(vec_);
        non_terminals(vec_);
    }

    std::size_t npos() const
    {
        return static_cast<std::size_t>(~0);
    }

private:
    using lexer_rules = typename lexertl::basic_rules<char, char_type>;
    using lexer_state_machine =
        typename lexertl::basic_state_machine<char_type>;
    using lexer_generator =
        typename lexertl::basic_generator<lexer_rules, lexer_state_machine>;
    using lexer_iterator =
        typename lexertl::iterator<const char_type *, lexer_state_machine,
        typename lexertl::recursive_match_results<const char_type *>>;

    enum type { LITERAL = 1, SYMBOL, PREC, OR };

    std::size_t _next_precedence;
    lexer_state_machine _rule_lexer;
    lexer_state_machine _token_lexer;
    string_size_t_map _terminals;
    token_info_vector _tokens_info;
    string_size_t_map _non_terminals;
    nt_location_vector _nt_locations;
    string _start;
    production_vector _grammar;

    token_info &info(const std::size_t id_)
    {
        if (_tokens_info.size() <= id_)
        {
            _tokens_info.resize(id_ + 1);
        }

        return _tokens_info[id_];
    }

    string name_from_id(const std::size_t id_)
    {
        string name_;

        for (const auto &pair_ : _non_terminals)
        {
            if (pair_.second == id_)
            {
                name_ = pair_.first;
                break;
            }
        }

        return name_;
    }

    void token(const char_type *names_, const std::size_t precedence_,
        const typename token_info::associativity associativity_,
        const char *func_)
    {
        lexer_iterator iter_(names_, str_end(names_), _token_lexer);
        lexer_iterator end_;
        string token_;
        std::size_t id_ = static_cast<std::size_t>(~0);

        for (; iter_ != end_; ++iter_)
        {
            if (iter_->id == _token_lexer.npos())
            {
                std::ostringstream ss_;

                ss_ << "Unrecognised char in " << func_ << "().";
                throw runtime_error(ss_.str());
            }

            token_ = iter_->str();
            id_ = insert_terminal(token_);

            token_info &token_info_ = info(id_);

            token_info_._precedence = precedence_;
            token_info_._associativity = associativity_;
        }
    }

    void validate(const char_type *name_) const
    {
        const char_type *start_ = name_;

        do
        {
            if (!(*name_ >= 'A' && *name_ <= 'Z') &&
                !(*name_ >= 'a' && *name_ <= 'z') &&
                *name_ != '_' && *name_ != '.' &&
                !(*name_ >= '0' && *name_ <= '9') &&
                *name_ != '-')
            {
                std::ostringstream ss_;

                ss_ << "Invalid name '";
                name_ = start_;
                narrow(name_, ss_);
                ss_ << "'.";
                throw runtime_error(ss_.str());
            }

            ++name_;
        } while (*name_);
    }

    std::size_t insert_terminal(const string &str_)
    {
        return _terminals.insert
            (string_size_t_pair(str_, _terminals.size())).first->second;
    }

    std::size_t nt_id(const string &str_)
    {
        return _non_terminals.insert
            (string_size_t_pair(str_, _non_terminals.size())).
                first->second;
    }

    const char_type *str_end(const char_type *str_)
    {
        while (*str_) ++str_;

        return str_;
    }

    void push_production(const string &lhs_, const string &rhs_)
    {
        const std::size_t lhs_id_ = nt_id(lhs_);
        nt_location &location_ = location(lhs_id_);
        production production_(_grammar.size());
        lexer_iterator iter_(rhs_.c_str(), rhs_.c_str() + rhs_.size(),
            _rule_lexer);
        lexer_iterator end_;

        if (location_._first_production == npos())
        {
            location_._first_production = production_._index;
        }

        if (location_._last_production != npos())
        {
            _grammar[location_._last_production]._next_lhs =
                production_._index;
        }

        location_._last_production = production_._index;
        production_._lhs = lhs_id_;

        for (; iter_ != end_; ++iter_)
        {
            switch (iter_->id)
            {
            case LITERAL:
            {
                string token_ = iter_->str();
                const std::size_t id_ = insert_terminal(token_);
                token_info &token_info_ = info(id_);

                production_._precedence = token_info_._precedence;
                production_._rhs.push_back(symbol(symbol::TERMINAL, id_));
                break;
            }
            case SYMBOL:
            {
                string token_ = iter_->str();
                typename string_size_t_map::const_iterator terminal_iter_ =
                    _terminals.find(token_);

                if (terminal_iter_ == _terminals.end())
                {
                    const std::size_t id_ = nt_id(token_);

                    // NON_TERMINAL
                    location(id_);
                    production_._rhs.push_back
                    (symbol(symbol::NON_TERMINAL, id_));
                }
                else
                {
                    const std::size_t id_ = terminal_iter_->second;
                    token_info &token_info_ = info(id_);

                    production_._precedence = token_info_._precedence;
                    production_._rhs.push_back(symbol(symbol::TERMINAL, id_));
                }

                break;
            }
            case PREC:
            {
                string token_ = iter_->str();
                const std::size_t id_ = insert_terminal(token_);

                if (id_ >= _tokens_info.size())
                {
                    std::ostringstream ss_;

                    ss_ << "Unknown token ";
                    narrow(token_.c_str(), ss_);
                    ss_ << '.';
                    throw runtime_error(ss_.str());
                }

                production_._precedence = info(id_)._precedence;
                break;
            }
            case OR:
            {
                std::size_t old_lhs_ = production_._lhs;
                std::size_t index_ = _grammar.size() + 1;
                nt_location &loc_ = location(old_lhs_);

                production_._next_lhs = loc_._last_production = index_;
                _grammar.push_back(production_);
                production_.clear();
                production_._lhs = old_lhs_;
                production_._index = index_;
                break;
            }
            default:
            {
                std::ostringstream ss_;
                const char_type *l_ = lhs_.c_str();
                const char_type *r_ = rhs_.c_str();

                assert(0);
                ss_ << "Syntax error in rule '";
                narrow(l_, ss_);
                ss_ << "': '";
                narrow(r_, ss_);
                ss_ << "'.";
                throw runtime_error(ss_.str());
                break;
            }
            }
        }

        _grammar.push_back(production_);
    }

    nt_location &location(const std::size_t id_)
    {
        if (_nt_locations.size() <= id_)
        {
            _nt_locations.resize(id_ + 1);
        }

        return _nt_locations[id_];
    }
};

using rules = basic_rules<char>;
using wrules = basic_rules<wchar_t>;
}

#endif

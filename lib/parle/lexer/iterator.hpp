// Based on lexertl/iterator.hpp

#ifndef PARLE_LEXER_ITERATOR_HPP
#define PARLE_LEXER_ITERATOR_HPP

#include <iterator>
#include <unordered_map>
#include "lexertl/lookup.hpp"
#include "lexertl/runtime_error.hpp"

#undef lookup

namespace parle
{
namespace lexer
{
template<typename iter, typename sm_type, typename results, typename lexer_obj_type, typename token_cb_type, typename id_type>
class iterator
{
public:
	using value_type = results;
	using difference_type = ptrdiff_t;
	using pointer = const value_type *;
	using reference = const value_type &;
	using iterator_category = std::forward_iterator_tag;
	using cb_map = std::unordered_map<id_type, token_cb_type>;

	iterator() :
		_results(iter(), iter()),
		_sm(nullptr),
		_lex(nullptr)
	{
	}

	iterator(const iter &start_, const iter &end_, lexer_obj_type &lex, bool do_next = false) :
		_results(start_, end_),
		_sm(&lex.sm),
		_lex(&lex)
	{

		if (do_next) {
			lookup();
		}
	}

	void set_bol(bool bol)
	{
		_results.bol = bol;
	}

	void reset(const iter &start_, const iter &end_)
	{
		if (_results.first > start_) {
			throw lexertl::runtime_error("Can only reset to a forward position");
		}
		_results.first = start_;
		_results.second = start_;
		_results.eoi = end_;
	}

	// Only need this because of warnings with gcc with -Weffc++
	iterator(const iterator &rhs_)
	{
		_results = rhs_._results;
		_sm = rhs_._sm;
		_lex = rhs_._lex;
	}

	// Only need this because of warnings with gcc with -Weffc++
	iterator &operator =(const iterator &rhs_)
	{
		if (&rhs_ != this)
		{
			_results = rhs_._results;
			_sm = rhs_._sm;
			_lex = rhs_._lex;
		}

		return *this;
	}

	iterator &operator ++()
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

	const value_type &operator *() const
	{
		return _results;
	}

	const value_type *operator ->() const
	{
		return &_results;
	}

	bool operator ==(const iterator &rhs_) const
	{
		return _sm == rhs_._sm && (_sm == nullptr ? true :
			_results == rhs_._results);
	}

	bool operator !=(const iterator &rhs_) const
	{
		return !(*this == rhs_);
	}

public:
	size_t line = 0;
private:
	value_type _results;
	const sm_type *_sm;
	lexer_obj_type *_lex;

	void lookup()
	{
		if (_results.bol) {
			line++;
			std::cout << "line: " << line << std::endl;
		}

		lexertl::lookup(*_sm, _results);

		if (_lex->cb_map.size() > 0) {
			auto it = _lex->cb_map.find(_results.id);
			if (_lex->cb_map.end() != it) {
				zval result;
				token_cb_type cb = it->second;
				zend_fcall_info fci;
				zend_fcall_info_cache fcc;

				if (FAILURE == zend_fcall_info_init(&cb.cb, 0, &fci, &fcc, NULL, NULL)) {
					zend_throw_exception_ex(ParleLexerException_ce, 0, "Failed to prepare function call");
					if (_results.first == _results.eoi) {
						_sm = nullptr;
					}
					return;
				}
				ZVAL_NULL(&result);
				fci.retval = &result;
				fci.param_count = 0;

				if (FAILURE == zend_call_function(&fci, &fcc)) {
					zend_throw_exception_ex(ParleLexerException_ce, 0, "Callback execution failed");
					if (_results.first == _results.eoi) {
						_sm = nullptr;
					}
					return;
				}

#if 0
				convert_to_boolean(&result);
				if (Z_TYPE(result) == IS_FALSE && _results.first != _results.eoi) {
					lexertl::lookup(*_sm, _results);
				}
#endif
			}
		}

		if (_results.first == _results.eoi) {
			_sm = nullptr;
		}
	}
};
}
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

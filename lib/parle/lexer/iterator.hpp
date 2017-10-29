// Based on lexertl/iterator.hpp

#ifndef PARLE_LEXER_ITERATOR_HPP
#define PARLE_LEXER_ITERATOR_HPP

#include <iterator>
#include "lexertl/lookup.hpp"
#include "lexertl/runtime_error.hpp"

#undef lookup

namespace parle
{
namespace lexer
{
template<typename iter, typename sm_type, typename results>
class iterator
{
public:
	using value_type = results;
	using difference_type = ptrdiff_t;
	using pointer = const value_type *;
	using reference = const value_type &;
	using iterator_category = std::forward_iterator_tag;

	iterator() :
		_results(iter(), iter()),
		_sm(nullptr)
	{
	}

	iterator(const iter &start_, const iter &end_, const sm_type &sm, bool do_next = false) :
		_results(start_, end_),
		_sm(&sm)
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
		_results.second = end_;
	}

	// Only need this because of warnings with gcc with -Weffc++
	iterator(const iterator &rhs_)
	{
		_results = rhs_._results;
		_sm = rhs_._sm;
	}

	// Only need this because of warnings with gcc with -Weffc++
	iterator &operator =(const iterator &rhs_)
	{
		if (&rhs_ != this)
		{
			_results = rhs_._results;
			_sm = rhs_._sm;
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

private:
	value_type _results;
	const sm_type *_sm;

	void lookup()
	{
		lexertl::lookup(*_sm, _results);

		if (_results.first == _results.eoi)
		{
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

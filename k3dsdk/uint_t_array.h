#ifndef K3DSDK_UINT_T_ARRAY_H
#define K3DSDK_UINT_T_ARRAY_H

// K-3D
// Copyright (c) 1995-2008, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include <k3dsdk/difference.h>
#include <k3dsdk/array.h>

#include <algorithm>
#include <vector>

namespace k3d
{

/// Strongly-typed dynamic array of k3d::uint_t values, based on std::vector.
/// We declare this is a special-type to prevent conflicts with k3d::typed_array.
class uint_t_array :
	public std::vector<uint_t>,
	public array
{
	typedef std::vector<uint_t> base_type;
	typedef uint_t_array this_type;

public:
	uint_t_array() :
		base_type()
	{
	}

	explicit uint_t_array(const uint_t count) :
		base_type(count)
	{
	}

	uint_t_array(const uint_t count, const uint_t& val) :
		base_type(count, val)
	{
	}

	uint_t_array(const this_type& right) :
		base_type(right),
		array(right.metadata)
	{
	}

	template<class IteratorT>
	uint_t_array(const IteratorT first, const IteratorT last) :
		base_type(first, last)
	{
	}

	const string_t type_string() const
	{
		// We have to hard-code our result here.
		return "k3d::uint_t";
	}

	void print(std::ostream& Stream) const
	{
		for(base_type::const_iterator i = base_type::begin(); i != base_type::end(); ++i)
			Stream << block_delimiter << *i;
	}

	array* clone_type() const
	{
		this_type* const result = new this_type();
		result->metadata = metadata;
		return result;
	}

	array* clone() const
	{
		this_type* const result = new this_type(*this);
		return result;
	}

	array* clone(const uint_t Begin, const uint_t End) const
	{
		this_type* const result = new this_type(this->begin() + Begin, this->begin() + End);
		result->metadata = metadata;
		return result;
	}

	void resize(const uint_t NewSize)
	{
		base_type::resize(NewSize);
	}

	void resize(const uint_t NewSize, const uint_t& Value)
	{
		base_type::resize(NewSize, Value);
	}

	uint_t size() const
	{
		return base_type::size();
	}

	bool_t empty() const
	{
		return base_type::empty();
	}

	void difference(const array& Other, difference::accumulator& Result) const
	{
		const this_type* const other = dynamic_cast<const this_type*>(&Other);
		Result.exact(other ? true : false);

		if(other)
			difference(*other, Result);
	}

	void difference(const this_type& Other, difference::accumulator& Result) const
	{
		Result.exact(metadata == Other.metadata);
		range_test(base_type::begin(), base_type::end(), Other.begin(), Other.end(), Result);
	}
};

namespace difference
{

/// Specialization of difference::test for k3d::uint_t_array
inline void test(const uint_t_array& A, const uint_t_array& B, accumulator& Result)
{
	A.difference(B, Result);
}

} // namespace difference

} // namespace k3d

#endif // !K3DSDK_UINT_T_ARRAY_H


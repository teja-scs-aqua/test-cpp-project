// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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

#include <k3dsdk/array.h>
#include <k3dsdk/table.h>
#include <k3dsdk/iomanip.h>
#include <k3dsdk/log.h>
#include <k3dsdk/type_registry.h>

namespace k3d
{

///////////////////////////////////////////////////////////////////////////
// table

const array* table::lookup(const string_t& Name) const
{
	const_iterator result = find(Name);
	return result == end() ? static_cast<const array*>(0) : result->second.get();
}

array* table::writable(const string_t& Name)
{
	iterator result = find(Name);
	return result == end() ? static_cast<array*>(0) : &result->second.writable();
}

table table::clone_types() const
{
	table result;

	for(const_iterator array = begin(); array != end(); ++array)
		result.insert(std::make_pair(array->first, array->second->clone_type()));

	return result;
}

table table::clone() const
{
	table result;

	for(const_iterator array = begin(); array != end(); ++array)
		result.insert(std::make_pair(array->first, array->second->clone()));

	return result;
}

table table::clone(const uint_t Begin, const uint_t End) const
{
	table result;

	for(const_iterator array = begin(); array != end(); ++array)
		result.insert(std::make_pair(array->first, array->second->clone(Begin, End)));

	return result;
}

void table::difference(const table& Other, difference::accumulator& Result) const
{
	// If we have differing numbers of arrays, we definitely aren't equal
	Result.exact(column_count() == Other.column_count());

	for(table::const_iterator a = begin(), b = Other.begin(); a != end() && b != Other.end(); ++a, ++b)
	{
		// Each pair of arrays must have equal names
		Result.exact(a->first == b->first);

		// Perform element-wise comparisons of the arrays
		a->second->difference(*b->second, Result);
	}
}

table table::clone_types(const table_collection& AttributeArrays)
{
	table result;

	if(AttributeArrays.size())
	{
		for(const_iterator array = AttributeArrays[0]->begin(); array != AttributeArrays[0]->end(); ++array)
			result.insert(std::make_pair(array->first, array->second->clone_type()));

/*
		{
			bool_t use_array = true;

			for(uint_t i = 1; i < AttributeArrays.size(); ++i)
			{
				
			}

			if(use_array)
				result.insert(std::make_pair(array->first, array->second->clone_type()));
		}
*/
	}

	return result;
}

uint_t table::column_count() const
{
	return base::size();
}

uint_t table::row_count() const
{
	for(const_iterator array = begin(); array != end(); )
		return array->second->size();
	return 0;
}

void table::set_row_count(const uint_t NewSize)
{
	for(iterator array = begin(); array != end(); ++array)
		array->second.writable().resize(NewSize);
}

////////////////////////////////////////////////////////////////////////////////////////////
// operator<<

std::ostream& operator<<(std::ostream& Stream, const table& RHS)
{
	for(table::const_iterator array_iterator = RHS.begin(); array_iterator != RHS.end(); ++array_iterator)
	{
		Stream << standard_indent << "array \"" << array_iterator->first << "\" [" << array_iterator->second->type_string() << "] (" << array_iterator->second->size() << "):\n";
		if(array_iterator->second->size())
			Stream << push_indent << start_block() << *array_iterator->second << finish_block << pop_indent << "\n";
	}

	return Stream;
}

} // namespace k3d


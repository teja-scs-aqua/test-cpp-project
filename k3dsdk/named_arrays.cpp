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

#include <k3dsdk/array.h>
#include <k3dsdk/iomanip.h>
#include <k3dsdk/log.h>
#include <k3dsdk/named_arrays.h>
#include <k3dsdk/type_registry.h>

namespace k3d
{

///////////////////////////////////////////////////////////////////////////
// named_arrays

const array* named_arrays::lookup(const string_t& Name) const
{
	const_iterator result = find(Name);
	return result == end() ? static_cast<const array*>(0) : result->second.get();
}

array* named_arrays::writable(const string_t& Name)
{
	iterator result = find(Name);
	return result == end() ? static_cast<array*>(0) : &result->second.writable();
}

named_arrays named_arrays::clone_types() const
{
	named_arrays result;

	for(const_iterator array = begin(); array != end(); ++array)
		result.insert(std::make_pair(array->first, array->second->clone_type()));

	return result;
}

named_arrays named_arrays::clone() const
{
	named_arrays result;

	for(const_iterator array = begin(); array != end(); ++array)
		result.insert(std::make_pair(array->first, array->second->clone()));

	return result;
}

void named_arrays::difference(const named_arrays& Other, difference::accumulator& Result) const
{
	// If we have differing numbers of arrays, we definitely aren't equal
	Result.exact(size() == Other.size());

	for(named_arrays::const_iterator a = begin(), b = Other.begin(); a != end() && b != Other.end(); ++a, ++b)
	{
		// Each pair of arrays must have equal names
		Result.exact(a->first == b->first);

		// Perform element-wise comparisons of the arrays 
		a->second->difference(*b->second, Result);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// operator<<

std::ostream& operator<<(std::ostream& Stream, const named_arrays& RHS)
{
	for(named_arrays::const_iterator array_iterator = RHS.begin(); array_iterator != RHS.end(); ++array_iterator)
	{
		Stream << standard_indent << "\"" << array_iterator->first << "\" [" << array_iterator->second->type_string() << "] (" << array_iterator->second->size() << "):\n";
		if(array_iterator->second->size())
			Stream << push_indent << start_block() << *array_iterator->second << finish_block << pop_indent << "\n";
	}
	
	return Stream;
}

} // namespace k3d


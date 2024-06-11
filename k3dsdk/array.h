#ifndef K3DSDK_ARRAY_H
#define K3DSDK_ARRAY_H

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
#include <k3dsdk/pipeline_data.h>
#include <map>

namespace k3d
{

/// Abstract interface that can be used to store heterogeneous collections of arrays.
/// Methods are provided for cloning arrays (virtual ctor pattern), plus type-agnostic
/// methods similar to std::vector, and storage for arbitrary metadata (name-value pairs).
class array
{
public:
	/// Storage for array metadata
	typedef std::map<string_t, string_t> metadata_t;

	array();
	array(const metadata_t& Metadata);
	virtual ~array();

	/// Returns the string representation for the type stored by this array. 
	virtual const string_t type_string() const = 0;
	/// Prints the array contents to a stream.
	virtual void print(std::ostream& Stream) const = 0;

	/// Returns an empty array with the same concrete type as this array (a variation on virtual ctor)
	virtual array* clone_type() const = 0;
	/// Returns a copy of this array (virtual ctor)
	virtual array* clone() const = 0;
	/// Returns a copy of a half-open range of this array (a variation on virtual ctor)
	virtual array* clone(const uint_t Begin, const uint_t End) const = 0;

	/// Sets the size of this array.
	virtual void resize(const uint_t NewSize) = 0;
	/// Returns the size of this array
	virtual uint_t size() const = 0;
	/// Returns true iff this array is empty
	virtual bool_t empty() const = 0;
	/// Returns the difference between this array and another, using the imprecise semantics of difference::test()
	/// \note: Returns false if given an array with a different concrete type.
	virtual void difference(const array& Other, difference::accumulator& Result) const = 0;

	/// Sets a new name-value pair, overwriting the value if the name already exists
	void set_metadata_value(const string_t& Name, const string_t& Value);
	/// Sets a collection of name-value pairs, overwriting any existing values
	void set_metadata(const metadata_t& Values);
	/// Returns the set of all name-value pairs
	metadata_t get_metadata() const;
	/// Returns a value by name, or empty-string if the name doesn't exist
	const string_t get_metadata_value(const string_t& Name) const;
	/// Erases an existing name-value pair
	void erase_metadata_value(const string_t& Name);

protected:
	/// Storage for array metadata
	metadata_t metadata;
};

/// Serialization
std::ostream& operator<<(std::ostream& Stream, const array& RHS);

/// Specialization of difference::test for k3d::array
namespace difference
{

inline void test(const array& A, const array& B, accumulator& Result)
{
	A.difference(B, Result);
}

} // namespace difference

/// Specialization of pipeline_data_traits for use with k3d::array
template<>
class pipeline_data_traits<array>
{
public:
	static array* clone(const array& Other)
	{
		return Other.clone();
	}
};

} // namespace k3d

#endif // !K3DSDK_ARRAY_H


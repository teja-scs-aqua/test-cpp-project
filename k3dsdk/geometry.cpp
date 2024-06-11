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

#include <k3dsdk/geometry.h>
#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/selection_validation.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/string_cast.h>

#include <boost/scoped_ptr.hpp>

namespace k3d
{

namespace geometry
{

namespace point_selection
{

//////////////////////////////////////////////////////////////////////
// const_storage

const_storage::const_storage(
	const mesh::indices_t& IndexBegin,
	const mesh::indices_t& IndexEnd,
	const mesh::selection_t& Weight
		) :
	index_begin(IndexBegin),
	index_end(IndexEnd),
	weight(Weight)
{
}

//////////////////////////////////////////////////////////////////////
// storage

storage::storage(
	mesh::indices_t& IndexBegin,
	mesh::indices_t& IndexEnd,
	mesh::selection_t& Weight
		) :
	index_begin(IndexBegin),
	index_end(IndexEnd),
	weight(Weight)
{
}

//////////////////////////////////////////////////////////////////////
// create

storage* create(k3d::selection::set& Set)
{
	k3d::selection::storage& generic_storage = Set.create("point");

	storage* const result = new storage(
		generic_storage.structure.create<mesh::indices_t>("index_begin"),
		generic_storage.structure.create<mesh::indices_t>("index_end"),
		generic_storage.structure.create<mesh::selection_t>("weight")
		);

	return result;
}

storage* create(k3d::selection::set& Set, const double_t Weight)
{
	storage* const result = create(Set);
	append(*result, Weight);
	return result;
}

//////////////////////////////////////////////////////////////////////
// validate

const_storage* validate(const k3d::selection::storage& Storage)
{
	if(Storage.type != "point")
		return 0;

	try
	{
		const mesh::indices_t& index_begin = require_array<mesh::indices_t >(Storage, "index_begin");
		const mesh::indices_t& index_end = require_array<mesh::indices_t>(Storage, "index_end");
		const mesh::selection_t& weight = require_array<mesh::selection_t>(Storage, "weight");

		require_array_size(Storage, index_end, "index_end", index_begin.size());
		require_array_size(Storage, weight, "weight", index_begin.size());

		return new const_storage(index_begin, index_end, weight);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// validate

storage* validate(k3d::selection::storage& Storage)
{
	if(Storage.type != "point")
		return 0;

	try
	{
		mesh::indices_t& index_begin = require_array<mesh::indices_t>(Storage, "index_begin");
		mesh::indices_t& index_end = require_array<mesh::indices_t>(Storage, "index_end");
		mesh::selection_t& weight = require_array<mesh::selection_t>(Storage, "weight");

		require_array_size(Storage, index_end, "index_end", index_begin.size());
		require_array_size(Storage, weight, "weight", index_begin.size());

		return new storage(index_begin, index_end, weight);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// append

void append(storage& Storage, const double_t Weight)
{
	Storage.index_begin.push_back(0);
	Storage.index_end.push_back(uint_t(-1));
	Storage.weight.push_back(Weight);
}

//////////////////////////////////////////////////////////////////////
// append

void append(storage& Storage, const uint_t Begin, const uint_t End, const double_t Weight)
{
	Storage.index_begin.push_back(Begin);
	Storage.index_end.push_back(End);
	Storage.weight.push_back(Weight);
}

//////////////////////////////////////////////////////////////////////
// merge

void merge(const_storage& Storage, mesh& Mesh)
{
	if(!Mesh.point_selection)
		return;

	mesh::selection_t& point_selection = Mesh.point_selection.writable();
	const uint_t point_selection_count = point_selection.size();

	const uint_t record_count = Storage.index_begin.size();
	for(uint_t record = 0; record != record_count; ++record)
	{
		const uint_t index_begin = Storage.index_begin[record];
		const uint_t index_end = std::max(index_begin, Storage.index_end[record]);
		const double_t weight = Storage.weight[record];

		const mesh::selection_t::iterator begin = point_selection.begin() + std::min(point_selection_count, index_begin);
		const mesh::selection_t::iterator end = point_selection.begin() + std::min(point_selection_count, index_end);

		std::fill(begin, end, weight);
	}
}

} // namespace point_selection

namespace primitive_selection
{

//////////////////////////////////////////////////////////////////////
// const_storage

const_storage::const_storage(
	const mesh::indices_t& PrimitiveBegin,
	const mesh::indices_t& PrimitiveEnd,
	const typed_array<k3d::int32_t>& PrimitiveSelectionType,
	const mesh::indices_t& PrimitiveFirstRange,
	const mesh::counts_t& PrimitiveRangeCount,
	const mesh::indices_t& IndexBegin,
	const mesh::indices_t& IndexEnd,
	const mesh::selection_t& Weight
		) :
	primitive_begin(PrimitiveBegin),
	primitive_end(PrimitiveEnd),
	primitive_selection_type(PrimitiveSelectionType),
	primitive_first_range(PrimitiveFirstRange),
	primitive_range_count(PrimitiveRangeCount),
	index_begin(IndexBegin),
	index_end(IndexEnd),
	weight(Weight)
{
}

//////////////////////////////////////////////////////////////////////
// storage

storage::storage(
	mesh::indices_t& PrimitiveBegin,
	mesh::indices_t& PrimitiveEnd,
	typed_array<k3d::int32_t>& PrimitiveSelectionType,
	mesh::indices_t& PrimitiveFirstRange,
	mesh::counts_t& PrimitiveRangeCount,
	mesh::indices_t& IndexBegin,
	mesh::indices_t& IndexEnd,
	mesh::selection_t& Weight
		) :
	primitive_begin(PrimitiveBegin),
	primitive_end(PrimitiveEnd),
	primitive_selection_type(PrimitiveSelectionType),
	primitive_first_range(PrimitiveFirstRange),
	primitive_range_count(PrimitiveRangeCount),
	index_begin(IndexBegin),
	index_end(IndexEnd),
	weight(Weight)
{
}

//////////////////////////////////////////////////////////////////////
// create

storage* create(k3d::selection::set& Set)
{
	k3d::selection::storage& generic_storage = Set.create("primitive");

	storage* const result = new storage(
		generic_storage.structure.create<mesh::indices_t>("primitive_begin"),
		generic_storage.structure.create<mesh::indices_t>("primitive_end"),
		generic_storage.structure.create<typed_array<k3d::int32_t> >("primitive_selection_type"),
		generic_storage.structure.create<mesh::indices_t>("primitive_first_range"),
		generic_storage.structure.create<mesh::counts_t>("primitive_range_count"),
		generic_storage.structure.create<mesh::indices_t>("index_begin"),
		generic_storage.structure.create<mesh::indices_t>("index_end"),
		generic_storage.structure.create<mesh::selection_t>("weight")
		);

	return result;
}

storage* create(k3d::selection::set& Set, const uint_t PrimitiveBegin, const uint_t PrimitiveEnd, const int32_t SelectionType)
{
	storage* const result = create(Set);

	result->primitive_begin.push_back(PrimitiveBegin);
	result->primitive_end.push_back(PrimitiveEnd);
	result->primitive_selection_type.push_back(SelectionType);
	result->primitive_first_range.push_back(0);
	result->primitive_range_count.push_back(0);

	return result;
}

storage* create(k3d::selection::set& Set, const int32_t SelectionType)
{
	storage* const result = create(Set);

	result->primitive_begin.push_back(0);
	result->primitive_end.push_back(uint_t(-1));
	result->primitive_selection_type.push_back(SelectionType);
	result->primitive_first_range.push_back(0);
	result->primitive_range_count.push_back(0);

	return result;
}

//////////////////////////////////////////////////////////////////////
// validate

const_storage* validate(const k3d::selection::storage& Storage)
{
	if(Storage.type != "primitive")
		return 0;

	try
	{
		const mesh::indices_t& primitive_begin = require_array<mesh::indices_t>(Storage, "primitive_begin");
		const mesh::indices_t& primitive_end = require_array<mesh::indices_t>(Storage, "primitive_end");
		const typed_array<k3d::int32_t>& primitive_selection_type = require_array<typed_array<k3d::int32_t> >(Storage, "primitive_selection_type");
		const mesh::indices_t& primitive_first_range = require_array<mesh::indices_t>(Storage, "primitive_first_range");
		const mesh::indices_t& primitive_range_count = require_array<mesh::counts_t>(Storage, "primitive_range_count");
		const mesh::indices_t& index_begin = require_array<mesh::indices_t>(Storage, "index_begin");
		const mesh::indices_t& index_end = require_array<mesh::indices_t>(Storage, "index_end");
		const mesh::selection_t& weight = require_array<mesh::selection_t>(Storage, "weight");

		require_array_size(Storage, primitive_end, "primitive_end", primitive_begin.size());
		require_array_size(Storage, primitive_selection_type, "primitive_selection_type", primitive_begin.size());
		require_array_size(Storage, primitive_first_range, "primitive_first_range", primitive_begin.size());
		require_array_size(Storage, primitive_range_count, "primitive_range_count", primitive_begin.size());
		require_array_size(Storage, index_end, "index_end", index_begin.size());
		require_array_size(Storage, weight, "weight", index_begin.size());

		return new const_storage(primitive_begin, primitive_end, primitive_selection_type, primitive_first_range, primitive_range_count, index_begin, index_end, weight);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// validate

storage* validate(k3d::selection::storage& Storage)
{
	if(Storage.type != "primitive")
		return 0;

	try
	{
		mesh::indices_t& primitive_begin = require_array<mesh::indices_t>(Storage, "primitive_begin");
		mesh::indices_t& primitive_end = require_array<mesh::indices_t>(Storage, "primitive_end");
		typed_array<k3d::int32_t>& primitive_selection_type = require_array<typed_array<k3d::int32_t> >(Storage, "primitive_selection_type");
		mesh::indices_t& primitive_first_range = require_array<mesh::indices_t>(Storage, "primitive_first_range");
		mesh::indices_t& primitive_range_count = require_array<mesh::counts_t>(Storage, "primitive_range_count");
		mesh::indices_t& index_begin = require_array<mesh::indices_t>(Storage, "index_begin");
		mesh::indices_t& index_end = require_array<mesh::indices_t>(Storage, "index_end");
		mesh::selection_t& weight = require_array<mesh::selection_t>(Storage, "weight");

		require_array_size(Storage, primitive_end, "primitive_end", primitive_begin.size());
		require_array_size(Storage, primitive_selection_type, "primitive_selection_type", primitive_begin.size());
		require_array_size(Storage, primitive_first_range, "primitive_first_range", primitive_begin.size());
		require_array_size(Storage, primitive_range_count, "primitive_range_count", primitive_begin.size());
		require_array_size(Storage, index_end, "index_end", index_begin.size());
		require_array_size(Storage, weight, "weight", index_begin.size());

		return new storage(primitive_begin, primitive_end, primitive_selection_type, primitive_first_range, primitive_range_count, index_begin, index_end, weight);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// append

void append(storage& Storage, const uint_t PrimitiveBegin, const uint_t PrimitiveEnd, const int32_t SelectionType, const uint_t Begin, const uint_t End, const double_t Weight)
{
	Storage.primitive_begin.push_back(PrimitiveBegin);
	Storage.primitive_end.push_back(PrimitiveEnd);
	Storage.primitive_selection_type.push_back(SelectionType);
	Storage.primitive_first_range.push_back(Storage.index_begin.size());
	Storage.primitive_range_count.push_back(1);

	Storage.index_begin.push_back(Begin);
	Storage.index_end.push_back(End);
	Storage.weight.push_back(Weight);
}

void append(storage& Storage, const int32_t SelectionType, const uint_t Begin, const uint_t End, const double_t Weight)
{
	Storage.primitive_begin.push_back(0);
	Storage.primitive_end.push_back(uint_t(-1));
	Storage.primitive_selection_type.push_back(SelectionType);
	Storage.primitive_first_range.push_back(Storage.index_begin.size());
	Storage.primitive_range_count.push_back(1);

	Storage.index_begin.push_back(Begin);
	Storage.index_end.push_back(End);
	Storage.weight.push_back(Weight);
}

void append(storage& Storage, const int32_t SelectionType, const double_t Weight)
{
	Storage.primitive_begin.push_back(0);
	Storage.primitive_end.push_back(uint_t(-1));
	Storage.primitive_selection_type.push_back(SelectionType);
	Storage.primitive_first_range.push_back(Storage.index_begin.size());
	Storage.primitive_range_count.push_back(1);

	Storage.index_begin.push_back(0);
	Storage.index_end.push_back(uint_t(-1));
	Storage.weight.push_back(Weight);
}

void append(storage& Storage, const uint_t Begin, const uint_t End, const double_t Weight)
{
	return_if_fail(Storage.primitive_begin.size());

	Storage.primitive_range_count.back() += 1;

	Storage.index_begin.push_back(Begin);
	Storage.index_end.push_back(End);
	Storage.weight.push_back(Weight);
}

void append(storage& Storage, const double_t Weight)
{
	return_if_fail(Storage.primitive_begin.size());

	Storage.primitive_range_count.back() += 1;

	Storage.index_begin.push_back(0);
	Storage.index_end.push_back(uint_t(-1));
	Storage.weight.push_back(Weight);
}

//////////////////////////////////////////////////////////////////////
// merge

class merge_primitive_selection
{
public:
	merge_primitive_selection(const_storage& Storage, const string_t& PrimitiveSelectionType, const uint_t PrimitiveFirstRange, const uint_t PrimitiveRangeCount) :
		m_storage(Storage),
		m_primitive_selection_type(PrimitiveSelectionType),
		m_primitive_first_range(PrimitiveFirstRange),
		m_primitive_range_count(PrimitiveRangeCount)
	{
	}

	void operator()(const string_t& StructureName, table& Structure, const string_t& ArrayName, pipeline_data<array>& Array)
	{
		if(StructureName != m_primitive_selection_type)
			return;

		if(Array->get_metadata_value(metadata::key::role()) != metadata::value::selection_role())
			return;

		mesh::selection_t* const array = dynamic_cast<mesh::selection_t*>(&Array.writable());
		if(!array)
		{
			log() << error << "unexpected type for array [" << ArrayName << "] with k3d:selection-component = " << m_primitive_selection_type << std::endl;
			return;
		}

		const uint_t range_begin = m_primitive_first_range;
		const uint_t range_end = range_begin + m_primitive_range_count;
		for(uint_t range = range_begin; range != range_end; ++range)
		{
			std::fill(
				array->begin() + std::min(array->size(), m_storage.index_begin[range]),
				array->begin() + std::min(array->size(), m_storage.index_end[range]),
				m_storage.weight[range]);
		}
	}

private:
	const_storage& m_storage;
	const string_t& m_primitive_selection_type;
	const uint_t m_primitive_first_range;
	const uint_t m_primitive_range_count;
};

void merge(const_storage& Storage, mesh& Mesh)
{
	const uint_t mesh_primitive_count = static_cast<uint_t>(Mesh.primitives.size());
	const uint_t component_count = Storage.primitive_begin.size();
	for(uint_t component = 0; component != component_count; ++component)
	{
		const uint_t primitive_begin = std::min(mesh_primitive_count, Storage.primitive_begin[component]);
		const uint_t primitive_end = std::min(mesh_primitive_count, std::max(primitive_begin, Storage.primitive_end[component]));
		const string_t primitive_selection_type = string_cast(static_cast<k3d::selection::type>(Storage.primitive_selection_type[component]));
		const uint_t primitive_first_range = Storage.primitive_first_range[component];
		const uint_t primitive_range_count = Storage.primitive_range_count[component];

		for(uint_t primitive = primitive_begin; primitive != primitive_end; ++primitive)
		{
			mesh::visit_arrays(Mesh.primitives[primitive].writable(), merge_primitive_selection(Storage, primitive_selection_type, primitive_first_range, primitive_range_count));
		}
	}
}

} // namespace primitive_selection

namespace selection
{

k3d::selection::set create(const double_t Weight)
{
	k3d::selection::set result;

	boost::scoped_ptr<point_selection::storage> point_selection_storage(point_selection::create(result));
	point_selection::append(*point_selection_storage, Weight);

	boost::scoped_ptr<primitive_selection::storage> primitive_selection_storage(primitive_selection::create(result));
	primitive_selection::append(*primitive_selection_storage, k3d::selection::CONSTANT, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::CURVE, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::EDGE, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::FACE, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::PARAMETER, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::PATCH, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::SURFACE, Weight);
	primitive_selection::append(*primitive_selection_storage, k3d::selection::VERTEX, Weight);

	return result;
}

void merge(const k3d::selection::set& Set, mesh& Mesh)
{
	for(k3d::selection::set::const_iterator storage = Set.begin(); storage != Set.end(); ++storage)
	{
		boost::scoped_ptr<point_selection::const_storage> point_selection_storage(point_selection::validate(**storage));
		if(point_selection_storage)
		{
			point_selection::merge(*point_selection_storage, Mesh);
			continue;
		}

		boost::scoped_ptr<primitive_selection::const_storage> primitive_selection_storage(primitive_selection::validate(**storage));
		if(primitive_selection_storage)
		{
			primitive_selection::merge(*primitive_selection_storage, Mesh);
			continue;
		}
	}
}

} // namespace selection

} // namespace geometry

} // namespace k3d


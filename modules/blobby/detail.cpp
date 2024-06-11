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

/** \file
	\author Timothy M. Shead (tshead@k-3d.com)
	\author Romain Behar (romainbehar@yahoo.com)
	\author Bart Janssens (bart.janssens@lid.kviv.be)
*/

#include "detail.h"

#include <k3dsdk/table_copier.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/mesh.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace blobby
{

namespace detail
{

void merge(const mesh_collection& Inputs, k3d::imaterial* const Material, const k3d::blobby::operator_type Operator, const k3d::bool_t VariableArguments, k3d::mesh& Output)
{
	// Collect all of the parameter and vertex arrays to be merged ...
	k3d::mesh::table_t::table_collection source_parameter_attributes;
	k3d::mesh::table_t::table_collection source_vertex_attributes;
	for(mesh_collection::const_iterator mesh = Inputs.begin(); mesh != Inputs.end(); ++mesh)
	{
		for(k3d::mesh::primitives_t::const_iterator primitive = (*mesh)->primitives.begin(); primitive != (*mesh)->primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::blobby::const_primitive> source_blobby(k3d::blobby::validate(**mesh, **primitive));
			if(!source_blobby)
				continue;

			source_parameter_attributes.push_back(&source_blobby->parameter_attributes);
			source_vertex_attributes.push_back(&source_blobby->vertex_attributes);
		}
	}

	// Setup the initial state of the output mesh ...
	boost::scoped_ptr<k3d::blobby::primitive> target_blobby(k3d::blobby::create(Output));

	target_blobby->parameter_attributes = k3d::table::clone_types(source_parameter_attributes);
	target_blobby->vertex_attributes = k3d::table::clone_types(source_vertex_attributes);

	target_blobby->first_primitives.push_back(0);
	target_blobby->primitive_counts.push_back(0);
	target_blobby->first_operators.push_back(0);
	target_blobby->operator_counts.push_back(0);
	target_blobby->materials.push_back(Material);

	k3d::mesh::indices_t blobby_roots;

	// Iterate over each input mesh ...
	for(mesh_collection::const_iterator mesh = Inputs.begin(); mesh != Inputs.end(); ++mesh)
	{
		for(k3d::mesh::primitives_t::const_iterator primitive = (*mesh)->primitives.begin(); primitive != (*mesh)->primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::blobby::const_primitive> source_blobby(k3d::blobby::validate(**mesh, **primitive));
			if(!source_blobby)
				continue;

			k3d::table_copier parameter_copier(source_blobby->parameter_attributes, target_blobby->parameter_attributes, k3d::table_copier::copy_subset());
			k3d::table_copier vertex_copier(source_blobby->vertex_attributes, target_blobby->vertex_attributes, k3d::table_copier::copy_subset());

			const k3d::uint_t blobby_begin = 0;
			const k3d::uint_t blobby_end = blobby_begin + source_blobby->first_primitives.size();
			for(k3d::uint_t blobby = blobby_begin; blobby != blobby_end; ++blobby)
			{
				const k3d::uint_t operand_offset = target_blobby->primitive_counts[0] + target_blobby->operator_counts[0];

				const k3d::uint_t source_primitive_count = source_blobby->primitive_counts[blobby];
				const k3d::uint_t source_primitives_begin = source_blobby->first_primitives[blobby];
				const k3d::uint_t source_primitives_end = source_primitives_begin + source_primitive_count;
				for(k3d::uint_t source_primitive = source_primitives_begin; source_primitive != source_primitives_end; ++source_primitive)
				{
					target_blobby->primitives.push_back(source_blobby->primitives[source_primitive]);
					target_blobby->primitive_first_floats.push_back(target_blobby->floats.size());
					target_blobby->primitive_float_counts.push_back(source_blobby->primitive_float_counts[source_primitive]);
					parameter_copier.push_back(source_primitive);
					vertex_copier.push_back(source_primitive);

					const k3d::uint_t source_floats_begin = source_blobby->primitive_first_floats[source_primitive];
					const k3d::uint_t source_floats_end = source_floats_begin + source_blobby->primitive_float_counts[source_primitive];
					for(k3d::uint_t source_float = source_floats_begin; source_float != source_floats_end; ++source_float)
						target_blobby->floats.push_back(source_blobby->floats[source_float]);
				}
				target_blobby->primitive_counts[0] += source_primitive_count;

				const k3d::uint_t source_operator_count = source_blobby->operator_counts[blobby];
				const k3d::uint_t source_operators_begin = source_blobby->first_operators[blobby];
				const k3d::uint_t source_operators_end = source_operators_begin + source_operator_count;
				for(k3d::uint_t source_operator = source_operators_begin; source_operator != source_operators_end; ++source_operator)
				{
					target_blobby->operators.push_back(source_blobby->operators[source_operator]);
					target_blobby->operator_first_operands.push_back(target_blobby->operands.size());
					target_blobby->operator_operand_counts.push_back(source_blobby->operator_operand_counts[source_operator]);

					const k3d::uint_t source_operands_begin = source_blobby->operator_first_operands[source_operator];
					const k3d::uint_t source_operands_end = source_operands_begin + source_blobby->operator_operand_counts[source_operator];
					for(k3d::uint_t source_operand = source_operands_begin; source_operand != source_operands_end; ++source_operand)
						target_blobby->operands.push_back(source_blobby->operands[source_operand] + operand_offset);
				}
				target_blobby->operator_counts[0] += source_operator_count;

				if(source_primitive_count || source_operator_count)
					blobby_roots.push_back(target_blobby->primitive_counts[0] + target_blobby->operator_counts[0] - 1);
			}
		}
	}	

	if(!blobby_roots.empty())
	{
		target_blobby->operator_counts[0] += 1;
		target_blobby->operators.push_back(Operator);
		target_blobby->operator_first_operands.push_back(target_blobby->operands.size());

		if(VariableArguments)
		{
			target_blobby->operator_operand_counts.push_back(blobby_roots.size() + 1);
			target_blobby->operands.push_back(blobby_roots.size());
		}
		else
		{
			target_blobby->operator_operand_counts.push_back(blobby_roots.size());
		}

		target_blobby->operands.insert(target_blobby->operands.end(), blobby_roots.begin(), blobby_roots.end());
	}
}

} // namespace detail

} // namespace blobby

} // namespace module


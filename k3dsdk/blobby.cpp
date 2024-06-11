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

#include <k3dsdk/blobby.h>
#include <k3dsdk/primitive_validation.h>

#include <numeric>

namespace k3d
{

namespace blobby
{

/////////////////////////////////////////////////////////////////////////////////////////////
// const_primitive

const_primitive::const_primitive(
	const mesh::indices_t& FirstPrimitives,
	const mesh::counts_t& PrimitiveCounts,
	const mesh::indices_t& FirstOperators,
	const mesh::counts_t& OperatorCounts,
	const mesh::materials_t& Materials,
	const typed_array<int32_t>& Primitives,
	const mesh::indices_t& PrimitiveFirstFloats,
	const mesh::counts_t& PrimitiveFloatCounts,
	const typed_array<int32_t>& Operators,
	const mesh::indices_t& OperatorFirstOperands,
	const mesh::counts_t& OperatorOperandCounts,
	const mesh::doubles_t& Floats,
	const mesh::indices_t& Operands,
	const mesh::table_t& ConstantAttributes,
	const mesh::table_t& SurfaceAttributes,
	const mesh::table_t& ParameterAttributes,
	const mesh::table_t& VertexAttributes
		) :
	first_primitives(FirstPrimitives),
	primitive_counts(PrimitiveCounts),
	first_operators(FirstOperators),
	operator_counts(OperatorCounts),
	materials(Materials),
	primitives(Primitives),
	primitive_first_floats(PrimitiveFirstFloats),
	primitive_float_counts(PrimitiveFloatCounts),
	operators(Operators),
	operator_first_operands(OperatorFirstOperands),
	operator_operand_counts(OperatorOperandCounts),
	floats(Floats),
	operands(Operands),
	constant_attributes(ConstantAttributes),
	surface_attributes(SurfaceAttributes),
	parameter_attributes(ParameterAttributes),
	vertex_attributes(VertexAttributes)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
// primitive

primitive::primitive(
	mesh::indices_t& FirstPrimitives,
	mesh::counts_t& PrimitiveCounts,
	mesh::indices_t& FirstOperators,
	mesh::counts_t& OperatorCounts,
	mesh::materials_t& Materials,
	typed_array<int32_t>& Primitives,
	mesh::indices_t& PrimitiveFirstFloats,
	mesh::counts_t& PrimitiveFloatCounts,
	typed_array<int32_t>& Operators,
	mesh::indices_t& OperatorFirstOperands,
	mesh::counts_t& OperatorOperandCounts,
	mesh::doubles_t& Floats,
	mesh::indices_t& Operands,
	mesh::table_t& ConstantAttributes,
	mesh::table_t& SurfaceAttributes,
	mesh::table_t& ParameterAttributes,
	mesh::table_t& VertexAttributes
		) :
	first_primitives(FirstPrimitives),
	primitive_counts(PrimitiveCounts),
	first_operators(FirstOperators),
	operator_counts(OperatorCounts),
	materials(Materials),
	primitives(Primitives),
	primitive_first_floats(PrimitiveFirstFloats),
	primitive_float_counts(PrimitiveFloatCounts),
	operators(Operators),
	operator_first_operands(OperatorFirstOperands),
	operator_operand_counts(OperatorOperandCounts),
	floats(Floats),
	operands(Operands),
	constant_attributes(ConstantAttributes),
	surface_attributes(SurfaceAttributes),
	parameter_attributes(ParameterAttributes),
	vertex_attributes(VertexAttributes)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////
// create

primitive* create(mesh& Mesh)
{
	mesh::primitive& generic_primitive = Mesh.primitives.create("blobby");

	primitive* const result = new primitive(
		generic_primitive.structure["surface"].create<mesh::indices_t>("first_primitives"),
		generic_primitive.structure["surface"].create<mesh::counts_t>("primitive_counts"),
		generic_primitive.structure["surface"].create<mesh::indices_t>("first_operators"),
		generic_primitive.structure["surface"].create<mesh::counts_t>("operator_counts"),
		generic_primitive.structure["surface"].create<mesh::materials_t>("materials"),
		generic_primitive.structure["vertex"].create<typed_array<int32_t> >("primitives"),
		generic_primitive.structure["vertex"].create<mesh::indices_t>("primitive_first_floats"),
		generic_primitive.structure["vertex"].create<mesh::counts_t>("primitive_float_counts"),
		generic_primitive.structure["operator"].create<typed_array<int32_t> >("operators"),
		generic_primitive.structure["operator"].create<mesh::indices_t>("operator_first_operands"),
		generic_primitive.structure["operator"].create<mesh::counts_t >("operator_operand_counts"),
		generic_primitive.structure["float"].create<mesh::doubles_t>("floats"),
		generic_primitive.structure["operand"].create<mesh::indices_t>("operands"),
		generic_primitive.attributes["constant"],
		generic_primitive.attributes["surface"],
		generic_primitive.attributes["parameter"],
		generic_primitive.attributes["vertex"]
		);

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// validate

const_primitive* validate(const mesh& Mesh, const mesh::primitive& Primitive)
{
	if(Primitive.type != "blobby")
		return 0;

	try
	{
		require_valid_primitive(Mesh, Primitive);

		const mesh::table_t& surface_structure = require_structure(Primitive, "surface");
		const mesh::table_t& vertex_structure = require_structure(Primitive, "vertex");
		const mesh::table_t& operator_structure = require_structure(Primitive, "operator");
		const mesh::table_t& float_structure = require_structure(Primitive, "float");
		const mesh::table_t& operand_structure = require_structure(Primitive, "operand");

		const mesh::table_t& constant_attributes = require_attributes(Primitive, "constant");
		const mesh::table_t& surface_attributes = require_attributes(Primitive, "surface");
		const mesh::table_t& parameter_attributes = require_attributes(Primitive, "parameter");
		const mesh::table_t& vertex_attributes = require_attributes(Primitive, "vertex");

		const mesh::indices_t& first_primitives = require_array<mesh::indices_t>(Primitive, surface_structure, "first_primitives");
		const mesh::counts_t& primitive_counts = require_array<mesh::counts_t>(Primitive, surface_structure, "primitive_counts");
		const mesh::indices_t& first_operators = require_array<mesh::indices_t>(Primitive, surface_structure, "first_operators");
		const mesh::counts_t& operator_counts = require_array<mesh::counts_t>(Primitive, surface_structure, "operator_counts");
		const mesh::materials_t& materials = require_array<mesh::materials_t>(Primitive, surface_structure, "materials");
		const typed_array<int32_t>& primitives = require_array<typed_array<int32_t> >(Primitive, vertex_structure, "primitives");
		const mesh::indices_t& primitive_first_floats = require_array<mesh::indices_t>(Primitive, vertex_structure, "primitive_first_floats");
		const mesh::counts_t& primitive_float_counts = require_array<mesh::counts_t>(Primitive, vertex_structure, "primitive_float_counts");
		const typed_array<int32_t>& operators = require_array<typed_array<int32_t> >(Primitive, operator_structure, "operators");
		const mesh::indices_t& operator_first_operands = require_array<mesh::indices_t>(Primitive, operator_structure, "operator_first_operands");
		const mesh::counts_t& operator_operand_counts = require_array<mesh::counts_t>(Primitive, operator_structure, "operator_operand_counts");
		const mesh::doubles_t& floats = require_array<mesh::doubles_t>(Primitive, float_structure, "floats");
		const mesh::indices_t& operands = require_array<mesh::indices_t>(Primitive, operand_structure, "operands");

		/** \todo Validate table lengths here */

		return new const_primitive(first_primitives, primitive_counts, first_operators, operator_counts, materials, primitives, primitive_first_floats, primitive_float_counts, operators, operator_first_operands, operator_operand_counts, floats, operands, constant_attributes, surface_attributes, parameter_attributes, vertex_attributes);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

primitive* validate(const mesh& Mesh, mesh::primitive& Primitive)
{
	if(Primitive.type != "blobby")
		return 0;

	try
	{
		require_valid_primitive(Mesh, Primitive);

		mesh::table_t& surface_structure = require_structure(Primitive, "surface");
		mesh::table_t& vertex_structure = require_structure(Primitive, "vertex");
		mesh::table_t& operator_structure = require_structure(Primitive, "operator");
		mesh::table_t& float_structure = require_structure(Primitive, "float");
		mesh::table_t& operand_structure = require_structure(Primitive, "operand");

		mesh::table_t& constant_attributes = require_attributes(Primitive, "constant");
		mesh::table_t& surface_attributes = require_attributes(Primitive, "surface");
		mesh::table_t& parameter_attributes = require_attributes(Primitive, "parameter");
		mesh::table_t& vertex_attributes = require_attributes(Primitive, "vertex");

		mesh::indices_t& first_primitives = require_array<mesh::indices_t>(Primitive, surface_structure, "first_primitives");
		mesh::counts_t& primitive_counts = require_array<mesh::counts_t>(Primitive, surface_structure, "primitive_counts");
		mesh::indices_t& first_operators = require_array<mesh::indices_t>(Primitive, surface_structure, "first_operators");
		mesh::counts_t& operator_counts = require_array<mesh::counts_t>(Primitive, surface_structure, "operator_counts");
		mesh::materials_t& materials = require_array<mesh::materials_t>(Primitive, surface_structure, "materials");
		typed_array<int32_t>& primitives = require_array<typed_array<int32_t> >(Primitive, vertex_structure, "primitives");
		mesh::indices_t& primitive_first_floats = require_array<mesh::indices_t>(Primitive, vertex_structure, "primitive_first_floats");
		mesh::counts_t& primitive_float_counts = require_array<mesh::counts_t>(Primitive, vertex_structure, "primitive_float_counts");
		typed_array<int32_t>& operators = require_array<typed_array<int32_t> >(Primitive, operator_structure, "operators");
		mesh::indices_t& operator_first_operands = require_array<mesh::indices_t>(Primitive, operator_structure, "operator_first_operands");
		mesh::counts_t& operator_operand_counts = require_array<mesh::counts_t>(Primitive, operator_structure, "operator_operand_counts");
		mesh::doubles_t& floats = require_array<mesh::doubles_t>(Primitive, float_structure, "floats");
		mesh::indices_t& operands = require_array<mesh::indices_t>(Primitive, operand_structure, "operands");

		/** \todo Validate table lengths here */

		return new primitive(first_primitives, primitive_counts, first_operators, operator_counts, materials, primitives, primitive_first_floats, primitive_float_counts, operators, operator_first_operands, operator_operand_counts, floats, operands, constant_attributes, surface_attributes, parameter_attributes, vertex_attributes);
	}
	catch(std::exception& e)
	{
		log() << error << e.what() << std::endl;
	}

	return 0;
}

primitive* validate(const mesh& Mesh, pipeline_data<mesh::primitive>& Primitive)
{
  if(!Primitive.get())
    return 0;

	if(Primitive->type != "blobby")
		return 0;

  return validate(Mesh, Primitive.writable());
}

} // namespace blobby

} // namespace k3d


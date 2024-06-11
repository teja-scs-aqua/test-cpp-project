// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
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

/** \file SDS painters
 */

#include "colored_selection_painter_gl.h"
#include "sds_cache.h"
#include "selection_cache.h"
#include "utility.h"

#include <k3d-i18n-config.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/gl/extension.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/hints.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/painter_render_state_gl.h>
#include <k3dsdk/painter_selection_state_gl.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/utility_gl.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

	/////////////////////////////////////////////////////////////////////////////
// sds_painter

template <typename selection_t>
class sds_painter :
	public colored_selection_painter
{
	typedef colored_selection_painter base;
public:
	sds_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document, const k3d::color Unselected = k3d::color(0.2,0.2,0.2), const k3d::color Selected = k3d::color(0.6,0.6,0.6)) :
		base(Factory, Document, Unselected, Selected),
		m_levels(init_owner(*this) + init_name("levels") + init_label(_("Levels")) + init_description(_("Number of SDS levels")) + init_value(2) + init_constraint(constraint::minimum<k3d::int32_t>(2)) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar)))
	{
		m_levels.changed_signal().connect(sigc::mem_fun(*this, &sds_painter<selection_t>::on_levels_changed));
	}
	
	void on_levels_changed(k3d::ihint* Hint)
	{
		k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::ASYNCHRONOUS);
	}
	
	void on_paint_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!has_sds_polyhedra(Mesh))
			return;
		k3d::gl::store_attributes attributes;
		
		enable_blending();
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			draw(**primitive, get_data<sds_cache>(&Mesh, this), get_data<selection_t>(&Mesh, this), RenderState);
		}
		disable_blending();
	}
	
	void on_select_mesh(const k3d::mesh& Mesh, const k3d::gl::painter_render_state& RenderState, const k3d::gl::painter_selection_state& SelectionState, k3d::iproperty::changed_signal_t& ChangedSignal)
	{
		if(!has_sds_polyhedra(Mesh))
			return;
		
		k3d::gl::store_attributes attributes;
		for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
		{
			select(**primitive, get_data<sds_cache>(&Mesh, this), SelectionState);
		}
	}
	
	void on_mesh_changed(const k3d::mesh& Mesh, k3d::ihint* Hint)
	{
		if(!has_sds_polyhedra(Mesh))
			return;

		schedule_data<selection_t>(&Mesh, Hint, this);
		schedule_data<sds_cache>(&Mesh, Hint, this);
	}

protected:
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_levels;
	
	// override to choose drawing mode
	virtual void draw(const k3d::mesh::primitive& Primitive, sds_cache& Cache, selection_t& Selection, const k3d::gl::painter_render_state& RenderState) = 0;
	
	// override to choose selection mode
	virtual void select(const k3d::mesh::primitive& Primitive, sds_cache& Cache, const k3d::gl::painter_selection_state& SelectionState) = 0;
};

////////////////////////////////:
// sds_face_painter

class sds_face_painter : public sds_painter<face_selection>
{
	typedef sds_painter<face_selection> base;
	typedef face_selection selection_t;
	typedef k3d::typed_array<std::string> strings_t;
public:
	sds_face_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document, k3d::color(0.2,0.2,0.2),k3d::color(0.6,0.6,0.6))
	{}
	
	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<sds_face_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
				k3d::uuid(0xf8578aba, 0x674bbc2d, 0x40622ea4, 0x9167eaf9),
		"OpenGLSDSFacePainter",
		_("Renders mesh as SDS faces using OpenGL 1.1"),
		"OpenGL Painter",
		k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}
protected:
	virtual void draw(const k3d::mesh::primitive& Primitive, sds_cache& Cache, selection_t& Selection, const k3d::gl::painter_render_state& RenderState)
	{
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		glFrontFace(RenderState.inside_out ? GL_CCW : GL_CW);
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		glEnable(GL_LIGHTING);
		
		const color_t color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
		const color_t selected_color = RenderState.show_component_selection ? selected_component_color() : color;
		
		const k3d::bool_t interpolateboundary = polyhedron->constant_attributes.lookup<strings_t>("interpolateboundary") ? true : false;
		k3d::mesh::indices_t companions;
		k3d::mesh::bools_t boundary_edges;
		k3d::mesh::bools_t boundary_faces;
		if (!interpolateboundary)
		{
			k3d::polyhedron::create_edge_adjacency_lookup(polyhedron->edge_points, polyhedron->clockwise_edges, boundary_edges, companions);
			k3d::polyhedron::create_boundary_face_lookup(polyhedron->face_first_loops, polyhedron->face_loop_counts, polyhedron->loop_first_edges, polyhedron->clockwise_edges, boundary_edges, companions, boundary_faces);
		}
		
		face_visitor visitor(Cache.point_count(), Cache.edge_count(&Primitive), polyhedron->face_first_loops.size());
		Cache.visit_surface(&Primitive, m_levels.pipeline_value(), visitor);
		
		k3d::uint_t face_count = visitor.face_starts.size();
		const selection_records_t& face_selection_records = Selection.records(&Primitive);
		glBegin(GL_QUADS);
		if (interpolateboundary)
		{
			assert_not_implemented();
/*
			if (!face_selection_records.empty())
			{
				for (selection_records_t::const_iterator record = face_selection_records.begin(); record != face_selection_records.end() && record->begin < face_count; ++record)
				{ // color by selection
					const color_t& face_color = record->weight ? selected_color : color;
					k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, k3d::color(face_color.red, face_color.green, face_color.blue), face_color.alpha);
					k3d::uint_t start = record->begin;
					k3d::uint_t end = record->end;
					end = end > face_count ? face_count : end;
					k3d::uint_t start_index = visitor.face_starts[start];
					k3d::uint_t end_index = end == face_count ? visitor.indices.size() : visitor.face_starts[end];
					for (k3d::uint_t i = start_index; i != end_index; ++i)
					{
						k3d::gl::normal3d(k3d::to_vector(visitor.normals_array[visitor.indices[i]]));
						k3d::gl::vertex3d(visitor.points_array[visitor.indices[i]]);
					}
				}
			}
			else
			{ // empty selection, everything has the same color
				k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, k3d::color(color.red, color.green, color.blue), color.alpha);
				for (k3d::uint_t i = 0; i != visitor.indices.size(); ++i)
				{
					k3d::gl::normal3d(k3d::to_vector(visitor.normals_array[visitor.indices[i]]));
					k3d::gl::vertex3d(visitor.points_array[visitor.indices[i]]);
				}
			}
*/
		}
		else // no boundary interpolation requires us not to render the faces of the mesh boundary
		{
			for (k3d::uint_t face = 0; face != visitor.face_starts.size(); ++face)
			{
				if (boundary_faces[face])
					continue;
				const color_t& face_color = polyhedron->face_selections[face] ? selected_color : color;
				k3d::gl::material(GL_FRONT_AND_BACK, GL_DIFFUSE, k3d::color(face_color.red, face_color.green, face_color.blue), face_color.alpha);
				k3d::uint_t start_index = visitor.face_starts[face];
				k3d::uint_t end_index = face == (visitor.face_starts.size()-1) ? visitor.indices.size() : visitor.face_starts[face+1]; 
				for (k3d::uint_t i = start_index; i != end_index; ++i)
				{
					k3d::gl::normal3d(k3d::to_vector(visitor.normals_array[visitor.indices[i]]));
					k3d::gl::vertex3d(visitor.points_array[visitor.indices[i]]);
				}
				
			}
		}
		glEnd();
	}
	virtual void select(const k3d::mesh::primitive& Primitive, sds_cache& Cache, const k3d::gl::painter_selection_state& SelectionState)
	{
		if (!SelectionState.select_uniform)
			return;
		
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		
		k3d::gl::store_attributes attributes;
				
		glDisable(GL_LIGHTING);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		k3d::gl::set(GL_CULL_FACE, !SelectionState.select_backfacing);

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		
		face_visitor visitor(Cache.point_count(), Cache.edge_count(&Primitive), polyhedron->face_first_loops.size());
		Cache.visit_surface(&Primitive, m_levels.pipeline_value(), visitor);
		
		for (k3d::uint_t face = 0; face != visitor.face_starts.size(); ++face)
		{
			k3d::gl::push_selection_token(k3d::selection::UNIFORM, face);
			glBegin(GL_QUADS);
			k3d::uint_t start_index = visitor.face_starts[face];
			k3d::uint_t end_index = face == (visitor.face_starts.size()-1) ? visitor.indices.size() : visitor.face_starts[face+1]; 
			for (k3d::uint_t i = start_index; i != end_index; ++i)
			{
				k3d::gl::normal3d(k3d::to_vector(visitor.normals_array[visitor.indices[i]]));
				k3d::gl::vertex3d(visitor.points_array[visitor.indices[i]]);
			}
			glEnd();
			k3d::gl::pop_selection_token(); // UNIFORM
		}
	}
};

k3d::iplugin_factory& sds_face_painter_factory()
{
	return sds_face_painter::get_factory();
}
	
////////////
// sds_edge_painter
////////////
	
class sds_edge_painter : public sds_painter<edge_selection>
{
	typedef sds_painter<edge_selection> base;
	typedef edge_selection selection_t;
public:
	sds_edge_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document, k3d::color(0.0,0.0,0.0),k3d::color(1.0,1.0,1.0))
	{}
	
	k3d::iplugin_factory& factory()
	{
		return get_factory();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<sds_edge_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
				k3d::uuid(0x55238c22, 0xed466597, 0xb86c86a3, 0x0de46700),
		"OpenGLSDSEdgePainter",
		_("Renders mesh as SDS patch borders using OpenGL 1.1"),
		"Development",
		k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}
	
private:
	virtual void draw(const k3d::mesh::primitive& Primitive, sds_cache& Cache, selection_t& Selection, const k3d::gl::painter_render_state& RenderState)
	{
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		glDisable(GL_LIGHTING);
		edge_visitor visitor(polyhedron->edge_points.size());
		Cache.visit_boundary(&Primitive, m_levels.pipeline_value(), visitor);
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_DOUBLE, 0, &visitor.points_array[0]);
		
		const color_t color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
		const color_t selected_color = RenderState.show_component_selection ? selected_component_color() : color;
		
		k3d::uint_t edge_count = visitor.edge_starts.size();
		
		glBegin(GL_LINES);
assert_not_implemented();
/*
		const selection_records_t& edge_selection_records = Selection.records(&Primitive);
		if (!edge_selection_records.empty())
		{
			for (selection_records_t::const_iterator record = edge_selection_records.begin(); record != edge_selection_records.end() && record->begin < edge_count; ++record)
			{ // color by selection
				color4d(record->weight ? selected_color : color);
				k3d::uint_t start = record->begin;
				k3d::uint_t end = record->end;
				end = end > edge_count ? edge_count : end;
				k3d::uint_t start_index = visitor.edge_starts[start];
				k3d::uint_t end_index = end == edge_count ? visitor.points_array.size() : visitor.edge_starts[end];
				for (k3d::uint_t i = start_index; i < end_index; ++i)
				{
					k3d::gl::vertex3d(visitor.points_array[i]);
				}
			}
		}
		else
		{ // empty selection, everything has the same color
			color4d(color);
			for (k3d::uint_t i = 0; i != edge_count; ++i)
			{
				k3d::gl::vertex3d(visitor.points_array[i]);
			}
		}
*/
		glEnd();
	}
	
	virtual void select(const k3d::mesh::primitive& Primitive, sds_cache& Cache, const k3d::gl::painter_selection_state& SelectionState)
	{
		if (!SelectionState.select_split_edges)
			return;
		
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		
		glDisable(GL_LIGHTING);
		edge_visitor visitor(polyhedron->edge_points.size());
		Cache.visit_boundary(&Primitive, m_levels.pipeline_value(), visitor);
		
		k3d::uint_t edge_count = visitor.edge_starts.size();
		
		for (k3d::uint_t edge = 0; edge != visitor.edge_starts.size(); ++edge)
		{
			k3d::gl::push_selection_token(k3d::selection::SPLIT_EDGE, edge);
			
			k3d::uint_t start_index = visitor.edge_starts[edge];
			k3d::uint_t end_index = edge == (edge_count-1) ? visitor.points_array.size() : visitor.edge_starts[edge+1];
			glBegin(GL_LINES);
			for (k3d::uint_t i = start_index; i < end_index; ++i)
			{
				k3d::gl::vertex3d(visitor.points_array[i]);
			}
			glEnd();
			
			k3d::gl::pop_selection_token(); // SPLIT_EDGE
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// sds_edge_painter_factory

k3d::iplugin_factory& sds_edge_painter_factory()
{
	return sds_edge_painter::get_factory();
}

////////////
// sds_point_painter
////////////
	
class sds_point_painter : public sds_painter<point_selection>
{
	typedef sds_painter<point_selection> base;
	typedef point_selection selection_t;
public:
	sds_point_painter(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document, k3d::color(0.0,0.0,0.0),k3d::color(1.0,1.0,1.0))
	{}
	
	k3d::iplugin_factory& factory()
	{
		return get_factory();
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<sds_point_painter, k3d::interface_list<k3d::gl::imesh_painter > > factory(
				k3d::uuid(0xc336f1ed, 0x0a4fab65, 0x7ceeb380, 0x4a67eca1),
		"OpenGLSDSPointPainter",
		_("Renders mesh as SDS patch corners using OpenGL 1.1"),
		"Development",
		k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}
	
private:
	virtual void draw(const k3d::mesh::primitive& Primitive, sds_cache& Cache, selection_t& Selection, const k3d::gl::painter_render_state& RenderState)
	{
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		glDisable(GL_LIGHTING);
		point_visitor visitor;
		Cache.visit_corners(&Primitive, m_levels.pipeline_value(), visitor);
		
		k3d::uint_t point_count = Cache.point_count();
		
		const color_t color = RenderState.node_selection ? selected_mesh_color() : unselected_mesh_color(RenderState.parent_selection);
		const color_t selected_color = RenderState.show_component_selection ? selected_component_color() : color;

assert_not_implemented();
/*
		glBegin(GL_POINTS);
		const selection_records_t& point_selection_records = Selection.records();
		if (!point_selection_records.empty())
		{
			for (selection_records_t::const_iterator record = point_selection_records.begin(); record != point_selection_records.end() && record->begin < point_count; ++record)
			{ // color by selection
				color4d(record->weight ? selected_color : color);
				k3d::uint_t start = record->begin;
				k3d::uint_t end = record->end;
				end = end > point_count ? point_count : end;
				for (k3d::uint_t i = start; i != end; ++i)
				{
					k3d::gl::vertex3d(visitor.points_array[i]);
				}
			}
		}
		else
		{ // empty selection, everything has the same color
			color4d(color);
			for (k3d::uint_t i = 0; i != point_count; ++i)
			{
				k3d::gl::vertex3d(visitor.points_array[i]);
			}
		}
		glEnd();
*/
	}
	
	virtual void select(const k3d::mesh::primitive& Primitive, sds_cache& Cache, const k3d::gl::painter_selection_state& SelectionState)
	{
		if (!SelectionState.select_points)
			return;
		boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Primitive));
		if(!polyhedron.get() || !k3d::polyhedron::is_sds(*polyhedron))
			return;
		glDisable(GL_LIGHTING);
		point_visitor visitor;
		Cache.visit_corners(&Primitive, m_levels.pipeline_value(), visitor);
		
		k3d::uint_t point_count = Cache.point_count();
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_DOUBLE, 0, &visitor.points_array[0]);
		
		for (k3d::uint_t point = 0; point != point_count; ++point)
		{
			k3d::gl::push_selection_token(k3d::selection::POINT, point);
			glBegin(GL_POINTS);
			k3d::gl::vertex3d(visitor.points_array[point]);
			glEnd();
			k3d::gl::pop_selection_token();
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// sds_point_painter_factory

k3d::iplugin_factory& sds_point_painter_factory()
{
	return sds_point_painter::get_factory();
}

} // namespace opengl

} // namespace painters

} // namespace module


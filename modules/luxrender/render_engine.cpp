// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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
	\author Tim Shead <tshead@k-3d.com>
*/

#include "light.h"
#include "material.h"
#include "texture.h"
#include "utility.h"

#include <k3d-i18n-config.h>
#include <k3d-version-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/color.h>
#include <k3dsdk/cone.h>
#include <k3dsdk/cylinder.h>
#include <k3dsdk/disk.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/file_range.h>
#include <k3dsdk/fstream.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/hyperboloid.h>
#include <k3dsdk/icamera.h>
#include <k3dsdk/iomanip.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/imesh_sink.h>
#include <k3dsdk/imesh_source.h>
#include <k3dsdk/inetwork_render_farm.h>
#include <k3dsdk/inetwork_render_frame.h>
#include <k3dsdk/inetwork_render_job.h>
#include <k3dsdk/inode_collection_sink.h>
#include <k3dsdk/iprojection.h>
#include <k3dsdk/irender_camera_animation.h>
#include <k3dsdk/irender_camera_frame.h>
#include <k3dsdk/irender_camera_preview.h>
#include <k3dsdk/imatrix_source.h>
#include <k3dsdk/material.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/network_render_farm.h>
#include <k3dsdk/node.h>
#include <k3dsdk/nodes.h>
#include <k3dsdk/nurbs_patch.h>
#include <k3dsdk/paraboloid.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/property.h>
#include <k3dsdk/resolutions.h>
#include <k3dsdk/sphere.h>
#include <k3dsdk/time_source.h>
#include <k3dsdk/torus.h>
#include <k3dsdk/transform.h>
#include <k3dsdk/triangulator.h>
#include <k3dsdk/utility_gl.h>

#include <boost/scoped_ptr.hpp>

#include <iomanip>
#include <iterator>
#include <map>
#include <set>

namespace module
{

namespace luxrender
{

/////////////////////////////////////////////////////////////////////////////
// render_engine

class render_engine :
	public k3d::node,
	public k3d::inode_collection_sink,
	public k3d::irender_camera_preview,
	public k3d::irender_camera_frame,
	public k3d::irender_camera_animation
{
	typedef k3d::node base;
// initialise properties
public:
	render_engine(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_visible_nodes(init_owner(*this) + init_name("visible_nodes") + init_label(_("Visible Nodes")) + init_description(_("A list of nodes that will be visible in the rendered output.")) + init_value(std::vector<k3d::inode*>())),
		m_enabled_lights(init_owner(*this) + init_name("enabled_lights") + init_label(_("Enabled Lights")) + init_description(_("A list of light sources that will contribute to the rendered output.")) + init_value(std::vector<k3d::inode*>())),
		m_resolution(init_owner(*this) + init_name("resolution") + init_label(_("Resolution")) + init_description(_("Choose a predefined image resolution")) + init_enumeration(k3d::resolution_values()) + init_value(k3d::string_t(""))),
		m_pixel_width(init_owner(*this) + init_name("pixel_width") + init_label(_("Pixel Width")) + init_description(_("The horizontal size in pixels of the rendered output image.")) + init_value(320) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar)) + init_constraint(constraint::minimum<k3d::int32_t>(1))),
		m_pixel_height(init_owner(*this) + init_name("pixel_height") + init_label(_("Pixel Height")) + init_description(_("The vertical size in pixels of the rendered output image.")) + init_value(240) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar)) + init_constraint(constraint::minimum<k3d::int32_t>(1))),
		m_halt_samples(init_owner(*this) + init_name("halt_samples") + init_label(_("Halt Samples")) + init_description(_("Halt rendering once the number of samples per pixel reaches this value.")) + init_value(200) + init_constraint(constraint::minimum<k3d::int32_t>(1)))
	{
		m_resolution.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_resolution_changed));
	}

	const k3d::inode_collection_sink::properties_t node_collection_properties()
	{
		k3d::inode_collection_sink::properties_t results;
		results.push_back(&m_visible_nodes);
		results.push_back(&m_enabled_lights);

		return results;
	}

	void on_resolution_changed(k3d::iunknown*)
	{
		const k3d::string_t new_resolution = m_resolution.pipeline_value();

		const k3d::resolutions_t& resolutions = k3d::resolutions();
		for(k3d::resolutions_t::const_iterator resolution = resolutions.begin(); resolution != resolutions.end(); ++resolution)
		{
			if(resolution->name != new_resolution)
				continue;

			m_pixel_width.set_value(resolution->width);
			m_pixel_height.set_value(resolution->height);
			return;
		}

		assert_not_reached();
	}

	k3d::bool_t render_camera_preview(k3d::icamera& Camera)
	{
		// Start a new render job ...
		k3d::inetwork_render_job& job = k3d::get_network_render_farm().create_job("k3d-luxrender-preview");

		// Add a single render frame to the job ...
		k3d::inetwork_render_frame& frame = job.create_frame("frame");

		// Create an output image path ...
		const k3d::filesystem::path output_image = frame.add_file("output");

		// Render it ...
		return_val_if_fail(render(Camera, frame, output_image, true), false);

		// View the output when it's done ...
		frame.add_view_command(output_image + ".png");

		// Start the job running ...
		k3d::get_network_render_farm().start_job(job);

		return true;
	}

	k3d::bool_t render_camera_frame(k3d::icamera& Camera, const k3d::filesystem::path& OutputImage, const k3d::bool_t ViewImage)
	{
		// Sanity checks ...
		return_val_if_fail(!OutputImage.empty(), false);

		// Start a new render job ...
		k3d::inetwork_render_job& job = k3d::get_network_render_farm().create_job("k3d-luxrender-render-frame");

		// Add a single render frame to the job ...
		k3d::inetwork_render_frame& frame = job.create_frame("frame");

		// Create an output image path ...
		const k3d::filesystem::path output_image = frame.add_file("output");

		// Render it ...
		return_val_if_fail(render(Camera, frame, output_image, false), false);

		// Copy the output image to its requested destination ...
		frame.add_copy_command(output_image + ".png", OutputImage);

		// View the output image when it's done ...
		if(ViewImage)
			frame.add_view_command(OutputImage);

		// Start the job running ...
		k3d::get_network_render_farm().start_job(job);

		return true;
	}

	k3d::bool_t render_camera_animation(k3d::icamera& Camera, k3d::iproperty& Time, const k3d::frames& Frames, const k3d::bool_t ViewCompletedImages)
	{
		// Start a new render job ...
		k3d::inetwork_render_job& job = k3d::get_network_render_farm().create_job("k3d-luxrender-render-animation");

		// For each frame to be rendered ...
		k3d::uint_t frame_index = 0;
		for(k3d::frames::const_iterator frame = Frames.begin(); frame != Frames.end(); ++frame, ++frame_index)
		{
			// Set the frame time ...
			k3d::property::set_internal_value(Time, frame->begin_time);

			// Redraw everything ...
			k3d::gl::redraw_all(document(), k3d::gl::irender_viewport::SYNCHRONOUS);

			// Add a render frame to the job ...
			std::stringstream buffer;
			buffer << "frame-" << frame_index;
			k3d::inetwork_render_frame& render_frame = job.create_frame(buffer.str());

			// Create an output image path ...
			const k3d::filesystem::path output_image = render_frame.add_file("output");

			// Render it (hidden rendering) ...
			return_val_if_fail(render(Camera, render_frame, output_image, false), false);

			// Copy the output image to its requested destination ...
			render_frame.add_copy_command(output_image + ".png", frame->destination);

			// View the output image when it's done ...
			if(ViewCompletedImages)
				render_frame.add_view_command(frame->destination);
		}

		// Start the job running ...
		k3d::get_network_render_farm().start_job(job);

		return true;
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<render_engine,
			k3d::interface_list<k3d::irender_camera_animation,
			k3d::interface_list<k3d::irender_camera_frame,
			k3d::interface_list<k3d::irender_camera_preview> > > > factory(
				k3d::uuid(0xe28cbcb2, 0x1940a2f6, 0x5a8228ba, 0xdf1cd742),
				"LuxRenderEngine",
				_("LuxRender Render Engine"),
				"LuxRender RenderEngine",
				k3d::iplugin_factory::EXPERIMENTAL);

		return factory;
	}

private:
	/// Helper class used to triangulate faces
	class create_triangles :
		public k3d::triangulator
	{
		typedef k3d::triangulator base;

	public:
		create_triangles(
			const k3d::mesh::points_t& FacePoints,
			const k3d::mesh::normals_t* VertexNormals,
			const k3d::mesh::normals_t* FaceNormals,
			const k3d::mesh::normals_t* EdgeNormals,
			const k3d::mesh::materials_t& FaceMaterials,
			k3d::mesh::points_t& TrianglePoints,
			k3d::mesh::normals_t& TriangleNormals,
			std::vector<k3d::imaterial*>& TriangleMaterials
			) :
			m_face_points(FacePoints),
			m_vertex_normals(VertexNormals),
			m_face_normals(FaceNormals),
			m_edge_normals(EdgeNormals),
			m_face_materials(FaceMaterials),
			m_triangle_points(TrianglePoints),
			m_triangle_normals(TriangleNormals),
			m_triangle_materials(TriangleMaterials)
		{
		}

	private:
		void start_face(const k3d::uint_t Face)
		{
			m_current_face = Face;
		}

		void add_vertex(const k3d::point3& Coordinates, k3d::uint_t Vertices[4], k3d::uint_t Edges[4], double Weights[4], k3d::uint_t& NewVertex)
		{
			NewVertex = m_face_points.size();
			m_face_points.push_back(Coordinates);
		}

		void add_triangle(k3d::uint_t Vertices[3], k3d::uint_t Edges[3])
		{
			m_triangle_points.push_back(m_face_points[Vertices[0]]);
			m_triangle_points.push_back(m_face_points[Vertices[1]]);
			m_triangle_points.push_back(m_face_points[Vertices[2]]);

			if(m_edge_normals)
			{
				m_triangle_normals.push_back((*m_edge_normals)[Edges[0]]);
				m_triangle_normals.push_back((*m_edge_normals)[Edges[1]]);
				m_triangle_normals.push_back((*m_edge_normals)[Edges[2]]);
			}
			else if(m_vertex_normals)
			{
				m_triangle_normals.push_back((*m_vertex_normals)[Vertices[0]]);
				m_triangle_normals.push_back((*m_vertex_normals)[Vertices[1]]);
				m_triangle_normals.push_back((*m_vertex_normals)[Vertices[2]]);
			}
			else if(m_face_normals)
			{
				m_triangle_normals.push_back((*m_face_normals)[m_current_face]);
				m_triangle_normals.push_back((*m_face_normals)[m_current_face]);
				m_triangle_normals.push_back((*m_face_normals)[m_current_face]);
			}

			m_triangle_materials.push_back(m_face_materials[m_current_face]);
		}

		k3d::mesh::points_t m_face_points;
		const k3d::mesh::normals_t* const m_vertex_normals;
		const k3d::mesh::normals_t* const m_face_normals;
		const k3d::mesh::normals_t* const m_edge_normals;
		const k3d::mesh::materials_t& m_face_materials;
		k3d::mesh::points_t& m_triangle_points;
		k3d::mesh::normals_t& m_triangle_normals;
		std::vector<k3d::imaterial*>& m_triangle_materials;

		k3d::uint_t m_current_face;
	};

	void render_cone(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::cone::const_primitive& Cone, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Cone.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Cone.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Cone.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"cone\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float height\" [" << Cone.heights[i] << "]\n";
			Stream << k3d::standard_indent << "\"float radius\" [" << Cone.radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Cone.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_cylinder(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::cylinder::const_primitive& Cylinder, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Cylinder.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Cylinder.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Cylinder.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"cylinder\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float zmin\" [" << Cylinder.z_min[i] << "]\n";
			Stream << k3d::standard_indent << "\"float zmax\" [" << Cylinder.z_max[i] << "]\n";
			Stream << k3d::standard_indent << "\"float radius\" [" << Cylinder.radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Cylinder.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_disk(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::disk::const_primitive& Disk, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Disk.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Disk.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Disk.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"disk\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float height\" [" << Disk.heights[i] << "]\n";
			Stream << k3d::standard_indent << "\"float radius\" [" << Disk.radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Disk.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_hyperboloid(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::hyperboloid::const_primitive& Hyperboloid, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Hyperboloid.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Hyperboloid.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Hyperboloid.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"hyperboloid\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"point p1\" [" << Hyperboloid.start_points[i] << "]\n";
			Stream << k3d::standard_indent << "\"point p2\" [" << Hyperboloid.end_points[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phi\" [" << k3d::degrees(Hyperboloid.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_nurbs_patch(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::nurbs_patch::const_primitive& Nurbs, std::ostream& Stream)
	{
		const k3d::mesh::points_t& mesh_points = *Mesh.points;

		Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
		Stream << k3d::standard_indent << "Transform [" << convert(k3d::node_to_world_matrix(MeshInstance)) << "]\n" << k3d::push_indent;

		const k3d::uint_t patch_begin = 0;
		const k3d::uint_t patch_end = patch_begin + Nurbs.patch_first_points.size();
		for(k3d::uint_t patch = patch_begin; patch != patch_end; ++patch)
		{
			material::use(MaterialNames, Nurbs.patch_materials[patch], Stream);

			Stream << k3d::standard_indent << "Shape \"nurbs\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"integer nu\" [" << Nurbs.patch_u_point_counts[patch] << "]\n";
			Stream << k3d::standard_indent << "\"integer nv\" [" << Nurbs.patch_v_point_counts[patch] << "]\n";
			Stream << k3d::standard_indent << "\"integer uorder\" [" << Nurbs.patch_u_orders[patch] << "]\n";
			Stream << k3d::standard_indent << "\"integer vorder\" [" << Nurbs.patch_v_orders[patch] << "]\n";

			Stream << k3d::standard_indent << "\"float uknots\" [";
			const k3d::uint_t u_knot_begin = Nurbs.patch_u_first_knots[patch];
			const k3d::uint_t u_knot_end = u_knot_begin + Nurbs.patch_u_point_counts[patch] + Nurbs.patch_u_orders[patch];
			for(k3d::uint_t knot = u_knot_begin; knot != u_knot_end; ++knot)
				Stream << " " << Nurbs.patch_u_knots[knot];
			Stream << "]\n";

			Stream << k3d::standard_indent << "\"float vknots\" [";
			const k3d::uint_t v_knot_begin = Nurbs.patch_v_first_knots[patch];
			const k3d::uint_t v_knot_end = v_knot_begin + Nurbs.patch_v_point_counts[patch] + Nurbs.patch_v_orders[patch];
			for(k3d::uint_t knot = v_knot_begin; knot != v_knot_end; ++knot)
				Stream << " " << Nurbs.patch_v_knots[knot];
			Stream << "]\n";

			Stream << k3d::standard_indent << "\"float Pw\" [";
			const k3d::uint_t point_begin = Nurbs.patch_first_points[patch];
			const k3d::uint_t point_end = point_begin + (Nurbs.patch_u_point_counts[patch] * Nurbs.patch_v_point_counts[patch]);
			for(k3d::uint_t point = point_begin; point != point_end; ++point)
			{
				const k3d::uint_t point_index = Nurbs.patch_points[point];
				const k3d::double_t point_weight = Nurbs.patch_point_weights[point];

				Stream << " " << point_weight * mesh_points[point_index][0];
				Stream << " " << point_weight * mesh_points[point_index][1];
				Stream << " " << point_weight * mesh_points[point_index][2];
				Stream << " " << point_weight;
			}
			Stream << "]\n";

			Stream << k3d::pop_indent;
		}

/*
		// For each material, render the triangles that use that material ...
		for(std::set<k3d::imaterial*>::const_iterator material = material_list.begin(); material != material_list.end(); ++material)
		{
			material::use(MaterialNames, *material, Stream);

			Stream << k3d::standard_indent << "Shape \"trianglemesh\"\n" << k3d::push_indent;

			const k3d::uint_t triangle_begin = 0;
			const k3d::uint_t triangle_end = triangle_begin + triangle_materials.size();

			Stream << k3d::standard_indent << "\"integer indices\" [";
			for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
			{
				if(triangle_materials[triangle] != *material)
					continue;

				Stream << " " << (triangle * 3) + 0 << " " << (triangle * 3) + 1 << " " << (triangle * 3) + 2;
			}
			Stream << "]\n";

			Stream << k3d::standard_indent << "\"point P\" [";
			for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
			{
				if(triangle_materials[triangle] != *material)
					continue;

				Stream << " " << triangle_points[(triangle * 3) + 0] << " " << triangle_points[(triangle * 3) + 1] << " " << triangle_points[(triangle * 3) + 2];
			}
			Stream << "]\n";

			if(triangle_normals.size())
			{
				Stream << k3d::standard_indent << "\"normal N\" [";
				for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
				{
					if(triangle_materials[triangle] != *material)
						continue;

					Stream << " " << triangle_normals[(triangle * 3) + 0] << " " << triangle_normals[(triangle * 3) + 1] << " " << triangle_normals[(triangle * 3) + 2];
				}
				Stream << "]\n";
			}

			Stream << k3d::pop_indent;
		}
*/

		Stream << k3d::pop_indent;
		Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
	}

	void render_paraboloid(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::paraboloid::const_primitive& Paraboloid, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Paraboloid.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Paraboloid.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Paraboloid.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"paraboloid\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float radius\" [" << Paraboloid.radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float zmin\" [" << Paraboloid.z_min[i] << "]\n";
			Stream << k3d::standard_indent << "\"float zmax\" [" << Paraboloid.z_max[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Paraboloid.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_polyhedron(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::polyhedron::const_primitive& Polyhedron, std::ostream& Stream)
	{
		// Triangulate the polyhedron faces ...
		const k3d::mesh::normals_t* const vertex_normals = Mesh.point_attributes.lookup<k3d::mesh::normals_t>("N");
		const k3d::mesh::normals_t* const face_normals = Polyhedron.face_attributes.lookup<k3d::mesh::normals_t>("N");
		const k3d::mesh::normals_t* const edge_normals = Polyhedron.edge_attributes.lookup<k3d::mesh::normals_t>("N");

		k3d::mesh::points_t triangle_points; // Receives the list of triangle vertices
		k3d::mesh::normals_t triangle_normals; // Receives the (optional) list of triangle normals
		std::vector<k3d::imaterial*> triangle_materials; // Recieves the list of triangle materials

		create_triangles(*Mesh.points, vertex_normals, face_normals, edge_normals, Polyhedron.face_materials, triangle_points, triangle_normals, triangle_materials).process(Mesh, Polyhedron);

		// Get the set of unique materials ...
		std::set<k3d::imaterial*> material_list(triangle_materials.begin(), triangle_materials.end());

		// Make it happen ...
		Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
		Stream << k3d::standard_indent << "Transform [" << convert(k3d::node_to_world_matrix(MeshInstance)) << "]\n" << k3d::push_indent;

		// For each material, render the triangles that use that material ...
		for(std::set<k3d::imaterial*>::const_iterator material = material_list.begin(); material != material_list.end(); ++material)
		{
			material::use(MaterialNames, *material, Stream);

			Stream << k3d::standard_indent << "Shape \"trianglemesh\"\n" << k3d::push_indent;

			const k3d::uint_t triangle_begin = 0;
			const k3d::uint_t triangle_end = triangle_begin + triangle_materials.size();

			Stream << k3d::standard_indent << "\"integer indices\" [";
			for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
			{
				if(triangle_materials[triangle] != *material)
					continue;

				Stream << " " << (triangle * 3) + 0 << " " << (triangle * 3) + 1 << " " << (triangle * 3) + 2;
			}
			Stream << "]\n";

			Stream << k3d::standard_indent << "\"point P\" [";
			for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
			{
				if(triangle_materials[triangle] != *material)
					continue;

				Stream << " " << triangle_points[(triangle * 3) + 0] << " " << triangle_points[(triangle * 3) + 1] << " " << triangle_points[(triangle * 3) + 2];
			}
			Stream << "]\n";

			if(triangle_normals.size())
			{
				Stream << k3d::standard_indent << "\"normal N\" [";
				for(k3d::uint_t triangle = triangle_begin; triangle != triangle_end; ++triangle)
				{
					if(triangle_materials[triangle] != *material)
						continue;

					Stream << " " << triangle_normals[(triangle * 3) + 0] << " " << triangle_normals[(triangle * 3) + 1] << " " << triangle_normals[(triangle * 3) + 2];
				}
				Stream << "]\n";
			}

			Stream << k3d::pop_indent;
		}

		Stream << k3d::pop_indent;
		Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
	}

	void render_sphere(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::sphere::const_primitive& Sphere, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Sphere.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Sphere.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Sphere.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"sphere\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float radius\" [" << Sphere.radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float zmin\" [" << Sphere.radii[i] * Sphere.z_min[i] << "]\n";
			Stream << k3d::standard_indent << "\"float zmax\" [" << Sphere.radii[i] * Sphere.z_max[i] << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Sphere.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_torus(const material::name_map& MaterialNames, k3d::inode& MeshInstance, const k3d::mesh& Mesh, k3d::torus::const_primitive& Torus, std::ostream& Stream)
	{
		for(k3d::uint_t i = 0; i != Torus.matrices.size(); ++i)
		{
			Stream << k3d::standard_indent << "AttributeBegin\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "ConcatTransform [" << convert(k3d::node_to_world_matrix(MeshInstance) * Torus.matrices[i]) << "]\n" << k3d::push_indent;

			material::use(MaterialNames, Torus.materials[i], Stream);

			Stream << k3d::standard_indent << "Shape \"torus\"\n" << k3d::push_indent;
			Stream << k3d::standard_indent << "\"float majorradius\" [" << Torus.major_radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float minorradius\" [" << Torus.minor_radii[i] << "]\n";
			Stream << k3d::standard_indent << "\"float thetamin\" [" << k3d::degrees(Torus.phi_min[i]) << "]\n";
			Stream << k3d::standard_indent << "\"float thetamax\" [" << k3d::degrees(Torus.phi_max[i]) << "]\n";
			Stream << k3d::standard_indent << "\"float phimax\" [" << k3d::degrees(Torus.sweep_angles[i]) << "]\n";
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent;
			Stream << k3d::pop_indent << k3d::standard_indent << "AttributeEnd\n";
		}
	}

	void render_mesh_instance(const material::name_map& MaterialNames, k3d::inode& MeshInstance, std::ostream& Stream)
	{
		const k3d::mesh* const mesh = k3d::property::pipeline_value<k3d::mesh*>(MeshInstance, "output_mesh");
		if(!mesh)
			return;

		for(k3d::mesh::primitives_t::const_iterator primitive = mesh->primitives.begin(); primitive != mesh->primitives.end(); ++primitive)
		{
			boost::scoped_ptr<k3d::cone::const_primitive> cone(k3d::cone::validate(*mesh, **primitive));
			if(cone)
			{
				render_cone(MaterialNames, MeshInstance, *mesh, *cone, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::cylinder::const_primitive> cylinder(k3d::cylinder::validate(*mesh, **primitive));
			if(cylinder)
			{
				render_cylinder(MaterialNames, MeshInstance, *mesh, *cylinder, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::disk::const_primitive> disk(k3d::disk::validate(*mesh, **primitive));
			if(disk)
			{
				render_disk(MaterialNames, MeshInstance, *mesh, *disk, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::hyperboloid::const_primitive> hyperboloid(k3d::hyperboloid::validate(*mesh, **primitive));
			if(hyperboloid)
			{
				render_hyperboloid(MaterialNames, MeshInstance, *mesh, *hyperboloid, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::nurbs_patch::const_primitive> nurbs_patch(k3d::nurbs_patch::validate(*mesh, **primitive));
			if(nurbs_patch)
			{
				render_nurbs_patch(MaterialNames, MeshInstance, *mesh, *nurbs_patch, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::paraboloid::const_primitive> paraboloid(k3d::paraboloid::validate(*mesh, **primitive));
			if(paraboloid)
			{
				render_paraboloid(MaterialNames, MeshInstance, *mesh, *paraboloid, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(*mesh, **primitive));
			if(polyhedron)
			{
				render_polyhedron(MaterialNames, MeshInstance, *mesh, *polyhedron, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::sphere::const_primitive> sphere(k3d::sphere::validate(*mesh, **primitive));
			if(sphere)
			{
				render_sphere(MaterialNames, MeshInstance, *mesh, *sphere, Stream);
				continue;
			}

			boost::scoped_ptr<k3d::torus::const_primitive> torus(k3d::torus::validate(*mesh, **primitive));
			if(torus)
			{
				render_torus(MaterialNames, MeshInstance, *mesh, *torus, Stream);
				continue;
			}
		}
	}

	void start_file(std::ostream& Stream)
	{
		Stream << k3d::standard_indent << "# LuxRender scene generated by K-3D Version " K3D_VERSION ", http://www.k-3d.org\n\n";
	}

	k3d::bool_t render(k3d::icamera& Camera, k3d::inetwork_render_frame& Frame, const k3d::filesystem::path& OutputImagePath, const k3d::bool_t VisibleRender)
	{
		try
		{
			// Start our luxrender XML file ...
			const k3d::filesystem::path scene_path = Frame.add_file("world.lxs");
			k3d::filesystem::ofstream scene_stream(scene_path);
			return_val_if_fail(scene_stream.good(), false);
			start_file(scene_stream);

			k3d::inetwork_render_frame::environment environment;

			k3d::inetwork_render_frame::arguments arguments;
			arguments.push_back(k3d::inetwork_render_frame::argument(scene_path.native_filesystem_string()));

			if(VisibleRender)
				Frame.add_exec_command("luxrender", environment, arguments);
			else
				Frame.add_exec_command("luxconsole", environment, arguments);

			// Setup the camera ...
			k3d::iperspective* const perspective = dynamic_cast<k3d::iperspective*>(&Camera.projection());
			if(!perspective)
				throw std::runtime_error("A perspective projection is required.");

			const k3d::matrix4 camera_matrix = k3d::property::pipeline_value<k3d::matrix4>(Camera.transformation().matrix_source_output());
			const k3d::point3 camera_from = k3d::position(camera_matrix);
			const k3d::point3 camera_to = camera_from + (camera_matrix * k3d::vector3(0, 0, 1));
			const k3d::vector3 camera_up = camera_matrix * k3d::vector3(0, 1, 0);
			scene_stream << k3d::standard_indent << "LookAt " << convert(camera_from) << " " << convert(camera_to) << " " << convert(camera_up) << "\n";

			const k3d::double_t camera_left = k3d::property::pipeline_value<k3d::double_t>(perspective->left());
			const k3d::double_t camera_right = k3d::property::pipeline_value<k3d::double_t>(perspective->right());
			const k3d::double_t camera_top = k3d::property::pipeline_value<k3d::double_t>(perspective->top());
			const k3d::double_t camera_bottom = k3d::property::pipeline_value<k3d::double_t>(perspective->bottom());
			const k3d::double_t camera_near = k3d::property::pipeline_value<k3d::double_t>(perspective->near());
			const k3d::double_t camera_far = k3d::property::pipeline_value<k3d::double_t>(perspective->far());

			const k3d::double_t camera_width = std::abs(camera_right - camera_left);
			const k3d::double_t camera_height = std::abs(camera_top - camera_bottom);
			const k3d::double_t camera_fov = k3d::degrees(2 * atan2(0.5 * std::min(camera_width, camera_height), camera_near));

			scene_stream << k3d::standard_indent << "Camera \"perspective\"\n" << k3d::push_indent;
			scene_stream << k3d::standard_indent << "\"float hither\" [" << camera_near << "]\n";
			scene_stream << k3d::standard_indent << "\"float yon\" [" << camera_far << "]\n";
			scene_stream << k3d::standard_indent << "\"float fov\" [" << camera_fov << "]\n";
			scene_stream << k3d::pop_indent;

			// Setup output options ...
			const k3d::int32_t pixel_width = m_pixel_width.pipeline_value();
			const k3d::int32_t pixel_height = m_pixel_height.pipeline_value();

			scene_stream << k3d::standard_indent << "Film \"fleximage\"\n" << k3d::push_indent;
			scene_stream << k3d::standard_indent << "\"string filename\" [\"" << OutputImagePath.native_filesystem_string() << "\"]\n";
			scene_stream << k3d::standard_indent << "\"integer xresolution\" [" << pixel_width << "]\n";
			scene_stream << k3d::standard_indent << "\"integer yresolution\" [" << pixel_height << "]\n";

			if(!VisibleRender)
				scene_stream << k3d::standard_indent << "\"integer haltspp\" [" << m_halt_samples.pipeline_value() << "]\n";

			scene_stream << k3d::pop_indent;

			// Scene setup ...
			scene_stream << k3d::standard_indent << "WorldBegin\n" << k3d::push_indent;

			// Setup a separate file for lights ...
			const k3d::filesystem::path light_path = Frame.add_file("lights.lxs");
			k3d::filesystem::ofstream light_stream(light_path);
			return_val_if_fail(light_stream.good(), false);
			start_file(light_stream);

			// Setup lights ...
			const k3d::inode_collection_property::nodes_t enabled_lights = m_enabled_lights.pipeline_value();
			for(k3d::inode_collection_property::nodes_t::const_iterator node = enabled_lights.begin(); node != enabled_lights.end(); ++node)
			{
				if(luxrender::light* const light = dynamic_cast<luxrender::light*>(*node))
				{
					light->setup(light_stream);
				}
			}
			scene_stream << k3d::standard_indent << "Include \"" << light_path.native_filesystem_string() << "\"\n";

			// Setup a separate file for textures and materials ...
			const k3d::filesystem::path material_path = Frame.add_file("materials.lxs");
			k3d::filesystem::ofstream material_stream(material_path);
			return_val_if_fail(material_stream.good(), false);
			start_file(material_stream);

			// Setup textures, assigning unique names as-we-go ...
			texture::name_map texture_names;
			const std::vector<texture*> textures = k3d::node::lookup<texture>(document());
			for(k3d::uint_t i = 0; i != textures.size(); ++i)
				textures[i]->setup(texture_names, material_stream);

			// Setup materials, assigning unique names as-we-go ...
			material::name_map material_names;
			const std::vector<material*> materials = k3d::node::lookup<material>(document());
			for(k3d::uint_t i = 0; i != materials.size(); ++i)
				materials[i]->setup(texture_names, material_names, material_stream);
			scene_stream << k3d::standard_indent << "Include \"" << material_path.native_filesystem_string() << "\"\n";

			// Setup a separate file for geometry ...
			const k3d::filesystem::path geometry_path = Frame.add_file("geometry.lxs");
			k3d::filesystem::ofstream geometry_stream(geometry_path);
			return_val_if_fail(geometry_stream.good(), false);
			start_file(geometry_stream);

			// Render geometry ...
			const k3d::inode_collection_property::nodes_t visible_nodes = m_visible_nodes.pipeline_value();
			for(k3d::inode_collection_property::nodes_t::const_iterator node = visible_nodes.begin(); node != visible_nodes.end(); ++node)
			{
				if((*node)->factory().factory_id() != k3d::classes::MeshInstance())
					continue;

				render_mesh_instance(material_names, **node, geometry_stream);
			}
			scene_stream << k3d::standard_indent << "Include \"" << geometry_path.native_filesystem_string() << "\"\n";

			scene_stream << k3d::pop_indent << k3d::standard_indent << "WorldEnd\n";
		}
		catch(std::exception& e)
		{
			k3d::log() << error << "exception: " << e.what() << std::endl;
			return false;
		}
		catch(...)
		{
			k3d::log() << error << "unknown exception" << std::endl;
			return false;
		}

		return true;
	}

	/// Helper class that limits the list of visible nodes to those that we can render
	template<typename value_t, class name_policy_t>
	class luxrender_visible_nodes_property :
		public k3d::data::writable_property<value_t, name_policy_t>,
		public k3d::inode_collection_property
	{
		typedef k3d::data::writable_property<value_t, name_policy_t> base;

	public:
		k3d::bool_t property_allow(k3d::inode& Node)
		{
			return Node.factory().factory_id() == k3d::classes::MeshInstance();
		}

	protected:
		template<typename init_t>
		luxrender_visible_nodes_property(const init_t& Init) :
			base(Init)
		{
		}
	};

	/// Helper class that limits the list of enabled lights
	template<typename value_t, class name_policy_t>
	class luxrender_enabled_lights_property :
		public k3d::data::writable_property<value_t, name_policy_t>,
		public k3d::inode_collection_property
	{
		typedef k3d::data::writable_property<value_t, name_policy_t> base;

	public:
		k3d::bool_t property_allow(k3d::inode& Node)
		{
			return dynamic_cast<luxrender::light*>(&Node) ? true : false;
		}

	protected:
		template<typename init_t>
		luxrender_enabled_lights_property(const init_t& Init) :
			base(Init)
		{
		}
	};

// define properties
	k3d_data(k3d::inode_collection_property::nodes_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, luxrender_visible_nodes_property, node_collection_serialization) m_visible_nodes;
	k3d_data(k3d::inode_collection_property::nodes_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, luxrender_enabled_lights_property, node_collection_serialization) m_enabled_lights;
	k3d_data(k3d::string_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, enumeration_property, with_serialization) m_resolution;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_pixel_width;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_pixel_height;
	k3d_data(k3d::int32_t, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_halt_samples;
};

k3d::iplugin_factory& render_engine_factory()
{
	return render_engine::get_factory();
}

} // namespace luxrender

} // namespace module


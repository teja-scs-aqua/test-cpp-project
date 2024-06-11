#ifndef MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_CACHED_TRIANGULATION_H
#define MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_CACHED_TRIANGULATION_H

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

/** \file Cached triangulation for use in mesh painters.
 * 	\author Bart Janssens (bart.janssens@lid.kviv.be)
 */

#include <k3dsdk/hints.h>
#include <k3dsdk/triangulator.h>

#include "painter_cache.h"

namespace module
{

namespace opengl
{

namespace painters
{

class cached_triangulation :
	public k3d::triangulator,
	public scheduler
{
public:
	cached_triangulation(const k3d::mesh* const Mesh) : m_mesh(Mesh) {}
	/// Links a single index to a list of indices
	typedef std::vector<k3d::mesh::indices_t> index_vectors_t;
	// 32 bit so arrays can be passed directly to OpenGL on 64bit platforms
	typedef std::vector<k3d::uint32_t> indices_t;

	/// The points that form the corners of the triangles. Duplicated per face to allow per-face normals
	k3d::mesh::points_t& points()
	{
		return m_points;
	}
	
	/// The indices into points() for the triangles
	indices_t& indices()
	{
		return m_indices;
	}
	
	/// First triangle corner (index into indices()) for each original face
	k3d::mesh::indices_t& face_starts()
	{
		return m_face_starts;
	}
	
	/// Link between original mesh point indices and the indices into points() of its copies
	index_vectors_t& point_links()
	{
		return m_point_links;
	}
	
	/// Link between a face index in the original mesh and its corner indices into points()
	index_vectors_t& face_points()
	{
		return m_face_points;
	}
	
private:

	void on_schedule(k3d::hint::mesh_geometry_changed* Hint, k3d::inode* Painter)
	{
		if (m_affected_indices.empty())
		{
			m_affected_indices = Hint->changed_points;
		}
	}

	void on_schedule(k3d::inode* Painter)
	{
		m_indices.clear();
		m_point_map.clear();
		m_points.clear();
		m_face_starts.clear();
		m_point_links.clear();
		m_progress = 0;
		m_face_points.clear();
		m_affected_indices.clear();
	}
	
	void on_execute(const k3d::mesh& Mesh, k3d::inode* Painter);

	virtual void start_face(const k3d::uint_t Face);
	virtual void add_vertex(const k3d::point3& Coordinates, k3d::uint_t Vertices[4], k3d::uint_t Edges[4], k3d::double_t Weights[4], k3d::uint_t& NewVertex);
	virtual void add_triangle(k3d::uint_t Vertices[3], k3d::uint_t Edges[3]);
	
	k3d::pipeline_data<k3d::mesh::points_t> m_input_points;
	
	// mapping between mesh point index and triangulation point index (valid for the face being processed only!)
	typedef std::map<k3d::uint_t, k3d::uint_t> point_map_t;
	point_map_t m_point_map;
	
	// points used by the triangles
	k3d::mesh::points_t m_points;
	
	// indices into m_points for the triangle corners
	indices_t m_indices;
	
	// First triangle corner (index into m_indices) for each original face
	k3d::mesh::indices_t m_face_starts;
	
	// Link between original point indices and the indices of its copies
	index_vectors_t m_point_links;
	
	// Link between a face and its corners in the triangle representation
	index_vectors_t m_face_points;
	
	k3d::mesh::indices_t m_affected_indices;
	
	// Keep track of what point we're at
	k3d::uint_t m_progress;
	
	const k3d::mesh* const m_mesh;
};

} // namespace opengl

} // namespace painters

} // namespace module

#endif // !MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_CACHED_TRIANGULATION_H

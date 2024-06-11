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

/** \file catmull_clark.cpp
		\author Bart Janssens (bart.janssens@lid.kviv.be)
		\created Feb 18, 2009
*/

#include <k3dsdk/basic_math.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/parallel/blocked_range.h>
#include <k3dsdk/parallel/parallel_for.h>
#include <k3dsdk/parallel/threads.h>
#include <k3dsdk/polyhedron.h>
#include <k3dsdk/selection.h>
#include <k3dsdk/subdivision_surface/catmull_clark.h>
#include <k3dsdk/table_copier.h>
#include <k3dsdk/utility.h>
#include <k3dsdk/vectors.h>

#include <boost/scoped_ptr.hpp>

namespace k3d
{

namespace sds
{

namespace detail
{

/// Replace the elements of Array with their cumulative sum
template<class ArrayT> void cumulative_sum(ArrayT& Array)
{
	const k3d::uint_t array_begin = 0;
	const k3d::uint_t array_end = Array.size();
	for(k3d::uint_t i = array_begin + 1; i != array_end; ++i)
		Array[i] += Array[i-1];
}

/// True if Face is the first face containing Point
k3d::bool_t first_corner(const k3d::uint_t Face, const k3d::uint_t Point, const std::vector<k3d::mesh::indices_t>& PointFaces)
{
	const k3d::mesh::indices_t& faces = PointFaces[Point];
	const k3d::uint_t face_begin = 0;
	const k3d::uint_t face_end = faces.size();
	for(k3d::uint_t i = face_begin; i != face_end; ++i)
	{
		if(faces[i] < Face)
			return false;
	}
	return true;
}

/// Stores references to commonly used arrays, and defines some common checks for faces and edges
class mesh_arrays
{
public:
	mesh_arrays(const k3d::mesh::selection_t& FaceSelection,
			const k3d::mesh::indices_t& FaceFirstLoops,
			const k3d::mesh::counts_t& FaceLoopCounts,
			const k3d::mesh::indices_t& LoopFirstEdges,
			const k3d::mesh::indices_t& ClockwiseEdges,
			const k3d::mesh::indices_t& EdgeFaces,
			const k3d::mesh::indices_t& Companions) :
				face_selection(FaceSelection),
				face_first_loops(FaceFirstLoops),
				face_loop_counts(FaceLoopCounts),
				loop_first_edges(LoopFirstEdges),
				clockwise_edges(ClockwiseEdges),
				edge_faces(EdgeFaces),
				companions(Companions)
	{}
	
	/// True if the given face is affected by the operation
	k3d::bool_t is_affected(const k3d::uint_t Face) const
	{
		return face_selection[Face] && face_loop_counts[Face] == 1;
	}
	
	/// True if the face adjacent to Edge is affected. In case of a boundary edge, returns the affected status of the face itself
	k3d::bool_t is_companion_affected(const k3d::uint_t Edge) const
	{
		return is_affected(edge_faces[companions[Edge]]);
	}
	
	/// True if the face that Edge belongs to comes before the face its companion belongs to, when considering midpoints
	k3d::bool_t first_midpoint(const k3d::uint_t Edge) const
	{
		const k3d::uint_t face = edge_faces[Edge];
		const k3d::uint_t companion_face = edge_faces[companions[Edge]];
		return ((face <= companion_face && !is_affected(face))
						|| is_affected(face) && (face <= companion_face || !is_affected(companion_face)));
	}
	
	/// True if the edge is part of the boundary of the mesh that is to be subdivided
	/**
	 * This means it is one of the following:
	 * - A real boundary edge
	 * - An edge next to an unselected face
	 * - An edge next to a face with holes
	 */
	k3d::bool_t boundary(const k3d::uint_t Edge) const
	{
		const k3d::uint_t companion = companions[Edge];
		return companion == Edge
					|| (is_affected(edge_faces[Edge]) && !is_affected(edge_faces[companion]))
					|| (!is_affected(edge_faces[Edge]) && is_affected(edge_faces[companion]));
	}
	
	const k3d::mesh::selection_t& face_selection;
	const k3d::mesh::indices_t& face_first_loops;
	const k3d::mesh::counts_t& face_loop_counts;
	const k3d::mesh::indices_t& loop_first_edges;
	const k3d::mesh::indices_t& clockwise_edges;
	const k3d::mesh::indices_t& edge_faces;
	const k3d::mesh::indices_t& companions;
};

/// For each old face index, count the number of subfaces, loops, edges and distinct points that will be in the new mesh
/**
 * The parameters FaceSubfaceCounts, FaceSubloopCounts, FaceEdgeCounts and FacePointCounts store the result and must
 * have the same length as the number of faces of the input mesh
 */
class per_face_component_counter
{
public:
	per_face_component_counter(const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& EdgePoints,
			const std::vector<k3d::mesh::indices_t>& PointFaces,
			k3d::mesh::counts_t& FaceSubfaceCounts,
			k3d::mesh::counts_t& FaceSubloopCounts,
			k3d::mesh::counts_t& FaceEdgeCounts,
			k3d::mesh::counts_t& FacePointCounts) :
		m_mesh_arrays(MeshArrays),
		m_edge_points(EdgePoints),
		m_point_faces(PointFaces),
		m_face_subface_counts(FaceSubfaceCounts),
		m_face_subloop_counts(FaceSubloopCounts),
		m_face_edge_counts(FaceEdgeCounts),
		m_face_point_counts(FacePointCounts)
	{}
	
	void operator()(const unsigned int Face)
	{
		k3d::uint_t& subface_count = m_face_subface_counts[Face];
		k3d::uint_t& subloop_count = m_face_subloop_counts[Face];
		k3d::uint_t& edge_count = m_face_edge_counts[Face];
		k3d::uint_t& point_count = m_face_point_counts[Face];
		subface_count = 0;
		subloop_count = 0;
		edge_count = 0;
		point_count = 0;
		if(!m_mesh_arrays.is_affected(Face)) // Faces that will not be split
		{
			++subface_count;
			subloop_count += m_mesh_arrays.face_loop_counts[Face];
			const k3d::uint_t loop_begin = m_mesh_arrays.face_first_loops[Face];
			const k3d::uint_t loop_end = loop_begin + m_mesh_arrays.face_loop_counts[Face];
			for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
			{
				const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[loop];
				for(k3d::uint_t edge = first_edge; ; )
				{
					++edge_count; // Count the original edge
					// Note: we assume here that in case of a boundary edge, companion == edge
					if(m_mesh_arrays.is_companion_affected(edge))
					{
						++edge_count; // Reserve space for an edge that is split because this or the adjacent face is selected
					} 
					
					// Count the new corner points, respecting face order
					
					if(first_corner(Face, m_edge_points[m_mesh_arrays.clockwise_edges[edge]], m_point_faces))
					{
						++point_count;
					}
					
					edge = m_mesh_arrays.clockwise_edges[edge];
					if(edge == first_edge)
						break;
				}
			}
		}
		else // Faces to split
		{
			const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[m_mesh_arrays.face_first_loops[Face]];
			// Count the face point
			++point_count;
			for(k3d::uint_t edge = first_edge; ; )
			{
				++subface_count;
				++subloop_count;
				edge_count += 4;
				
				if(m_mesh_arrays.first_midpoint(edge))
				{
					++point_count; // Count the midpoint
				}
				
				if(first_corner(Face, m_edge_points[m_mesh_arrays.clockwise_edges[edge]], m_point_faces))
					++point_count;
				
				edge = m_mesh_arrays.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}
	
private:
	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_edge_points;
	const std::vector<k3d::mesh::indices_t>& m_point_faces;
	k3d::mesh::counts_t& m_face_subface_counts;
	k3d::mesh::counts_t& m_face_subloop_counts;
	k3d::mesh::counts_t& m_face_edge_counts;
	k3d::mesh::counts_t& m_face_point_counts;
};

/// Calculates the new indices of corner points, edge midpoints and face centers 
class point_index_calculator
{
public:
	point_index_calculator(const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& EdgePoints,
			std::vector<k3d::mesh::indices_t>& PointFaces,
			const k3d::mesh::counts_t& FacePointCounts,
			k3d::mesh::indices_t& CornerPoints,
			k3d::mesh::indices_t& EdgeMidpoints,
			k3d::mesh::indices_t& FaceCenters
			) :
				m_mesh_arrays(MeshArrays),
				m_edge_points(EdgePoints),
				m_point_faces(PointFaces),
				m_face_point_counts(FacePointCounts),
				m_corner_points(CornerPoints),
				m_edge_midpoints(EdgeMidpoints),
				m_face_centers(FaceCenters)
			{}
	
	void operator()(const k3d::uint_t Face)
	{
		k3d::uint_t point_count = Face == 0 ? 0 : m_face_point_counts[Face - 1];
		if(!m_mesh_arrays.is_affected(Face)) // Faces that will not be split
		{
			const k3d::uint_t loop_begin = m_mesh_arrays.face_first_loops[Face];
			const k3d::uint_t loop_end = loop_begin + m_mesh_arrays.face_loop_counts[Face];
			for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
			{
				const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[loop];
				for(k3d::uint_t edge = first_edge; ; )
				{
					if(first_corner(Face, m_edge_points[m_mesh_arrays.clockwise_edges[edge]], m_point_faces))
					{
						const k3d::uint_t clockwise = m_mesh_arrays.clockwise_edges[edge]; 
						m_corner_points[m_edge_points[clockwise]] = point_count;
						++point_count;
					}
					
					edge = m_mesh_arrays.clockwise_edges[edge];
					if(edge == first_edge)
						break;
				}
			}
		}
		else // Faces to split
		{
			const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[m_mesh_arrays.face_first_loops[Face]];
			m_face_centers[Face] = point_count;
			++point_count;
			for(k3d::uint_t edge = first_edge; ; )
			{
				const k3d::uint_t clockwise = m_mesh_arrays.clockwise_edges[edge];
				if(m_mesh_arrays.first_midpoint(edge))
				{
					m_edge_midpoints[edge] = point_count;
					m_edge_midpoints[m_mesh_arrays.companions[edge]] = point_count;
					++point_count; // Count the midpoint
				}
				
				if(first_corner(Face, m_edge_points[m_mesh_arrays.clockwise_edges[edge]], m_point_faces))
				{ 
					m_corner_points[m_edge_points[clockwise]] = point_count;
					++point_count;
				}
				
				edge = clockwise;
				if(edge == first_edge)
					break;
			}
		}
	}
	
private:
	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_edge_points;
	std::vector<k3d::mesh::indices_t>& m_point_faces;
	const k3d::mesh::counts_t& m_face_point_counts;
	k3d::mesh::indices_t& m_corner_points;
	k3d::mesh::indices_t& m_edge_midpoints;
	k3d::mesh::indices_t& m_face_centers;
};

/// Create the subdivided mesh topology
class topology_subdivider
{
public:
	topology_subdivider(const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& InputEdgePoints,
			const k3d::mesh::materials_t& InputFaceMaterials,
			const k3d::mesh::counts_t& FaceSubfaceCounts,
			const k3d::mesh::counts_t& FaceSubloopCounts,
			const k3d::mesh::counts_t& FaceEdgeCounts,
			const k3d::mesh::indices_t& CornerPoints,
			const k3d::mesh::indices_t& EdgeMidpoints,
			const k3d::mesh::indices_t& FaceCenters,
			const k3d::mesh::indices_t& InputFaceShells,
			k3d::mesh::indices_t& OutputEdgePoints,
			k3d::mesh::indices_t& OutputClockwiseEdges,
			k3d::mesh::indices_t& OutputLoopFirstEdges,
			k3d::mesh::indices_t& OutputFaceFirstLoops,
			k3d::mesh::counts_t& OutputFaceLoopCounts,
			k3d::mesh::materials_t& OutputFaceMaterials,
			k3d::mesh::selection_t& OutputFaceSelection,
			k3d::mesh::indices_t& OutputFaceShells
			) :
				m_mesh_arrays(MeshArrays),
				m_input_edge_points(InputEdgePoints),
				m_input_face_materials(InputFaceMaterials),
				m_face_subface_counts(FaceSubfaceCounts),
				m_face_subloop_counts(FaceSubloopCounts),
				m_face_edge_counts(FaceEdgeCounts),
				m_corner_points(CornerPoints),
				m_edge_midpoints(EdgeMidpoints),
				m_face_centers(FaceCenters),
				m_input_face_shells(InputFaceShells),
				m_output_edge_points(OutputEdgePoints),
				m_output_clockwise_edges(OutputClockwiseEdges),
				m_output_loop_first_edges(OutputLoopFirstEdges),
				m_output_face_first_loops(OutputFaceFirstLoops),
				m_output_face_loop_counts(OutputFaceLoopCounts),
				m_output_face_materials(OutputFaceMaterials),
				m_output_face_selection(OutputFaceSelection),
				m_output_face_shells(OutputFaceShells)
				{}
	
	void operator()(const k3d::uint_t Face)
	{
		const k3d::uint_t first_new_face = Face == 0 ? 0 : m_face_subface_counts[Face - 1];
		const k3d::uint_t first_new_loop = Face == 0 ? 0 : m_face_subloop_counts[Face - 1];

		if(!m_mesh_arrays.is_affected(Face))
		{ // copy unaffected face, splitting edges adjacent to affected faces
			copy_face(Face);
		}
		else
		{ // subdivide affected faces
			const k3d::uint_t face_center = m_face_centers[Face];
			const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[m_mesh_arrays.face_first_loops[Face]];
			k3d::uint_t edgenumber = 0;
			k3d::uint_t first_new_edge = Face == 0 ? 0 : m_face_edge_counts[Face - 1];
			m_output_loop_first_edges[first_new_loop] = first_new_edge;
			m_output_face_first_loops[first_new_face] = first_new_loop;
			m_output_face_materials[first_new_face] = m_input_face_materials[Face];
			m_output_face_selection[first_new_face] = m_mesh_arrays.face_selection[Face];
			m_output_face_shells[first_new_face] = m_input_face_shells[Face];

			for(k3d::uint_t edge = first_edge; ; )
			{
				const k3d::uint_t clockwise = m_mesh_arrays.clockwise_edges[edge];
				const k3d::uint_t mid1 = m_edge_midpoints[edge];
				const k3d::uint_t mid2 = m_edge_midpoints[clockwise];
				const k3d::uint_t center_to_mid1 = first_new_edge;
				const k3d::uint_t mid1_to_corner = center_to_mid1 + 1;
				const k3d::uint_t corner_to_mid2 = mid1_to_corner + 1;
				const k3d::uint_t mid2_to_center = corner_to_mid2 + 1;
				m_output_edge_points[mid1_to_corner] = mid1;
				m_output_edge_points[corner_to_mid2] = m_corner_points[m_input_edge_points[clockwise]];
				m_output_edge_points[mid2_to_center] = mid2;
				m_output_edge_points[center_to_mid1] = face_center;
				m_output_clockwise_edges[mid1_to_corner] = corner_to_mid2;
				m_output_clockwise_edges[corner_to_mid2] = mid2_to_center;
				m_output_clockwise_edges[mid2_to_center] = center_to_mid1;
				m_output_clockwise_edges[center_to_mid1] = mid1_to_corner;

				// Append face data
				const k3d::uint_t newloop = first_new_loop + edgenumber;
				m_output_loop_first_edges[newloop] = center_to_mid1; 
				m_output_face_first_loops[first_new_face + edgenumber] = newloop;
				m_output_face_materials[first_new_face + edgenumber] = m_input_face_materials[Face];
				m_output_face_selection[first_new_face + edgenumber] = m_mesh_arrays.face_selection[Face];
				m_output_face_shells[first_new_face] = m_input_face_shells[Face];

				first_new_edge += 4;

				edge = clockwise;
				++edgenumber;
				if(edge == first_edge)
					break;
			}
		}
	}

private:
	/// Copies the face to the correct location in the output
	void copy_face(const k3d::uint_t Face)
	{
		const k3d::uint_t first_new_edge = Face == 0 ? 0 : m_face_edge_counts[Face - 1];
		const k3d::uint_t first_new_face = Face == 0 ? 0 : m_face_subface_counts[Face - 1];
		const k3d::uint_t first_new_loop = Face == 0 ? 0 : m_face_subloop_counts[Face - 1];

		const k3d::uint_t loop_begin = m_mesh_arrays.face_first_loops[Face];
		const k3d::uint_t loop_end = loop_begin + m_mesh_arrays.face_loop_counts[Face];
		k3d::uint_t edgenumber = 0;
		for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[loop];
			k3d::uint_t loopedgenumber = 0;
			m_output_loop_first_edges[first_new_loop + loop - loop_begin] = first_new_edge + edgenumber;
			for(k3d::uint_t edge = first_edge; ; )
			{
				const k3d::uint_t newedge = first_new_edge + edgenumber + loopedgenumber;
				m_output_edge_points[newedge] = m_corner_points[m_input_edge_points[edge]];
				m_output_clockwise_edges[newedge] = newedge + 1;
				if(m_mesh_arrays.is_companion_affected(edge))
				{
					m_output_edge_points[newedge + 1] = m_edge_midpoints[m_mesh_arrays.companions[edge]];
					m_output_clockwise_edges[newedge + 1] = newedge + 2;
					++loopedgenumber;
				}

				edge = m_mesh_arrays.clockwise_edges[edge];
				if(edge == first_edge)
					break;
				++loopedgenumber;
			}
			m_output_clockwise_edges[first_new_edge + edgenumber + loopedgenumber] = first_new_edge + edgenumber; // close the loop
			edgenumber += loopedgenumber + 1;
		}

		// Set face data
		m_output_face_loop_counts[first_new_face] = m_mesh_arrays.face_loop_counts[Face];
		m_output_face_first_loops[first_new_face] = first_new_loop;
		m_output_face_materials[first_new_face] = m_input_face_materials[Face];
		m_output_face_selection[first_new_face] = m_mesh_arrays.face_selection[Face];
		m_output_face_shells[first_new_face] = m_input_face_shells[Face];
	}

	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_input_edge_points;
	const k3d::mesh::materials_t& m_input_face_materials;
	const k3d::mesh::counts_t& m_face_subface_counts;
	const k3d::mesh::counts_t& m_face_subloop_counts;
	const k3d::mesh::counts_t& m_face_edge_counts;
	const k3d::mesh::indices_t& m_corner_points;
	const k3d::mesh::indices_t& m_edge_midpoints;
	const k3d::mesh::indices_t& m_face_centers;
	const k3d::mesh::indices_t& m_input_face_shells;
	k3d::mesh::indices_t& m_output_edge_points;
	k3d::mesh::indices_t& m_output_clockwise_edges;
	k3d::mesh::indices_t& m_output_loop_first_edges;
	k3d::mesh::indices_t& m_output_face_first_loops;
	k3d::mesh::counts_t& m_output_face_loop_counts;
	k3d::mesh::materials_t& m_output_face_materials;
	k3d::mesh::selection_t& m_output_face_selection;
	k3d::mesh::indices_t& m_output_face_shells;
};

/// Calculates face centers
class face_center_calculator
{
public:
	/// Constructs a new functor that will calculate the face centers of all selected faces with one loop
	/*
	 * \param FaceCenters The point index of the center for each face (only defined for selected faces with one loop)
	 */
	face_center_calculator(
			const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& InputEdgePoints,
			const k3d::mesh::indices_t& OutputFaceFirstLoops,
			const k3d::mesh::indices_t& OutputLoopFirstEdges,
			const k3d::mesh::indices_t& OutputClockwiseEdges,
			const k3d::mesh::indices_t& FaceCenters,
			const k3d::mesh::counts_t& FaceSubfaceCounts,
			const k3d::mesh::points_t& InputPoints,
			k3d::mesh::points_t& OutputPoints,
			k3d::table_copier& FaceCopier,
			k3d::table_copier& EdgeAttributesCopier,
			k3d::table_copier& VertexAttributesCopier,
			k3d::table_copier& PointAttributesCopier) :
		m_mesh_arrays(MeshArrays),
		m_input_edge_points(InputEdgePoints),
		m_output_face_first_loops(OutputFaceFirstLoops),
		m_output_loop_first_edges(OutputLoopFirstEdges),
		m_output_clockwise_edges(OutputClockwiseEdges),
		m_face_centers(FaceCenters),
		m_face_subface_counts(FaceSubfaceCounts),
		m_input_points(InputPoints),
		m_output_points(OutputPoints),
		m_uniform_copier(FaceCopier),
		m_edge_attributes_copier(EdgeAttributesCopier),
		m_vertex_attributes_copier(VertexAttributesCopier),
		m_point_attributes_copier(PointAttributesCopier)
	{}
			
	void operator()(const k3d::uint_t Face)
	{
		const k3d::uint_t first_new_face = Face == 0 ? 0 : m_face_subface_counts[Face - 1];
		
		// Copy named arrays for unaffected faces
		if(!m_mesh_arrays.is_affected(Face))
		{
			m_uniform_copier.copy(Face, first_new_face);
			const k3d::uint_t loop_begin = m_mesh_arrays.face_first_loops[Face];
			const k3d::uint_t loop_end = loop_begin + m_mesh_arrays.face_loop_counts[Face];
			const k3d::uint_t output_first_loop = m_output_face_first_loops[first_new_face];
			for(k3d::uint_t loop = loop_begin; loop != loop_end; ++loop)
			{
				const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[loop];
				k3d::uint_t output_edge = m_output_loop_first_edges[output_first_loop + loop - loop_begin];
				for(k3d::uint_t edge = first_edge; ; )
				{
					m_edge_attributes_copier.copy(edge, output_edge);
					m_vertex_attributes_copier.copy(edge, output_edge);
					
					output_edge = m_output_clockwise_edges[output_edge];
					edge = m_mesh_arrays.clockwise_edges[edge];
					if(edge == first_edge)
						break;
				}
			}
		}
		else
		{
			const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[m_mesh_arrays.face_first_loops[Face]];
			k3d::point3& center = m_output_points[m_face_centers[Face]];
			center = k3d::point3(0,0,0);
			k3d::uint_t count = 0;
			
			for(k3d::uint_t edge = first_edge; ; )
			{
				center += k3d::to_vector(m_input_points[m_input_edge_points[edge]]);
				++count;
	
				edge = m_mesh_arrays.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
			center /= count;
			
			//indices for target of the varying data copy
			k3d::mesh::indices_t edges(count);
			k3d::mesh::indices_t points(count);
			k3d::mesh::weights_t weights(count, 1.0/static_cast<double>(count));
			k3d::uint_t i = 0;
			for(k3d::uint_t edge = first_edge; ; )
			{
				edges[i] = edge;
				points[i] = m_input_edge_points[edge];
				++i;
	
				edge = m_mesh_arrays.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
			k3d::uint_t output_face = first_new_face;
			for(k3d::uint_t edge = first_edge; ; )
			{
				const k3d::uint_t output_first_edge = m_output_loop_first_edges[m_output_face_first_loops[output_face]];
				m_edge_attributes_copier.copy(count, &edges[0], &weights[0], output_first_edge); // varying data for the edge starting at the face center
				m_edge_attributes_copier.copy(m_mesh_arrays.clockwise_edges[edge], m_output_clockwise_edges[m_output_clockwise_edges[output_first_edge]]); // varying data for the edge starting at the corner
				m_vertex_attributes_copier.copy(count, &edges[0], &weights[0], output_first_edge); // varying data for the edge starting at the face center
				m_vertex_attributes_copier.copy(m_mesh_arrays.clockwise_edges[edge], m_output_clockwise_edges[m_output_clockwise_edges[output_first_edge]]); // varying data for the edge starting at the corner
				m_uniform_copier.copy(Face, output_face);
	
				++output_face;
				edge = m_mesh_arrays.clockwise_edges[edge];
				if(edge == first_edge)
					break;
			}
			m_point_attributes_copier.copy(count, &points[0], &weights[0], m_face_centers[Face]);
		}
	}
	
private:
	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_input_edge_points;
	const k3d::mesh::indices_t& m_output_face_first_loops;
	const k3d::mesh::indices_t& m_output_loop_first_edges;
	const k3d::mesh::indices_t& m_output_clockwise_edges;
	const k3d::mesh::indices_t& m_face_centers;
	const k3d::mesh::counts_t& m_face_subface_counts;
	const k3d::mesh::points_t& m_input_points;
	k3d::mesh::points_t& m_output_points;
	k3d::table_copier& m_uniform_copier;
	k3d::table_copier& m_edge_attributes_copier;
	k3d::table_copier& m_vertex_attributes_copier;
	k3d::table_copier& m_point_attributes_copier;
};

/// Calculates edge midpoints
class edge_midpoint_calculator
{
public:
	/// Constructs a new functor that will calculate the midpoints of all edges belonging to selected faces with one loop
	/*
	 * \param EdgeMidpoints The point index of the midpoint for each edge (only defined for selected faces with one loop)
	 */
	edge_midpoint_calculator(
			const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& InputEdgePoints,
			const k3d::mesh::indices_t& OutputFaceFirstLoops,
			const k3d::mesh::indices_t& OutputLoopFirstEdges,
			const k3d::mesh::indices_t& OutputClockwiseEdges,
			const k3d::mesh::indices_t& EdgeMidpoints,
			const k3d::mesh::indices_t& FaceCenters,
			const k3d::mesh::counts_t& FaceSubfaceCounts,
			const k3d::mesh::points_t& InputPoints,
			k3d::mesh::points_t& OutputPoints,
			k3d::table_copier& EdgeAttributesCopier,
			k3d::table_copier& VertexAttributesCopier,
			k3d::table_copier& PointAttributesCopier,
			k3d::table_copier& PointAttributesMixer) :
		m_mesh_arrays(MeshArrays),
		m_input_edge_points(InputEdgePoints),
		m_output_face_first_loops(OutputFaceFirstLoops),
		m_output_loop_first_edges(OutputLoopFirstEdges),
		m_output_clockwise_edges(OutputClockwiseEdges),
		m_edge_midpoints(EdgeMidpoints),
		m_face_centers(FaceCenters),
		m_face_subface_counts(FaceSubfaceCounts),
		m_input_points(InputPoints),
		m_output_points(OutputPoints),
		m_edge_attributes_copier(EdgeAttributesCopier),
		m_vertex_attributes_copier(VertexAttributesCopier),
		m_point_attributes_copier(PointAttributesCopier),
		m_point_attributes_mixer(PointAttributesMixer)
	{}


	void operator()(const k3d::uint_t Face)
	{
		if(!m_mesh_arrays.is_affected(Face))
			return;
		
		const k3d::uint_t first_edge = m_mesh_arrays.loop_first_edges[m_mesh_arrays.face_first_loops[Face]];
		
		// Get the edge indices of the own face, for named array copying
		for(k3d::uint_t edge = first_edge; ; )
		{
			edge = m_mesh_arrays.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}
		
		const k3d::uint_t first_new_face = Face == 0 ? 0 : m_face_subface_counts[Face - 1];
		k3d::uint_t output_face = first_new_face;
		for(k3d::uint_t edge = first_edge; ; )
		{
			return_if_fail(m_edge_midpoints[edge] != 0);
			k3d::point3& midpoint = m_output_points[m_edge_midpoints[edge]];
			const k3d::uint_t companion = m_mesh_arrays.companions[edge];
			const k3d::uint_t output_first_edge = m_output_loop_first_edges[m_output_face_first_loops[output_face]];
			if(m_mesh_arrays.first_midpoint(edge))
			{
				if(m_mesh_arrays.boundary(edge))
				{
					midpoint = 0.5 * (m_input_points[m_input_edge_points[edge]] + k3d::to_vector(m_input_points[m_input_edge_points[m_mesh_arrays.clockwise_edges[edge]]]));
					const k3d::uint_t indices[] = {m_input_edge_points[edge], m_input_edge_points[m_mesh_arrays.clockwise_edges[edge]]};
					const k3d::double_t weights[] = {0.5, 0.5};
					m_point_attributes_copier.copy(2, indices, weights, m_edge_midpoints[edge]);
				}
				else
				{
					midpoint = (m_input_points[m_input_edge_points[edge]] + k3d::to_vector(m_input_points[m_input_edge_points[m_mesh_arrays.clockwise_edges[edge]]])); // midpoint
					midpoint += k3d::to_vector(m_output_points[m_face_centers[Face]]); // Add the face center of the current face
					midpoint += k3d::to_vector(m_output_points[m_face_centers[m_mesh_arrays.edge_faces[companion]]]);
					midpoint *= 0.25;
					const k3d::uint_t corner_indices[] = {m_input_edge_points[edge],
												m_input_edge_points[m_mesh_arrays.clockwise_edges[edge]]};
					const k3d::double_t corner_weights[] = {0.5, 0.5};
					m_point_attributes_copier.copy(2, corner_indices, corner_weights, m_edge_midpoints[edge]); // Copy the corner contribution
					const k3d::double_t face_weights[] = {0.5, 0.25, 0.25};
					const k3d::uint_t face_indices[] = {m_edge_midpoints[edge], m_face_centers[Face],
																	m_face_centers[m_mesh_arrays.edge_faces[companion]]};
					m_point_attributes_mixer.copy(3, face_indices, face_weights, m_edge_midpoints[edge]); // Mix in the adjacent face values
				}
			}
			// copy varying data
			const k3d::uint_t output_edge1 = m_output_clockwise_edges[m_output_clockwise_edges[m_output_clockwise_edges[output_first_edge]]]; // Edge from clockwise midpoint to center
			const k3d::uint_t next_output_face = m_mesh_arrays.clockwise_edges[edge] == first_edge ? first_new_face : output_face + 1;
			const k3d::uint_t next_output_first_edge = m_output_loop_first_edges[m_output_face_first_loops[next_output_face]];
			const k3d::uint_t output_edge2 = m_output_clockwise_edges[next_output_first_edge]; // Edge from clockwise midpoint to corner
			const k3d::double_t weights[] = {0.5, 0.5};
			const k3d::uint_t varying_indices[] = {m_mesh_arrays.clockwise_edges[edge], m_mesh_arrays.clockwise_edges[m_mesh_arrays.clockwise_edges[edge]]};
			m_edge_attributes_copier.copy(2, varying_indices, weights, output_edge1);
			m_edge_attributes_copier.copy(2, varying_indices, weights, output_edge2);
			m_vertex_attributes_copier.copy(2, varying_indices, weights, output_edge1);
			m_vertex_attributes_copier.copy(2, varying_indices, weights, output_edge2);

			++output_face;
			edge = m_mesh_arrays.clockwise_edges[edge];
			if(edge == first_edge)
				break;
		}
	}

private:
	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_input_edge_points;
	const k3d::mesh::indices_t& m_output_face_first_loops;
	const k3d::mesh::indices_t& m_output_loop_first_edges;
	const k3d::mesh::indices_t& m_output_clockwise_edges;
	const k3d::mesh::indices_t& m_edge_midpoints;
	const k3d::mesh::indices_t& m_face_centers;
	const k3d::mesh::counts_t& m_face_subface_counts;
	const k3d::mesh::points_t& m_input_points;
	k3d::mesh::points_t& m_output_points;
	k3d::table_copier& m_edge_attributes_copier;
	k3d::table_copier& m_vertex_attributes_copier;
	k3d::table_copier& m_point_attributes_copier;
	k3d::table_copier& m_point_attributes_mixer;
};

/// Calculates patch corner positions
class corner_point_calculator
{
public:
	corner_point_calculator(
			const mesh_arrays& MeshArrays,
			const k3d::mesh::indices_t& InputEdgePoints,
			const k3d::mesh::indices_t& CornerPoints,
			const k3d::mesh::indices_t& EdgeMidpoints,
			const k3d::mesh::indices_t& FaceCenters,
			const std::vector<k3d::mesh::indices_t>& PointOutEdges,
			const k3d::mesh::points_t& InputPoints,
			k3d::mesh::points_t& OutputPoints,
			k3d::table_copier& PointAttributesCopier,
			k3d::table_copier& PointAttributesMixer) :
		m_mesh_arrays(MeshArrays),
		m_input_edge_points(InputEdgePoints),
		m_corner_points(CornerPoints),
		m_edge_midpoints(EdgeMidpoints),
		m_face_centers(FaceCenters),
		m_input_points(InputPoints),
		m_point_out_edges(PointOutEdges),
		m_output_points(OutputPoints),
		m_point_attributes_copier(PointAttributesCopier),
		m_point_attributes_mixer(PointAttributesMixer)
	{}


	void operator()(const k3d::uint_t Point)
	{
		// Set initial position
		const k3d::uint_t output_idx = m_corner_points[Point];
		k3d::point3& output_position = m_output_points[output_idx];
		output_position = m_input_points[Point];
		
		// Get the number of outbound affected and boundary edges
		k3d::uint_t affected_edge_count = 0;
		k3d::uint_t boundary_edge_count = 0;
		const k3d::mesh::indices_t& out_edges = m_point_out_edges[Point];
		const k3d::uint_t valence = out_edges.size();
		const k3d::uint_t start_index = 0;
		const k3d::uint_t end_index = valence;
		for(k3d::uint_t index = start_index; index != end_index; ++index)
		{
			const k3d::uint_t edge = out_edges[index];
			const k3d::uint_t face = m_mesh_arrays.edge_faces[edge];
			if(m_mesh_arrays.is_affected(face))
				++affected_edge_count;
			if(m_mesh_arrays.boundary(edge))
				++boundary_edge_count;
		}
		
		if(affected_edge_count == valence && boundary_edge_count == 0 && valence != 0) // Interior point of the subdivided surface
		{
			const k3d::double_t own_weight = static_cast<double>(valence - 2.0) / static_cast<double>(valence); // Weight attributed to Point
			const k3d::double_t neighbour_weight = 1.0 / static_cast<double>(valence * valence); // Weight attributed to surrounding corners and face vertices
			output_position *=  own_weight; // adjust initial position
			k3d::vector3 sum(0, 0, 0);
			k3d::mesh::indices_t corner_indices(valence + 1); // indices of neighbor corners, for vertex attribute data
			k3d::mesh::indices_t face_indices(valence + 1); // indices of neighbor faces face vertices, for vertex attribute data
			for(k3d::uint_t index = start_index; index != end_index; ++index)
			{
				const k3d::uint_t edge = out_edges[index];
				const k3d::uint_t clockwise = m_mesh_arrays.clockwise_edges[edge];
				const k3d::uint_t face = m_mesh_arrays.edge_faces[edge];
				const k3d::vector3 next_corner = k3d::to_vector(m_input_points[m_input_edge_points[clockwise]]);
				const k3d::vector3 face_vertex = k3d::to_vector(m_output_points[m_face_centers[face]]);
				corner_indices[index - start_index] = m_input_edge_points[clockwise];
				face_indices[index - start_index] = m_face_centers[face];
				sum += next_corner + face_vertex;
			}
			sum *= neighbour_weight;
			output_position += sum;
			k3d::mesh::weights_t corner_weights(corner_indices.size(), neighbour_weight);
			corner_indices.back() = Point; // Append the current point and its weight
			corner_weights.back() = own_weight;
			m_point_attributes_copier.copy(corner_indices.size(), &corner_indices[0], &corner_weights[0], output_idx); // Contribution of Point and its neighbor corners
			k3d::mesh::weights_t face_weights(face_indices.size(), neighbour_weight);
			face_indices.back() = output_idx;
			face_weights.back() = 1.0;
			m_point_attributes_mixer.copy(face_indices.size(), &face_indices[0], &face_weights[0], output_idx); // Contribution of the face vertices
		}
		else if(affected_edge_count != 0) // Boundary of the subdivided surface
		{
			output_position *= 0.5;
			m_point_attributes_copier.copy(Point, m_corner_points[Point]);
			k3d::double_t boundary_weights[] = {0.5, 0.25};
			k3d::uint_t boundary_indices[] = {m_corner_points[Point], 0};
			for(k3d::uint_t index = start_index; index != end_index; ++index)
			{
				const k3d::uint_t edge = out_edges[index];
				// we might also need to account for the counter-clockwise edge, since point_edges only stores outbound edges
				k3d::uint_t counter_clockwise = edge;
				for(; ;)
				{
					const k3d::uint_t clockwise = m_mesh_arrays.clockwise_edges[counter_clockwise];
					if(clockwise == edge)
						break;
					counter_clockwise = clockwise;
				}
				if(m_mesh_arrays.companions[counter_clockwise] == counter_clockwise && m_mesh_arrays.is_affected(m_mesh_arrays.edge_faces[counter_clockwise]))
				{
					output_position += 0.25 * k3d::to_vector(m_output_points[m_edge_midpoints[counter_clockwise]]);
					boundary_indices[1] = m_edge_midpoints[counter_clockwise];
					m_point_attributes_mixer.copy(2, boundary_indices, boundary_weights, m_corner_points[Point]);
					boundary_weights[0] = 1.0;
				}
				if(m_mesh_arrays.boundary(edge))
				{
					output_position += 0.25 * k3d::to_vector(m_output_points[m_edge_midpoints[edge]]);
					boundary_indices[1] = m_edge_midpoints[edge];
					m_point_attributes_mixer.copy(2, boundary_indices, boundary_weights, m_corner_points[Point]);
					boundary_weights[0] = 1.0;
				}
			}
		}
	}

private:
	const mesh_arrays& m_mesh_arrays;
	const k3d::mesh::indices_t& m_input_edge_points;
	const k3d::mesh::indices_t& m_corner_points;
	const k3d::mesh::indices_t& m_edge_midpoints;
	const k3d::mesh::indices_t& m_face_centers;
	const std::vector<k3d::mesh::indices_t>& m_point_out_edges;
	const k3d::mesh::points_t& m_input_points;
	k3d::mesh::points_t& m_output_points;
	k3d::table_copier& m_point_attributes_copier;
	k3d::table_copier& m_point_attributes_mixer;
};

/// Helper for TBB
class corner_worker
{
public:
	corner_worker(const k3d::mesh::indices_t& VertexPoints, corner_point_calculator& CornerCalculator) : m_vertex_points(VertexPoints), m_corner_calculator(CornerCalculator) {}
	void operator()(const k3d::parallel::blocked_range<k3d::uint_t>& range) const
	{
		const k3d::uint_t point_begin = range.begin();
		const k3d::uint_t point_end = range.end();
		for(k3d::uint_t point = point_begin; point != point_end; ++point)
		{
			m_corner_calculator(m_vertex_points[point]);
		}
	}
private:
	const k3d::mesh::indices_t& m_vertex_points;
	corner_point_calculator& m_corner_calculator;
};

template<typename FunctorT>
struct worker
{
	FunctorT& f;
	worker(FunctorT& F) : f(F) {}
	void operator()(const k3d::parallel::blocked_range<k3d::uint_t>& range) const
	{
		const k3d::uint_t begin = range.begin();
		const k3d::uint_t end = range.end();
		for(k3d::uint_t i = begin; i != end; ++i)
		{
			f(i);
		}
	}
};

} // namespace detail

class catmull_clark_subdivider::implementation
{
public:
	implementation(const k3d::uint_t Levels) :
		m_levels(Levels),
		m_intermediate_points(m_levels),
		m_intermediate_polyhedra(m_levels),
		m_intermediate_point_data(m_levels),
		m_topology_data(m_levels)
	{
	}
	
	void create_mesh(const k3d::mesh::points_t& InputPoints, const k3d::polyhedron::const_primitive& InputPolyhedron, const k3d::mesh::selection_t& InputFaceSelection, k3d::inode* Node)
	{
		for(k3d::uint_t level = 0; level != m_levels; ++level)
		{
			topology_data_t& topology_data = m_topology_data[level];
			const k3d::mesh::points_t& input_points = level == 0 ? InputPoints : m_intermediate_points[level - 1];
			k3d::mesh points_mesh; // temporary mesh so we have the point count in a form that is compatible with the lookup functions in k3d::polyhedron
			points_mesh.points.create().resize(input_points.size());
			boost::scoped_ptr<const k3d::polyhedron::const_primitive> input_polyhedron_ptr(0);
			if (level != 0)
			{
				input_polyhedron_ptr.reset(create_polyhedron_const_primitive(m_intermediate_polyhedra[level - 1]));
			}
			const k3d::polyhedron::const_primitive& input_polyhedron = input_polyhedron_ptr.get() ? *input_polyhedron_ptr : InputPolyhedron;
			k3d::mesh::points_t& output_points = m_intermediate_points[level];
			polyhedron& output_polyhedron = m_intermediate_polyhedra[level];
			const k3d::mesh::selection_t& input_face_selection = level == 0 ? InputFaceSelection : input_polyhedron.face_selections;
			
			// Copy the unaffected constant data
			output_polyhedron.constant_attributes = input_polyhedron.constant_attributes;

			const k3d::uint_t input_edge_count = input_polyhedron.clockwise_edges.size();
			const k3d::uint_t input_face_count = input_polyhedron.face_first_loops.size();

			// store some common arrays
			detail::mesh_arrays mesh_arrays(input_face_selection,
					input_polyhedron.face_first_loops,
					input_polyhedron.face_loop_counts,
					input_polyhedron.loop_first_edges,
					input_polyhedron.clockwise_edges,
					topology_data.edge_faces,
					topology_data.companions);

			// Get the "companion" edge for each edge
			k3d::mesh::bools_t boundary_edges;
			k3d::polyhedron::create_edge_adjacency_lookup(input_polyhedron.vertex_points, input_polyhedron.clockwise_edges, boundary_edges, topology_data.companions);
			std::vector<k3d::mesh::indices_t> point_faces;
			k3d::polyhedron::create_point_face_lookup(points_mesh, input_polyhedron, point_faces);

			// For each edge, get the face it belongs to
			topology_data.edge_faces.resize(input_edge_count);
			k3d::polyhedron::create_edge_face_lookup(input_polyhedron, topology_data.edge_faces);
			// Count the number of components of the new mesh per old face
			topology_data.face_subface_counts.resize(input_face_count);
			k3d::mesh::indices_t face_subloop_counts(input_face_count);
			k3d::mesh::indices_t face_edge_counts(input_face_count);
			k3d::mesh::indices_t face_point_counts(input_face_count);
			detail::per_face_component_counter per_face_component_counter(mesh_arrays,
						input_polyhedron.vertex_points,
						point_faces,
						topology_data.face_subface_counts,
						face_subloop_counts,
						face_edge_counts,
						face_point_counts);
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(0, input_face_count, k3d::parallel::grain_size()),
				detail::worker<detail::per_face_component_counter>(per_face_component_counter));

			// Turn these counts into cumulative sums
			detail::cumulative_sum(topology_data.face_subface_counts);
			detail::cumulative_sum(face_subloop_counts);
			detail::cumulative_sum(face_edge_counts);
			detail::cumulative_sum(face_point_counts);
			// We now have the following relationships between old and new geometry:
			// first new component index = ..._counts[old component index - 1]
			
			topology_data.corner_points.resize(input_points.size(), 0);
			topology_data.edge_midpoints.resize(input_polyhedron.vertex_points.size());
			topology_data.face_centers.resize(input_polyhedron.face_first_loops.size());
			detail::point_index_calculator point_index_calculator(mesh_arrays,					
					input_polyhedron.vertex_points,
					point_faces,
					face_point_counts,
					topology_data.corner_points,
					topology_data.edge_midpoints,
					topology_data.face_centers);
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(0, input_face_count, k3d::parallel::grain_size()),
				detail::worker<detail::point_index_calculator>(point_index_calculator));
					
			// Allocate required memory
			output_points.resize(face_point_counts.back());
			output_polyhedron.vertex_points.resize(face_edge_counts.back(), 0);
			output_polyhedron.clockwise_edges.resize(face_edge_counts.back(), 0);
			output_polyhedron.loop_first_edges.resize(face_subloop_counts.back());
			output_polyhedron.face_first_loops.resize(topology_data.face_subface_counts.back());
			output_polyhedron.face_loop_counts.resize(topology_data.face_subface_counts.back(), 1);
			output_polyhedron.face_selections.resize(topology_data.face_subface_counts.back(), 0.0);
			output_polyhedron.face_materials.resize(topology_data.face_subface_counts.back());
			output_polyhedron.edge_attributes.set_row_count(face_edge_counts.back());
			output_polyhedron.face_attributes.set_row_count(topology_data.face_subface_counts.back());
			output_polyhedron.face_shells.resize(topology_data.face_subface_counts.back());
			
			detail::topology_subdivider topology_subdivider(mesh_arrays,
					input_polyhedron.vertex_points,
					input_polyhedron.face_materials,
					topology_data.face_subface_counts,
					face_subloop_counts,
					face_edge_counts,
					topology_data.corner_points,
					topology_data.edge_midpoints,
					topology_data.face_centers,
					input_polyhedron.face_shells,
					output_polyhedron.vertex_points,
					output_polyhedron.clockwise_edges,
					output_polyhedron.loop_first_edges,
					output_polyhedron.face_first_loops,
					output_polyhedron.face_loop_counts,
					output_polyhedron.face_materials,
					output_polyhedron.face_selections,
					output_polyhedron.face_shells);
			
			// Connect face centers to edge midpoints
			const k3d::uint_t face_start = 0;
			const k3d::uint_t face_end = face_start + input_polyhedron.face_shells.size();
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(face_start, face_end, k3d::parallel::grain_size()),
				detail::worker<detail::topology_subdivider>(topology_subdivider));

			// Set the per-polyhedron arrays
			output_polyhedron.shell_types = input_polyhedron.shell_types;
			
			// Update selection arrays
			output_polyhedron.edge_selections.assign(output_polyhedron.vertex_points.size(), 0.0);
			
			// Calculate vertex valences, needed for corner point updates.
			k3d::polyhedron::create_point_out_edge_lookup(points_mesh, input_polyhedron, topology_data.point_out_edges);

			// Assign a default vertex selection
			output_polyhedron.vertex_selections = input_polyhedron.vertex_selections;
			output_polyhedron.vertex_selections.assign(output_polyhedron.vertex_points.size(), 0.0);
		}
	}
	
	void update_mesh(const k3d::mesh::points_t& InputPoints, const k3d::polyhedron::const_primitive& InputPolyhedron, const k3d::table& InputPointData, const k3d::mesh::selection_t& InputFaceSelection, k3d::inode* Node)
	{
		for(k3d::uint_t level = 0; level != m_levels; ++level)
		{
			topology_data_t& topology_data = m_topology_data[level];
			const k3d::mesh::points_t& input_points = level == 0 ? InputPoints : m_intermediate_points[level - 1];
			boost::scoped_ptr<const k3d::polyhedron::const_primitive> input_polyhedron_ptr(0);
			if (level != 0)
			{
				input_polyhedron_ptr.reset(create_polyhedron_const_primitive(m_intermediate_polyhedra[level - 1]));
			}
			const k3d::polyhedron::const_primitive& input_polyhedron = input_polyhedron_ptr.get() ? *input_polyhedron_ptr : InputPolyhedron;
			const k3d::table& input_point_data = level == 0 ? InputPointData : m_intermediate_point_data[level - 1];
			k3d::mesh::points_t& output_points = m_intermediate_points[level];
			polyhedron& output_polyhedron = m_intermediate_polyhedra[level];
			k3d::table& output_point_data = m_intermediate_point_data[level];
			const k3d::mesh::selection_t& input_face_selection = level == 0 ? InputFaceSelection : input_polyhedron.face_selections;
		
			const k3d::uint_t face_count = input_polyhedron.face_first_loops.size();
			
			// store some common arrays
			detail::mesh_arrays mesh_arrays(input_face_selection,
					input_polyhedron.face_first_loops,
					input_polyhedron.face_loop_counts,
					input_polyhedron.loop_first_edges,
					input_polyhedron.clockwise_edges,
					topology_data.edge_faces,
					topology_data.companions
					);
			
			// Create copiers for the uniform and varying data
			output_polyhedron.face_attributes = input_polyhedron.face_attributes.clone_types();
			output_polyhedron.edge_attributes = input_polyhedron.edge_attributes.clone_types();
			output_polyhedron.vertex_attributes = input_polyhedron.vertex_attributes.clone_types();
			output_point_data = input_point_data.clone_types();
			output_polyhedron.face_attributes.set_row_count(output_polyhedron.face_first_loops.size());
			output_polyhedron.edge_attributes.set_row_count(output_polyhedron.vertex_points.size());
			output_polyhedron.vertex_attributes.set_row_count(output_polyhedron.vertex_points.size());
			output_point_data.set_row_count(output_points.size());
			k3d::table_copier face_attributes_copier(input_polyhedron.face_attributes, output_polyhedron.face_attributes);
			k3d::table_copier edge_attributes_copier(input_polyhedron.edge_attributes, output_polyhedron.edge_attributes);
			k3d::table_copier vertex_attributes_copier(input_polyhedron.vertex_attributes, output_polyhedron.vertex_attributes);
			k3d::table_copier point_data_copier(input_point_data, output_point_data);
			k3d::table_copier point_data_mixer(output_point_data, output_point_data);
	
			output_points.assign(output_points.size(), k3d::point3(0,0,0));

			// Calculate face centers
			detail::face_center_calculator face_center_calculator(
					mesh_arrays,
					input_polyhedron.vertex_points,
					output_polyhedron.face_first_loops,
					output_polyhedron.loop_first_edges,
					output_polyhedron.clockwise_edges,
					topology_data.face_centers,
					topology_data.face_subface_counts,
					input_points,
					output_points,
					face_attributes_copier,
					edge_attributes_copier,
					vertex_attributes_copier,
					point_data_copier);
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(0, face_count, k3d::parallel::grain_size()),
				detail::worker<detail::face_center_calculator>(face_center_calculator));
	
			// Calculate edge midpoints
			detail::edge_midpoint_calculator edge_midpoint_calculator(
					mesh_arrays,
					input_polyhedron.vertex_points,
					output_polyhedron.face_first_loops,
					output_polyhedron.loop_first_edges,
					output_polyhedron.clockwise_edges,
					topology_data.edge_midpoints,
					topology_data.face_centers,
					topology_data.face_subface_counts,
					input_points,
					output_points,
					edge_attributes_copier,
					vertex_attributes_copier,
					point_data_copier,
					point_data_mixer);
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(0, face_count, k3d::parallel::grain_size()),
				detail::worker<detail::edge_midpoint_calculator>(edge_midpoint_calculator));

			// Calculate new point positions
			detail::corner_point_calculator corner_point_calculator(
					mesh_arrays,
					input_polyhedron.vertex_points,
					topology_data.corner_points,
					topology_data.edge_midpoints,
					topology_data.face_centers,
					topology_data.point_out_edges,
					input_points,
					output_points,
					point_data_copier,
					point_data_mixer);
			const k3d::uint_t points_begin = 0;
			const k3d::uint_t points_end = input_polyhedron.vertex_points.size();
			k3d::parallel::parallel_for(
				k3d::parallel::blocked_range<k3d::uint_t>(points_begin, points_end, k3d::parallel::grain_size()),
				detail::corner_worker(input_polyhedron.vertex_points, corner_point_calculator));
		}
	}
	
	void copy_output(k3d::mesh::points_t& Points, k3d::polyhedron::primitive& Polyhedron, k3d::table& PointData)
	{
		const k3d::uint_t point_offset = Points.size();
		const k3d::mesh::points_t& new_points = m_intermediate_points[m_levels - 1];
		const k3d::uint_t new_point_count = new_points.size();
		Points.resize(point_offset + new_point_count);
		std::copy(new_points.begin(), new_points.end(), Points.begin() + point_offset);
		copy_output_polyhedron(m_intermediate_polyhedra[m_levels - 1], Polyhedron, point_offset);
		k3d::table_copier point_copier(m_intermediate_point_data[m_levels - 1], PointData);
		for(k3d::uint_t i = 0; i != new_point_count; ++i)
			point_copier.push_back(i);
	}
	
	void visit_surface(const k3d::uint_t Level, ipatch_surface_visitor& Visitor) const
	{
		k3d::uint_t last_count = 0;
		for(k3d::uint_t face = 0; face != m_topology_data[0].face_subface_counts.size(); ++face)
		{
			const k3d::uint_t face_count = m_topology_data[0].face_subface_counts[face]; 
			if(face_count - last_count > 1)
			{
				Visitor.start_face(face);
				visit_subfacets(Level, 0, face, Visitor);
				Visitor.finish_face(face);
			}
			last_count = face_count;
		}
	}
	
	void visit_boundary(const k3d::polyhedron::const_primitive& Polyhedron, const k3d::uint_t Level, ipatch_boundary_visitor& Visitor) const
	{
		const k3d::uint_t edge_count = m_topology_data[0].edge_midpoints.size();
		const k3d::mesh::indices_t& input_edge_points = Polyhedron.vertex_points;
		const k3d::mesh::indices_t& input_clockwise_edges = Polyhedron.clockwise_edges;
		for(k3d::uint_t edge = 0; edge != edge_count; ++edge)
		{
			Visitor.start_edge(edge);
			k3d::uint_t c0 = input_edge_points[edge];
			k3d::uint_t c1 = input_edge_points[input_clockwise_edges[edge]];
			k3d::uint_t first_edge = edge;
			for(k3d::uint_t level = 0; level != Level - 1; ++level)
			{
				c0 = m_topology_data[level].corner_points[c0];
				c1 = m_topology_data[level].corner_points[c1];
				const k3d::uint_t midpoint = m_topology_data[level].edge_midpoints[first_edge];
				const k3d::mesh::indices_t& point_edges = m_topology_data[level+1].point_out_edges[c0];
				const k3d::uint_t point_edge_begin = 0;
				const k3d::uint_t point_edge_end = point_edges.size();
				const polyhedron& polyhedron_at_level = m_intermediate_polyhedra[level];
				const k3d::mesh::indices_t& edge_points = polyhedron_at_level.vertex_points;
				const k3d::mesh::indices_t& clockwise_edges = polyhedron_at_level.clockwise_edges;
				for(k3d::uint_t point_edge_index = point_edge_begin; point_edge_index != point_edge_end; ++point_edge_index)
				{
					const k3d::uint_t point_edge = point_edges[point_edge_index];
					if(edge_points[clockwise_edges[point_edge]] == midpoint)
					{
						first_edge = point_edge;
						break;
					}
				}
			}
			const polyhedron& polyhedron_at_level = m_intermediate_polyhedra[Level - 2];
			const k3d::mesh::indices_t& edge_points = polyhedron_at_level.vertex_points;
			const k3d::mesh::indices_t& clockwise_edges = polyhedron_at_level.clockwise_edges;
			const k3d::mesh::indices_t& corner_points = m_topology_data[Level-1].corner_points;
			const k3d::mesh::indices_t& edge_midpoints = m_topology_data[Level-1].edge_midpoints;
			const k3d::mesh::indices_t& companions = m_topology_data[Level-1].companions;
			const k3d::mesh::points_t& points = m_intermediate_points[Level-1];
			return_if_fail(edge_points[first_edge] == c0);
			for(k3d::uint_t subedge = first_edge; ;)
			{ 
				Visitor.add_vertex(points[corner_points[edge_points[subedge]]]);
				Visitor.add_vertex(points[edge_midpoints[subedge]]);
				
				if(edge_points[clockwise_edges[subedge]] == c1)
					break;
				subedge = clockwise_edges[companions[clockwise_edges[subedge]]];
			}
			Visitor.finish_edge(edge);
		}
	}
	
	void visit_corners(const k3d::uint_t Level, ipatch_corner_visitor& Visitor) const
	{
		const k3d::uint_t point_count = m_topology_data[0].corner_points.size();
		for(k3d::uint_t point = 0; point != point_count; ++point)
		{
			k3d::uint_t corner = point;
			for(k3d::uint_t level = 0; level != Level; ++level)
			{
				corner = m_topology_data[level].corner_points[corner];
			}
			Visitor.add_vertex(m_intermediate_points[Level-1].at(corner));
		}
	}
	
	const k3d::mesh::points_t& points(const k3d::uint_t Level) const
	{
		return m_intermediate_points[Level - 1];
	}

	const k3d::mesh::normals_t& point_normals(const k3d::uint_t Level) const
	{
		const k3d::table& point_data = m_intermediate_point_data[Level - 1];
		const k3d::mesh::normals_t* normals = point_data.lookup<k3d::mesh::normals_t>("sds_normals");
		if(!normals)
			throw std::runtime_error("sds::catmull_clark_subdivider: mesh did not have normals");
		return *normals;
	}

private:
	/// Used to recurse through levels to associate an original face with its subfaces
	void visit_subfacets(const k3d::uint_t MaxLevel, const k3d::uint_t Level, const k3d::uint_t Face, ipatch_surface_visitor& Visitor) const
	{
		const k3d::uint_t face_begin = Face == 0 ? 0 : m_topology_data[Level].face_subface_counts[Face - 1];
		const k3d::uint_t face_end = m_topology_data[Level].face_subface_counts[Face];
		if(Level < MaxLevel - 1)
		{
			for(k3d::uint_t face = face_begin; face != face_end; ++face)
				visit_subfacets(MaxLevel, Level + 1, face, Visitor);
		}
		else
		{
			k3d::uint_t corners[4];
			const polyhedron& polyhedron_at_level = m_intermediate_polyhedra[Level];
			for(k3d::uint_t face = face_begin; face != face_end; ++face)
			{
				const k3d::uint_t first_edge = polyhedron_at_level.loop_first_edges[polyhedron_at_level.face_first_loops[face]];
				k3d::uint_t corner = 0;
				for(k3d::uint_t edge = first_edge; ;)
				{
					return_if_fail(corner < 4);
					corners[corner++] = polyhedron_at_level.vertex_points[edge];
					
					edge = polyhedron_at_level.clockwise_edges[edge];
					if(edge == first_edge)
						break;
				}
				Visitor.add_quad(corners[0], corners[1], corners[2], corners[3]);
			}
		}
	}
	
	/// Some arrays containing additional topology data
	struct topology_data_t
	{
		k3d::mesh::indices_t corner_points; // New point index, for each original point
		k3d::mesh::indices_t edge_midpoints; // Midpoint index for each edge (if any)
		k3d::mesh::indices_t face_centers; // Face center index, for each face (if any)
		k3d::mesh::indices_t companions; // Companion edges
		k3d::mesh::indices_t edge_faces; // For each original edge, the original owning face
		std::vector<k3d::mesh::indices_t> point_out_edges; // Outgoing edge adjacency list
		k3d::mesh::counts_t face_subface_counts; // Cumulative subface count for each input face (needed to copy uniform and face varying data)
	};
	
	struct polyhedron
	{
		k3d::typed_array<int32_t> shell_types;
		k3d::mesh::indices_t face_shells;
		k3d::mesh::indices_t face_first_loops;
		k3d::mesh::counts_t face_loop_counts;
		k3d::mesh::selection_t face_selections;
		k3d::mesh::materials_t face_materials;
		k3d::mesh::indices_t loop_first_edges;
		k3d::mesh::indices_t clockwise_edges;
		k3d::mesh::selection_t edge_selections;
		k3d::mesh::indices_t vertex_points;
		k3d::mesh::selection_t vertex_selections;
		k3d::mesh::table_t constant_attributes;
		k3d::mesh::table_t face_attributes;
		k3d::mesh::table_t edge_attributes;
		k3d::mesh::table_t vertex_attributes;
	};
	
	k3d::polyhedron::const_primitive* create_polyhedron_const_primitive(const polyhedron& Polyhedron)
	{
		return new k3d::polyhedron::const_primitive(
				Polyhedron.shell_types,
				Polyhedron.face_shells,
				Polyhedron.face_first_loops,
				Polyhedron.face_loop_counts,
				Polyhedron.face_selections,
				Polyhedron.face_materials,
				Polyhedron.loop_first_edges,
				Polyhedron.clockwise_edges,
				Polyhedron.edge_selections,
				Polyhedron.vertex_points,
				Polyhedron.vertex_selections,
				Polyhedron.constant_attributes,
				Polyhedron.face_attributes,
				Polyhedron.edge_attributes,
				Polyhedron.vertex_attributes
				);
	}
	
	template<typename ArrayT>
	void copy_array(const ArrayT& Source, ArrayT& Destination)
	{
		Destination.resize(Source.size());
		std::copy(Source.begin(), Source.end(), Destination.begin());
	}
	
	void copy_output_polyhedron(const polyhedron& Input, k3d::polyhedron::primitive& Output, const k3d::uint_t PointOffset)
	{
		copy_array(Input.shell_types, Output.shell_types);
		copy_array(Input.face_shells, Output.face_shells);
		copy_array(Input.face_first_loops, Output.face_first_loops);
		copy_array(Input.face_loop_counts, Output.face_loop_counts);
		copy_array(Input.face_selections, Output.face_selections);
		copy_array(Input.face_materials, Output.face_materials);
		copy_array(Input.loop_first_edges, Output.loop_first_edges);
		Output.vertex_points.resize(Input.vertex_points.size());
		for(k3d::uint_t i = 0; i != Input.vertex_points.size(); ++i)
			Output.vertex_points[i] = (Input.vertex_points[i] + PointOffset);
		copy_array(Input.clockwise_edges, Output.clockwise_edges);
		copy_array(Input.edge_selections, Output.edge_selections);
		Output.constant_attributes = Input.constant_attributes;
		Output.face_attributes = Input.face_attributes;
		Output.edge_attributes = Input.edge_attributes;
		Output.vertex_attributes = Input.vertex_attributes;
		Output.vertex_selections = Input.vertex_selections;
	}
	
	const k3d::uint_t m_levels; // The number of SDS levels to create
	typedef std::vector<k3d::mesh::points_t> points_t;
	typedef std::vector<polyhedron> polyhedra_t;
	typedef std::vector<k3d::table> arrays_t;
	points_t m_intermediate_points;
	polyhedra_t m_intermediate_polyhedra;
	arrays_t m_intermediate_point_data;
	std::vector<topology_data_t> m_topology_data;
};

catmull_clark_subdivider::catmull_clark_subdivider(const k3d::uint_t Levels)
{
	m_implementation = new implementation(Levels);
}

catmull_clark_subdivider::~catmull_clark_subdivider()
{
	delete m_implementation;
}

void catmull_clark_subdivider::set_levels(const k3d::uint_t Levels)
{
	if(m_implementation)
		delete m_implementation;
	m_implementation = new implementation(Levels);
}

void catmull_clark_subdivider::create_mesh(const k3d::mesh::points_t& InputPoints, const k3d::polyhedron::const_primitive& InputPolyhedron, const k3d::mesh::selection_t& InputFaceSelection, k3d::inode* Node)
{
	m_implementation->create_mesh(InputPoints, InputPolyhedron, InputFaceSelection, Node);
}

void catmull_clark_subdivider::update_mesh(const k3d::mesh::points_t& InputPoints, const k3d::polyhedron::const_primitive& InputPolyhedron, const k3d::table& InputPointData, const k3d::mesh::selection_t& InputFaceSelection, k3d::inode* Node)
{
	m_implementation->update_mesh(InputPoints, InputPolyhedron, InputPointData, InputFaceSelection, Node);
}

void catmull_clark_subdivider::copy_output(k3d::mesh::points_t& Points, k3d::polyhedron::primitive& Polyhedron, k3d::table& PointData)
{
	m_implementation->copy_output(Points, Polyhedron, PointData);
}

void catmull_clark_subdivider::visit_surface(const k3d::uint_t Level, ipatch_surface_visitor& Visitor) const
{
	m_implementation->visit_surface(Level, Visitor);
}

void catmull_clark_subdivider::visit_boundary(const k3d::polyhedron::const_primitive Polyhedron, const k3d::uint_t Level, ipatch_boundary_visitor& Visitor) const
{
	m_implementation->visit_boundary(Polyhedron, Level, Visitor);
}

void catmull_clark_subdivider::visit_corners(const k3d::uint_t Level, ipatch_corner_visitor& Visitor) const
{
	m_implementation->visit_corners(Level, Visitor);
}

const k3d::mesh::points_t& catmull_clark_subdivider::points(const k3d::uint_t Level) const
{
	return m_implementation->points(Level);
}

const k3d::mesh::normals_t& catmull_clark_subdivider::point_normals(const k3d::uint_t Level) const
{
	return m_implementation->point_normals(Level);
}

} // namespace sds

} // namespace k3d

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

#include <k3dsdk/euler_operations.h>
#include <k3dsdk/table_copier.h>

#include <set>

namespace k3d
{

namespace euler
{

namespace detail
{

template<class ArrayT> void cumulative_sum(const ArrayT& InputArray, ArrayT& OutputArray)
{
	return_if_fail(InputArray.size() == OutputArray.size());
	if(InputArray.empty())
		return;
	const uint_t array_begin = 0;
	const uint_t array_end = InputArray.size();
	OutputArray[0] = InputArray[0];
	for(uint_t i = array_begin + 1; i != array_end; ++i)
		OutputArray[i] = OutputArray[i-1] + InputArray[i];
}

/// Sets the loop number associated with each edge in the loop to Loop
void set_edge_loop(const uint_t Loop, const uint_t FirstEdge, const mesh::indices_t& ClockwiseEdges, mesh::indices_t& EdgeLoops)
{
	for(uint_t edge = FirstEdge; ;)
	{
		EdgeLoops[edge] = Loop;
		
		edge = ClockwiseEdges[edge];
		if(edge == FirstEdge)
			break;
	}
}

/// True if the given loop is really a polyline of adjacent edges
bool_t is_polyline(const uint_t FirstEdge, const mesh::indices_t& ClockwiseEdges, const mesh::indices_t& EdgeLoops, const mesh::indices_t& AdjacentEdges)
{
	for(uint_t edge = FirstEdge; ;)
	{
		if(AdjacentEdges[edge] == edge || EdgeLoops[AdjacentEdges[edge]] != EdgeLoops[edge])
			return false;
		
		edge = ClockwiseEdges[edge];
		if(edge == FirstEdge)
			break;
	}
	
	return true;
}

/// Deletes the given loop
void delete_loop(const uint_t Loop, const mesh::indices_t& LoopFaces, const mesh::indices_t& FaceFirstLoops, const mesh::indices_t& LoopFirstEdges, const mesh::indices_t& ClockwiseEdges, mesh::indices_t& FacesToDelete, mesh::indices_t& LoopsToDelete, mesh::indices_t& EdgesToDelete, mesh::indices_t& Companions, mesh::indices_t& OutputFaceLoopCounts)
{
	const uint_t face = LoopFaces[Loop];
	--OutputFaceLoopCounts[face];
	if(Loop == FaceFirstLoops[face])
	{
		FacesToDelete[face] = 1;
		for(uint_t loop = 0; loop != LoopFaces.size(); ++loop)
		{
			if(LoopFaces[loop] == face && !LoopsToDelete[loop])
			{
				LoopsToDelete[loop] = 1;
				--OutputFaceLoopCounts[face];
				const uint_t first_edge2 = LoopFirstEdges[loop];
				for(uint_t edge = first_edge2; ;)
				{
					//return_if_fail(!EdgesToDelete[edge]);
					EdgesToDelete[edge] = 1;
					
					edge = ClockwiseEdges[edge];
					if(edge == first_edge2)
						break;
				}
			}
		}
		LoopsToDelete[Loop] = 1;
	}
}

/// Deletes the given loop if it consists of two edges, as well as the face if tis was the last loop
void delete_loop_if_degenerate(const uint_t Loop, const mesh::indices_t& LoopFaces, const mesh::indices_t& FaceFirstLoops, const mesh::indices_t& LoopFirstEdges, const mesh::indices_t& ClockwiseEdges, mesh::indices_t& FacesToDelete, mesh::indices_t& LoopsToDelete, mesh::indices_t& EdgesToDelete, mesh::indices_t& Companions, mesh::indices_t& OutputFaceLoopCounts)
{
	const uint_t first_edge = LoopFirstEdges[Loop];
	if(first_edge != ClockwiseEdges[ClockwiseEdges[first_edge]]) // not a degenerate loop
		return;
	
	delete_loop(Loop, LoopFaces, FaceFirstLoops, LoopFirstEdges, ClockwiseEdges, FacesToDelete, LoopsToDelete, EdgesToDelete, Companions, OutputFaceLoopCounts);
	const uint_t companion1 = Companions[first_edge];
	const uint_t companion2 = Companions[ClockwiseEdges[first_edge]];
	Companions[companion1] = companion2;
	Companions[companion2] = companion1;
}

/// Update output arrays based on geometry that is marked for deletion
void remove_deleted_geometry(polyhedron::primitive& Output,
		const mesh::indices_t& FaceFirstLoops,
		const mesh::counts_t& FaceLoopCounts,
		const mesh::indices_t& LoopFirstEdges,
		const mesh::indices_t& EdgePoints,
		const mesh::indices_t& ClockwiseEdges,
		const mesh::indices_t& FacesToDelete,
		const mesh::indices_t& LoopsToDelete,
		const mesh::indices_t& EdgesToDelete,
		const mesh::indices_t& LoopFaces,
		const mesh::selection_t& FaceSelection,
		const mesh::indices_t& FaceShells)
{
	const uint_t edge_count = EdgePoints.size();
	const uint_t face_count = FaceFirstLoops.size();
	mesh::counts_t edges_to_delete_sum(edge_count);
	detail::cumulative_sum(EdgesToDelete, edges_to_delete_sum);
	mesh::indices_t loop_map(LoopFirstEdges.size()); // mapping between old and new loop numbers
	mesh::indices_t& output_loop_first_edges = Output.loop_first_edges;
	output_loop_first_edges.clear();
	mesh::indices_t& output_face_first_loops = Output.face_first_loops;
	output_face_first_loops.clear();
	mesh::indices_t& output_face_loop_counts = Output.face_loop_counts;
	output_face_loop_counts.clear();
	mesh::selection_t& output_face_selection = Output.face_selections;
	output_face_selection.clear();
	Output.face_shells.clear();
	mesh::indices_t face_map(face_count);

	const k3d::table input_edge_attributes = Output.edge_attributes;
	const k3d::table input_vertex_attributes = Output.vertex_attributes;
	const k3d::table input_face_attributes = Output.face_attributes;
	Output.edge_attributes = input_edge_attributes.clone_types();
	Output.vertex_attributes = input_vertex_attributes.clone_types();
	Output.face_attributes = input_face_attributes.clone_types();
	k3d::table_copier edge_copier(input_edge_attributes, Output.edge_attributes);
	k3d::table_copier vertex_copier(input_vertex_attributes, Output.vertex_attributes);
	k3d::table_copier face_copier(input_face_attributes, Output.face_attributes);

	for(uint_t loop = 0; loop != LoopFirstEdges.size(); ++loop)
	{
		if(!LoopsToDelete[loop] && !FacesToDelete[LoopFaces[loop]])
		{
			const uint_t face = LoopFaces[loop];
			//return_if_fail(!FacesToDelete[face]);
			if(loop == FaceFirstLoops[face])
			{
				face_map[face] = output_face_first_loops.size();
				output_face_first_loops.push_back(output_loop_first_edges.size());
				output_face_loop_counts.push_back(FaceLoopCounts[face]);
				output_face_selection.push_back(FaceSelection[face]);
				face_copier.push_back(face);
				Output.face_shells.push_back(FaceShells[face]);
				return_if_fail(!EdgesToDelete[LoopFirstEdges[loop]]);
				output_loop_first_edges.push_back(LoopFirstEdges[loop] - edges_to_delete_sum[LoopFirstEdges[loop]]);
				output_loop_first_edges.resize(output_loop_first_edges.size() + FaceLoopCounts[face] - 1);
			}
		}
	}
	mesh::counts_t added_loops(output_face_first_loops.size(), 1);
	for(uint_t loop = 0; loop != LoopFirstEdges.size(); ++loop)
	{
		if(!LoopsToDelete[loop] && !FacesToDelete[LoopFaces[loop]])
		{
			const uint_t face = LoopFaces[loop];
			const uint_t new_face = face_map[face];
			if(loop != FaceFirstLoops[face])
			{
				output_loop_first_edges[output_face_first_loops[new_face] + added_loops[new_face]] = LoopFirstEdges[loop] - edges_to_delete_sum[LoopFirstEdges[loop]];
				++added_loops[new_face];
			}
		}
	}
	mesh::indices_t& output_edge_points = Output.vertex_points;
	output_edge_points.clear();
	mesh::indices_t& output_clockwise_edges = Output.clockwise_edges;
	output_clockwise_edges.clear();
	for(uint_t edge = 0; edge != edge_count; ++edge)
	{
		if(!EdgesToDelete[edge])
		{
			output_edge_points.push_back(EdgePoints[edge]);
			output_clockwise_edges.push_back(ClockwiseEdges[edge] - edges_to_delete_sum[ClockwiseEdges[edge]]);
			edge_copier.push_back(edge);
			vertex_copier.push_back(edge);
		}
	}
	std::set<uint_t> shells_to_erase;
	const uint_t shell_count = Output.shell_types.size();
	for(uint_t shell_idx = 0; shell_idx != shell_count; ++shell_idx)
	{
		if(!std::count(Output.face_shells.begin(), Output.face_shells.end(), shell_idx))
		{
			shells_to_erase.insert(shell_idx);
		}
	}
	for(std::set<uint_t>::const_reverse_iterator it = shells_to_erase.rbegin(); it != shells_to_erase.rend(); ++it)
	{
		const uint_t faces_end = Output.face_shells.size();
		for(uint_t face = 0; face != faces_end; ++face)
		{
			if(Output.face_shells[face] > *it)
				--Output.face_shells[face];
		}
	}
	for(typed_array<int32_t>::iterator shell = Output.shell_types.begin(); shell != Output.shell_types.end();)
	{
		if(shells_to_erase.count(shell - Output.shell_types.begin()))
		{
			shell = Output.shell_types.erase(shell);
		}
		else
		{
			++shell;
		}
	}
}

/// Creates lookup arrays linking edges to their loops, and loops to their faces
void create_edge_loop_face_lookup(const mesh::indices_t& FaceFirstLoops,
		const mesh::counts_t& FaceLoopCounts,
		const mesh::indices_t& LoopFirstEdges,
		const mesh::indices_t& ClockwiseEdges,
		mesh::indices_t& EdgeLoops,
		mesh::indices_t& LoopFaces)
{
	const uint_t face_count = FaceFirstLoops.size();
	EdgeLoops.resize(ClockwiseEdges.size());
	LoopFaces.resize(LoopFirstEdges.size());
	for(uint_t face = 0; face != face_count; ++face) // fill the above arrays
	{
		const uint_t loop_begin = FaceFirstLoops[face];
		const uint_t loop_end = loop_begin + FaceLoopCounts[face];
		for(uint_t loop = loop_begin; loop != loop_end; ++loop)
		{
			LoopFaces[loop] = face;
			const uint_t first_edge = LoopFirstEdges[loop];
			for(uint_t edge = first_edge; ;)
			{
				EdgeLoops[edge] = loop;

				edge = ClockwiseEdges[edge];
				if(edge == first_edge)
					break;
			}
		}
	}
}

} // namespace detail

void kill_edge_make_loop(polyhedron::primitive& Output, const mesh::indices_t& EdgeList, const mesh::bools_t BoundaryEdges, const mesh::indices_t& AdjacentEdges, const mesh::points_t& Points, const mesh::normals_t& FaceNormals)
{

	// Copies, so we can use them as temp storage between the individiual KEML operations
	const mesh::indices_t face_first_loops = Output.face_first_loops;
	mesh::counts_t face_loop_counts = Output.face_loop_counts;
	mesh::indices_t loop_first_edges = Output.loop_first_edges;
	mesh::indices_t clockwise_edges = Output.clockwise_edges;
	const mesh::indices_t edge_points = Output.vertex_points;
	
	const double_t threshold = 1e-1; // Threshold for normal comparison
	
	// Initiate some helper arrays
	const uint_t face_count = face_first_loops.size();
	const uint_t loop_count = loop_first_edges.size();
	const uint_t edge_count = clockwise_edges.size();
	mesh::selection_t face_selection(face_count, 0.0);
	mesh::counts_t faces_to_delete(face_count, 0);
	mesh::counts_t loops_to_delete(loop_count, 0);
	mesh::counts_t edges_to_delete(edge_count, 0);
	mesh::indices_t loop_faces(loop_count); // for each loop, the face it belongs to
	mesh::indices_t edge_loops(edge_count); // for each edge, the loop it belongs to
	detail::create_edge_loop_face_lookup(face_first_loops, face_loop_counts, loop_first_edges, clockwise_edges, edge_loops, loop_faces);
	mesh::indices_t counter_clockwise_edges(edge_count);
	for(uint_t edge = 0; edge != edge_count; ++edge)
	{
		counter_clockwise_edges[clockwise_edges[edge]] = edge;
	}
	
	mesh::selection_t marked_edges(edge_count, 0.0); 
	
	for(uint_t i = 0; i != EdgeList.size(); ++i)
		marked_edges[EdgeList[i]] = 1.0;
	
	
	for(uint_t edge_list_index = 0; edge_list_index != EdgeList.size(); ++edge_list_index)
	{
		const uint_t edge = EdgeList[edge_list_index];
		const uint_t companion = AdjacentEdges[edge];
		if(!marked_edges[edge] && !marked_edges[companion])
			continue;
		if(BoundaryEdges[edge])
			continue;
		if(edges_to_delete[edge])
			continue;
		return_if_fail(!edges_to_delete[companion]);
		
		const uint_t loop = edge_loops[edge];
		const uint_t companion_loop = edge_loops[companion];
		const uint_t face = loop_faces[loop];
		const uint_t companion_face = loop_faces[companion_loop];
		
		// Treat this case when we hit the companion edge
		if(loop == face_first_loops[face] && companion_loop != face_first_loops[companion_face])
		{  
			continue;
		}
		
		return_if_fail(!faces_to_delete[face]);
		return_if_fail(!faces_to_delete[companion_face]);
		return_if_fail(!loops_to_delete[loop]);
		return_if_fail(!loops_to_delete[companion_loop]);
		
		return_if_fail(clockwise_edges[counter_clockwise_edges[edge]] == edge);
		return_if_fail(clockwise_edges[counter_clockwise_edges[companion]] == companion);
		return_if_fail(counter_clockwise_edges[clockwise_edges[edge]] == edge);
		return_if_fail(counter_clockwise_edges[clockwise_edges[companion]] == companion);
		
		edges_to_delete[edge] = 1;
		edges_to_delete[companion] = 1;
		
		face_selection[face] = 1.0;
		
		if(edge == clockwise_edges[clockwise_edges[edge]])
		{
			return_if_fail(loop == companion_loop);
			return_if_fail(loop != face_first_loops[face]);
			return_if_fail(counter_clockwise_edges[counter_clockwise_edges[edge]] == edge);
			loops_to_delete[loop] = 1;
			--face_loop_counts[face];
			continue;
		}
		
		bool_t loop_is_polyline = detail::is_polyline(edge, clockwise_edges, edge_loops, AdjacentEdges);
		bool_t companion_loop_is_polyline = detail::is_polyline(companion, clockwise_edges, edge_loops, AdjacentEdges);
		
		// Reconnect edges
		clockwise_edges[counter_clockwise_edges[edge]] = clockwise_edges[companion];
		clockwise_edges[counter_clockwise_edges[companion]] = clockwise_edges[edge];
		counter_clockwise_edges[clockwise_edges[edge]] = counter_clockwise_edges[companion];
		counter_clockwise_edges[clockwise_edges[companion]] = counter_clockwise_edges[edge];
		
		if(loop == companion_loop)
		{
			return_if_fail(face == companion_face);
			if(clockwise_edges[edge] == companion)
			{
				loop_first_edges[loop] = clockwise_edges[companion];
				continue;
			}
			if(clockwise_edges[companion] == edge)
			{
				loop_first_edges[loop] = clockwise_edges[edge];
				continue;
			}
			if(!detail::is_polyline(clockwise_edges[companion], clockwise_edges, edge_loops, AdjacentEdges)
				&& std::abs(normalize(polyhedron::normal(edge_points, clockwise_edges, Points, clockwise_edges[companion])) * FaceNormals[face] - 1) < threshold)
			{
				loop_first_edges[loop] = clockwise_edges[companion];
				const uint_t new_loop = loop_first_edges.size();
				loop_first_edges.push_back(clockwise_edges[edge]);
				detail::set_edge_loop(new_loop, clockwise_edges[edge], clockwise_edges, edge_loops);
				loop_faces.push_back(face);
			}
			else
			{
				if(loop == face_first_loops[face]) // first loops should not degenerate to a polyline
				{
					return_if_fail(!detail::is_polyline(clockwise_edges[edge], clockwise_edges, edge_loops, AdjacentEdges));
					return_if_fail(std::abs(normalize(polyhedron::normal(edge_points, clockwise_edges, Points, clockwise_edges[edge])) * FaceNormals[face] - 1) < threshold);
				}
				loop_first_edges[loop] = clockwise_edges[edge];
				const uint_t new_loop = loop_first_edges.size();
				loop_first_edges.push_back(clockwise_edges[companion]);
				detail::set_edge_loop(new_loop, clockwise_edges[companion], clockwise_edges, edge_loops);
				loop_faces.push_back(face);
			}
			++face_loop_counts[face];
			loops_to_delete.push_back(0);
		}
		else
		{
			if(companion_loop == face_first_loops[companion_face])
			{
				loop_first_edges[loop] = counter_clockwise_edges[edge];
				detail::set_edge_loop(loop, loop_first_edges[loop], clockwise_edges, edge_loops);
				loops_to_delete[companion_loop] = 1;
				--face_loop_counts[companion_face];
				faces_to_delete[companion_face] = 1;
				for(uint_t l = 0; l != loop_faces.size(); ++l)
				{
					if(loop_faces[l] == companion_face && !loops_to_delete[l])
					{
						loop_faces[l] = face;
						++face_loop_counts[face];
					}
				}
			}
			else
			{
				assert_not_implemented();
			}
		}
	}
	return_if_fail(loops_to_delete.size() == loop_first_edges.size());
	return_if_fail(loop_faces.size() == loop_first_edges.size());
	mesh::indices_t edges_to_delete_sum(edges_to_delete.size());
	detail::cumulative_sum(edges_to_delete, edges_to_delete_sum);
	// Update the output arrays
	const k3d::mesh::indices_t face_shells = Output.face_shells;
	detail::remove_deleted_geometry(Output, face_first_loops, face_loop_counts, loop_first_edges, edge_points, clockwise_edges, faces_to_delete, loops_to_delete, edges_to_delete, loop_faces, face_selection, face_shells);
	Output.face_materials.assign(Output.face_first_loops.size(), static_cast<imaterial*>(0));
	Output.edge_selections.assign(Output.vertex_points.size(), 0.0);
	Output.vertex_selections.assign(Output.vertex_points.size(), 0.0);

}

void kill_edge_and_vertex(polyhedron::primitive& Output, const mesh::indices_t& EdgeList, const mesh::bools_t BoundaryEdges, const mesh::indices_t& AdjacentEdges, const uint_t PointCount)
{
	const mesh::indices_t face_first_loops = Output.face_first_loops;
	mesh::counts_t face_loop_counts = Output.face_loop_counts;
	mesh::indices_t loop_first_edges = Output.loop_first_edges;
	mesh::indices_t clockwise_edges = Output.clockwise_edges;
	mesh::indices_t edge_points = Output.vertex_points;
	// Copy the companions array, since it will change
	mesh::indices_t companions = AdjacentEdges;
	
	// Initiate some helper arrays
	const uint_t face_count = face_first_loops.size();
	const uint_t loop_count = loop_first_edges.size();
	const uint_t edge_count = clockwise_edges.size();
	mesh::selection_t face_selection(face_count, 0.0);
	mesh::counts_t faces_to_delete(face_count, 0);
	mesh::counts_t loops_to_delete(loop_count, 0);
	mesh::counts_t edges_to_delete(edge_count, 0);
	mesh::indices_t loop_faces(loop_count); // for each loop, the face it belongs to
	mesh::indices_t edge_loops(edge_count); // for each edge, the loop it belongs to
	detail::create_edge_loop_face_lookup(face_first_loops, face_loop_counts, loop_first_edges, clockwise_edges, edge_loops, loop_faces);
	mesh::indices_t counter_clockwise_edges(edge_count);
	for(uint_t edge = 0; edge != edge_count; ++edge)
	{
		const uint_t clockwise = clockwise_edges[edge];
		counter_clockwise_edges[clockwise] = edge;
	}
	
	// maps old point indices to the indices that replace them.
	mesh::indices_t point_map(PointCount, 0.0);
	for(uint_t point = 0; point != PointCount; ++point)
		point_map[point] = point;
	
	for(uint_t edge_list_index = 0; edge_list_index != EdgeList.size(); ++edge_list_index)
	{
		const uint_t edge = EdgeList[edge_list_index];
		//const uint_t edge = affected_edges[i];
		const uint_t companion = companions[edge];
		const uint_t loop = edge_loops[edge];
		const uint_t face = loop_faces[loop];
		
		if(!edges_to_delete[companion])
		{
			const uint_t edge_point = edge_points[edge];
			if(edge_point == point_map[edge_point])
				point_map[edge_point] = edge_points[clockwise_edges[edge]];
		}
		
		return_if_fail(clockwise_edges[counter_clockwise_edges[edge]] == edge);

		// Mark edge to delete
		edges_to_delete[edge] = 1;
		
		// Reconnect edges
		if(companion == counter_clockwise_edges[edge]) // first antenna case
		{
			counter_clockwise_edges[clockwise_edges[edge]] = companions[clockwise_edges[edge]];
			loop_first_edges[loop] = companions[clockwise_edges[edge]];
		}
		else if(companion == clockwise_edges[edge]) // second antenna case
		{
			clockwise_edges[counter_clockwise_edges[edge]] = companions[counter_clockwise_edges[edge]];
			loop_first_edges[loop] = companions[counter_clockwise_edges[edge]];
		}
		else // normal edge
		{
			clockwise_edges[counter_clockwise_edges[edge]] = clockwise_edges[edge];
			counter_clockwise_edges[clockwise_edges[edge]] = counter_clockwise_edges[edge];
			return_if_fail(edge_loops[counter_clockwise_edges[edge]] == edge_loops[clockwise_edges[edge]]);
			loop_first_edges[loop] = clockwise_edges[edge];
		}
		// Check if we created a loop that has only 2 edges, and if we did, delete it
		detail::delete_loop_if_degenerate(loop, loop_faces, face_first_loops, loop_first_edges, clockwise_edges, faces_to_delete, loops_to_delete, edges_to_delete, companions, face_loop_counts);
	}
	
	// Set edge points
	for(uint_t edge = 0; edge != edge_count; ++edge)
	{
		if(!edges_to_delete[edge] && edge_points[edge] != edge_points[clockwise_edges[edge]])
		{
			uint_t new_edge_point = edge_points[edge];
			// recursively get the new edge point
			while(new_edge_point != point_map[new_edge_point])
			{
				new_edge_point = point_map[new_edge_point];
				// if we get a loop, leave the point unchanged
				if(new_edge_point != point_map[new_edge_point] && new_edge_point == point_map[point_map[new_edge_point]])
				{
					new_edge_point = edge_points[edge];
					break;
				}
			}
			if(edge_points[edge] != new_edge_point)
			{
				face_selection[loop_faces[edge_loops[edge]]] = 1.0;
			}
			edge_points[edge] = new_edge_point;
		}
	}
	
	// Remove edges that have the same start and end
	for(uint_t edge = 0; edge != edge_count; ++edge)
	{
		if(edge_points[edge] == edge_points[clockwise_edges[edge]])
		{
			edges_to_delete[edge] = 1;
			clockwise_edges[counter_clockwise_edges[edge]] = clockwise_edges[edge];
			counter_clockwise_edges[clockwise_edges[edge]] = counter_clockwise_edges[edge];
			loop_first_edges[edge_loops[edge]] = clockwise_edges[edge];
		}
	}
	
	// Update output arrays
	const k3d::mesh::indices_t face_shells = Output.face_shells;
	detail::remove_deleted_geometry(Output,face_first_loops, face_loop_counts, loop_first_edges, edge_points, clockwise_edges, faces_to_delete, loops_to_delete, edges_to_delete, loop_faces, face_selection, face_shells);
	Output.face_materials.assign(Output.face_first_loops.size(), static_cast<imaterial*>(0));
	Output.edge_selections.assign(Output.clockwise_edges.size(), 0.0);
	Output.vertex_selections.assign(Output.clockwise_edges.size(), 0.0);
}

} // namespace euler

} // namespace k3d


#include "integration.h"

#include <k3dsdk/algebra.h>
#include <k3dsdk/polyhedron.h>

#include <lib3ds/file.h>
#include <lib3ds/camera.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#include <lib3ds/material.h>
#include <lib3ds/matrix.h>
#include <lib3ds/vector.h>
#include <lib3ds/light.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace f3ds
{

namespace io
{

static void add_nodes(k3d::mesh& Mesh, Lib3dsFile* file, Lib3dsNode* node, k3d::uint_t &offset, k3d::imaterial* const Material, k3d::polyhedron::primitive& polyhedron)
{
	if(node->type == LIB3DS_OBJECT_NODE)
	{
		for(Lib3dsNode* p = node->childs; p != 0; p = p->next)
			add_nodes(Mesh, file, p, offset, Material, polyhedron);

		if(strcmp(node->name,"$$$DUMMY") == 0)
			return;
		
		Lib3dsMesh* mesh = lib3ds_file_mesh_by_name(file, node->data.object.morph);
		if(!mesh)
			mesh = lib3ds_file_mesh_by_name(file, node->name);

		if(!mesh)
			return;

		for(int p = 0; p < mesh->faces; ++p)
		{
			Lib3dsFace* const f = &mesh->faceL[p];

			polyhedron.face_shells.push_back(0);
			polyhedron.face_first_loops.push_back(polyhedron.loop_first_edges.size());
			polyhedron.face_loop_counts.push_back(1);
			polyhedron.face_selections.push_back(0);
			polyhedron.face_materials.push_back(Material);
			polyhedron.loop_first_edges.push_back(polyhedron.clockwise_edges.size());

			for(int j = 0; j != 3; ++j)
			{
				polyhedron.clockwise_edges.push_back(polyhedron.clockwise_edges.size() + 1);
				polyhedron.edge_selections.push_back(0);
				polyhedron.vertex_points.push_back(f->points[j] + offset);
				polyhedron.vertex_selections.push_back(0);
			}
			polyhedron.clockwise_edges.back() = polyhedron.loop_first_edges.back();
		}

		k3d::mesh::points_t& points = Mesh.points.writable();
		k3d::mesh::selection_t& point_selection = Mesh.point_selection.writable();
		for(int i = 0; i < mesh->points; ++i)
		{
			points.push_back(k3d::point3(mesh->pointL[i].pos[0], mesh->pointL[i].pos[1], mesh->pointL[i].pos[2]));
			point_selection.push_back(0);
		}

		offset += mesh->points;
	}
}

f3dsParser::f3dsParser(const char* filename, k3d::imaterial* const Material, k3d::mesh& Mesh)
{
	Lib3dsFile* const file = lib3ds_file_load(filename);
	if(!file) 
	{
		k3d::log() << error << "Not a 3DS file: " << filename << std::endl;
		return;
	}

	Mesh.points.create();
	Mesh.point_selection.create();

	boost::scoped_ptr<k3d::polyhedron::primitive> polyhedron(k3d::polyhedron::create(Mesh));
	polyhedron->shell_types.push_back(k3d::polyhedron::POLYGONS);

	/* No nodes?  Fabricate nodes to display all the meshes. */
	if(!file->nodes)
	{
		k3d::log() << warning << "3DS file doesn't contain any nodes, creating virtual nodes instead." << std::endl;
	
		for(Lib3dsMesh* mesh = file->meshes; mesh; mesh = mesh->next)
		{
			Lib3dsNode* const node = lib3ds_node_new_object();
			strcpy(node->name, mesh->name);
			node->parent_id = LIB3DS_NO_PARENT;
			lib3ds_file_insert_node(file, node);
		}
	}

	k3d::uint_t offset = 0;
	for(Lib3dsNode* p = file->nodes; p; p = p->next)
		add_nodes(Mesh, file, p, offset, Material, *polyhedron);
}

} // namespace io

} // namespace f3ds

} // namespace module


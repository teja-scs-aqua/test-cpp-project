#python

import k3d

context.document.start_change_set()
try:
	# Create a FrozenMesh node to act as a mesh source ...
	frozen_mesh = k3d.plugin.create("FrozenMesh", context.document)
	frozen_mesh.name = "Simple Polyhedron"

	# Create a mesh ...
	mesh = frozen_mesh.create_mesh()

	# Add two triangles to the mesh ...
	vertices = [k3d.point3(-3, -3, 0), k3d.point3(3, -3, 0), k3d.point3(3, 3, 0), k3d.point3(-3, 3, 0)]
	vertex_counts = [3, 3]
	vertex_indices = [0, 1, 3, 1, 2, 3]
	material = None
	polyhedron = k3d.polyhedron.create(mesh, vertices, vertex_counts, vertex_indices, material)
	
	# Connect the FrozenMesh to a MeshInstance to place it in the scene ...
	mesh_instance = k3d.plugin.create("MeshInstance", context.document)
	mesh_instance.name = "Simple Polyhedron Instance"
	mesh_instance.gl_painter = k3d.node.lookup_one(context.document, "GL Default Painter")
	mesh_instance.ri_painter = k3d.node.lookup_one(context.document, "RenderMan Default Painter")
	k3d.property.connect(context.document, frozen_mesh.get_property("output_mesh"), mesh_instance.get_property("input_mesh"))

	# Make the MeshInstance visible to render engines ...
	k3d.node.show(context.document, mesh_instance)

	context.document.finish_change_set("Create Simple Polyhedron")

except:
	context.document.cancel_change_set()
	raise


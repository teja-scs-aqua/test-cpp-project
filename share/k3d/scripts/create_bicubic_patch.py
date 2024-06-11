#python

import k3d

context.document.start_change_set()
try:
	# Create a FrozenMesh node to act as a mesh source ...
	frozen_mesh = k3d.plugin.create("FrozenMesh", context.document)
	frozen_mesh.name = "Bicubic Patch"

	# Create a mesh ...
	mesh = frozen_mesh.create_mesh()

	# Add geometric points to the mesh ...
	points = mesh.create_points()
	point_selection = mesh.create_point_selection()

	positions = [
		(-5, -5, 0), (-2, -5, 2), (2, -5, -2), (5, -5, 0),
		(-5, -2, 2), (-2, -2, 5), (2, -2, -5), (5, -2, -2),
		(-5, 2, 2), (-2, 2, 5), (2, 2, -5), (5, 2, -2),
		(-5, 5, 0), (-2, 5, 2), (2, 5, -2), (5, 5, 0)
		]

	for position in positions:
		points.append(k3d.point3(position[0], position[2], -position[1]))
		point_selection.append(0)

	# Create a bicubic_patch primitive ...
	patches = k3d.bicubic_patch.create(mesh)

	# Create a custom attribute array to store color values at each parametric corner of each patch ...
	Cs = patches.parameter_attributes().create("Cs", "k3d::color")

	# Add a single patch to the primitive ...
	patches.patch_selections().append(0)
	patches.patch_materials().append(None)
	for i in range(16):
		patches.patch_points().append(len(patches.patch_points()))

	# Add some colors to the patch corners ...
	Cs.append(k3d.color(1, 0, 0))
	Cs.append(k3d.color(0, 1, 0))
	Cs.append(k3d.color(0, 0, 1))
	Cs.append(k3d.color(1, 1, 1))

	# Connect the FrozenMesh to a MeshInstance to place it in the scene ...
	mesh_instance = k3d.plugin.create("MeshInstance", context.document)
	mesh_instance.name = "Bicubic Patch Instance"
	mesh_instance.gl_painter = k3d.node.lookup_one(context.document, "GL Default Painter")
	mesh_instance.ri_painter = k3d.node.lookup_one(context.document, "RenderMan Default Painter")

	k3d.property.connect(context.document, frozen_mesh.get_property("output_mesh"), mesh_instance.get_property("input_mesh"))

	# Make the MeshInstance visible to render engines ...
	k3d.node.show(context.document, mesh_instance)

	context.document.finish_change_set("Create Bicubic Patch")

except:
	context.document.cancel_change_set()
	raise


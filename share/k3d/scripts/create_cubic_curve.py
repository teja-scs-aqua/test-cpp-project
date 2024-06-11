#python

import k3d

context.document.start_change_set()
try:
	# Create a FrozenMesh node to act as a mesh source ...
	frozen_mesh = k3d.plugin.create("FrozenMesh", context.document)
	frozen_mesh.name = "Cubic Curve"

	# Create a mesh ...
	mesh = frozen_mesh.create_mesh()

	# Add geometric points to the mesh ...
	points = mesh.create_points()
	point_selection = mesh.create_point_selection()

	positions = [k3d.point3(-5, 0, -5), k3d.point3(5, 0, -5), k3d.point3(-5, 0, 5), k3d.point3(5, 0, 5)]
	for position in positions:
		points.append(position)
		point_selection.append(0.0)

	# Create a cubic curve primitive ...
	curves = k3d.cubic_curve.create(mesh)

	# Create a custom attribute array to control the width of the curve ...
	width = curves.constant_attributes().create("constantwidth", "k3d::double_t")

	# Create a custom attribute array to store color values at each curve vertex ...
	Cs = curves.vertex_attributes().create("Cs", "k3d::color")

	# Add a single curve to the primitive ...
	curves.periodic().append(False)
	curves.material().append(None)
	curves.curve_first_points().append(len(curves.curve_points()))
	curves.curve_point_counts().append(len(positions))
	curves.curve_selections().append(0.0)
	for i in range(len(positions)):
		curves.curve_points().append(i)

	width.append(0.2)

	# Assign some colors to the curve vertices ...
	colors = [k3d.color(1, 0, 0), k3d.color(0, 1, 0), k3d.color(0, 0, 1), k3d.color(1, 1, 1)]
	for color in colors:
		Cs.append(color)

	# Connect the FrozenMesh to a MeshInstance to place it in the scene ...
	mesh_instance = k3d.plugin.create("MeshInstance", context.document)
	mesh_instance.name = "Cubic Curve Instance"
	mesh_instance.gl_painter = k3d.node.lookup_one(context.document, "GL Default Painter")
	mesh_instance.ri_painter = k3d.node.lookup_one(context.document, "RenderMan Default Painter")

	k3d.property.connect(context.document, frozen_mesh.get_property("output_mesh"), mesh_instance.get_property("input_mesh"))

	# Make the MeshInstance visible to render engines ...
	k3d.node.show(context.document, mesh_instance)

	context.document.finish_change_set("Create Cubic Curve")

except:
	context.document.cancel_change_set()
	raise


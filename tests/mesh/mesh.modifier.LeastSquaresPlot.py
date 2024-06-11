#python

import k3d
import testing

setup = testing.setup_mesh_modifier_test("FrozenMesh", "LeastSquaresPlot")

mesh = setup.source.create_mesh()

positions = [(-5, 1, 3), (4, 2, 4), (3, 3, -5), (-4, 4, -3)]

points = mesh.create_points()
point_selection = mesh.create_point_selection()

for position in positions:
	points.append(k3d.point3(position[0], position[1], position[2]))
	point_selection.append(0.0)

k3d.property.create(setup.modifier, "k3d::string_t","function_1", "Function 1", "Function 1")
k3d.property.create(setup.modifier, "k3d::string_t","function_2", "Function 2", "Function 2")
k3d.property.create(setup.modifier, "k3d::string_t","function_3", "Function 3", "Function 3")

setup.modifier.function_1 = "u^2"
setup.modifier.function_2 = "sin(v)"
setup.modifier.function_3 = "u*v"

testing.require_valid_mesh(setup.document, setup.modifier.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.modifier.get_property("output_mesh"), "mesh.source.LeastSquaresPlot", 4096)


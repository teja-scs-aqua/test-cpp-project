#python

import k3d
import testing

setup = testing.setup_mesh_modifier_test("PolyGrid", "GrowSelection")

selection = k3d.geometry.selection.create(0)

point_selection = k3d.geometry.point_selection.create(selection)
k3d.geometry.point_selection.append(point_selection, 15, 16, 1)
k3d.geometry.point_selection.append(point_selection, 18, 19, 1)

edge_selection = k3d.geometry.primitive_selection.create(selection, k3d.selection.type.EDGE)
k3d.geometry.primitive_selection.append(edge_selection, 17, 18, 1)
k3d.geometry.primitive_selection.append(edge_selection, 55, 56, 1)

face_selection = k3d.geometry.primitive_selection.create(selection, k3d.selection.type.FACE)
k3d.geometry.primitive_selection.append(face_selection, 12, 13, 1)

setup.modifier.mesh_selection = selection

testing.require_valid_mesh(setup.document, setup.modifier.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.modifier.get_property("output_mesh"), "mesh.selection.GrowSelection", 2)


#python

import testing
import k3d
from math import pi

document = k3d.new_document()

curve1 = k3d.plugin.create("NurbsCurve", document)
curve2 = k3d.plugin.create("NurbsCircle", document)
transform = k3d.plugin.create("RotatePoints", document)
merge_mesh = k3d.plugin.create("MergeMesh", document)
modifier = k3d.plugin.create("NurbsCurveTraversal", document)

k3d.property.create(merge_mesh, "k3d::mesh*", "input_mesh1", "Input Mesh 1", "")
k3d.property.create(merge_mesh, "k3d::mesh*", "input_mesh2", "Input Mesh 2", "")

transform_selection = k3d.geometry.selection.create(0)
k3d.geometry.point_selection.create(transform_selection, 1)
transform.mesh_selection = transform_selection
transform.y = 0.5*pi

modifier.mesh_selection = k3d.geometry.selection.create(1)

k3d.property.connect(document, curve2.get_property("output_mesh"), transform.get_property("input_mesh"))
k3d.property.connect(document, curve1.get_property("output_mesh"), merge_mesh.get_property("input_mesh1"))
k3d.property.connect(document, transform.get_property("output_mesh"), merge_mesh.get_property("input_mesh2"))
k3d.property.connect(document, merge_mesh.get_property("output_mesh"), modifier.get_property("input_mesh"))


testing.require_valid_mesh(document, modifier.get_property("output_mesh"))
testing.require_similar_mesh(document, modifier.get_property("output_mesh"), "mesh.modifier.NurbsCurveTraversal", 1)

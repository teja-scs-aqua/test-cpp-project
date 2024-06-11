#python

import k3d
import testing

setup = testing.setup_mesh_reader_test("PLYMeshReader", "mesh.modifier.PGPRemesh.torus.ply")

modifier = setup.k3d.plugin.create("PGPRemesh", document)
modifier.use_smooth = True
modifier.steps = 15
modifier.h = 1000
modifier.omega = 10
modifier.div = 4
modifier.triangulate = True
k3d.property.connect(document, setup.source.get_property(", modifier.get_property("input_mesh")output_mesh"))

#print "source output: " + repr(source.output_mesh)
#print "triangles output: " + repr(triangles.output_mesh)
#print "modifier output: " + repr(modifier.output_mesh)

testing.require_valid_mesh(setup.document, modifier.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, modifier.get_property("output_mesh"), "mesh.modifier.PGPRemesh.torus", 1)


#python

import testing

setup = testing.setup_mesh_reader_test("STLMeshReader", "mesh.source.STLMeshReader.stl")

testing.require_valid_mesh(setup.document, setup.source.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.source.get_property("output_mesh"), "mesh.source.STLMeshReader", 1)


#python

import testing

setup = testing.setup_mesh_reader_test("MD2MeshReader", "mesh.source.MD2MeshReader.md2")

testing.require_valid_mesh(setup.document, setup.source.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.source.get_property("output_mesh"), "mesh.source.MD2MeshReader", 1)


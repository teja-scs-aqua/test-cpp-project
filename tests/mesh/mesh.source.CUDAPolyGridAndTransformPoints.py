#python

import k3d
import testing

setup = testing.setup_mesh_source_test("CUDAPolyGridAndTransformPoints")

setup.source.input_matrix = k3d.translate3(k3d.vector3(0, 0, 1))


testing.require_valid_mesh(setup.document, setup.source.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.source.get_property("output_mesh"), "mesh.source.PolyGridAndTransformPoints", 1)

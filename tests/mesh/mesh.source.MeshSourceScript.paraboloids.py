#python

import k3d
import testing

script_path = k3d.share_path() / k3d.filesystem.generic_path("scripts/MeshSourceScript/paraboloids.py")
script_file = file(str(script_path), "r")

setup = testing.setup_mesh_source_test("MeshSourceScript")
setup.source.script = script_file.read()


testing.require_valid_mesh(setup.document, setup.source.get_property("output_mesh"))
testing.require_similar_mesh(setup.document, setup.source.get_property("output_mesh"), "mesh.source.MeshSourceScript.paraboloids", 2)


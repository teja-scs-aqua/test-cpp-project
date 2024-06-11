#python

import k3d

def test_equal(a, b, test):
	result = k3d.difference.accumulator()
	k3d.difference.test(a, b, result)
	if result.exact_min() != True or result.ulps_max() > 0:
		print repr(a)
		print repr(b)
		raise Exception(test + " should test equal")

def test_unequal(a, b, test):
	result = k3d.difference.accumulator()
	k3d.difference.test(a, b, result)
	if result.exact_min() == True and result.ulps_max() == 0:
		print repr(a)
		print repr(b)
		raise Exception(test + " should test unequal")

document = k3d.new_document()

source_a = k3d.plugin.create("FrozenMesh", document)
source_b = k3d.plugin.create("FrozenMesh", document)

mesh_a = source_a.create_mesh()
mesh_b = source_b.create_mesh()

test_equal(mesh_a, mesh_b, "empty meshes")

points_a = mesh_a.create_points()
test_unequal(mesh_a, mesh_b, "mesh without point array")

points_b = mesh_b.create_points()
test_equal(mesh_a, mesh_b, "meshes with empty point arrays")

points_a.append(k3d.point3(1, 2, 3))
test_unequal(mesh_a, mesh_b, "mismatched point arrays")

points_b.append(k3d.point3(1, 2, 3))
test_equal(mesh_a, mesh_b, "matched point arrays")

primitive_a = mesh_a.primitives().create("foo")
test_unequal(mesh_a, mesh_b, "mesh without primitive")

primitive_b = mesh_b.primitives().create("foo")
test_equal(mesh_a, mesh_b, "matched primitives")

structure_a = primitive_a.structure().create("uniform")
test_unequal(mesh_a, mesh_b, "primitive with missing structure")

structure_b = primitive_b.structure().create("uniform")
test_equal(mesh_a, mesh_b, "primitive with matched structures")

array_a = structure_a.create("indices", "k3d::uint_t")
test_unequal(mesh_a, mesh_b, "structure with missing array")

array_b = structure_b.create("indices", "k3d::uint_t")
test_equal(mesh_a, mesh_b, "structure with matched arrays")

array_a.append(5)
test_unequal(mesh_a, mesh_b, "mismatched structure arrays")

array_b.append(5)
test_equal(mesh_a, mesh_b, "matched structure_arrays")

attributes_a = primitive_a.attributes().create("uniform")
test_unequal(mesh_a, mesh_b, "primitive with missing attributes")

attributes_b = primitive_b.attributes().create("uniform")
test_equal(mesh_a, mesh_b, "matched primitive attributes")

attribute_a = attributes_a.create("Cs", "k3d::color")
test_unequal(mesh_a, mesh_b, "missing attribute arrays")

attribute_b = attributes_b.create("Cs", "k3d::color")
test_equal(mesh_a, mesh_b, "matched attribute arrays")

attribute_a.append(k3d.color(1, 2, 3))
test_unequal(mesh_a, mesh_b, "mismatched attributes")

attribute_b.append(k3d.color(1, 2, 3))
test_equal(mesh_a, mesh_b, "matched attributes")



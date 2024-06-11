#python

# At one point on OSX, I was accidentally linking our Python code against the system
# Python (2.3), while boost.python was linked against a later version of Python (2.5)
# provided by MacPorts.  This didn't cause link errors, and the 'k3d' module could be
# loaded without error at runtime, but the module itself was empty.
#
# This test checks the contents of the 'k3d' module to ensure that it isn't empty.
# You will have to modify this script whenever new symbols are added-to or removed-from
# the 'k3d' module.

import k3d

expected_contents = ['angle_axis', 'atk', 'table', 'batch_mode', 'bezier_triangle_patch', 'bicubic_patch', 'bilinear_patch', 'bitmap', 'blobby', 'bounding_box3', 'check_node_environment', 'close_document', 'color', 'cone', 'const_mesh', 'const_table', 'const_bitmap', 'const_named_arrays', 'const_named_tables', 'const_typed_array_bool_t', 'const_typed_array_color', 'const_typed_array_double_t', 'const_typed_array_imaterial', 'const_typed_array_inode', 'const_typed_array_int16_t', 'const_typed_array_int32_t', 'const_typed_array_int64_t', 'const_typed_array_int8_t', 'const_typed_array_matrix4', 'const_typed_array_normal3', 'const_typed_array_point2', 'const_typed_array_point3', 'const_typed_array_point4', 'const_typed_array_string_t', 'const_typed_array_texture3', 'const_typed_array_uint16_t', 'const_typed_array_uint32_t', 'const_typed_array_uint64_t', 'const_typed_array_uint8_t', 'const_typed_array_vector2', 'const_typed_array_vector3', 'const_uint_t_array', 'create_plugin', 'cubic_curve', 'cylinder', 'difference', 'disk', 'documents', 'euler', 'euler_angles', 'exit', 'file_change_receiver', 'file_signal', 'filesystem', 'geometry', 'get_time', 'hyperboloid', 'identity3', 'idocument', 'intersect_lines', 'inverse', 'iunknown', 'length', 'linear_curve', 'log', 'log_critical', 'log_debug', 'log_error', 'log_info', 'log_warning', 'matrix4', 'mesh', 'mime', 'named_arrays', 'named_tables', 'new_document', 'node', 'normal3', 'nurbs_curve', 'nurbs_patch', 'open_document', 'paraboloid', 'parallel', 'plugin', 'plugins', 'particle', 'point2', 'point3', 'point4', 'polyhedron', 'property', 'resource', 'ri', 'rotate3', 'scale3', 'script', 'selection', 'share_path', 'sphere', 'teapot', 'texture3', 'torus', 'to_vector3', 'translate3', 'typed_array_bool_t', 'typed_array_color', 'typed_array_double_t', 'typed_array_imaterial', 'typed_array_inode', 'typed_array_int16_t', 'typed_array_int32_t', 'typed_array_int64_t', 'typed_array_int8_t', 'typed_array_matrix4', 'typed_array_normal3', 'typed_array_point2', 'typed_array_point3', 'typed_array_point4', 'typed_array_string_t', 'typed_array_texture3', 'typed_array_uint16_t', 'typed_array_uint32_t', 'typed_array_uint64_t', 'typed_array_uint8_t', 'typed_array_vector2', 'typed_array_vector3', 'ui', 'uint_t_array', 'uuid', 'vector2', 'vector3', 'vector4', 'world_position']

actual_contents = dir(k3d)

error = False
for expected in expected_contents:
	if expected not in actual_contents:
		error = True
		print "Missing symbol", expected

for actual in actual_contents:
	if actual[0:2] == "__":
		continue

	if actual not in expected_contents:
		error = True
		print "Unexpected symbol", actual

if error:
	raise Exception("Python module 'k3d' contents changed unexpectedly")


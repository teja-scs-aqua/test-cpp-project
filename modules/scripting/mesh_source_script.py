#python

import k3d

positions = [(-5, -5, 0), (5, -5, 0), (5, 5, 0), (-5, 5, 0)]

points = context.output.create_points()
point_selection = context.output.create_point_selection()
for position in positions:
	points.append(k3d.point3(position[0], position[1], position[2]))
	point_selection.append(0.0)

polyhedron = k3d.polyhedron.create(context.output)
polyhedron.shell_types().append(k3d.polyhedron.shell_type.POLYGONS)
polyhedron.face_shells().append(0)
polyhedron.face_first_loops().append(0)
polyhedron.face_loop_counts().append(1)
polyhedron.face_materials().append(None)
polyhedron.face_selections().append(0.0)
polyhedron.loop_first_edges().append(0)
polyhedron.clockwise_edges().assign([1, 2, 3, 0])
polyhedron.edge_selections().assign([0, 0, 0, 0])
polyhedron.vertex_points().assign([0, 1, 2, 3])
polyhedron.vertex_selections().assign([0, 0, 0, 0])


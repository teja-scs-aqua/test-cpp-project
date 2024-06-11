#python

import k3d
k3d.check_node_environment(context, "MeshSourceScript")

# Perform required one-time setup to store geometric points in the mesh ...
points = context.output.create_points()
point_selection = context.output.create_point_selection()

# Create two polyhedra ...
for i in range(2):
	polyhedron = k3d.polyhedron.create(context.output)

	# Create an (optional) array to store per-face colors ...
	Cs = polyhedron.face_attributes().create("Cs", "k3d::color")

	polyhedron.shell_types().append(k3d.polyhedron.shell_type.POLYGONS)

	# Create three faces in each polyhedron ...
	for j in range(3):
		# Each face has a single loop (its exterior boundary) ...
		polyhedron.face_shells().append(0)
		polyhedron.face_first_loops().append(len(polyhedron.loop_first_edges()))
		polyhedron.face_loop_counts().append(1)
		polyhedron.face_materials().append(None)
		polyhedron.face_selections().append(0.0)

		Cs.append(k3d.color(1, j / 2.0, i / 1.0))

		polyhedron.loop_first_edges().append(len(polyhedron.clockwise_edges()))

		# Each loop has four edges, each of which points to the next edge in clockwise-order ...
		polyhedron.clockwise_edges().append(len(polyhedron.clockwise_edges()) + 1)
		polyhedron.clockwise_edges().append(len(polyhedron.clockwise_edges()) + 1)
		polyhedron.clockwise_edges().append(len(polyhedron.clockwise_edges()) + 1)
		polyhedron.clockwise_edges().append(len(polyhedron.clockwise_edges()) - 3)

		polyhedron.edge_selections().append(0.0)
		polyhedron.edge_selections().append(0.0)
		polyhedron.edge_selections().append(0.0)
		polyhedron.edge_selections().append(0.0)

		# Each edge has a vertex that references a mesh point ...
		polyhedron.vertex_points().append(len(points) + 0)
		polyhedron.vertex_points().append(len(points) + 1)
		polyhedron.vertex_points().append(len(points) + 2)
		polyhedron.vertex_points().append(len(points) + 3)

		polyhedron.vertex_selections().append(0.0)
		polyhedron.vertex_selections().append(0.0)
		polyhedron.vertex_selections().append(0.0)
		polyhedron.vertex_selections().append(0.0)

		positions = [(-5, 0, 5), (5, 0, 5), (5, 0, -5), (-5, 0, -5)]

		for position in positions:
			points.append(k3d.point3(position[0] + (j * 11.0), position[1] + (i * -11.0), position[2]))
			point_selection.append(0.0)


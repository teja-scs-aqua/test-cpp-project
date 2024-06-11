#python

import k3d
k3d.check_node_environment(context, "MeshSourceScript")

# Perform required one-time setup to store geometric points in the mesh ...
points = context.output.create_points()
point_selection = context.output.create_point_selection()

# Construct a point group mesh primitive ...
particles = k3d.particle.create(context.output)

# Create an (optional) array to store per-group point widths
constantwidth = particles.constant_attributes().create("constantwidth", "k3d::double_t")

# Create an (optional) array to store per-point point colors
Cs = particles.vertex_attributes().create("Cs", "k3d::color")

# Add some points ...
particles.material().append(None)
constantwidth.append(0.5)

for x in range(-5, 6):
	for z in range (-5, 6):
		particles.points().append(len(points))

		points.append(k3d.point3(x, 0, z))
		point_selection.append(0.0)
		Cs.append(k3d.color((x / 10.0) + 0.5, 1, (z / 10.0) + 0.5))


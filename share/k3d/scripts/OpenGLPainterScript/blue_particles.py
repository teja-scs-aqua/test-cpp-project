#python

import k3d
k3d.check_node_environment(context, "OpenGLPainterScript")

from OpenGL.GL import *

points = context.mesh.points()
if points:
	glPushAttrib(GL_ALL_ATTRIB_BITS)
	glDisable(GL_LIGHTING)

	glEnable(GL_POINT_SMOOTH)
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST)

	glEnable(GL_BLEND)
	glBlendFunc(GL_ONE, GL_ONE)

	glPointSize(3)
	glColor3d(1, 1, 1)

	glBegin(GL_POINTS)
	for point in points:
		glVertex3d(point[0], point[1], point[2])
	glEnd()

	glPointSize(7)
	glColor3d(0.05, 0.05, 1)

	glBegin(GL_POINTS)
	for point in points:
		glVertex3d(point[0], point[1], point[2])
	glEnd()

	glPopAttrib()

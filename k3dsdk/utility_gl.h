#ifndef K3DSDK_UTILITY_GL_H
#define K3DSDK_UTILITY_GL_H

// K-3D
// Copyright (c) 1995-2006, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/algebra.h>
#include <k3dsdk/bitmap.h>
#include <k3dsdk/color.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/irender_viewport_gl.h>

namespace k3d
{

class bounding_box3;
class icamera;
class idocument;
class plane;

namespace gl
{

/// Converts an OpenGL matrix into a standard K-3D matrix
inline const matrix4 matrix(GLdouble* GLMatrix)
{
	matrix4 result;
	memcpy(&result[0][0], &GLMatrix[0], 16 * sizeof(GLdouble));
	return transpose(result);
}

/// Converts an OpenGL matrix into a standard K-3D matrix
inline const matrix4 matrix(const GLfloat GLMatrix[16])
{
	matrix4 result;
	for(uint_t i = 0; i != 4; ++i)
	{
		for(uint_t j = 0; j != 4; ++j)
		{
			result[i][j] = static_cast<double_t>(GLMatrix[j*4+i]);
		}
	}
	return result;
}

/// Pushes a matrix onto the OpenGL matrix stack
inline void push_matrix(const matrix4& Matrix)
{
	double glmatrix[16];
	transpose(Matrix).CopyArray(glmatrix);
	glMultMatrixd(glmatrix);
}

/// Passes a k3d::point4 to glVertex4d()
inline void vertex4d(const point4& Vertex)
{
	glVertex4d(Vertex.n[0], Vertex.n[1], Vertex.n[2], Vertex.n[3]);
}

/// Passes a k3d::point3 to glVertex3d()
inline void vertex3d(const point3& Vertex)
{
	glVertex3d(Vertex.n[0], Vertex.n[1], Vertex.n[2]);
}

/// Passes a k3d::vector3 to glNormal3d()
inline void normal3d(const vector3& Vector)
{
	glNormal3d(Vector.n[0], Vector.n[1], Vector.n[2]);
}

/// Passes a k3d::normal3 to glNormal3d()
inline void normal3d(const normal3& Vector)
{
	glNormal3d(Vector.n[0], Vector.n[1], Vector.n[2]);
}

/// Passes a k3d::color to glColor3d()
inline void color3d(const color& Color)
{
	glColor3d(Color.red, Color.green, Color.blue);
}

/// Passes a k3d::color to glMaterialfv()
inline void material(GLenum Face, GLenum PName, const color& Color, double Alpha = 1.0)
{
	GLfloat color[] = { static_cast<GLfloat>(Color.red), static_cast<GLfloat>(Color.green), static_cast<GLfloat>(Color.blue), static_cast<GLfloat>(Alpha) };
	glMaterialfv(Face, PName, color);
}

/// Sets an OpenGL state flag
inline void set(GLenum Flag, const bool State)
{
	if(State)
		glEnable(Flag);
	else
		glDisable(Flag);
}

/// Provides exception- and return-safe RAII behavior for saving / restoring OpenGL attributes
struct store_attributes
{
	store_attributes(const GLbitfield Mask = GL_ALL_ATTRIB_BITS)
	{
		glPushAttrib(Mask);
	}

	~store_attributes()
	{
		glPopAttrib();
	}
};

/// Convenience function for refreshing all OpenGL render engines
void redraw_all(idocument& Document, const irender_viewport::redraw_type_t RedrawType);
/// Draws a 1x1 plane
void draw(const plane& Plane);
/// Draws a box
void draw(const bounding_box3& Box);
/// Draws a standard bounding box to make it easier to visualize an object
void draw_bounding_box(const bounding_box3& Box);
/// Passes a k3d::bitmap to glTexImage2D(), handling non-power-of-two sizes and translations between image formats
void tex_image_2d(const bitmap& Bitmap);

/// Projects a point in world coordinates into screen space, returning the 2D widget coordinates and Z-buffer depth
const point3 project(const point3& WorldCoords);

/// Calculates the settings for a projection for the given camera
void calculate_projection(k3d::icamera& Camera, const uint_t PixelWidth, const uint_t PixelHeight, k3d::rectangle& WindowRect, k3d::rectangle& CameraRect, double& Near, double& Far, bool& Orthographic);

} // namespace gl

} // namespace k3d

#endif // !K3DSDK_UTILITY_GL_H


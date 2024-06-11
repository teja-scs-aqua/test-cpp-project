#ifndef K3DSDK_RENDER_STATE_GL_H
#define K3DSDK_RENDER_STATE_GL_H

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
#include <k3dsdk/gl.h>
#include <k3dsdk/point3.h>
#include <k3dsdk/types.h>

namespace k3d
{

class icamera;

namespace gl
{

/// Used to pass (potentially) useful rendering state from the engine to the object being rendered
class render_state
{
public:
	render_state(icamera& Camera) :
		camera(Camera),
		orthographic(false),
		draw_two_sided(true)
	{
	}

	/// Stores the camera used for drawing
	icamera& camera;

	/// Set to true iff the OpenGL viewing frustum used for drawing is orthographic
	bool_t orthographic;
	
	bool_t draw_two_sided;
	
	//@{
	/** Stores the OpenGL viewing frustum used for drawing (perspective or orthographic).
	 * Note: these are the actual values passed to glFrustum() or glOrtho() to fill the
	 * render window, in general they will be different from the viewing frustum defined
	 * by the camera for rendering. */
	double_t gl_window_frustum_left;
	double_t gl_window_frustum_right;
	double_t gl_window_frustum_top;
	double_t gl_window_frustum_bottom;
	double_t gl_window_frustum_near;
	double_t gl_window_frustum_far;
	//@}
	
	//@{
	/** Stores an imaginary OpenGL viewing frustum that represents the viewing frustum
	 * defined by the camera for rendering.  Note that the values may not be the same
	 * as those returned by the camera object.  */
	double_t gl_camera_frustum_left;
	double_t gl_camera_frustum_right;
	double_t gl_camera_frustum_top;
	double_t gl_camera_frustum_bottom;
	double_t gl_camera_frustum_near;
	double_t gl_camera_frustum_far;
	//@}

	/// Stores the current OpenGL projection matrix
	GLfloat gl_projection_matrix[16];
	
	/// Stores the current OpenGL viewport
	GLint gl_viewport[4];
	
	/// Stores the selection state of the calling node
	double_t node_selection;
	/// Stores the selection state of the parent of the calling node
	double_t parent_selection;
};

} // namespace gl

} // namespace k3d

#endif // !K3DSDK_RENDER_STATE_GL_H


#ifndef K3DSDK_IRENDERABLE_GL_H
#define K3DSDK_IRENDERABLE_GL_H

// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
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

#include <k3dsdk/iunknown.h>
#include <k3dsdk/types.h>

namespace k3d
{

namespace gl
{

class render_state;
class selection_state;
	
/// Abstract interface implemented by objects that can render themselves using the OpenGL API
class irenderable :
	public virtual iunknown
{
public:
	/// Returns the layer on which this renderable should be drawn.  Lower-numbered layers are rendered earlier than higher-numbered layers.
	/// The (arbitrary) default layer for most 3D objects is 1024.  Layer zero is reserved for the viewport background.
	virtual uint_t gl_layer() = 0;
	virtual void gl_draw(const render_state& State) = 0;
	virtual void gl_select(const render_state& State, const selection_state& SelectState) = 0;

protected:
	irenderable() {}
	irenderable(const irenderable&) {}
	irenderable& operator=(const irenderable&) { return *this; }
	virtual ~irenderable() {}
};
} // namespace gl

} // namespace k3d

#endif // !K3DSDK_IRENDERABLE_GL_H


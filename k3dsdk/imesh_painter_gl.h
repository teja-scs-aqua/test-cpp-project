#ifndef K3DSDK_IMESH_PAINTER_GL_H
#define K3DSDK_IMESH_PAINTER_GL_H

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

#include <k3dsdk/iunknown.h>
#include <k3dsdk/iproperty.h>

namespace k3d
{

class ihint;
class mesh;

namespace gl
{

class painter_render_state;
class painter_selection_state;
	
/// Abstract interface implemented by painter objects that can render a mesh using OpenGL
class imesh_painter :
	public virtual iunknown
{
public:
	/// Called to render a mesh for on-screen display
	/**
	 * The ChangedSignal argument is a signal that is emitted when the supplied mesh changes. It could be used by painters caching data
	 * to notify them when the cache needs updating
	 */
	virtual void paint_mesh(const mesh& Mesh, const painter_render_state& RenderState, iproperty::changed_signal_t& ChangedSignal) = 0;
	/// Called to render a mesh for interactive selection
	virtual void select_mesh(const mesh& Mesh, const painter_render_state& RenderState, const painter_selection_state& SelectionState, iproperty::changed_signal_t& ChangedSignal) = 0;
};

} // namespace gl

} // namespace k3d

#endif // !K3DSDK_IMESH_PAINTER_GL_H


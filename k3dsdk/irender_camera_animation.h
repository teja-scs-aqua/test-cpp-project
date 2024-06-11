#ifndef K3DSDK_IRENDER_CAMERA_ANIMATION_H
#define K3DSDK_IRENDER_CAMERA_ANIMATION_H

// K-3D
// Copyright (c) 1995-2008, Timothy M. Shead
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

#include <k3dsdk/frames.h>
#include <k3dsdk/iunknown.h>

namespace k3d
{

class icamera;
class iproperty;
	
/// Abstract interface implemented by objects that can render a sequence of still images using a camera
class irender_camera_animation :
	public virtual iunknown
{
public:
	virtual bool render_camera_animation(icamera& Camera, iproperty& Time, const frames& Frames, const bool ViewCompletedImages) = 0;

protected:
	irender_camera_animation() {}
	irender_camera_animation(const irender_camera_animation& Other) : iunknown(Other) {}
	irender_camera_animation& operator = (const irender_camera_animation&) { return *this; }
	virtual ~irender_camera_animation() {}
};

} // namespace k3d

#endif // !K3DSDK_IRENDER_CAMERA_ANIMATION_H


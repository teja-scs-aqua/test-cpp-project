// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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
	\author Timothy M. Shead (tshead@k-3d.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/gl/extension.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/log.h>
#include <k3dsdk/result.h>
#include <k3dsdk/virtual_document_plugin_factory.h>

namespace module
{

namespace gl
{

namespace virtual_painters
{

class virtual_face_painter_factory:
	public k3d::virtual_document_plugin_factory<k3d::interface_list<k3d::gl::imesh_painter> >
{
	typedef k3d::virtual_document_plugin_factory<k3d::interface_list<k3d::gl::imesh_painter> > base;

public:
	virtual_face_painter_factory() :
		base(k3d::uuid(0x7e6e8354, 0x604f05ae, 0x1d4d658b, 0x96538240),
			"VirtualOpenGLFacePainter",
			_("Renders mesh faces"),
			"OpenGL Painter",
			k3d::iplugin_factory::STABLE),
		delegate(0)
	{
		// Note ... this ctor may be executed before k3d::application is initialized, so
		// we can't do very much here
	}

	k3d::inode* create_plugin(k3d::iplugin_factory& Factory, k3d::idocument& Document)
	{
		if(!delegate)
		{
			if(k3d::gl::extension::query_vbo())
				delegate = dynamic_cast<k3d::idocument_plugin_factory*>(k3d::plugin::factory::lookup("OpenGLVBOFacePainter"));

			if(!delegate)
				delegate = dynamic_cast<k3d::idocument_plugin_factory*>(k3d::plugin::factory::lookup("OpenGLFacePainter"));

			if(delegate)
				k3d::log() << info << this->name() << " delegating to " << dynamic_cast<k3d::iplugin_factory*>(delegate)->name() << std::endl;
		}

		return_val_if_fail(delegate, 0);
		return delegate->create_plugin(*this, Document);
	}

private:
	k3d::idocument_plugin_factory* delegate;
};

k3d::iplugin_factory& face_painter_factory()
{
	static virtual_face_painter_factory instance;
	return instance;
}

} // namespace virtual_painters

} // namespace gl

} // namespace module



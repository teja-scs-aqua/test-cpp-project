// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
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
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/idocument.h>
#include <k3dsdk/imesh_painter_gl.h>
#include <k3dsdk/plugin.h>
#include <k3dsdk/property.h>
#include <k3dsdk/transform.h>

namespace k3d
{

namespace qtui
{

void populate_new_document(idocument& Document)
{
	static unsigned long document_number = 0;

	const k3d::ustring new_title = k3d::ustring::from_utf8(k3d::string_cast(boost::format(_("Untitled Document %1%")) % ++document_number));
	k3d::property::set_internal_value(Document.title(), new_title);

	k3d::imetadata* const node_selection = k3d::plugin::create<k3d::imetadata>("NodeSelection", Document, "Node Selection");
	return_if_fail(node_selection);
	node_selection->set_metadata_value("ngui:unique_node", "node_selection"); // metadata to ensure this node is found by the UI layer

	k3d::plugin::create(k3d::classes::Axes(), Document, "Axes");
	k3d::iunknown* gl_engine = k3d::plugin::create(k3d::classes::OpenGLEngine(), Document, "GL Engine");
	k3d::plugin::create(k3d::classes::TimeSource(), Document, "TimeSource");
	
	k3d::inode* const multi_painter = k3d::plugin::create<k3d::inode>("OpenGLMultiPainter", Document, "GL Default Painter");
	return_if_fail(multi_painter);

/*
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "points", "Points", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLPointPainter", Document, "GL Point Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "edges", "Edges", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLEdgePainter", Document, "GL Edge Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "faces", "Faces", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLFacePainter", Document, "GL Face Painter"));

	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "sds_points", "SDS Points", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLSDSPointPainter", Document, "SDS Point Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "sds_edges", "SDS Edges", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLSDSEdgePainter", Document, "SDS Edge Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "sds_faces", "SDS Faces", "", k3d::plugin::create<k3d::gl::imesh_painter>("VirtualOpenGLSDSFacePainter", Document, "SDS Face Painter"));

	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "linear_curves", "Linear Curves", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLLinearCurvePainter", Document, "GL Linear Curve Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "cubic_curves", "Cubic Curves", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLCubicCurvePainter", Document, "GL Cubic Curve Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "nurbs_curves", "NURBS Curves", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLNURBSCurvePainter", Document, "GL NURBS Curve Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "bilinear_patches", "Bilinear Patches", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLBilinearPatchPainter", Document, "GL Bilinear Patch Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "bicubic_patches", "Bicubic Patches", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLBicubicPatchPainter", Document, "GL Bicubic Patch Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "nurbs_patches", "NURBS Patches", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLNURBSPatchPainter", Document, "GL NURBS Patch Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "bezier_triangle_patches", "Bezier Triangle Patches", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLBezierTrianglePatchPainter", Document, "GL Bezier Triangle Patch Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "blobbies", "Blobbies", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLBlobbyPointPainter", Document, "GL Blobby Point Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "face_normals", "Face Normals", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLFaceNormalPainter", Document, "GL Face Normal Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "face_orientation", "Face Orientation", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLFaceOrientationPainter", Document, "GL Face Orientation Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "cones", "Cones", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLConePainter", Document, "GL Cone Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "cylinders", "Cylinders", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLCylinderPainter", Document, "GL Cylinder Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "disks", "Disks", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLDiskPainter", Document, "GL Disk Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "hyperboloids", "Hyperboloids", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLHyperboloidPainter", Document, "GL Hyperboloid Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "paraboloids", "Paraboloids", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLParaboloidPainter", Document, "GL Paraboloid Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "spheres", "Spheres", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLSpherePainter", Document, "GL Sphere Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "teapots", "Teapots", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLTeapotPainter", Document, "GL Teapot Painter"));
	k3d::property::create<k3d::gl::imesh_painter*>(*multi_painter, "tori", "Tori", "", k3d::plugin::create<k3d::gl::imesh_painter>("OpenGLTorusPainter", Document, "GL Torus Painter"));
*/

	k3d::property::set_internal_value(*gl_engine, "node_selection", dynamic_cast<k3d::inode*>(node_selection));
	return_if_fail(k3d::plugin::factory::lookup("Camera"));

	k3d::inode* const camera = k3d::plugin::create<k3d::inode>("Camera", Document, "Camera");
	return_if_fail(camera);

	const k3d::point3 origin = k3d::point3(0, 0, 0);
	const k3d::vector3 world_up = k3d::vector3(0, 0, 1);

	const k3d::point3 position = k3d::point3(-15, 20, 10);
	const k3d::vector3 look_vector = origin - position;
	const k3d::vector3 right_vector = look_vector ^ world_up;
	const k3d::vector3 up_vector = right_vector ^ look_vector;

	k3d::inode* const camera_transformation = k3d::set_matrix(*camera, k3d::view_matrix(look_vector, up_vector, position));
	return_if_fail(camera_transformation);

	camera_transformation->set_name("Camera Transformation");
	k3d::property::set_internal_value(*camera, "world_target", k3d::point3(0, 0, 0));
}

} // namespace qtui

} // namespace k3d


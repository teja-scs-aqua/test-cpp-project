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
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3d-i18n-config.h>
#include <k3dsdk/algebra.h>
#include <k3dsdk/classes.h>
#include <k3dsdk/color.h>
#include <k3dsdk/data.h>
#include <k3dsdk/document_plugin_factory.h>
#include <k3dsdk/gl.h>
#include <k3dsdk/icamera.h>
#include <k3dsdk/icrop_window.h>
#include <k3dsdk/ilight_gl.h>
#include <k3dsdk/inode_selection.h>
#include <k3dsdk/iparentable.h>
#include <k3dsdk/ipipeline_profiler.h>
#include <k3dsdk/iprojection.h>
#include <k3dsdk/irender_viewport_gl.h>
#include <k3dsdk/irenderable_gl.h>
#include <k3dsdk/imatrix_source.h>
#include <k3dsdk/iuser_interface.h>
#include <k3dsdk/measurement.h>
#include <k3dsdk/node.h>
#include <k3dsdk/property.h>
#include <k3dsdk/property_group_collection.h>
#include <k3dsdk/rectangle.h>
#include <k3dsdk/render_state_gl.h>
#include <k3dsdk/selection_state_gl.h>
#include <k3dsdk/time_source.h>
#include <k3dsdk/utility_gl.h>

#include <iostream>

#ifdef	WIN32
#ifdef	near
#undef	near
#endif	//near
#ifdef	far
#undef	far
#endif	//far
#endif	//WIN32

namespace module
{

namespace opengl
{

namespace detail
{

/// Functor object for initializing light sources during OpenGL drawing
class light_setup
{
public:
	light_setup() :
		m_light_number(0)
	{
	}

	void operator()(k3d::inode* const Object)
	{
		k3d::gl::ilight* const light = dynamic_cast<k3d::gl::ilight*>(Object);
		if(light)
			light->setup_gl_light(++m_light_number);
	}

private:
	unsigned long m_light_number;
};

/// Sorts drawables so they can be rendered.  Currently, drawables are sorted by-layer, but
/// this could be extended to to sort based on selection-state, depth (for transparency), etc.
struct render_order
{
	bool operator()(k3d::gl::irenderable* const A, k3d::gl::irenderable* const B) const
	{
		return A->gl_layer() < B->gl_layer();
	}
};

/// Functor for drawing objects during OpenGL drawing
class draw
{
public:
	draw(k3d::gl::render_state& State, k3d::inode_selection* NodeSelection) :
		m_state(State),
		m_node_selection(NodeSelection)
	{
	}

	void operator()(k3d::gl::irenderable* const Renderable)
	{
		if(m_node_selection)
		{
			m_state.node_selection = m_node_selection->selection_weight(*dynamic_cast<k3d::inode*>(Renderable));
			k3d::node* parent = 0;
			k3d::iparentable* const parentable = dynamic_cast<k3d::iparentable*>(Renderable);
			if(parentable)
				k3d::node* parent = dynamic_cast<k3d::node*>(k3d::property::pipeline_value<k3d::inode*>(parentable->parent()));
			m_state.parent_selection = parent ? m_node_selection->selection_weight(*parent) : 0.0;
		}
		else
		{
			m_state.node_selection = 0.0;
			m_state.parent_selection = 0.0;
		}
		
		Renderable->gl_draw(m_state);
	}

private:
	k3d::gl::render_state& m_state; // Note: no longer const, so selection weights can be set
	k3d::inode_selection* m_node_selection;
};

/// Functor for selecting objects during OpenGL drawing
class draw_selection
{
public:
	draw_selection(const k3d::gl::render_state& State, const k3d::gl::selection_state& SelectState, k3d::inode_selection* NodeSelection) :
		m_state(State),
		m_selection_state(SelectState),
		m_node_selection(NodeSelection)
	{
	}

	void operator()(k3d::gl::irenderable* const Renderable)
	{
		k3d::double_t selection_weight = 0.0;
		if(m_node_selection)
			selection_weight = m_node_selection->selection_weight(*dynamic_cast<k3d::inode*>(Renderable));
		if(m_selection_state.exclude_unselected_nodes && !selection_weight)
			return;
		Renderable->gl_select(m_state, m_selection_state);
	}

private:
	const k3d::gl::render_state& m_state;
	const k3d::gl::selection_state& m_selection_state;
	k3d::inode_selection* m_node_selection;
};

void gl_reset(const k3d::color BackgroundColor, const double PointSize)
{
	// Clear background ...
	glClearDepth(1.0);
	glClearColor(
		static_cast<GLfloat>(BackgroundColor.red),
		static_cast<GLfloat>(BackgroundColor.green),
		static_cast<GLfloat>(BackgroundColor.blue),
		0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set point size ...
	glPointSize(static_cast<GLfloat>(PointSize));

	// Setup stippling for path normals / bezier control lines ...
	glLineStipple(2, 0xaaaa);
	glDisable(GL_LINE_STIPPLE);
	glDisable(GL_POLYGON_STIPPLE);
}

void gl_setup_textures()
{
	// Setup texture matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
}

void gl_draw_2d_widgets(k3d::icamera& Camera, const k3d::rectangle& WindowRect, const k3d::rectangle& CameraRect, const bool DrawFrustum, const bool DrawCropWindow, const bool DrawSafeZone, const bool DrawAimPoint, k3d::idocument& Document)
{
	// Set ourselves up to draw 2D overlays ...
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(WindowRect.x1, WindowRect.x2, WindowRect.y2, WindowRect.y1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if(DrawFrustum)
	{
		glColor4d(0, 0, 0, 1);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINE_LOOP);
			glVertex2d(CameraRect.x1, CameraRect.y1);
			glVertex2d(CameraRect.x2, CameraRect.y1);
			glVertex2d(CameraRect.x2, CameraRect.y2);
			glVertex2d(CameraRect.x1, CameraRect.y2);
		glEnd();
	}

	// Optionally show the extents of the crop window
	if(DrawCropWindow)
	{
		const double left = k3d::property::pipeline_value<double>(Camera.crop_window().crop_left());
		const double right = k3d::property::pipeline_value<double>(Camera.crop_window().crop_right());
		const double bottom = k3d::property::pipeline_value<double>(Camera.crop_window().crop_bottom());
		const double top = k3d::property::pipeline_value<double>(Camera.crop_window().crop_top());

		glColor4d(0, 0, 0.5, 1);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINE_LOOP);
			glVertex2d(k3d::mix(CameraRect.x1, CameraRect.x2, left), k3d::mix(CameraRect.y1, CameraRect.y2, top));
			glVertex2d(k3d::mix(CameraRect.x1, CameraRect.x2, right), k3d::mix(CameraRect.y1, CameraRect.y2, top));
			glVertex2d(k3d::mix(CameraRect.x1, CameraRect.x2, right), k3d::mix(CameraRect.y1, CameraRect.y2, bottom));
			glVertex2d(k3d::mix(CameraRect.x1, CameraRect.x2, left), k3d::mix(CameraRect.y1, CameraRect.y2, bottom));
		glEnd();
	}

	// Optionally draw a safe zone for video ...
	if(DrawSafeZone)
	{
		glColor4d(0, 0, 0, 1);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINE_LOOP);
			glVertex2d(CameraRect.x1 * 0.9, CameraRect.y1 * 0.9);
			glVertex2d(CameraRect.x2 * 0.9, CameraRect.y1 * 0.9);
			glVertex2d(CameraRect.x2 * 0.9, CameraRect.y2 * 0.9);
			glVertex2d(CameraRect.x1 * 0.9, CameraRect.y2 * 0.9);
		glEnd();
	}

	// Optionally draw the "aimpoint" (crosshairs aligned with the center of the viewport)
	if(DrawAimPoint)
	{
		glColor4d(0, 0, 0, 1);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINES);
			glVertex2d(CameraRect.x1 * 0.05, 0);
			glVertex2d(CameraRect.x2 * 0.05, 0);
			glVertex2d(0, CameraRect.y1 * 0.05);
			glVertex2d(0, CameraRect.y2 * 0.05);
		glEnd();
	}
}

void gl_setup_lights(const bool Headlight)
{
	// Setup lights ...
	glEnable(GL_LIGHTING);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// Make sure all lights are turned off initially ...
	GLint maxlights = 0;
	glGetIntegerv(GL_MAX_LIGHTS, &maxlights);
	for(GLint i = 0; i < maxlights; ++i)
		glDisable(GLenum(GL_LIGHT0 + i));

	// Setup the headlight ...
	if(Headlight)
	{
		// Enable the light ...
		glEnable(GL_LIGHT0);

		// Setup color ...
		const GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT, color);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
		glLightfv(GL_LIGHT0, GL_SPECULAR, color);

		// Setup light direction ...
		const GLfloat position[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, position);
	}
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// render_engine

class render_engine :
	public k3d::node,
	public k3d::gl::irender_viewport,
	public k3d::property_group_collection
{
	typedef k3d::node base;

public:
	render_engine(k3d::iplugin_factory& Factory, k3d::idocument& Document) :
		base(Factory, Document),
		m_point_size(init_owner(*this) + init_name("point_size") + init_label(_("Point Size")) + init_description(_("OpengGL point size")) + init_value(4) + init_constraint(constraint::minimum<double>(0.0)) + init_step_increment(1) + init_units(typeid(k3d::measurement::scalar))),
		m_background_color(init_owner(*this) + init_name("background_color") + init_label(_("Background Color")) + init_description(_("Background color")) + init_value(k3d::color(0.8, 0.8, 0.8))),
		m_fog(init_owner(*this) + init_name("fog") + init_label(_("Fog")) + init_description(_("Fog")) + init_value(false)),
		m_fog_near(init_owner(*this) + init_name("fog_near") + init_label(_("Fog Near")) + init_description(_("Fog near distance")) + init_value(0.0) + init_constraint(constraint::minimum<double>(0.0)) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_fog_far(init_owner(*this) + init_name("fog_far") + init_label(_("Fog Far")) + init_description(_("Fog far distance")) + init_value(100.0) + init_constraint(constraint::minimum<double>(0.0)) + init_step_increment(0.1) + init_units(typeid(k3d::measurement::distance))),
		m_headlight(init_owner(*this) + init_name("headlight") + init_label(_("Headlight")) + init_description(_("Headlight")) + init_value(true)),
		m_show_lights(init_owner(*this) + init_name("show_lights") + init_label(_("Show Lights")) + init_description(_("Show lights (on/off)")) + init_value(false)),
		m_draw_two_sided(init_owner(*this) + init_name("draw_two_sided") + init_label(_("Draw Two Sided")) + init_description(_("Draw two sided")) + init_value(true)),
		m_draw_safe_zone(init_owner(*this) + init_name("draw_safe_zone") + init_label(_("Draw Safe Zone")) + init_description(_("Draw Safe Zone")) + init_value(false)),
		m_draw_aimpoint(init_owner(*this) + init_name("draw_aimpoint") + init_label(_("Draw Aim Point")) + init_description(_("Draw center screen cross")) + init_value(true)),
		m_draw_crop_window(init_owner(*this) + init_name("draw_crop_window") + init_label(_("Draw Crop Window")) + init_description(_("Draw bounding rectangle for output rendering")) + init_value(true)),
		m_draw_frustum(init_owner(*this) + init_name("draw_frustum") + init_label(_("Draw Frustum")) + init_description(_("Draw Camera Frustum")) + init_value(true)),
		m_node_selection(init_owner(*this) + init_name("node_selection") + init_label(_("Node Selection")) + init_description(_("Node storing the currently selected nodes")) + init_value(static_cast<k3d::inode_selection*>(0)))
	{
		k3d::iproperty_group_collection::group visibility_group("Visibility");
		visibility_group.properties.push_back(&static_cast<k3d::iproperty&>(m_draw_two_sided));
		visibility_group.properties.push_back(&static_cast<k3d::iproperty&>(m_draw_safe_zone));
		visibility_group.properties.push_back(&static_cast<k3d::iproperty&>(m_draw_aimpoint));
		visibility_group.properties.push_back(&static_cast<k3d::iproperty&>(m_draw_crop_window));
		visibility_group.properties.push_back(&static_cast<k3d::iproperty&>(m_draw_frustum));

		register_property_group(visibility_group);
		
		m_point_size.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_background_color.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_fog.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_fog_near.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_fog_far.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_headlight.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_show_lights.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_draw_two_sided.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_draw_safe_zone.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_draw_aimpoint.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_draw_crop_window.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_draw_frustum.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		m_node_selection.changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_node_selection_changed));
	}
	
	~render_engine()
	{
		m_selection_changed_connection.disconnect();
	}

	void on_redraw(k3d::iunknown*)
	{
		m_redraw_request_signal.emit(k3d::gl::irender_viewport::ASYNCHRONOUS);
	}
	
	void on_node_selection_changed(k3d::iunknown*)
	{
		k3d::inode_selection* node_selection = m_node_selection.pipeline_value();
		if(node_selection)
		{
			m_selection_changed_connection = node_selection->selection_changed_signal().connect(sigc::mem_fun(*this, &render_engine::on_redraw));
		}
	}

	static k3d::iplugin_factory& get_factory()
	{
		static k3d::document_plugin_factory<render_engine > factory(
			k3d::classes::OpenGLEngine(),
			"OpenGLEngine",
			_("OpenGL Render Engine"),
			"OpenGL RenderEngine",
			k3d::iplugin_factory::STABLE);

		return factory;
	}

	bool get_ndc(k3d::icamera& Camera, const unsigned long PixelWidth, const unsigned long PixelHeight, k3d::rectangle& CameraRect, k3d::rectangle& WindowRect)
	{
		return_val_if_fail(PixelWidth && PixelHeight, false);

		double near = 0;
		double far = 0;
		bool orthographic = false;
		k3d::gl::calculate_projection(Camera, PixelWidth, PixelHeight, WindowRect, CameraRect, near, far, orthographic);

		return true;
	}

	void render_viewport(k3d::icamera& Camera, const unsigned long PixelWidth, const unsigned long PixelHeight, GLdouble ViewMatrix[16], GLdouble ProjectionMatrix[16], GLint Viewport[4])
	{
		k3d::ipipeline_profiler::profile profile(document().pipeline_profiler(), *this, "Render Viewport");
		k3d::gl::render_state state(Camera);
		k3d::rectangle unused(0, 0, 0, 0);
		if(!draw_scene(Camera, PixelWidth, PixelHeight, ViewMatrix, ProjectionMatrix, Viewport, false, unused, state))
			return;

		// Setup fog ...
		if(m_fog.pipeline_value())
		{
			const k3d::color background_color(m_background_color.pipeline_value());

			GLfloat fogdata[4];
			fogdata[0] = background_color.red;
			fogdata[1] = background_color.green;
			fogdata[2] = background_color.blue;
			fogdata[3] = 1.0f;

			glFogfv(GL_FOG_COLOR, fogdata);
			glFogf(GL_FOG_START, static_cast<GLfloat>(m_fog_near.pipeline_value()));
			glFogf(GL_FOG_END, static_cast<GLfloat>(m_fog_far.pipeline_value()));
			glHint(GL_FOG_HINT, GL_NICEST);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glEnable(GL_FOG);
		}
		else
		{
			glDisable(GL_FOG);
		}

		if(m_show_lights.pipeline_value())
			std::for_each(document().nodes().collection().begin(), document().nodes().collection().end(), detail::light_setup());

		k3d::inode_selection* const node_selection = m_node_selection.pipeline_value();
		std::vector<k3d::gl::irenderable*> renderable_nodes = k3d::node::lookup<k3d::gl::irenderable>(document());
		std::sort(renderable_nodes.begin(), renderable_nodes.end(), detail::render_order());
		std::for_each(renderable_nodes.begin(), renderable_nodes.end(), detail::draw(state, node_selection));

/* I really hate to lose this feedback, but the GLU NURBS routines generate large numbers of errors, which ruins its utility :-(
		for(GLenum gl_error = glGetError(); gl_error != GL_NO_ERROR; gl_error = glGetError())
			k3d::log() << error << "OpenGL error: " << reinterpret_cast<const char*>(gluErrorString(gl_error)) << std::endl;
*/
	}

	void render_viewport_selection(const k3d::gl::selection_state& SelectState, k3d::icamera& Camera, const unsigned long PixelWidth, const unsigned long PixelHeight, const k3d::rectangle& Region, GLdouble ViewMatrix[16], GLdouble ProjectionMatrix[16], GLint Viewport[4])
	{
		k3d::gl::render_state state(Camera);
		if(!draw_scene(Camera, PixelWidth, PixelHeight, ViewMatrix, ProjectionMatrix, Viewport, true, Region, state))
			return;

		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LIGHTING);

		k3d::inode_selection* const node_selection = m_node_selection.pipeline_value();
		std::vector<k3d::gl::irenderable*> renderable_nodes = k3d::node::lookup<k3d::gl::irenderable>(document());
		std::sort(renderable_nodes.begin(), renderable_nodes.end(), detail::render_order());
		std::for_each(renderable_nodes.begin(), renderable_nodes.end(), detail::draw_selection(state, SelectState, node_selection));
	}

	redraw_request_signal_t& redraw_request_signal()
	{
		return m_redraw_request_signal;
	}

private:
	bool draw_scene(k3d::icamera& Camera, const unsigned long PixelWidth, const unsigned long PixelHeight, GLdouble ViewMatrix[16], GLdouble ProjectionMatrix[16], GLint Viewport[4], const bool Select, const k3d::rectangle& SelectionRegion, k3d::gl::render_state& RenderState)
	{
		// If width or height are zero, we're done ...
		if(!PixelWidth || !PixelHeight)
			return false;

		if(!Select)
			detail::gl_reset(m_background_color.pipeline_value(), m_point_size.pipeline_value());

		// Setup culling ...
		glFrontFace(GL_CW);
		glCullFace(GL_BACK);

		// Setup dithering ...
		glDisable(GL_DITHER);

		// Setup antialiasing ...
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		glLineWidth(1.0f);

		// Disable stencil ...
		glDisable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilMask(0x00);

		// Set Z buffer options ...
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glEnable(GL_DEPTH_TEST);

		// Normalization ...
		glShadeModel(Select ? GL_FLAT : GL_SMOOTH);
		glEnable(GL_NORMALIZE);
		glEnable(GL_AUTO_NORMAL);

		// Prepare texture options ...
		glDisable(GL_TEXTURE_2D);

		if(!Select)
			detail::gl_setup_textures();

		// Setup viewport ...
		glViewport(0, 0, PixelWidth, PixelHeight);
		glGetIntegerv(GL_VIEWPORT, static_cast<GLint*>(RenderState.gl_viewport));
		glGetIntegerv(GL_VIEWPORT, Viewport);

		k3d::rectangle window_rect(0, 0, 0, 0);
		k3d::rectangle camera_rect(0, 0, 0, 0);
		double near = 0;
		double far = 0;
		bool orthographic = false;
		k3d::gl::calculate_projection(Camera, PixelWidth, PixelHeight, window_rect, camera_rect, near, far, orthographic);

		if(!Select)
			detail::gl_draw_2d_widgets(Camera, window_rect, camera_rect, m_draw_frustum.pipeline_value(), m_draw_crop_window.pipeline_value(), m_draw_safe_zone.pipeline_value(), m_draw_aimpoint.pipeline_value(), document());

		// Setup projection ...
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if(orthographic)
		{
			const k3d::matrix4 transform_matrix = k3d::property::pipeline_value<k3d::matrix4>(Camera.transformation().matrix_source_output());
			const k3d::point3 world_position = transform_matrix * k3d::point3(0, 0, 0);
			const k3d::point3 world_target = boost::any_cast<k3d::point3>(Camera.world_target().property_internal_value());
			const double distance = k3d::distance(world_position, world_target);

			const double window_aspect = (window_rect.x2 - window_rect.x1) / (window_rect.y1 - window_rect.y2);
			const double window_tan_fov = (window_rect.y1 - window_rect.y2) * 0.5 / near;
			const double window_size = distance * window_tan_fov;

			RenderState.orthographic = true;
			RenderState.draw_two_sided = m_draw_two_sided.pipeline_value();
			
			RenderState.gl_window_frustum_left = -window_size * window_aspect;
			RenderState.gl_window_frustum_right = window_size * window_aspect;
			RenderState.gl_window_frustum_top = window_size;
			RenderState.gl_window_frustum_bottom = -window_size;
			RenderState.gl_window_frustum_near = near;
			RenderState.gl_window_frustum_far = far;
			
			const double camera_aspect = (camera_rect.x2 - camera_rect.x1) / (camera_rect.y1 - camera_rect.y2);

			if(camera_aspect > window_aspect)
			{
				RenderState.gl_camera_frustum_left = RenderState.gl_window_frustum_left;
				RenderState.gl_camera_frustum_right = RenderState.gl_window_frustum_right;
				RenderState.gl_camera_frustum_top = RenderState.gl_window_frustum_top * window_aspect / camera_aspect;
				RenderState.gl_camera_frustum_bottom = RenderState.gl_window_frustum_bottom * window_aspect / camera_aspect;
				RenderState.gl_camera_frustum_near = near;
				RenderState.gl_camera_frustum_far = far;
			}
			else
			{
				RenderState.gl_camera_frustum_left = RenderState.gl_window_frustum_left * camera_aspect / window_aspect;
				RenderState.gl_camera_frustum_right = RenderState.gl_window_frustum_right * camera_aspect / window_aspect;
				RenderState.gl_camera_frustum_top = RenderState.gl_window_frustum_top;
				RenderState.gl_camera_frustum_bottom = RenderState.gl_window_frustum_bottom;
				RenderState.gl_camera_frustum_near = near;
				RenderState.gl_camera_frustum_far = far;
			}

			glOrtho(-window_size * window_aspect, window_size * window_aspect, -window_size, window_size, near, far);

			if(Select)
			{
				// Setup the projection matrix with the pick matrix ...
				glLoadIdentity();

				const double width  = SelectionRegion.width();
				const double height = SelectionRegion.height();
				gluPickMatrix(SelectionRegion.x1 + (width * 0.5), RenderState.gl_viewport[3] - (SelectionRegion.y1 + (height * 0.5)), width, height, static_cast<GLint*>(RenderState.gl_viewport));

				glOrtho(-window_size * window_aspect, window_size * window_aspect, -window_size, window_size, near, far);
			}
		}
		else
		{
			RenderState.orthographic = false;
			
			RenderState.gl_window_frustum_left = window_rect.x1;
			RenderState.gl_window_frustum_right = window_rect.x2;
			RenderState.gl_window_frustum_top = window_rect.y1;
			RenderState.gl_window_frustum_bottom = window_rect.y2;
			RenderState.gl_window_frustum_near = near;
			RenderState.gl_window_frustum_far = far;

			RenderState.gl_camera_frustum_left = camera_rect.x1;
			RenderState.gl_camera_frustum_right = camera_rect.x2;
			RenderState.gl_camera_frustum_top = camera_rect.y1;
			RenderState.gl_camera_frustum_bottom = camera_rect.y2;
			RenderState.gl_camera_frustum_near = near;
			RenderState.gl_camera_frustum_far = far;

			glFrustum(window_rect.x1, window_rect.x2, window_rect.y2, window_rect.y1, near, far);

			if(Select)
			{
				// Setup the projection matrix with the pick matrix ...
				glLoadIdentity();

				const double width  = SelectionRegion.width();
				const double height = SelectionRegion.height();
				gluPickMatrix(SelectionRegion.x1 + (width * 0.5), RenderState.gl_viewport[3] - (SelectionRegion.y1 + (height * 0.5)), width, height, static_cast<GLint*>(RenderState.gl_viewport));

				glFrustum(window_rect.x1, window_rect.x2, window_rect.y2, window_rect.y1, near, far);
			}
		}

		glGetFloatv(GL_PROJECTION_MATRIX, RenderState.gl_projection_matrix);
		glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);

		// Setup modelview matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		if(!Select)
			detail::gl_setup_lights(m_headlight.pipeline_value());

		const k3d::matrix4 transform_matrix = k3d::property::pipeline_value<k3d::matrix4>(Camera.transformation().matrix_source_output());
		const k3d::angle_axis orientation(k3d::euler_angles(transform_matrix, k3d::euler_angles::ZXYstatic));
		const k3d::point3 position(k3d::position(transform_matrix));

		glScaled(1.0, 1.0, -1.0);
		glRotated(-k3d::degrees(orientation.angle), orientation.axis[0], orientation.axis[1], orientation.axis[2]);
		glTranslated(-position[0], -position[1], -position[2]);

		glGetDoublev(GL_MODELVIEW_MATRIX, ViewMatrix);

		return true;
	}

	redraw_request_signal_t m_redraw_request_signal;

	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, no_constraint, measurement_property, with_serialization) m_point_size;
	k3d_data(k3d::color, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_background_color;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_fog;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_fog_near;
	k3d_data(double, immutable_name, change_signal, with_undo, local_storage, with_constraint, measurement_property, with_serialization) m_fog_far;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_headlight;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_show_lights;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_draw_two_sided;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_draw_safe_zone;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_draw_aimpoint;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_draw_crop_window;
	k3d_data(bool, immutable_name, change_signal, with_undo, local_storage, no_constraint, writable_property, with_serialization) m_draw_frustum;
	k3d_data(k3d::inode_selection*, k3d::data::immutable_name, k3d::data::change_signal, k3d::data::with_undo, k3d::data::node_storage, k3d::data::no_constraint, k3d::data::node_property, k3d::data::node_serialization) m_node_selection;
	sigc::connection m_selection_changed_connection;
};

/////////////////////////////////////////////////////////////////////////////
// render_engine_factory

k3d::iplugin_factory& render_engine_factory()
{
	return render_engine::get_factory();
}

} // namespace opengl

} // namespace module


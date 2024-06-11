// K-3D
// Copyright (c) 1995-2005, Timothy M. Shead
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

#include <gdkmm/cursor.h>
#include <gtkmm/widget.h>

#include <k3d-i18n-config.h>
#include <k3dsdk/color.h>
#include <k3dsdk/high_res_timer.h>
#include <k3dsdk/icamera.h>
#include <k3dsdk/imatrix_source.h>
#include <k3dsdk/ngui/document_state.h>
#include <k3dsdk/ngui/icons.h>
#include <k3dsdk/ngui/keyboard.h>
#include <k3dsdk/ngui/panel_mediator.h>
#include <k3dsdk/ngui/selection.h>
#include <k3dsdk/ngui/selection_input_model.h>
#include <k3dsdk/ngui/utility.h>
#include <k3dsdk/ngui/viewport.h>
#include <k3dsdk/property.h>
#include <k3dsdk/property_collection.h>
#include <k3dsdk/rectangle.h>
#include <k3dsdk/system.h>

namespace k3d
{

namespace ngui
{

/////////////////////////////////////////////////////////////////////////////
// selection_input_model::implementation

struct selection_input_model::implementation :
	public k3d::property_collection
{
	implementation(document_state& DocumentState) :
		m_document_state(DocumentState),
		m_start_selection(k3d::selection::record::empty_record()),
		m_motion_type(MOTION_NONE),
		m_extended_mode(true),
		m_extended_component_mode(true),
		m_paint_mode(true),
		m_double_click_mode(true),
		m_pick_backfacing(false),
		m_paint_backfacing(false),
		m_rubber_band_backfacing(true)
	{
	}

	k3d::idocument& document()
	{
		return m_document_state.document();
	}

	void on_button_down(viewport::control& Viewport, const GdkEventButton& Event)
	{
		m_start_selection = Viewport.pick_object(k3d::point2(Event.x, Event.y), m_pick_backfacing);
		m_motion_type = MOTION_NONE;
	}

	void on_button_click(viewport::control& Viewport, const GdkEventButton& Event)
	{
		const k3d::key_modifiers modifiers = convert(Event.state);

		enum
		{
			NONE,
			SELECT,
			DESELECT,
			REPLACE
		} operation = NONE;

		if(modifiers.shift())
		{
			operation = SELECT;
		}
		else if(modifiers.control())
		{
			operation = DESELECT;
		}
		else
		{
			operation = REPLACE;

			// If something's under the mouse cursor
			if(k3d::selection::get_node(m_start_selection))
			{
				bool extended_mode;
				switch(selection::state(m_document_state.document()).current_mode())
				{
					case selection::NODE:
						// Extended selection
						extended_mode = m_extended_mode;
					break;

					default:
						// Extended component selection
						extended_mode = m_extended_component_mode;
				}

				if(extended_mode)
					operation = selection::state(m_document_state.document()).is_selected(m_start_selection) ? DESELECT : SELECT;
			}
		}

		switch(operation)
		{
			case NONE:
				break;

			case SELECT:
			{
				k3d::record_state_change_set change_set(m_document_state.document(), _("Select"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).select(m_start_selection);

				break;
			}

			case DESELECT:
			{
				k3d::record_state_change_set change_set(m_document_state.document(), _("Deselect"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect(m_start_selection);
				break;
			}

			case REPLACE:
			{
				k3d::record_state_change_set change_set(m_document_state.document(), _("Replace"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect_all();
				selection::state(m_document_state.document()).select(m_start_selection);
				break;
			}
		}

    switch(operation)
    {
      case SELECT:
      case REPLACE:
      {
        if(selection::NODE == selection::state(m_document_state.document()).current_mode())
        {
          if(k3d::inode* const node = k3d::selection::get_node(m_start_selection))
          {
            panel::mediator(m_document_state.document()).set_focus(*node, *this);
          }
        }

        break;
      }
    }
	}

	void on_button_double_click(viewport::control& Viewport, const GdkEventButton& Event)
	{
		if(!m_double_click_mode)
			return;

		switch(selection::state(m_document_state.document()).current_mode())
		{
			case selection::NODE:
			{
				k3d::record_state_change_set change_set(m_document_state.document(), _("Deselect All"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect_all();
				break;
			}

			default:
			{
				k3d::record_state_change_set change_set(m_document_state.document(), _("Node Selection"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).set_current_mode(selection::NODE);
				break;
			}
		}
	}

	void on_button_start_drag(viewport::control& Viewport, const GdkEventMotion& Event)
	{
		m_timer.restart();

		const k3d::key_modifiers modifiers = convert(Event.state);

		if(modifiers.shift())
		{
			//m_motion_type = (m_paint_mode && !m_start_selection.empty()) ? MOTION_PAINT_SELECT : MOTION_RUBBER_BAND_SELECT;
			m_motion_type = MOTION_RUBBER_BAND_SELECT;
		}
		else if(modifiers.control())
		{
			//m_motion_type = (m_paint_mode && !m_start_selection.empty()) ? MOTION_PAINT_DESELECT : MOTION_RUBBER_BAND_DESELECT;
			m_motion_type = MOTION_RUBBER_BAND_DESELECT;
		}
		else
		{
			if(m_start_selection.empty())
			{
				m_motion_type = MOTION_RUBBER_BAND_REPLACE;
			}
			else
			{
				if(m_paint_mode)
				{
					m_motion_type = selection::state(m_document_state.document()).is_selected(m_start_selection) ? MOTION_PAINT_DESELECT : MOTION_PAINT_SELECT;
				}
				else
				{
					m_motion_type = selection::state(m_document_state.document()).is_selected(m_start_selection) ? MOTION_RUBBER_BAND_DESELECT : MOTION_RUBBER_BAND_SELECT;
				}
			}
		}

		switch(m_motion_type)
		{
			case MOTION_NONE:
				break;

			case MOTION_PAINT_SELECT:
			{
				k3d::start_state_change_set(m_document_state.document(), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).select(m_start_selection);

				break;
			}

			case MOTION_PAINT_DESELECT:
			{
				k3d::start_state_change_set(m_document_state.document(), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect(m_start_selection);

				break;
			}

			case MOTION_RUBBER_BAND_REPLACE:
			case MOTION_RUBBER_BAND_SELECT:
			case MOTION_RUBBER_BAND_DESELECT:
			{
				if(inode* const rubber_band = selection::state(m_document_state.document()).rubber_band())
				{
					property::set_internal_value(*rubber_band, "camera", dynamic_cast<inode*>(Viewport.camera()));
					property::set_internal_value(*rubber_band, "color", color(1, 0, 0));
					property::set_internal_value(*rubber_band, "opacity", 0.1);
					property::set_internal_value(*rubber_band, "border_color", color(1, 0, 0));
					property::set_internal_value(*rubber_band, "border_opacity", 0.5);
					property::set_internal_value(*rubber_band, "rectangle", rectangle(Event.x, Event.x, Viewport.get_height() - Event.y, Viewport.get_height() - Event.y));
				}

				break;
			}
		}
	}

	void on_button_drag(viewport::control& Viewport, const GdkEventMotion& Event)
	{
		switch(m_motion_type)
		{
			case MOTION_NONE:
				break;

			case MOTION_PAINT_SELECT:
			{
				const k3d::selection::record selection = Viewport.pick_object(k3d::point2(Event.x, Event.y), m_paint_backfacing);
				if(!selection::state(m_document_state.document()).is_selected(selection))
				{
					selection::state(m_document_state.document()).select(selection);
				}
				else
				{
				}
				break;
			}

			case MOTION_PAINT_DESELECT:
			{
				const k3d::selection::record selection = Viewport.pick_object(k3d::point2(Event.x, Event.y), m_paint_backfacing);
				if(selection::state(m_document_state.document()).is_selected(selection))
				{
					selection::state(m_document_state.document()).deselect(selection);
				}
				else
				{
				}
				break;
			}

			case MOTION_RUBBER_BAND_REPLACE:
			case MOTION_RUBBER_BAND_SELECT:
			case MOTION_RUBBER_BAND_DESELECT:
			{
				if(inode* const rubber_band = selection::state(m_document_state.document()).rubber_band())
				{
					rectangle new_rectangle = property::pipeline_value<rectangle>(*rubber_band, "rectangle");
					new_rectangle.x2 = Event.x;
					new_rectangle.y2 = Viewport.get_height() - Event.y;
					property::set_internal_value(*rubber_band, "rectangle", new_rectangle);
				}

				break;
			}
		}
	}

	void on_button_end_drag(viewport::control& Viewport, const GdkEventButton& Event)
	{
		switch(m_motion_type)
		{
			case MOTION_NONE:
				break;

			case MOTION_PAINT_SELECT:
			{
				k3d::finish_state_change_set(m_document_state.document(), _("Paint Select"), K3D_CHANGE_SET_CONTEXT);
				break;
			}

			case MOTION_PAINT_DESELECT:
			{
				k3d::finish_state_change_set(m_document_state.document(), _("Paint Deselect"), K3D_CHANGE_SET_CONTEXT);
				break;
			}

			case MOTION_RUBBER_BAND_REPLACE:
			{
				rectangle selection_rectangle;
				if(inode* const rubber_band = selection::state(m_document_state.document()).rubber_band())
				{
					selection_rectangle = property::pipeline_value<rectangle>(*rubber_band, "rectangle");
					selection_rectangle.y1 = Viewport.get_height() - selection_rectangle.y1;
					selection_rectangle.y2 = Viewport.get_height() - selection_rectangle.y2;
					property::set_internal_value(*rubber_band, "camera", static_cast<inode*>(0));
				}

				const k3d::selection::records selection = Viewport.get_object_selectables(selection_rectangle, m_rubber_band_backfacing);

				k3d::record_state_change_set change_set(m_document_state.document(), _("Rubber Band Replace"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect_all();
				selection::state(m_document_state.document()).select(selection);
				break;
			}

			case MOTION_RUBBER_BAND_SELECT:
			{
				rectangle selection_rectangle;
				if(inode* const rubber_band = selection::state(m_document_state.document()).rubber_band())
				{
					selection_rectangle = property::pipeline_value<rectangle>(*rubber_band, "rectangle");
					selection_rectangle.y1 = Viewport.get_height() - selection_rectangle.y1;
					selection_rectangle.y2 = Viewport.get_height() - selection_rectangle.y2;
					property::set_internal_value(*rubber_band, "camera", static_cast<inode*>(0));
				}

				const k3d::selection::records selection = Viewport.get_object_selectables(selection_rectangle, m_rubber_band_backfacing);

				k3d::record_state_change_set change_set(m_document_state.document(), _("Rubber Band Select"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).select(selection);
				break;
			}

			case MOTION_RUBBER_BAND_DESELECT:
			{
				rectangle selection_rectangle;
				if(inode* const rubber_band = selection::state(m_document_state.document()).rubber_band())
				{
					selection_rectangle = property::pipeline_value<rectangle>(*rubber_band, "rectangle");
					selection_rectangle.y1 = Viewport.get_height() - selection_rectangle.y1;
					selection_rectangle.y2 = Viewport.get_height() - selection_rectangle.y2;
					property::set_internal_value(*rubber_band, "camera", static_cast<inode*>(0));
				}

				const k3d::selection::records selection = Viewport.get_object_selectables(selection_rectangle, m_rubber_band_backfacing);

				k3d::record_state_change_set change_set(m_document_state.document(), _("Rubber Band Deselect"), K3D_CHANGE_SET_CONTEXT);
				selection::state(m_document_state.document()).deselect(selection);
				break;
			}
		}
		m_motion_type = MOTION_NONE;
	}

	/// Stores a reference to the owning document
	document_state& m_document_state;

	k3d::selection::record m_start_selection;
	enum
	{
		MOTION_NONE,
		MOTION_PAINT_SELECT,
		MOTION_PAINT_DESELECT,
		MOTION_RUBBER_BAND_REPLACE,
		MOTION_RUBBER_BAND_SELECT,
		MOTION_RUBBER_BAND_DESELECT,
	} m_motion_type;

	bool m_extended_mode;
	bool m_extended_component_mode;
	bool m_paint_mode;
	bool m_double_click_mode;
	bool m_pick_backfacing;
	bool m_paint_backfacing;
	bool m_rubber_band_backfacing;

	k3d::timer m_timer;
};

/////////////////////////////////////////////////////////////////////////////
// selection_input_model

selection_input_model::selection_input_model(document_state& DocumentState) :
	m_implementation(new implementation(DocumentState))
{
}

selection_input_model::~selection_input_model()
{
	delete m_implementation;
}

void selection_input_model::set_extended_mode(const bool Enabled)
{
	m_implementation->m_extended_mode = Enabled;
}

void selection_input_model::set_extended_component_mode(const bool Enabled)
{
	m_implementation->m_extended_component_mode = Enabled;
}

void selection_input_model::set_paint_mode(const bool Enabled)
{
	m_implementation->m_paint_mode = Enabled;
}

void selection_input_model::set_double_click_mode(const bool Enabled)
{
	m_implementation->m_double_click_mode = Enabled;
}

void selection_input_model::set_pick_backfacing(const bool Enabled)
{
	m_implementation->m_pick_backfacing = Enabled;
}

void selection_input_model::set_paint_backfacing(const bool Enabled)
{
	m_implementation->m_paint_backfacing = Enabled;
}

void selection_input_model::set_rubber_band_backfacing(const bool Enabled)
{
	m_implementation->m_rubber_band_backfacing = Enabled;
}

void selection_input_model::on_button_down(viewport::control& Viewport, const GdkEventButton& Event)
{
	m_implementation->on_button_down(Viewport, Event);
}

void selection_input_model::on_button_click(viewport::control& Viewport, const GdkEventButton& Event)
{
	m_implementation->on_button_click(Viewport, Event);
}

void selection_input_model::on_button_double_click(viewport::control& Viewport, const GdkEventButton& Event)
{
	m_implementation->on_button_double_click(Viewport, Event);
}

void selection_input_model::on_button_start_drag(viewport::control& Viewport, const GdkEventMotion& Event)
{
	m_implementation->on_button_start_drag(Viewport, Event);
}

void selection_input_model::on_button_drag(viewport::control& Viewport, const GdkEventMotion& Event)
{
	m_implementation->on_button_drag(Viewport, Event);
}

void selection_input_model::on_button_end_drag(viewport::control& Viewport, const GdkEventButton& Event)
{
	m_implementation->on_button_end_drag(Viewport, Event);
}

} // namespace ngui

} // namespace k3d


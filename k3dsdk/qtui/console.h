#ifndef K3DSDK_QTUI_CONSOLE_H
#define K3DSDK_QTUI_CONSOLE_H

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
*/

#include <QPlainTextEdit>

namespace k3d
{

namespace qtui
{

namespace console
{

/////////////////////////////////////////////////////////////////////////////
// widget

/// Provides a "console" widget that can be used for line-oriented output
class widget :
        public QPlainTextEdit
{
	typedef QPlainTextEdit base;

	Q_OBJECT;

public:
	widget(QWidget* Parent = 0);
	~widget();

Q_SIGNALS:
	/// Signal that will be emitted whenever the user enters a command to be executed.
	void execute(const QString& Command);

public Q_SLOTS:
	/// Writes the supplied text to the console.
	void print_text(const QString& String);
	/// Writes the supplied HTML to the console.
	void print_html(const QString& HTML);
	/// Scrolls the console to the end of its output.
	void scroll_to_end();

/*
	/// Set the completion key
	void set_completion_key(const uint_t KeySym);

	/// Connects a slot to a signal that will be emitted when the "completion request" key (see set_completion_key) is pressed
	sigc::connection connect_complete_key_pressed_signal(const sigc::slot<void, const string_t&>& Slot);
*/

private Q_SLOTS:
	void on_text_changed();

private:
	void print();
	void showEvent(QShowEvent* Event);
	void keyPressEvent(QKeyEvent* Event);

	class implementation;
	implementation* const internal;
};

} // namespace console

} // namespace qtui

} // namespace k3d

#endif // !K3DSDK_QTUI_CONSOLE_H


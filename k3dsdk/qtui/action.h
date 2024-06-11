#ifndef K3DSDK_QTUI_ACTION_H
#define K3DSDK_QTUI_ACTION_H

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

#include <k3dsdk/signal_system.h>
#include <QAction>

namespace k3d
{

class iplugin_factory;

namespace qtui
{

/////////////////////////////////////////////////////////////////////////////
// action

class action :
	public QAction
{
	Q_OBJECT;

	typedef QAction base;

public:
	action(QObject* Parent, const sigc::slot<void>& Slot);
	action(const QString& Text, QObject* Parent, const sigc::slot<void>& Slot);
	action(const QIcon& Icon, const QString& Text, QObject* Parent, const sigc::slot<void>& Slot);

	/// Creates a standarized action based on a K-3D plugin type
	static action* create(iplugin_factory& Factory, QObject* Parent, const sigc::slot<void>& Slot);

private Q_SLOTS:
	void initialize(const sigc::slot<void>& Slot);
	void on_triggered(bool);

private:
	sigc::signal<void, bool> triggered_signal;
};

} // namespace qtui

} // namespace k3d

#endif // !K3DSDK_QTUI_ACTION_H


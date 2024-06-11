#ifndef MODULES_QTUI_VIEWPORT_H
#define MODULES_QTUI_VIEWPORT_H

// K-3D
// Copyright (c) 1995-2010, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your argument) any later version.
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

#include <QGraphicsView>

namespace module
{

namespace qtui
{

//////////////////////////////////////////////////////////////////////////
// viewport

class viewport :
	public QGraphicsView
{
	Q_OBJECT;

public:
	viewport(QWidget* parent = 0);
	~viewport();

protected:
	void resizeEvent(QResizeEvent *event);
};

} // namespace qtui

} // namespace module

#endif // !MODULES_QTUI_VIEWPORT_H


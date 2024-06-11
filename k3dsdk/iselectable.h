#ifndef K3DSDK_ISELECTABLE_H
#define K3DSDK_ISELECTABLE_H

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
		\brief Declares iselectable, an abstract interface for selectable objects
		\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/iunknown.h>

namespace k3d
{

/// Abstract interface for an object that supports the "selection" concept, affecting downstream editing operations
/** \deprecated Get rid of this once k3d::legacy::mesh is gone */
class iselectable :
	public virtual iunknown
{
public:
	/// Returns the selection weight of the object, 0 == not selected, non-zero == selected, modifiers may interpret the weight as a boolean, or use linear-interpolation.  Weights may be negative or greater than 1.
	virtual double get_selection_weight() = 0;
	/// Sets the selection weight of the object
	virtual void set_selection_weight(const double Weight) = 0;

protected:
	iselectable() {}
	iselectable(const iselectable& Other) : iunknown(Other) {}
	iselectable& operator=(const iselectable&) { return *this; }
	virtual ~iselectable() {}
};

} // namespace k3d

#endif // !K3DSDK_ISELECTABLE_H


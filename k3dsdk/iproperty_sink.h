#ifndef K3DSDK_IPROPERTY_SINK_H
#define K3DSDK_IPROPERTY_SINK_H

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
*/

#include <k3dsdk/iunknown.h>

namespace k3d
{

class iproperty;

/// Abstract interface for objects that can be optionally 'bound' to a specific property
class iproperty_sink :
	public virtual iunknown
{
public:
	virtual ~iproperty_sink() {}

	virtual void set_property(iproperty* const Property) = 0;

protected:
	iproperty_sink() {}
	iproperty_sink(const iproperty_sink&) {}
	iproperty_sink& operator = (const iproperty_sink&) { return *this; }
};

} // namespace k3d

#endif // !K3DSDK_IPROPERTY_SINK_H


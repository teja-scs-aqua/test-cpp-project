#ifndef K3DSDK_IOMANIP_H
#define K3DSDK_IOMANIP_H

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

#include <k3dsdk/types.h>
#include <iosfwd>

namespace k3d
{

/// Returns the current indentation for a stream
long& current_indent(std::ios& Stream);
/// Increments a stream's indentation
std::ostream& push_indent(std::ostream& Stream);
/// Decrements a stream's indentation
std::ostream& pop_indent(std::ostream& Stream);
/// Inserts whitespace into a stream, proportional to its indentation level
std::ostream& standard_indent(std::ostream& Stream);

struct start_block
{
	start_block(const uint_t BlockSize = 8);
	uint_t block_size;
};

std::ostream& operator<<(std::ostream& Stream, const start_block& RHS);
std::ostream& block_delimiter(std::ostream& Stream);
std::ostream& finish_block(std::ostream& Stream);

} // namespace k3d

#endif // !K3DSDK_IOMANIP_H


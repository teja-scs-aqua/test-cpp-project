#ifndef K3DSDK_BASE64_H
#define K3DSDK_BASE64_H

// K-3D
// Copyright (c) 1995-2004, Timothy M. Shead
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

#include <iosfwd>

namespace k3d
{

namespace base64
{

/// Encodes the input stream into base64, sending the results to the output stream
void encode(std::istream& Input, std::ostream& Output, const unsigned long LineWidth = 72);

/// Decodes the input stream from base64, sending the results to the output stream
void decode(std::istream& Input, std::ostream& Output);

} // namespace base64

} // namespace k3d

#endif // !K3DSDK_BASE64_H


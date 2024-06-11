#ifndef K3DSDK_IDOCUMENT_IMPORTER_H
#define K3DSDK_IDOCUMENT_IMPORTER_H

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

#include <k3dsdk/imetadata.h>
#include <k3dsdk/iunknown.h>

namespace k3d
{

class idocument;
namespace filesystem { class path; }

/// Abstract interface for objects that can import data into an existing K-3D document
class idocument_importer :
	public virtual iunknown
{
public:
	virtual ~idocument_importer() {}

	/// Return metadata extracted from the file.
	virtual imetadata::metadata_t get_file_metadata(const filesystem::path& File) = 0;
	/// Read the file data into a document.
	virtual bool_t read_file(const filesystem::path& File, idocument& Document) = 0;

protected:
	idocument_importer() {}
	idocument_importer(const idocument_importer&) {}
	idocument_importer& operator = (const idocument_importer&) { return *this; }
};

} // namespace k3d

#endif // !K3DSDK_IDOCUMENT_IMPORTER_H


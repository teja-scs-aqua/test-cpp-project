#ifndef MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_SELECTION_CACHE_H
#define MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_SELECTION_CACHE_H

// K-3D
// Copyright (c) 1995-2007, Timothy M. Shead
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

/** \file Caches for selection data
 * 	\author Bart Janssens (bart.janssens@lid.kviv.be)
 */

#include <k3dsdk/polyhedron.h>

#include <boost/scoped_ptr.hpp>

namespace module
{

namespace opengl
{

namespace painters
{

/// Storage for selection data
//typedef std::vector<k3d::mesh_selection::record> selection_records_t;
typedef std::vector<int> selection_records_t;

namespace detail
{

inline void copy_selection(const k3d::mesh::selection_t& Selection, selection_records_t& Records)
{
	assert_not_implemented();
/*
	for (k3d::uint_t i = 0; i < Selection.size();)
	{
		k3d::uint_t start = i;
		k3d::mesh_selection::record record(start, i+1, Selection[i]);
		while (i < Selection.size() && record.weight == Selection[i])
		{
			record.end = i+1;
			++i;
		}
		Records.push_back(record);
	}
*/
}

} // namespace detail

/// Keep track of component selections
class component_selection : public scheduler
{
public:
	component_selection(const k3d::mesh* const Mesh) {}
	/// Provide access to the stored selection records
	const selection_records_t& records(const k3d::mesh::primitive* Polyhedron)
	{
		return m_selection_records[Polyhedron];
	}

protected:
	void on_schedule(k3d::inode* Painter)
	{
		m_selection_records.clear();
	}
	void on_schedule(k3d::hint::selection_changed* Hint, k3d::inode* Painter)
	{
		on_schedule(Painter);
	}
	std::map<const k3d::mesh::primitive*, selection_records_t> m_selection_records;
};

/// point selection
class point_selection : public scheduler
{
public:
	point_selection(const k3d::mesh* const Mesh) {}
	
	const selection_records_t& records() const
	{
		return m_selection_records;
	}
protected:
	void on_execute(const k3d::mesh& Mesh, k3d::inode* Painter)
	{
		if (m_selection_records.empty())
		{
			detail::copy_selection(*Mesh.point_selection, m_selection_records);
		}
	}
	void on_schedule(k3d::inode* Painter)
	{
		m_selection_records.clear();
	}
	void on_schedule(k3d::hint::selection_changed* Hint, k3d::inode* Painter)
	{
		on_schedule(Painter);
	}
	selection_records_t m_selection_records;
};

/// Implement component_selection::on_execute for an edge selection
class edge_selection : public component_selection
{
public:
	edge_selection(const k3d::mesh* const Mesh) : component_selection(Mesh) {}
protected:
	void on_execute(const k3d::mesh& Mesh, k3d::inode* Painter)
	{
		if (m_selection_records.empty())
		{
			for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
			{
				boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
				if(!polyhedron.get())
					continue;
				detail::copy_selection(polyhedron->edge_selections, m_selection_records[primitive->get()]);
			}
		}
	}
};

/// Implement component_selection::on_execute for a face selection
class face_selection : public component_selection
{
public:
	face_selection(const k3d::mesh* const Mesh) : component_selection(Mesh) {}
protected:
	void on_execute(const k3d::mesh& Mesh, k3d::inode* Painter)
	{
		if (m_selection_records.empty())
		{
			for(k3d::mesh::primitives_t::const_iterator primitive = Mesh.primitives.begin(); primitive != Mesh.primitives.end(); ++primitive)
			{
				boost::scoped_ptr<k3d::polyhedron::const_primitive> polyhedron(k3d::polyhedron::validate(Mesh, **primitive));
				if(!polyhedron.get())
					continue;
				detail::copy_selection(polyhedron->face_selections, m_selection_records[primitive->get()]);
			}
		}
	}
};

} // namespace painters

} // namespace opengl

} // namespace module

#endif // !MODULES_ADVANCED_OPENGL_PAINTERS_ATTIC_SELECTION_CACHE_H

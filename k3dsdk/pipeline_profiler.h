#ifndef K3DSDK_PIPELINE_PROFILER_H
#define K3DSDK_PIPELINE_PROFILER_H

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

#include <k3dsdk/ipipeline_profiler.h>

namespace k3d
{

/// Concrete implementation of ipipeline_profiler that collects and distributes profiling data for the K-3D visualization pipeline
class pipeline_profiler :
	public ipipeline_profiler
{
public:
	pipeline_profiler();
	~pipeline_profiler();
	
	void start_execution(inode& Node, const string_t& Task);
	void start_execution(inode& Node, const string_t& Task, const double Adjustment);
	void finish_execution(inode& Node, const string_t& Task);
	void add_timing_entry(inode& Node, const string_t& Task, const double TimingValue);
	
	sigc::connection connect_node_execution_signal(const sigc::slot<void, inode&, const string_t&, double>& Slot);

private:
	class implementation;
	implementation* const m_implementation;
};

} // namespace k3d

#endif // !K3DSDK_PIPELINE_PROFILER_H


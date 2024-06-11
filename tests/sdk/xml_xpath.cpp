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

#include <k3dsdk/log_control.h>
#include <k3dsdk/xml.h>
#include <k3dsdk/xpath.h>

using namespace k3d::xml;

#include <iostream>
#include <stdexcept>
#include <sstream>

#define test_expression(expression) \
{ \
  if(!(expression)) \
    { \
    std::ostringstream buffer; \
    buffer << "Expression failed at line " << __LINE__ << ": " << #expression; \
    throw std::runtime_error(buffer.str()); \
    } \
}

int main(int argc, char* argv[])
{
	k3d::log_color_level(true);
	k3d::log_show_level(true);
	k3d::log_minimum_level(k3d::K3D_LOG_LEVEL_DEBUG);

	try
	{
		element document("k3d",
			element("nodes",
				element("node",
					attribute("class", "foo")
					),
				element("node",
					attribute("factory", "bar"),
					element("properties",
						element("property",
							attribute("user_property", ""),
							attribute("type", "double")
							)
						)
					)
				),
			element("dependencies"
				)
			);

		xpath::result_set results;

		results = xpath::match(document, "");
		test_expression(results.size() == 0);

		results = xpath::match(document, "/");
		test_expression(results.size() == 0);

		results = xpath::match(document, "/foo");
		test_expression(results.size() == 0);

		results = xpath::match(document, "/k3d");
		test_expression(results.size() == 1);
		test_expression(results[0]->name == "k3d");

		results = xpath::match(document, "/k3d/*");
		test_expression(results.size() == 2);
		test_expression(results[0]->name == "nodes");
		test_expression(results[1]->name == "dependencies");

		results = xpath::match(document, "/k3d/nodes");
		test_expression(results.size() == 1);
		test_expression(results[0]->name == "nodes");

		results = xpath::match(document, "/k3d/nodes/node");
		test_expression(results.size() == 2);
		test_expression(results[0]->name == "node");
		test_expression(results[1]->name == "node");

		results = xpath::match(document, "/k3d/nodes/node[@class]");
		test_expression(results.size() == 1);
		test_expression(find_attribute(*results[0], "class"));
		test_expression(!find_attribute(*results[0], "factory"));

		results = xpath::match(document, "/k3d/nodes/node/properties/property[@user_property][@type='double']");
		test_expression(results.size() == 1);

		results = xpath::match(document, "nodes");
		test_expression(results.size() == 1);
		test_expression(results[0]->name == "nodes");

		results = xpath::match(document, "nodes/node");
		test_expression(results.size() == 2);
		test_expression(results[0]->name == "node");
		test_expression(results[1]->name == "node");
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}


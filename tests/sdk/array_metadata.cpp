#include <k3dsdk/metadata_keys.h>
#include <k3dsdk/typed_array.h>
#include <k3dsdk/uint_t_array.h>

#include <boost/scoped_ptr.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>

#define test_expression(expression) \
	if(!(expression)) \
	{ \
		std::ostringstream buffer; \
		buffer << #expression << " failed at " << __FILE__ << ": " << __LINE__; \
		throw std::runtime_error(buffer.str()); \
	} \

int main(int argc, char* argv[])
{
	try
	{
		// Validate the initially-empty state ...
		k3d::uint_t_array a;
		test_expression(a.empty());
		test_expression(a.get_metadata().empty());

		// Put some data in the array ...
		a.push_back(1);
		a.push_back(2);
		test_expression(a.size() == 2);

		// Assign some metadata ...
		a.set_metadata_value(k3d::metadata::key::domain(), k3d::metadata::value::point_indices_domain());
		test_expression(a.get_metadata().size() == 1);
		test_expression(a.get_metadata_value(k3d::metadata::key::domain()) == k3d::metadata::value::point_indices_domain());

		// Verify that metadata is copied correctly ...
		k3d::uint_t_array b = a;
		test_expression(b.size() == 2);
		test_expression(b.get_metadata().size() == 1);
		test_expression(b.get_metadata_value(k3d::metadata::key::domain()) == k3d::metadata::value::point_indices_domain());

		boost::scoped_ptr<k3d::array> c(a.clone());
		test_expression(c->size() == 2);
		test_expression(c->get_metadata().size() == 1);
		test_expression(c->get_metadata_value(k3d::metadata::key::domain()) == k3d::metadata::value::point_indices_domain());

		boost::scoped_ptr<k3d::array> d(a.clone_type());
		test_expression(d->empty());
		test_expression(d->get_metadata().size() == 1);
		test_expression(d->get_metadata_value(k3d::metadata::key::domain()) == k3d::metadata::value::point_indices_domain());

		// Verify that metadata is handled correctly when testing for equality ...
		k3d::uint_t_array e = a;
		test_expression(boost::accumulators::min(k3d::difference::test(e, a).exact) != false);
		
		e.set_metadata_value("foo", "bar");
		test_expression(boost::accumulators::min(k3d::difference::test(e, a).exact) != true);

		e.erase_metadata_value("foo");
		test_expression(boost::accumulators::min(k3d::difference::test(e, a).exact) != false);

		e.set_metadata_value(k3d::metadata::key::domain(), "../array[@name=\"clockwise_edges\"]/indices()");
		test_expression(boost::accumulators::min(k3d::difference::test(e, a).exact) != true);
	}
	catch(std::exception& e)
	{
		std::cerr << "uncaught exception: " << e.what() << std::endl;
		return 1;
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl;
	}
	
	return 0;
}


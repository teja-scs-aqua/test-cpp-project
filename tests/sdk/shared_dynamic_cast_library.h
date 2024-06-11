#ifndef TESTS_SDK_SHARED_DYNAMIC_CAST_LIBRARY_H
#define TESTS_SDK_SHARED_DYNAMIC_CAST_LIBRARY_H

namespace k3d
{

class iunknown
{
public:
	virtual ~iunknown() {}
};

class istuff :
	public virtual iunknown
{
public:

protected:
	istuff() {}
	istuff(const istuff&) {}
	istuff& operator=(const istuff&) { return *this; }
	virtual ~istuff() {}
};

} // namespace k3d

typedef k3d::iunknown* (*make_stuff_entry_point)();

#endif // !TESTS_SDK_SHARED_DYNAMIC_CAST_LIBRARY_H

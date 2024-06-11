#ifndef K3DSDK_BASIC_MATH_H
#define K3DSDK_BASIC_MATH_H

// K-3D
// Copyright (c) 1995-2009, Timothy M. Shead
//
// Contact: tshead@k-3d.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/** \file
	\author Tim Shead (tshead@k-3d.com)
*/

#include <k3dsdk/types.h>

#include <algorithm>
#include <cmath>

namespace k3d
{

/// Pi
inline double_t pi()
{
	return 3.1415926535897932384626433832795;
}

/// Pi divided-by two
inline double_t pi_over_2()
{
	return pi() * 0.5;
}

/// Pi times two
inline double_t pi_times_2()
{
	return pi() * 2.0;
}

/// Converts degrees to radians
inline double_t radians(const double_t degrees)
{
	return degrees * 0.01745329252;
}

/// Converts radians to degrees
inline double_t degrees(const double_t radians)
{
	return radians * 57.2957795131;
}

/// Computes N!
inline double_t factorial(double_t N)
{
	double_t result = 1;
	for(double_t i = 2; i <= N; ++i)
		result *= i;

	return result;
}

/// Computes an integer power of a base via successive squaring
template <typename Type>
Type fast_pow(Type base, int exponent)
{
	bool invert = exponent < 0;
	if (invert)
		exponent = -exponent;

	// loop invariant: prod * base^exponent
	Type prod = Type(1);
	while (exponent > 0) {
		if (exponent % 2 == 0) {
			base *= base;
			exponent /= 2;
		}
		else {
			prod *= base;
			--exponent;
		}
	}

	if (invert)
		prod = Type(1) / prod;

	return prod;
}

/// Returns the sign of an argument: -1 if negative, 0 if zero, +1 if positive
template<class type> double_t sign(const type& Arg)
{
	if(Arg > 0.0) return 1.0;
	if(Arg < 0.0) return -1.0;

	return 0.0;
}

/// Rounds a value to the closest integer
inline double_t round(const double_t Value)
{
	return (Value - std::floor(Value)) < 0.5 ? std::floor(Value) : std::ceil(Value);
}

/// Clamps a value within the specified range (conforms to SL usage)
template<class type>
type clamp(const type& x, const type& minval, const type& maxval)
{
	return std::min(std::max(x, minval), maxval);
}

/// Returns the linear interpolation of two values
template<class type>
type mix(const type& x, const type& y, const double_t alpha)
{
	return x * (1 - alpha) + y * (alpha);
}

/// Returns the ratio of two values (handles type-casting to a floating-point value)
template<typename type>
double_t ratio(const type& x, const type& y)
{
	return static_cast<double_t>(x) / static_cast<double_t>(y);
}

} // namespace k3d

#endif // !K3DSDK_BASIC_MATH_H


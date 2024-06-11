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
	\author Tim Shead (tshead@k-3d.com)
	\author Romain Behar (romainbehar@yahoo.com)
*/

#include <k3dsdk/algebra.h>
#include <k3dsdk/imaterial.h>
#include <k3dsdk/stream_ri.h>
#include <k3dsdk/stream_io_ri.h>
#include <k3dsdk/types_ri.h>
#include <k3dsdk/vectors.h>

#include <boost/array.hpp>

#include <iostream>
#include <numeric>
#include <set>

namespace k3d
{

namespace ri
{

namespace detail
{

long& indentation_storage(std::ios& Stream)
{
	static const int index = std::ios::xalloc();
	return Stream.iword(index);
}

std::ostream& reset_indentation(std::ostream& Stream)
{
	indentation_storage(Stream) = 0;
	return Stream;
}

std::ostream& push_indent(std::ostream& Stream)
{
	indentation_storage(Stream)++;
	return Stream;
}

std::ostream& pop_indent(std::ostream& Stream)
{
	long& indent = indentation_storage(Stream);
	indent -= (indent > 0);
	return Stream;
}

std::ostream& indentation(std::ostream& Stream)
{
	const long& indent = indentation_storage(Stream);
	for(long i = 0; i < indent; i++)
		Stream << "   ";

	return Stream;
}

} // namespace detail

/////////////////////////////////////////////////////////////////////////////
// stream::implementation

class stream::implementation
{
public:
	implementation(std::ostream& Stream) :
		m_stream(Stream),
		m_light_handle(0),
		m_object_handle(0),
		m_frame_block(false),
		m_world_block(false),
		m_object_block(false),
		m_motion_block(false)
	{
	}

	/// Stores the RIB output stream
	std::ostream& m_stream;
	/// Stores the current light handle
	light_handle m_light_handle;
	/// Stores the current object handle
	object_handle m_object_handle;
	/// Set to true within a frame block
	bool m_frame_block;
	/// Set to true within a world block
	bool m_world_block;
	/// Set to true within an object block
	bool m_object_block;
	/// Set to true within a motion block
	bool m_motion_block;
};

/////////////////////////////////////////////////////////////////////////////
// stream

stream::stream(std::ostream& Stream) :
	m_implementation(new implementation(Stream))
{
	// Enable inline type declarations by default ...
	k3d::ri::set_inline_types(m_implementation->m_stream, true);

	// Start out without any indentation ...
	detail::reset_indentation(m_implementation->m_stream);

	// Start writing the RIB file header ...
	RiStructure("RenderMan RIB-Structure 1.0");
	m_implementation->m_stream << "version 3.03" << "\n";
}

stream::~stream()
{
	delete m_implementation;
}

bool stream::set_inline_types(const bool Inline)
{
	return k3d::ri::set_inline_types(m_implementation->m_stream, Inline);
}

void stream::RiDeclare(const string& Name, const string& Type)
{
	// Sanity checks ...
	return_if_fail(Name.size());
	return_if_fail(Type.size());

	m_implementation->m_stream << detail::indentation << "Declare " << format_string(Name) << " " << format_string(Type) << "\n";
}

void stream::RiFrameBegin(const unsigned_integer FrameNumber)
{
	// Sanity checks ...
	if(m_implementation->m_frame_block)
	{
		log() << error << "Cannot nest calls to RiFrameBegin()" << std::endl;
		return;
	}

	m_implementation->m_frame_block = true;
	m_implementation->m_stream << detail::indentation << detail::indentation << "FrameBegin " << FrameNumber << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiFrameEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "FrameEnd" << "\n";
	m_implementation->m_frame_block = false;
}

void stream::RiWorldBegin()
{
	// Sanity checks ...
	if(m_implementation->m_world_block)
	{
		log() << error << "Cannot nest calls to RiWorldBegin()" << std::endl;
		return;
	}

	m_implementation->m_world_block = true;
	m_implementation->m_stream << detail::indentation << "WorldBegin" << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiWorldEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "WorldEnd" << "\n";
	m_implementation->m_world_block = false;
}

void stream::RiFormat(const unsigned_integer XResolution, const unsigned_integer YResolution, const real AspectRatio)
{
	m_implementation->m_stream << detail::indentation << "Format " << XResolution << " " << YResolution << " " << AspectRatio << "\n";
}

void stream::RiFrameAspectRatio(real AspectRatio)
{
	m_implementation->m_stream << detail::indentation << "FrameAspectRatio " << AspectRatio << "\n";
}

void stream::RiScreenWindow(real Left, real Right, real Bottom, real Top)
{
	m_implementation->m_stream << detail::indentation << "ScreenWindow " << Left << " " << Right << " " << Bottom << " " << Top << "\n";
}

void stream::RiCropWindow(real XMin, real XMax, real YMin, real YMax)
{
	m_implementation->m_stream << detail::indentation << "CropWindow " << XMin << " " << XMax << " " << YMin << " " << YMax << "\n";
}

void stream::RiProjectionV(const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Projection " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiClipping(real NearPlane, real FarPlane)
{
	m_implementation->m_stream << detail::indentation << "Clipping " << NearPlane << " " << FarPlane << "\n";
}

void stream::RiDepthOfField(real FStop, real FocalLength, real FocalDistance)
{
	m_implementation->m_stream << detail::indentation << "DepthOfField " << FStop << " " << FocalLength << " " << FocalDistance << "\n";
}

void stream::RiShutter(real OpenTime, real CloseTime)
{
	m_implementation->m_stream << detail::indentation << "Shutter " << OpenTime << " " << CloseTime << "\n";
}

void stream::RiPixelFilter(const string& FilterName, real XWidth, real YWidth)
{
	m_implementation->m_stream << detail::indentation << "PixelFilter " << format_string(FilterName) << " " << XWidth << " " << YWidth << "\n";
}

void stream::RiPixelVariance(real Variation)
{
	m_implementation->m_stream << detail::indentation << "PixelVariance " << Variation << "\n";
}
void stream::RiPixelSamples(real XSamples, real YSamples)
{
	m_implementation->m_stream << detail::indentation << "PixelSamples " << XSamples << " " << YSamples << "\n";
}

void stream::RiExposure(real Gain, real Gamma)
{
	m_implementation->m_stream << detail::indentation << "Exposure " << Gain << " " << Gamma << "\n";
}

void stream::RiImagerV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Imager " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiQuantize(const string& Type, integer One, integer QMin, integer QMax, real Amplitude)
{
	m_implementation->m_stream << detail::indentation << "Quantize " << format_string(Type) << " " << One << " " << QMin << " " << QMax << " " << Amplitude << "\n";
}

void stream::RiDisplayV(const string& Name, const string& Type, const string& Mode, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Display " << format_string(Name) << " " << format_string(Type) << " " << format_string(Mode) << " " << Parameters << "\n";
}

void stream::RiHiderV(const string& Type, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Hider " << format_string(Type) << " " << Parameters << "\n";
}

void stream::RiColorSamples(const unsigned_integer ParameterCount, const reals& nRGB, const reals& RGBn)
{
	return_if_fail(ParameterCount == nRGB.size());
	return_if_fail(ParameterCount == RGBn.size());

	m_implementation->m_stream << detail::indentation << "ColorSamples " << format_array(nRGB.begin(), nRGB.end()) << " " << format_array(RGBn.begin(), RGBn.end()) << "\n";
}

void stream::RiRelativeDetail(real RelativeDetail)
{
	m_implementation->m_stream << detail::indentation << "RelativeDetail " << RelativeDetail << "\n";
}

void stream::RiOptionV(const string& Name, const parameter_list& Parameters)
{
	const bool old_state = k3d::ri::set_inline_types(m_implementation->m_stream, false);

	m_implementation->m_stream << detail::indentation << "Option " << format_string(Name) << " " << Parameters << "\n";

	k3d::ri::set_inline_types(m_implementation->m_stream, old_state);
}

void stream::RiAttributeBegin()
{
	m_implementation->m_stream << detail::indentation << "AttributeBegin" << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiAttributeEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "AttributeEnd" << "\n";
}

void stream::RiColor(const color& Color)
{
	m_implementation->m_stream << detail::indentation << "Color " << Color << "\n";
}

void stream::RiOpacity(const color& Opacity)
{
	m_implementation->m_stream << detail::indentation << "Opacity " << Opacity << "\n";
}

void stream::RiTextureCoordinates(real S1, real T1, real S2, real T2, real S3, real T3, real S4, real T4)
{
	m_implementation->m_stream << detail::indentation << "TextureCoordinates " << S1 << " " << T1 << " " << S2 << " " << T2 << " " << S3 << " " << T3 << " " << S4 << " " << T4 << "\n";
}

const light_handle stream::RiLightSourceV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "LightSource " << format_string(Name) << " " << ++m_implementation->m_light_handle << " " << Parameters << "\n";
	return m_implementation->m_light_handle;
}

const light_handle stream::RiAreaLightSourceV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "AreaLightSource " << format_string(Name) << " " << ++m_implementation->m_light_handle << " " << Parameters << "\n";
	return m_implementation->m_light_handle;
}

void stream::RiIlluminate(const light_handle LightHandle, bool OnOff)
{
	m_implementation->m_stream << detail::indentation << "Illuminate " << LightHandle << " " << OnOff << "\n";
}

void stream::RiSurfaceV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Surface " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiAtmosphereV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Atmosphere " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiInteriorV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Interior " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiExteriorV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Exterior " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiShadingRate(real Size)
{
	m_implementation->m_stream << detail::indentation << "ShadingRate " << Size << "\n";
}

void stream::RiShadingInterpolation(const string& Type)
{
	m_implementation->m_stream << detail::indentation << "ShadingInterpolation " << format_string(Type) << "\n";
}

void stream::RiMatte(bool OnOff)
{
	m_implementation->m_stream << detail::indentation << "Matte " << OnOff << "\n";
}

void stream::RiBound(const boost::array<real, 6>& Bound)
{
	m_implementation->m_stream << detail::indentation << "Bound " << format_array(Bound.begin(), Bound.end()) << "\n";
}

void stream::RiDetail(const boost::array<real, 6>& Bound)
{
	m_implementation->m_stream << detail::indentation << "Detail " << format_array(Bound.begin(), Bound.end()) << "\n";
}

void stream::RiDetailRange(const real MinVis, const real LowTran, const real UpTran, const real MaxVis)
{
	m_implementation->m_stream << detail::indentation << "DetailRange " << MinVis << " " << LowTran << " " << UpTran << " " << MaxVis << "\n";
}

void stream::RiGeometricApproximation(const string& Type, real Value)
{
	m_implementation->m_stream << detail::indentation << "GeometricApproximation " << format_string(Type) << " " << Value << "\n";
}

void stream::RiGeometricRepresentation(const string& Type)
{
	m_implementation->m_stream << detail::indentation << "GeometricRepresentation " << format_string(Type) << "\n";
}

void stream::RiOrientation(const string& Orientation)
{
	m_implementation->m_stream << detail::indentation << "Orientation " << format_string(Orientation) << "\n";
}

void stream::RiReverseOrientation()
{
	m_implementation->m_stream << detail::indentation << "ReverseOrientation" << "\n";
}

void stream::RiSides(const unsigned_integer Sides)
{
	m_implementation->m_stream << detail::indentation << "Sides " << Sides << "\n";
}

void stream::RiIdentity()
{
	m_implementation->m_stream << detail::indentation << "Identity" << "\n";
}

void stream::RiTransform(const matrix& Transform)
{
	m_implementation->m_stream << detail::indentation << "Transform " << format_matrix(Transform) << "\n";
}

void stream::RiConcatTransform(const matrix& Transform)
{
	m_implementation->m_stream << detail::indentation << "ConcatTransform " << format_matrix(Transform) << "\n";
}

void stream::RiPerspective(real FieldOfView)
{
	m_implementation->m_stream << detail::indentation << "Perspective " << FieldOfView << "\n";
}

void stream::RiTranslate(real DX, real DY, real DZ)
{
	m_implementation->m_stream << detail::indentation << "Translate " << DX << " " << DY << " " << DZ << "\n";
}

void stream::RiRotate(real Angle, real DX, real DY, real DZ)
{
	m_implementation->m_stream << detail::indentation << "Rotate " << Angle << " " << DX << " " << DY << " " << DZ << "\n";
}

void stream::RiScale(real DX, real DY, real DZ)
{
	m_implementation->m_stream << detail::indentation << "Scale " << DX << " " << DY << " " << DZ << "\n";
}

void stream::RiSkew(real Angle, real DX1, real DY1, real DZ1, real DX2, real DY2, real DZ2)
{
	m_implementation->m_stream << detail::indentation << "Skew " << Angle << " " << DX1 << " " << DY1 << " " << DZ1 << " " << DX2 << " " << DY2 << " " << DZ2 << "\n";
}

void stream::RiDeformationV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Deformation " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiDisplacementV(const path& Path, const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Displacement " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiCoordinateSystem(const string& Space)
{
	m_implementation->m_stream << detail::indentation << "CoordinateSystem " << format_string(Space) << "\n";
}

void stream::RiCoordSysTransform(const string& Space)
{
	m_implementation->m_stream << detail::indentation << "CoordSysTransform " << format_string(Space) << "\n";
}

void stream::RiTransformBegin()
{
	m_implementation->m_stream << detail::indentation << "TransformBegin" << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiTransformEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "TransformEnd" << "\n";
}

void stream::RiAttributeV(const string& Name, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Attribute " << format_string(Name) << " " << Parameters << "\n";
}

void stream::RiPointsV(const unsigned_integer VertexCount, const parameter_list& Parameters)
{
	// Sanity checks ...
	return_if_fail(VertexCount);

	m_implementation->m_stream << detail::indentation << "Points " << Parameters << "\n";
}

void stream::RiPolygonV(const unsigned_integer VertexCount, const parameter_list& Parameters)
{
	// Sanity checks ...
	return_if_fail(VertexCount);

	m_implementation->m_stream << detail::indentation << "Polygon " << Parameters << "\n";
}

void stream::RiGeneralPolygonV(const unsigned_integers& VertexCounts, const parameter_list& Parameters)
{
	// Do some simple sanity checks ...
	return_if_fail(VertexCounts.size());

	m_implementation->m_stream << detail::indentation << "GeneralPolygon " << format_array(VertexCounts.begin(), VertexCounts.end()) << " " << Parameters << "\n";
}

void stream::RiPointsPolygonsV(const unsigned_integers& VertexCounts, const unsigned_integers& VertexIDs, const parameter_list& Parameters)
{
	// Sanity checks ...
	return_if_fail(VertexCounts.size());
	return_if_fail(VertexIDs.size() == std::accumulate(VertexCounts.begin(), VertexCounts.end(), 0UL));

	m_implementation->m_stream << detail::indentation << "PointsPolygons " << format_array(VertexCounts.begin(), VertexCounts.end()) << " " << format_array(VertexIDs.begin(), VertexIDs.end()) << " " << Parameters << "\n";
}

void stream::RiPointsGeneralPolygonsV(const unsigned_integers& LoopCounts, const unsigned_integers& VertexCounts, const unsigned_integers& VertexIDs, const parameter_list& Parameters)
{
	// Sanity checks ...
	return_if_fail(LoopCounts.size());
	return_if_fail(VertexCounts.size() == std::accumulate(LoopCounts.begin(), LoopCounts.end(), 0UL));
	return_if_fail(VertexIDs.size() == std::accumulate(VertexCounts.begin(), VertexCounts.end(), 0UL));

	m_implementation->m_stream << detail::indentation << "PointsGeneralPolygons " << format_array(LoopCounts.begin(), LoopCounts.end()) << " " << format_array(VertexCounts.begin(), VertexCounts.end()) << " " << format_array(VertexIDs.begin(), VertexIDs.end()) << " " << Parameters << "\n";
}

void stream::RiBasis(const matrix& UBasis, const unsigned_integer UStep, const matrix& VBasis, const unsigned_integer VStep)
{
	m_implementation->m_stream << detail::indentation << "Basis " << format_matrix(UBasis) << " " << UStep << " " << format_matrix(VBasis) << " " << VStep << "\n";
}

void stream::RiBasis(const string& UBasis, const unsigned_integer UStep, const string& VBasis, const unsigned_integer VStep)
{
	m_implementation->m_stream << detail::indentation << "Basis " << format_string(UBasis) << " " << UStep << " " << format_string(VBasis) << " " << VStep << "\n";
}

void stream::RiPatchV(const string& Type, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Patch " << format_string(Type) << " " << Parameters << "\n";
}

void stream::RiPatchMeshV(const string& Type, const unsigned_integer UCount, const string& UWrap, const unsigned_integer VCount, const string& VWrap, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "PatchMesh " << format_string(Type) << " " << UCount << " " << format_string(UWrap) << " " << VCount << " " << format_string(VWrap) << " " << Parameters << "\n";
}

void stream::RiNuPatchV(const unsigned_integer UCount, const unsigned_integer UOrder, const reals& UKnot, const real UMin, const real UMax, const unsigned_integer VCount, const unsigned_integer VOrder, const reals& VKnot, const real VMin, const real VMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "NuPatch " << UCount << " " << UOrder << " " << format_array(UKnot.begin(), UKnot.end()) << " " << UMin << " " << UMax << " " << VCount << " " << VOrder << " " << format_array(VKnot.begin(), VKnot.end()) << " " << VMin << " " << VMax << " " << Parameters << "\n";
}

void stream::RiTrimCurve(const unsigned_integers& CurveCounts, const unsigned_integers& Orders, const reals& Knots, const reals& Minimums, const reals& Maximums, const unsigned_integers& PointCounts, const reals& U, const reals& V, const reals& W)
{
	m_implementation->m_stream << detail::indentation << "TrimCurve " << " " << format_array(CurveCounts.begin(), CurveCounts.end()) << " " << format_array(Orders.begin(), Orders.end()) << " " << format_array(Knots.begin(), Knots.end()) << " " << format_array(Minimums.begin(), Minimums.end()) << " " << format_array(Maximums.begin(), Maximums.end()) << " " << format_array(PointCounts.begin(), PointCounts.end()) << " " << format_array(U.begin(), U.end()) << " " << format_array(V.begin(), V.end()) << " " << format_array(W.begin(), W.end()) << "\n";
}

void stream::RiSphereV(real Radius, real ZMin, real ZMax, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Sphere " << Radius << " " << ZMin << " " << ZMax << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiConeV(real Height, real Radius, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Cone " << Height << " " << Radius << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiCylinderV(real Radius, real ZMin, real ZMax, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Cylinder " << Radius << " " << ZMin << " " << ZMax << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiHyperboloidV(const point& Point1, const point& Point2, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Hyperboloid " << Point1 << " " << Point2 << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiParaboloidV(real RMax, real ZMin, real ZMax, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Paraboloid " << RMax << " " << ZMin << " " << ZMax << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiDiskV(real Height, real Radius, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Disk " << Height << " " << Radius << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiTorusV(real MajorRadius, real MinorRadius, real PhiMin, real PhiMax, real ThetaMax, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Torus " << MajorRadius << " " << MinorRadius << " " << PhiMin << " " << PhiMax << " " << ThetaMax << " " << Parameters << "\n";
}

void stream::RiCurvesV(const string& Type, const unsigned_integers& VertexCounts, const string& Wrap, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Curves " << format_string(Type) << " " << format_array(VertexCounts.begin(), VertexCounts.end()) << " " << format_string(Wrap) << " " << Parameters << "\n";
}

void stream::RiGeometryV(const string& Type, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Geometry " << format_string(Type) << " " << Parameters << "\n";
}

void stream::RiSolidBegin(const string& Type)
{
	m_implementation->m_stream << detail::indentation << "SolidBegin " << format_string(Type) << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiSolidEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "SolidEnd" << "\n";
}

const object_handle stream::RiObjectBegin()
{
	// Sanity checks ...
	if(m_implementation->m_object_block)
	{
		log() << error << "Cannot nest calls to RiObjectBegin()" << std::endl;
		return 0;
	}

	m_implementation->m_object_block = true;
	m_implementation->m_stream << detail::indentation << "ObjectBegin " << ++m_implementation->m_object_handle << "\n";
	detail::push_indent(m_implementation->m_stream);
	return m_implementation->m_object_handle;
}

void stream::RiObjectEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "ObjectEnd" << "\n";
	m_implementation->m_object_block = false;
}

void stream::RiObjectInstance(const object_handle Handle)
{
	m_implementation->m_stream << detail::indentation << "ObjectInstance " << Handle << "\n";
}

void stream::RiMotionBeginV(const sample_times_t& Times)
{
	// Sanity checks ...
	if(m_implementation->m_motion_block)
	{
		log() << error << "Cannot nest calls to RiMotionBegin()" << std::endl;
		return;
	}

	m_implementation->m_motion_block = true;
	m_implementation->m_stream << detail::indentation << "MotionBegin " << format_array(Times.begin(), Times.end()) << "\n";
	detail::push_indent(m_implementation->m_stream);
}

void stream::RiMotionEnd()
{
	detail::pop_indent(m_implementation->m_stream);
	m_implementation->m_stream << detail::indentation << "MotionEnd" << "\n";
	m_implementation->m_motion_block = false;
}

void stream::RiErrorHandler(const string& Style)
{
	m_implementation->m_stream << detail::indentation << "ErrorHandler " << format_string(Style) << "\n";
}

void stream::RiComment(const string& Comment)
{
	m_implementation->m_stream << detail::indentation << "#" << Comment << "\n";
}

void stream::RiNewline()
{
	m_implementation->m_stream << "\n";
}

void stream::RiReadArchive(const path& Archive)
{
	m_implementation->m_stream << detail::indentation << "ReadArchive " << format_string(Archive.native_filesystem_string()) << "\n";
}

void stream::RiProcDelayedReadArchive(const path& Archive, const bound& Bound)
{
	m_implementation->m_stream << detail::indentation << "Procedural " << format_string("DelayedReadArchive") << " [ " <<  format_string(Archive.native_filesystem_string()) << " ] [ " << Bound.nx << " " << Bound.px << " " << Bound.ny << " " << Bound.py << " " << Bound.nz << " " << Bound.pz << " ]\n";
}

void stream::RiStructure(const string& Structure)
{
	m_implementation->m_stream << "##" << Structure << "\n";
}

void stream::RiSubdivisionMeshV(const string& Scheme, const unsigned_integers& VertexCounts, const unsigned_integers& VertexIDs, const strings& Tags, const unsigned_integers& ArgCounts, const integers& IntegerArgs, const reals& FloatArgs, const parameter_list& Parameters)
{
	// Sanity checks ...
	return_if_fail(VertexIDs.size() == std::accumulate(VertexCounts.begin(), VertexCounts.end(), 0UL));

	m_implementation->m_stream << detail::indentation << "SubdivisionMesh " << format_string(Scheme) << " " << format_array(VertexCounts.begin(), VertexCounts.end()) << " " << format_array(VertexIDs.begin(), VertexIDs.end()) << " " << format_array(Tags.begin(), Tags.end()) << " " << format_array(ArgCounts.begin(), ArgCounts.end()) << " " << format_array(IntegerArgs.begin(), IntegerArgs.end()) << " " << format_array(FloatArgs.begin(), FloatArgs.end()) << " " << Parameters << "\n";
}

void stream::RiMakeCubeFaceEnvironmentV(const string& px, const string& nx, const string& py, const string& ny, const string& pz, const string& nz, const string& texturename, const real fov, const string& swrap, const string& twrap, const string& filterfunc, const real swidth, const real twidth, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "MakeCubeFaceEnvironment " << format_string(px) << " " << format_string(nx) << " " << format_string(py) << " " << format_string(ny) << " " << format_string(pz) << " " << format_string(nz) << " " << format_string(texturename) << " ";
	m_implementation->m_stream << fov << " " << format_string(swrap) << " " << format_string(twrap) << " " << format_string(filterfunc) << " " << swidth << " " << twidth << " " << Parameters;
}

void stream::RiMakeLatLongEnvironmentV(const string& picturename, const string& texturename, const string& filterfunc, const real swidth, const real twidth, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "MakeLatLongEnvironment " << format_string(picturename) << " " << format_string(texturename) << " " << format_string(filterfunc) << " " << swidth << " " << twidth << " " << Parameters;
}

void stream::RiMakeShadowV(const string& picturename, const string& texturename, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "MakeShadow " << format_string(picturename) << " " << format_string(texturename) << " " << Parameters;
}

void stream::RiMakeTextureV(const string& picturename, const string& texturename, const string& swrap, const string& twrap, const string& filterfunc, const real swidth, const real twidth, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "MakeTexture " << format_string(picturename) << " " << format_string(texturename) << " " << format_string(swrap) << " " << format_string(twrap) << " " << format_string(filterfunc) << " " << swidth << " " << twidth << " " << Parameters;
}

void stream::RiBlobbyV(const unsigned_integer NLeaf, const unsigned_integers& Codes, const reals& Floats, const strings& Strings, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "Blobby " << NLeaf << " " << format_array(Codes.begin(), Codes.end()) << " " << format_array(Floats.begin(), Floats.end()) << " " << format_array(Strings.begin(), Strings.end()) << " " << Parameters << "\n";
}

void stream::RiShaderLayerV(const std::string& type, const path& Path, const std::string& name, const std::string& layername, const parameter_list& Parameters)
{
	m_implementation->m_stream << detail::indentation << "ShaderLayer " << format_string(type) << " " << format_string(name) << " " << format_string(layername) << " " << Parameters << "\n";
}

void stream::RiConnectShaderLayers(const std::string& type, const std::string& layer1, const std::string& variable1, const std::string& layer2, const std::string& variable2)
{
	m_implementation->m_stream << detail::indentation << "ConnectShaderLayers " << format_string(type) << " " << format_string(layer1) << " " << format_string(variable1) << " " << format_string(layer2) << " " << format_string(variable2) << "\n";
}

} // namespace ri

} // namespace k3d


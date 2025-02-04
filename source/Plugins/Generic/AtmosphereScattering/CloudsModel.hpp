/*
See LICENSE file in root folder
*/
#ifndef ___C3DAS_CloudsModel_H___
#define ___C3DAS_CloudsModel_H___

#include "AtmosphereCameraUbo.hpp"
#include "AtmosphereModel.hpp"
#include "AtmosphereScatteringUbo.hpp"
#include "CloudsUbo.hpp"

#include <Castor3D/Shader/Shaders/SdwModule.hpp>

#include <ShaderWriter/BaseTypes/Array.hpp>
#include <ShaderWriter/BaseTypes/Int.hpp>
#include <ShaderWriter/BaseTypes/CombinedImage.hpp>
#include <ShaderWriter/CompositeTypes/Function.hpp>
#include <ShaderWriter/CompositeTypes/StructHelper.hpp>
#include <ShaderWriter/MatTypes/Mat4.hpp>
#include <ShaderWriter/VecTypes/Vec4.hpp>

#include <tuple>

namespace atmosphere_scattering
{
	struct CloudsModel
	{
		CloudsModel( sdw::ShaderWriter & writer
			, castor3d::shader::Utils & utils
			, AtmosphereModel & atmosphere
			, ScatteringModel & scattering
			, CloudsData const & clouds
			, uint32_t & binding
			, uint32_t set );
		sdw::RetVec4 applyClouds( castor3d::shader::Ray const & ray
			, sdw::Float const & objectId
			, sdw::Float const & linearDepth
			, sdw::IVec2 const & fragCoord
			, sdw::Vec3 & sunLuminance
			, sdw::Vec3 & skyLuminance
			, sdw::Float & skyBlendFactor );

	private:
		sdw::Vec2 getSphericalProjection( sdw::Vec3 const & p );
		sdw::RetFloat getHeightFraction( sdw::Vec3 const & inPos );
		sdw::RetVec3 skewSamplePointWithWind( sdw::Vec3 const & point
			, sdw::Float const & heightFraction );
		sdw::RetFloat getRelativeHeightInAtmosphere( sdw::Vec3 const & pos
			, sdw::Vec3 const & startPos
			, Ray const & ray );
		sdw::RetFloat getDensityHeightGradientForPoint( sdw::Float const & heightFraction
			, sdw::Float const & cloudType );
		sdw::RetFloat sampleLowFrequency( sdw::Vec2 const & unskewedUV
			, sdw::Vec2 const & skewedUV
			, sdw::Float const & heightFraction
			, sdw::Float const & lod );
		sdw::RetFloat erodeWithHighFrequency( sdw::Float const & baseDensity
			, sdw::Vec3 const & skewedSamplePoint
			, sdw::Vec2 const & skewedUV
			, sdw::Float const & heightFraction
			, sdw::Float const & lod );
		sdw::RetFloat sampleCloudDensity( sdw::Vec3 const & samplePoint
			, sdw::Boolean const & expensive
			, sdw::Float const & heightFraction
			, sdw::Float const & lod );
		sdw::RetFloat raymarchToLight( sdw::Vec3 const & viewDir
			, sdw::Vec3 const & pos
			, sdw::Float const & stepSize
			, sdw::Vec3 const & lightDir );
		sdw::RetVec4 raymarchToCloud( Ray const & ray
			, sdw::Vec3 const & startPos
			, sdw::Vec3 const & endPos
			, sdw::IVec2 const & fragCoord
			, sdw::Vec3 const & sunColor
			, sdw::Float & planetShadow );
		sdw::RetFloat computeFogAmount( sdw::Vec3 const & startPos
			, sdw::Vec3 const & wolrdPos
			, sdw::Float const & factor
			, sdw::Float const & viewHeight );
		sdw::RetFloat henyeyGreenstein( sdw::Float const & g
			, sdw::Float const & cosTheta );
		sdw::Float henyeyGreenstein( sdw::Float const & g
			, sdw::Float const & cosTheta
			, sdw::Float const & silverIntensity
			, sdw::Float const & silverSpread );
		sdw::RetFloat henyeyGreensteinPhase( sdw::Float const & g
			, sdw::Float const & cosTheta );
		sdw::RetVec4 computeLighting( Ray const & ray
			, sdw::Vec3 const & sunRadiance
			, sdw::Vec3 const & sunLuminance
			, sdw::Vec3 & skyLuminance
			, sdw::Float const & fadeOut
			, sdw::Float const & planetShadow
			, sdw::Vec4 const & rayMarchResult );
		sdw::Float getLightEnergy( sdw::Float cosTheta
			, sdw::Float const & coneDensity );

	private:
		sdw::ShaderWriter & writer;
		castor3d::shader::Utils & utils;
		AtmosphereModel & atmosphere;
		ScatteringModel & scattering;
		CloudsData const & clouds;
		sdw::CombinedImage3DRgba32 perlinWorleyNoiseMap;
		sdw::CombinedImage3DRgba32 worleyNoiseMap;
		sdw::CombinedImage2DRg32 curlNoiseMap;
		sdw::CombinedImage2DRg32 weatherMap;

	public:
		sdw::Float cloudsInnerRadius;
		sdw::Float cloudsOuterRadius;
		sdw::Float cloudsThickness;

	private:
		sdw::Function< sdw::Float
			, sdw::InVec3 > m_getHeightFraction;
		sdw::Function< sdw::Float
			, sdw::InVec3
			, sdw::InVec3
			, InRay > m_getRelativeHeightInAtmosphere;
		sdw::Function< sdw::Float
			, sdw::InFloat
			, sdw::InFloat > m_getDensityHeightGradientForPoint;
		sdw::Function< sdw::Vec3
			, sdw::InVec3
			, sdw::InFloat > m_skewSamplePointWithWind;
		sdw::Function< sdw::Float
			, sdw::InVec2
			, sdw::InVec2
			, sdw::InFloat
			, sdw::InFloat > m_sampleLowFrequency;
		sdw::Function< sdw::Float
			, sdw::InFloat
			, sdw::InVec3
			, sdw::InVec2
			, sdw::InFloat
			, sdw::InFloat > m_erodeWithHighFrequency;
		sdw::Function< sdw::Float
			, sdw::InVec3
			, sdw::InBoolean
			, sdw::InFloat
			, sdw::InFloat > m_sampleCloudDensity;
		sdw::Function< sdw::Float
			, sdw::InVec3
			, sdw::InVec3
			, sdw::InFloat
			, sdw::InVec3 > m_raymarchToLight;
		sdw::Function< sdw::Vec4
			, InRay
			, sdw::InVec3
			, sdw::InVec3
			, sdw::InIVec2
			, sdw::InVec3
			, sdw::OutFloat > m_raymarchToCloud;
		sdw::Function< sdw::Float
			, sdw::InVec3
			, sdw::InVec3
			, sdw::InFloat
			, sdw::InFloat > m_computeFogAmount;
		sdw::Function< sdw::Float
			, sdw::InFloat
			, sdw::InFloat > m_henyeyGreenstein;
		sdw::Function< sdw::Float
			, sdw::InFloat
			, sdw::InFloat > m_henyeyGreensteinPhase;
		sdw::Function< sdw::Vec4
			, InRay
			, sdw::InVec3
			, sdw::InVec3
			, sdw::InOutVec3
			, sdw::InFloat
			, sdw::InFloat
			, sdw::InVec4 > m_computeLighting;
		sdw::Function< sdw::Vec4
			, castor3d::shader::InRay
			, sdw::InFloat
			, sdw::InFloat
			, sdw::InIVec2
			, sdw::InOutVec3
			, sdw::InOutVec3
			, sdw::OutFloat > m_applyClouds;
	};
}

#endif

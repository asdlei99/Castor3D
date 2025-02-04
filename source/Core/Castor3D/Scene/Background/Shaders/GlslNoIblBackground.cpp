#include "Castor3D/Scene/Background/Shaders/GlslNoIblBackground.hpp"

#include "Castor3D/Render/RenderPipeline.hpp"
#include "Castor3D/Shader/Shaders/GlslBlendComponents.hpp"
#include "Castor3D/Shader/Shaders/GlslLight.hpp"
#include "Castor3D/Shader/Shaders/GlslLighting.hpp"
#include "Castor3D/Shader/Shaders/GlslUtils.hpp"

#include <ShaderWriter/Source.hpp>

namespace castor3d::shader
{
	castor::String const NoIblBackgroundModel::Name = cuT( "c3d.no_ibl" );

	NoIblBackgroundModel::NoIblBackgroundModel( sdw::ShaderWriter & writer
		, Utils & utils
		, VkExtent2D targetSize
		, uint32_t & binding
		, uint32_t set )
		: BackgroundModel{ writer, utils, std::move( targetSize ) }
	{
		m_writer.declCombinedImg< FImgCubeRgba32 >( "c3d_mapBackground"
			, binding++
			, set );
	}

	BackgroundModelPtr NoIblBackgroundModel::create( Engine const & engine
		, sdw::ShaderWriter & writer
		, Utils & utils
		, VkExtent2D targetSize
		, bool needsForeground
		, uint32_t & binding
		, uint32_t set )
	{
		return std::make_unique< NoIblBackgroundModel >( writer
			, utils
			, std::move( targetSize )
			, binding
			, set );
	}

	void NoIblBackgroundModel::computeReflections( sdw::Vec3 const & pwsNormal
		, sdw::Vec3 const & pdifF
		, sdw::Vec3 const & pspcF
		, sdw::Vec3 const & pV
		, sdw::Float const & pNdotV
		, BlendComponents & components
		, sdw::CombinedImage2DRgba32 const & pbrdf
		, sdw::Vec3 & preflectedDiffuse
		, sdw::Vec3 & preflectedSpecular )
	{
		if ( !m_computeReflections )
		{
			m_computeReflections = m_writer.implementFunction< sdw::Void >( "c3d_noiblbg_computeReflections"
				, [&]( sdw::Vec3 const & wsIncident
					, sdw::Vec3 const & wsNormal
					, sdw::CombinedImageCubeRgba32 const & backgroundMap
					, sdw::Vec3 const & specular
					, sdw::Float const & roughness
					, sdw::Vec3 reflectedDiffuse
					, sdw::Vec3 reflectedSpecular )
				{
					auto reflected = m_writer.declLocale( "reflected"
						, reflect( wsIncident, wsNormal ) );
					reflectedDiffuse = vec3( 0.0_f );
					reflectedSpecular = backgroundMap.lod( reflected, roughness * 8.0_f ).xyz() * specular;
				}
				, sdw::InVec3{ m_writer, "wsIncident" }
				, sdw::InVec3{ m_writer, "wsNormal" }
				, sdw::InCombinedImageCubeRgba32{ m_writer, "brdfMap" }
				, sdw::InVec3{ m_writer, "specular" }
				, sdw::InFloat{ m_writer, "roughness" }
				, sdw::OutVec3{ m_writer, "reflectedDiffuse" }
				, sdw::OutVec3{ m_writer, "reflectedSpecular" } );
		}

		auto backgroundMap = m_writer.getVariable< sdw::CombinedImageCubeRgba32 >( "c3d_mapBackground" );
		m_computeReflections( -pV
			, pwsNormal
			, backgroundMap
			, components.f0
			, components.roughness
			, preflectedDiffuse
			, preflectedSpecular );
	}

	sdw::RetVec3 NoIblBackgroundModel::computeRefractions( sdw::Vec3 const & pwsNormal
		, sdw::Vec3 const & pV
		, sdw::Float const & prefractionRatio
		, BlendComponents & components )
	{
		if ( !m_computeRefractions )
		{
			m_computeRefractions = m_writer.implementFunction< sdw::Vec3 >( "c3d_noiblbg_computeRefractions"
				, [&]( sdw::Vec3 const & wsIncident
					, sdw::Vec3 const & wsNormal
					, sdw::CombinedImageCubeRgba32 const & backgroundMap
					, sdw::Float const & refractionRatio
					, sdw::Vec3 albedo
					, sdw::Float const & roughness )
				{
					auto alb = m_writer.declLocale( "alb"
						, albedo );
					auto refracted = m_writer.declLocale( "refracted"
						, refract( wsIncident, wsNormal, refractionRatio ) );
					albedo = vec3( 0.0_f );
					m_writer.returnStmt( backgroundMap.lod( refracted, roughness * 8.0_f ).xyz()
						* alb );
				}
				, sdw::InVec3{ m_writer, "wsIncident" }
				, sdw::InVec3{ m_writer, "wsNormal" }
				, sdw::InCombinedImageCubeRgba32{ m_writer, "backgroundMap" }
				, sdw::InFloat{ m_writer, "refractionRatio" }
				, sdw::InOutVec3{ m_writer, "albedo" }
				, sdw::InFloat{ m_writer, "roughness" } );
		}

		auto backgroundMap = m_writer.getVariable< sdw::CombinedImageCubeRgba32 >( "c3d_mapBackground" );
		return m_computeRefractions( -pV
			, pwsNormal
			, backgroundMap
			, prefractionRatio
			, components.colour
			, components.roughness );
	}
}

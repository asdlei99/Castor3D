#include "AtmosphereScattering/AtmosphereBackgroundModel.hpp"

#include <ShaderWriter/Writer.hpp>
#include <ShaderWriter/BaseTypes/Float.hpp>
#include <ShaderWriter/Intrinsics/Intrinsics.hpp>

namespace atmosphere_scattering
{
	namespace model
	{
		template< typename DataT >
		DataT getData( sdw::UniformBuffer & ubo, std::string const & name )
		{
			auto result = ubo.declMember< DataT >( name );
			ubo.end();
			return result;
		}
	}

	castor::String const AtmosphereBackgroundModel::Name = cuT( "c3d.atmosphere" );

	AtmosphereBackgroundModel::AtmosphereBackgroundModel( sdw::ShaderWriter & writer
		, castor3d::shader::Utils & utils
		, VkExtent2D targetSize
		, uint32_t & binding
		, uint32_t set )
		: castor3d::shader::BackgroundModel{ writer, utils, std::move( targetSize ) }
		, cameraBuffer{ writer.declUniformBuffer<>( CameraUbo::Buffer
			, binding++
			, set ) }
		, cameraData{ model::getData< CameraData >( cameraBuffer, CameraUbo::Data ) }
		, atmosphereBuffer{ writer.declUniformBuffer<>( AtmosphereScatteringUbo::Buffer
			, binding++
			, set ) }
		, atmosphereData{ model::getData< AtmosphereData >( atmosphereBuffer, AtmosphereScatteringUbo::Data ) }
		, atmosphere{ m_writer
			, atmosphereData
			, { &cameraData, false, true, true, true }
			, { 256u, 64u } }
		, scattering{ m_writer
			, atmosphere
			, false, true, true, true, false
			, binding
			, set
			, false }
	{
	}

	castor3d::shader::BackgroundModelPtr AtmosphereBackgroundModel::create( sdw::ShaderWriter & writer
		, castor3d::shader::Utils & utils
		, VkExtent2D targetSize
		, uint32_t & binding
		, uint32_t set )
	{
		return std::make_unique< AtmosphereBackgroundModel >( writer
			, utils
			, std::move( targetSize )
			, binding
			, set );
	}

	sdw::RetVec3 AtmosphereBackgroundModel::computeReflections( sdw::Vec3 const & wsIncident
		, sdw::Vec3 const & wsNormal
		, castor3d::shader::LightMaterial const & material
		, sdw::CombinedImage2DRg32 const & brdf )
	{
		if ( !m_computeReflections )
		{
			m_computeReflections = m_writer.implementFunction< sdw::Vec3 >( "c3d_atmbg_computeReflections"
				, [&]()
				{
					m_writer.returnStmt( vec3( 0.0_f ) );
				} );
		}

		return m_computeReflections();
	}

	sdw::RetVec3 AtmosphereBackgroundModel::computeRefractions( sdw::Vec3 const & wsIncident
		, sdw::Vec3 const & wsNormal
		, sdw::Float const & refractionRatio
		, sdw::Vec3 const & transmission
		, castor3d::shader::LightMaterial const & material )
	{
		if ( !m_computeRefractions )
		{
			m_computeRefractions = m_writer.implementFunction< sdw::Vec3 >( "c3d_atmbg_computeRefractions"
				, [&]()
				{
					m_writer.returnStmt( vec3( 0.0_f ) );
				} );
		}

		return m_computeRefractions();
	}

	sdw::RetVoid AtmosphereBackgroundModel::mergeReflRefr( sdw::Vec3 const & wsIncident
		, sdw::Vec3 const & wsNormal
		, sdw::Float const & refractionRatio
		, sdw::Vec3 const & transmission
		, castor3d::shader::LightMaterial const & material
		, sdw::Vec3 & reflection
		, sdw::Vec3 & refraction )
	{
		if ( !m_mergeReflRefr )
		{
			m_mergeReflRefr = m_writer.implementFunction< sdw::Void >( "c3d_atmbg_mergeReflRefr"
				, [&]()
				{
				} );
		}

		return m_mergeReflRefr();
	}

	sdw::Vec3 AtmosphereBackgroundModel::getSunRadiance( sdw::Vec3 const & psunDir )
	{
		return scattering.getSunRadiance( psunDir );
	}
}

#include "Castor3D/Shader/Ubos/GpInfoUbo.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/UniformBufferPool.hpp"
#include "Castor3D/Render/RenderSystem.hpp"
#include "Castor3D/Scene/Camera.hpp"
#include "Castor3D/Shader/Shaders/GlslUtils.hpp"

#include <ShaderWriter/Source.hpp>

namespace castor3d
{
	//*********************************************************************************************

	namespace shader
	{
		GpInfoData::GpInfoData( sdw::ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled )
			: StructInstance{ writer, std::move( expr ), enabled }
			, m_mtxInvViewProj{ getMember< sdw::Mat4 >( "mtxInvViewProj" ) }
			, m_mtxInvView{ getMember< sdw::Mat4 >( "mtxInvView" ) }
			, m_mtxInvProj{ getMember< sdw::Mat4 >( "mtxInvProj" ) }
			, m_mtxGView{ getMember< sdw::Mat4 >( "mtxGView" ) }
			, m_mtxGProj{ getMember< sdw::Mat4 >( "mtxGProj" ) }
			, m_renderSize{ getMember< sdw::Vec2 >( "renderSize" ) }
		{
		}

		ast::type::BaseStructPtr GpInfoData::makeType( ast::type::TypesCache & cache )
		{
			auto result = cache.getStruct( ast::type::MemoryLayout::eStd140
				, "C3D_GPInfoData" );

			if ( result->empty() )
			{
				result->declMember( "mtxInvViewProj", ast::type::Kind::eMat4x4F );
				result->declMember( "mtxInvView", ast::type::Kind::eMat4x4F );
				result->declMember( "mtxInvProj", ast::type::Kind::eMat4x4F );
				result->declMember( "mtxGView", ast::type::Kind::eMat4x4F );
				result->declMember( "mtxGProj", ast::type::Kind::eMat4x4F );
				result->declMember( "renderSize", ast::type::Kind::eVec2F );
			}

			return result;
		}

		std::unique_ptr< sdw::Struct > GpInfoData::declare( sdw::ShaderWriter & writer )
		{
			return std::make_unique< sdw::Struct >( writer
				, makeType( writer.getTypesCache() ) );
		}

		sdw::Vec3 GpInfoData::readNormal( sdw::Vec3 const & input )const
		{
			return -( transpose( inverse( m_mtxGView ) ) * vec4( input, 1.0 ) ).xyz();
		}

		sdw::Vec3 GpInfoData::writeNormal( sdw::Vec3 const & input )const
		{
			return ( transpose( inverse( m_mtxInvView ) ) * vec4( -input, 1.0 ) ).xyz();
		}

		sdw::Vec3 GpInfoData::projToWorld( Utils & utils
			, sdw::Vec2 const & texCoord
			, sdw::Float const & depth )const
		{
			return utils.calcWSPosition( texCoord, depth, m_mtxInvViewProj );
		}

		sdw::Vec3 GpInfoData::projToView( Utils & utils
			, sdw::Vec2 const & texCoord
			, sdw::Float const & depth )const
		{
			return utils.calcVSPosition( texCoord, depth, m_mtxInvProj );
		}

		sdw::Vec2 GpInfoData::calcTexCoord( Utils & utils
			, sdw::Vec2 const & fragCoord )const
		{
			return utils.calcTexCoord( fragCoord, m_renderSize );
		}
	}

	//*********************************************************************************************

	GpInfoUbo::GpInfoUbo( RenderDevice const & device )
		: m_device{ device }
		, m_ubo{ m_device.uboPool->getBuffer< GpInfoUboConfiguration >( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) }
	{
	}

	GpInfoUbo::~GpInfoUbo()
	{
		m_device.uboPool->putBuffer< GpInfoUboConfiguration >( m_ubo );
	}

	void GpInfoUbo::cpuUpdate( castor::Size const & renderSize
		, Camera const & camera )
	{
		CU_Require( m_ubo );

		auto invView = camera.getView().getInverse();
		auto projection = camera.getProjection( true );
		auto invProj = projection.getInverse();
		auto invViewProj = ( projection * camera.getView() ).getInverse();

		auto & configuration = m_ubo.getData();
		configuration.invViewProj = invViewProj;
		configuration.invView = invView;
		configuration.invProj = invProj;
		configuration.gView = camera.getView();
		configuration.gProj = projection;
		configuration.renderSize = castor::Point2f( renderSize.getWidth(), renderSize.getHeight() );
	}

	//************************************************************************************************
}

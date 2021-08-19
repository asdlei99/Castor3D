#include "Castor3D/Render/EnvironmentMap/EnvironmentMapPass.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/UniformBufferPools.hpp"
#include "Castor3D/Material/Texture/TextureLayout.hpp"
#include "Castor3D/Material/Texture/TextureUnit.hpp"
#include "Castor3D/Material/Texture/TextureView.hpp"
#include "Castor3D/Render/RenderModule.hpp"
#include "Castor3D/Render/Viewport.hpp"
#include "Castor3D/Render/Culling/FrustumCuller.hpp"
#include "Castor3D/Render/EnvironmentMap/EnvironmentMap.hpp"
#include "Castor3D/Render/Passes/BackgroundPass.hpp"
#include "Castor3D/Render/Technique/ForwardRenderTechniquePass.hpp"
#include "Castor3D/Render/Ssao/SsaoConfig.hpp"
#include "Castor3D/Scene/Camera.hpp"
#include "Castor3D/Scene/Scene.hpp"
#include "Castor3D/Scene/SceneNode.hpp"
#include "Castor3D/Scene/Background/Background.hpp"

#include <CastorUtils/Graphics/RgbaColour.hpp>

#include <RenderGraph/RunnablePasses/GenerateMipmaps.hpp>

using namespace castor;

namespace castor3d
{
	namespace
	{
		CameraSPtr doCreateCamera( SceneNode & node
			, VkExtent3D const & size )
		{
			float const aspect = float( size.width ) / size.height;
			float const nearZ = 0.1f;
			float const farZ = 1000.0f;
			Viewport viewport{ *node.getScene()->getEngine() };
			viewport.setPerspective( 90.0_degrees
				, aspect
				, nearZ
				, farZ );
			viewport.resize( { size.width, size.height } );
			viewport.update();
			auto camera = std::make_shared< Camera >( cuT( "EnvironmentMap_" ) + node.getName()
				, *node.getScene()
				, node
				, std::move( viewport ) );
			camera->update();
			return camera;
		}
	}

	EnvironmentMapPass::EnvironmentMapPass( crg::FrameGraph & graph
		, RenderDevice const & device
		, EnvironmentMap & environmentMap
		, SceneNodeSPtr faceNode
		, uint32_t index
		, CubeMapFace face
		, SceneBackground & background )
		: OwnedBy< EnvironmentMap >{ environmentMap }
		, Named{ "Env" + castor::string::toString( index ) + "_" + castor::string::toString( uint32_t( face ) ) }
		, m_device{ device }
		, m_graph{ graph }
		, m_background{ background }
		, m_node{ faceNode }
		, m_index{ index }
		, m_face{ face }
		, m_camera{ doCreateCamera( *faceNode, getOwner()->getSize() ) }
		, m_culler{ std::make_unique< FrustumCuller >( *m_camera ) }
		, m_matrixUbo{ m_device }
		, m_hdrConfigUbo{ m_device }
		, m_sceneUbo{ m_device }
		, m_colourView{ environmentMap.getColourViewId( m_index, m_face ) }
		, m_backgroundRenderer{ castor::makeUnique< BackgroundRenderer >( m_graph
			, nullptr
			, m_device
			, getName()
			, m_background
			, m_hdrConfigUbo
			, m_sceneUbo
			, m_colourView ) }
		, m_opaquePassDesc{ &doCreateOpaquePass( &m_backgroundRenderer->getPass() ) }
		, m_transparentPassDesc{ &doCreateTransparentPass( m_opaquePassDesc ) }
	{
		doCreateGenMipmapsPass( m_transparentPassDesc );
		m_matrixUbo.cpuUpdate( m_camera->getView()
			, m_camera->getProjection( false ) );
		m_sceneUbo.setWindowSize( m_camera->getSize() );
		log::trace << "Created EnvironmentMapPass " << getName() << std::endl;
	}

	EnvironmentMapPass::~EnvironmentMapPass()
	{
		m_camera->getParent()->detach();
	}

	void EnvironmentMapPass::update( CpuUpdater & updater )
	{
		if ( !m_currentNode )
		{
			return;
		}

		auto position = m_currentNode->getDerivedPosition();
		m_camera->getParent()->setPosition( position );
		m_camera->getParent()->update();
		m_camera->update();
		m_culler->compute();
		updater.camera = m_camera;

		m_backgroundRenderer->update( updater );
		m_opaquePass->update( updater );
		m_transparentPass->update( updater );
		m_matrixUbo.cpuUpdate( m_camera->getView()
			, m_camera->getProjection( false ) );
		m_hdrConfigUbo.cpuUpdate( m_camera->getHdrConfig() );
	}

	void EnvironmentMapPass::update( GpuUpdater & updater )
	{
		if ( !m_currentNode )
		{
			return;
		}

		updater.camera = m_camera;

		RenderInfo info;
		m_sceneUbo.cpuUpdate( *m_camera->getScene(), m_camera.get() );
		m_backgroundRenderer->update( updater );
		m_opaquePass->update( updater );
		m_transparentPass->update( updater );
	}

	void EnvironmentMapPass::attachTo( SceneNode & node )
	{
		m_currentNode = &node;

		if ( m_opaquePass )
		{
			m_opaquePass->setIgnoredNode( node );
		}

		if ( m_transparentPass )
		{
			m_transparentPass->setIgnoredNode( node );
		}
	}

	crg::FramePass & EnvironmentMapPass::doCreateOpaquePass( crg::FramePass const * previousPass )
	{
		auto & result = m_graph.createPass( getName() + "OpaquePass"
			, [this]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				auto result = std::make_unique< ForwardRenderTechniquePass >( pass
					, context
					, graph
					, m_device
					, cuT( "EnvironmentMap" )
					, getName() + cuT( "Opaque" )
					, SceneRenderPassDesc{ getOwner()->getSize(), m_matrixUbo, *m_culler }
					, RenderTechniquePassDesc{ true, SsaoConfig{} } );
				m_node->getScene()->getEngine()->registerTimer( "EnvironmentMap" + std::to_string( m_index )
					, result->getTimer() );
				m_opaquePass = result.get();
				return result;
			} );
		result.addDependency( *previousPass );
		result.addOutputDepthView( getOwner()->getDepthViewId( m_index, m_face )
			, defaultClearDepthStencil );
		result.addInOutColourView( m_colourView );
		return result;
	}

	crg::FramePass & EnvironmentMapPass::doCreateTransparentPass( crg::FramePass const * previousPass )
	{
		auto & result = m_graph.createPass( getName() + "TransparentPass"
			, [this]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				auto result = std::make_unique< ForwardRenderTechniquePass >( pass
					, context
					, graph
					, m_device
					, cuT( "EnvironmentMap" )
					, getName() + cuT( "Transparent" )
					, SceneRenderPassDesc{ getOwner()->getSize(), m_matrixUbo, *m_culler, false }
					, RenderTechniquePassDesc{ true, SsaoConfig{} } );
				m_node->getScene()->getEngine()->registerTimer( "EnvironmentMap" + std::to_string( m_index )
					, result->getTimer() );
				m_transparentPass = result.get();
				return result;
			} );
		result.addDependency( *previousPass );
		result.addInputDepthView( getOwner()->getDepthViewId( m_index, m_face ) );
		result.addInOutColourView( m_colourView );
		return result;
	}

	void EnvironmentMapPass::doCreateGenMipmapsPass( crg::FramePass const * previousPass )
	{
		auto & mipsGen = m_graph.createPass( getName() + "GenMips"
			, [this]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
			{
				auto result = std::make_unique< crg::GenerateMipmaps >( pass
					, context
					, graph
					, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
				m_node->getScene()->getEngine()->registerTimer( "EnvironmentMap" + std::to_string( m_index )
					, result->getTimer() );
				return result;
			} );
		mipsGen.addDependency( *previousPass );
		mipsGen.addTransferInOutView( m_colourView );
	}
}

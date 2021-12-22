#include "FFTOceanRendering/OceanFFTRenderPass.hpp"

#include "FFTOceanRendering/OceanFFT.hpp"

#include <Castor3D/Engine.hpp>
#include <Castor3D/Cache/ShaderCache.hpp>
#include <Castor3D/Material/Pass/Pass.hpp>
#include <Castor3D/Render/RenderPipeline.hpp>
#include <Castor3D/Render/RenderQueue.hpp>
#include <Castor3D/Render/RenderSystem.hpp>
#include <Castor3D/Render/RenderTarget.hpp>
#include <Castor3D/Render/EnvironmentMap/EnvironmentMap.hpp>
#include <Castor3D/Render/GlobalIllumination/LightPropagationVolumes/LightVolumePassResult.hpp>
#include <Castor3D/Render/Node/BillboardRenderNode.hpp>
#include <Castor3D/Render/Node/SubmeshRenderNode.hpp>
#include <Castor3D/Render/Node/QueueCulledRenderNodes.hpp>
#include <Castor3D/Render/Technique/RenderTechnique.hpp>
#include <Castor3D/Render/Technique/RenderTechniqueVisitor.hpp>
#include <Castor3D/Scene/Camera.hpp>
#include <Castor3D/Scene/Scene.hpp>
#include <Castor3D/Scene/Background/Background.hpp>
#include <Castor3D/Shader/Program.hpp>
#include <Castor3D/Shader/Shaders/GlslCookTorranceBRDF.hpp>
#include <Castor3D/Shader/Shaders/GlslFog.hpp>
#include <Castor3D/Shader/Shaders/GlslGlobalIllumination.hpp>
#include <Castor3D/Shader/Shaders/GlslMaterial.hpp>
#include <Castor3D/Shader/Shaders/GlslOutputComponents.hpp>
#include <Castor3D/Shader/Shaders/GlslReflection.hpp>
#include <Castor3D/Shader/Shaders/GlslSurface.hpp>
#include <Castor3D/Shader/Shaders/GlslTextureAnimation.hpp>
#include <Castor3D/Shader/Shaders/GlslTextureConfiguration.hpp>
#include <Castor3D/Shader/Shaders/GlslUtils.hpp>
#include <Castor3D/Shader/Ubos/BillboardUbo.hpp>
#include <Castor3D/Shader/Ubos/MatrixUbo.hpp>
#include <Castor3D/Shader/Ubos/ModelUbo.hpp>
#include <Castor3D/Shader/Ubos/MorphingUbo.hpp>
#include <Castor3D/Shader/Ubos/SceneUbo.hpp>

#include <ShaderWriter/Source.hpp>

#include <RenderGraph/RunnablePasses/ImageCopy.hpp>

#include <ashespp/Image/StagingTexture.hpp>

#define Ocean_DebugPixelShader 0
#define Ocean_DebugFFTGraph 0

namespace ocean_fft
{
	namespace
	{
#if Ocean_DebugFFTGraph
		class DummyRunnable
			: public crg::RunnablePass
		{
		public:
			DummyRunnable( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & graph )
				: crg::RunnablePass{ pass
					, context
					, graph
					, { crg::RunnablePass::InitialiseCallback( [this](){ doInitialise(); } )
						, crg::RunnablePass::GetSemaphoreWaitFlagsCallback( [this](){ return doGetSemaphoreWaitFlags(); } ) } }
			{
			}

		private:
			void doInitialise()
			{
			}

			VkPipelineStageFlags doGetSemaphoreWaitFlags()const
			{
				return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
		};

		void createFakeCombinePass( castor::String const & name
			, crg::FrameGraph & graph
			, crg::FramePassArray previousPasses
			, castor3d::Texture const & a
			, castor3d::Texture const & b
			, castor3d::Texture const & c )
		{
			auto & result = graph.createPass( name + "/FakeCombine"
				, []( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runnableGraph )
			{
				return std::make_unique< DummyRunnable >( framePass
					, context
					, runnableGraph );
			} );

			result.addDependencies( previousPasses );
			result.addImplicitColourView( a.sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitColourView( b.sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitColourView( c.sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		}
#endif

		template< ast::var::Flag FlagT >
		struct PatchT
			: public sdw::StructInstance
		{
			PatchT( sdw::ShaderWriter & writer
				, ast::expr::ExprPtr expr
				, bool enabled )
				: StructInstance{ writer, std::move( expr ), enabled }
				, patchPosBase{ getMember< sdw::Vec3 >( "patchPosBaseWorldPos" ) }
				, patchLods{ getMember< sdw::Vec4 >( "patchLodsGradNormTex" ) }
				, material{ getMember< sdw::UInt >( "material" ) }
				, worldPosition{ patchPosBase }
				, gradNormalTexcoord{ patchLods }
			{
			}

			SDW_DeclStructInstance( , PatchT );

			static ast::type::IOStructPtr makeIOType( ast::type::TypesCache & cache )
			{
				auto result = cache.getIOStruct( ast::type::MemoryLayout::eC
					, "C3DORFFT_" + ( FlagT == sdw::var::Flag::eShaderOutput
						? std::string{ "Output" }
						: std::string{ "Input" } ) + "Patch"
					, FlagT );

				if ( result->empty() )
				{
					uint32_t index = 0u;
					result->declMember( "patchPosBaseWorldPos"
						, ast::type::Kind::eVec3F
						, ast::type::NotArray
						, index++ );
					result->declMember( "patchLodsGradNormTex"
						, ast::type::Kind::eVec4F
						, ast::type::NotArray
						, index++ );
					result->declMember( "material"
						, ast::type::Kind::eUInt
						, ast::type::NotArray
						, index++ );
				}

				return result;
			}

			sdw::Vec3 patchPosBase;
			sdw::Vec4 patchLods;
			sdw::UInt material;
			sdw::Vec3 worldPosition;
			sdw::Vec4 gradNormalTexcoord;

		private:
			using sdw::StructInstance::getMember;
			using sdw::StructInstance::getMemberArray;
		};

		enum WaveIdx : uint32_t
		{
			eLightBuffer = uint32_t( castor3d::PassUboIdx::eCount ),
			eOceanUbo,
			eHeightDisplacement,
			eGradientJacobian,
			eNormals,
			eSceneNormals,
			eSceneDepth,
			eSceneResult,
		};

		void bindTexture( VkImageView view
			, VkSampler sampler
			, ashes::WriteDescriptorSetArray & writes
			, uint32_t & index )
		{
			writes.push_back( { index++
				, 0u
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, { { sampler
				, view
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } } } );
		}

		crg::FramePass const & createCopyPass( castor::String const & name
			, castor3d::RenderDevice const & device
			, crg::FrameGraph & graph
			, crg::FramePassArray const & previousPasses
			, crg::ImageViewId input
			, crg::ImageViewId output
			, std::shared_ptr< IsRenderPassEnabled > isEnabled )
		{
			auto extent = getExtent( input );
			auto & result = graph.createPass( name + "Copy"
				, [name, extent, isEnabled, &device]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runnableGraph )
				{
						auto result = std::make_unique< crg::ImageCopy >( framePass
							, context
							, runnableGraph
							, extent
							, crg::ru::Config{}
							, crg::RunnablePass::GetPassIndexCallback( [](){ return 0u; } )
							, crg::RunnablePass::IsEnabledCallback( [isEnabled](){ return ( *isEnabled )(); } ) );
						device.renderSystem.getEngine()->registerTimer( runnableGraph.getName() + "/" + framePass.name
							, result->getTimer() );
						return result;
				} );
			result.addDependencies( previousPasses );
			result.addTransferInputView( input );
			result.addTransferOutputView( output );
			return result;
		}

		crg::FramePassArray createNodesPass( castor::String const & name
			, castor3d::RenderDevice const & device
			, crg::FrameGraph & graph
			, castor3d::RenderTechnique & technique
			, castor3d::TechniquePasses & renderPasses
			, crg::FramePassArray previousPasses
			, std::shared_ptr< OceanUbo > oceanUbo
			, std::shared_ptr< OceanFFT > oceanFFT
			, castor3d::TexturePtr colourInput
			, castor3d::TexturePtr depthInput
			, std::shared_ptr< IsRenderPassEnabled > isEnabled )
		{
			auto extent = getExtent( technique.getResultImg() );
			auto & result = graph.createPass( name +"/NodesPass"
				, [name, extent, colourInput, depthInput, oceanUbo, oceanFFT, isEnabled, &device, &technique, &renderPasses]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runnableGraph )
			{
					auto res = std::make_unique< OceanRenderPass >( &technique
						, framePass
						, context
						, runnableGraph
						, device
						, name
						, std::move( oceanUbo )
						, std::move( oceanFFT )
						, std::move( colourInput )
						, std::move( depthInput )
						, castor3d::SceneRenderPassDesc{ extent
							, technique.getMatrixUbo()
							, technique.getRenderTarget().getCuller() }.safeBand( true )
						, castor3d::RenderTechniquePassDesc{ false, technique.getSsaoConfig() }
							.ssao( technique.getSsaoResult() )
							.lpvConfigUbo( technique.getLpvConfigUbo() )
							.llpvConfigUbo( technique.getLlpvConfigUbo() )
							.vctConfigUbo( technique.getVctConfigUbo() )
							.lpvResult( technique.getLpvResult() )
							.llpvResult( technique.getLlpvResult() )
							.vctFirstBounce( technique.getFirstVctBounce() )
							.vctSecondaryBounce( technique.getSecondaryVctBounce() )
						, isEnabled );
					renderPasses[size_t( OceanRenderPass::Event )].push_back( res.get() );
					device.renderSystem.getEngine()->registerTimer( runnableGraph.getName() + "/" + name
						, res->getTimer() );
					return res;
			} );

			result.addDependencies( previousPasses );
#if Ocean_DebugFFTGraph
			result.addImplicitColourView( colourInput->sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitDepthView( depthInput->sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addInOutDepthStencilView( technique.getDepthTargetView() );
			result.addInOutColourView( technique.getResultTargetView() );
#else
			result.addDependencies( oceanFFT->getLastPasses() );
			result.addImplicitColourView( colourInput->sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitDepthView( depthInput->sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitColourView( oceanFFT->getNormals().sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitColourView( oceanFFT->getHeightDisplacement().sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addImplicitColourView( oceanFFT->getGradientJacobian().sampledViewId
				, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
			result.addInOutDepthStencilView( technique.getDepthTargetView() );
			result.addInOutColourView( technique.getResultTargetView() );
#endif
			return { &result };
		}

		static uint32_t constexpr OutputVertices = 1u;
	}

	castor::String const OceanRenderPass::Type = cuT( "c3d.fft_ocean" );
	castor::String const OceanRenderPass::FullName = cuT( "FFT Ocean rendering" );
	castor::String const OceanRenderPass::Param = cuT( "OceanConfig" );
	castor::String const OceanRenderPass::ParamFFT = cuT( "OceanFFTConfig" );

	OceanRenderPass::OceanRenderPass( castor3d::RenderTechnique * parent
		, crg::FramePass const & pass
		, crg::GraphContext & context
		, crg::RunnableGraph & graph
		, castor3d::RenderDevice const & device
		, castor::String const & category
		, std::shared_ptr< OceanUbo > oceanUbo
		, std::shared_ptr< OceanFFT > oceanFFT
		, std::shared_ptr< castor3d::Texture > colourInput
		, std::shared_ptr< castor3d::Texture > depthInput
		, castor3d::SceneRenderPassDesc const & renderPassDesc
		, castor3d::RenderTechniquePassDesc const & techniquePassDesc
			, std::shared_ptr< IsRenderPassEnabled > isEnabled )
		: castor3d::RenderTechniquePass{ parent
			, pass
			, context
			, graph
			, device
			, Type
			, category
			, "Wave"
			, renderPassDesc
			, techniquePassDesc }
		, m_isEnabled{ isEnabled }
		, m_ubo{ oceanUbo }
		, m_oceanFFT{ std::move( oceanFFT ) }
		, m_colourInput{ std::move( colourInput ) }
		, m_depthInput{ std::move( depthInput ) }
		, m_linearWrapSampler{ device->createSampler( getName()
			, VK_SAMPLER_ADDRESS_MODE_REPEAT
			, VK_SAMPLER_ADDRESS_MODE_REPEAT
			, VK_SAMPLER_ADDRESS_MODE_REPEAT
			, VK_FILTER_LINEAR
			, VK_FILTER_LINEAR
			, VK_SAMPLER_MIPMAP_MODE_LINEAR ) }
		, m_pointClampSampler{ device->createSampler( getName()
			, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, VK_FILTER_NEAREST
			, VK_FILTER_NEAREST
			, VK_SAMPLER_MIPMAP_MODE_NEAREST ) }
	{
		m_isEnabled->setPass( *this );
		auto params = getEngine()->getRenderPassTypeConfiguration( Type );

		if ( params.get( Param, m_configuration ) )
		{
			auto & data = m_ubo->getUbo().getData();
			data = m_configuration;
		}
	}

	OceanRenderPass::~OceanRenderPass()
	{
		m_colourInput->destroy();
		m_colourInput.reset();
		m_depthInput->destroy();
		m_depthInput.reset();
	}

	crg::FramePassArray OceanRenderPass::create( castor3d::RenderDevice const & device
		, castor3d::RenderTechnique & technique
		, castor3d::TechniquePasses & renderPasses
		, crg::FramePassArray previousPasses )
	{
		auto isEnabled = std::make_shared< IsRenderPassEnabled >();
		auto & graph = technique.getRenderTarget().getGraph();
		auto extent = getExtent( technique.getResultImg() );
		auto colourInput = std::make_shared< castor3d::Texture >( device
			, graph.getHandler()
			, OceanFFT::Name + "/Colour"
			, 0u
			, extent
			, 1u
			, 1u
			, technique.getResultImg().data->info.format
			, ( VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT ) );
		colourInput->create();
		crg::FramePassArray passes;
		passes.push_back( &createCopyPass( OceanFFT::Name + "/Colour"
			, device
			, graph
			, previousPasses
			, technique.getResultImgView()
			, colourInput->sampledViewId
			, isEnabled ) );

		auto depthInput = std::make_shared< castor3d::Texture >( device
			, graph.getHandler()
			, OceanFFT::Name + "Depth"
			, 0u
			, extent
			, 1u
			, 1u
			, technique.getDepthImg().data->info.format
			, ( VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT ) );
		depthInput->create();
		passes.push_back( &createCopyPass( OceanFFT::Name + "/Depth"
			, device
			, graph
			, previousPasses
			, technique.getDepthWholeView()
			, depthInput->wholeViewId
			, isEnabled ) );

		auto oceanUbo = std::make_shared< OceanUbo >( device );
#if Ocean_DebugFFTGraph
		crg::FrameGraph fftGraph{ graph.getHandler(), OceanFFT::Name };
		auto oceanFFT = std::make_shared< OceanFFT >( device
			, fftGraph
			, previousPasses
			, *oceanUbo );
		createFakeCombinePass( OceanFFT::Name
			, fftGraph
			, oceanFFT->getLastPasses()
			, oceanFFT->getGradientJacobian()
			, oceanFFT->getHeightDisplacement()
			, oceanFFT->getNormals() );
		auto runnable = fftGraph.compile( device.makeContext() );
		runnable->record();
#else
		auto oceanFFT = std::make_shared< OceanFFT >( device
			, graph
			, previousPasses
			, *oceanUbo
			, isEnabled );
#endif

		return createNodesPass( OceanFFT::Name
			, device
			, graph
			, technique
			, renderPasses
			, std::move( passes )
			, std::move( oceanUbo )
			, std::move( oceanFFT )
			, std::move( colourInput )
			, std::move( depthInput )
			, isEnabled );
	}

	castor3d::TextureFlags OceanRenderPass::getTexturesMask()const
	{
		return castor3d::TextureFlags{ castor3d::TextureFlag::eAll };
	}

	void OceanRenderPass::accept( castor3d::RenderTechniqueVisitor & visitor )
	{
		if ( visitor.getFlags().renderPassType == m_typeID )
		{
#if Ocean_Debug
			visitor.visit( cuT( "Debug" )
				, m_configuration.debug
				, getOceanDisplayDataNames()
				, nullptr );
#endif
			visitor.visit( cuT( "Refraction ratio" )
				, m_configuration.refractionRatio
				, nullptr );
			visitor.visit( cuT( "Refraction distortion factor" )
				, m_configuration.refractionDistortionFactor
				, nullptr );
			visitor.visit( cuT( "Refraction height factor" )
				, m_configuration.refractionHeightFactor
				, nullptr );
			visitor.visit( cuT( "Refraction distance factor" )
				, m_configuration.refractionDistanceFactor
				, nullptr );
			visitor.visit( cuT( "Depth softening distance" )
				, m_configuration.depthSofteningDistance
				, nullptr );
			visitor.visit( cuT( "SSR settings" )
				, m_configuration.ssrSettings
				, nullptr );

			m_oceanFFT->accept( visitor );
		}
	}

	void OceanRenderPass::doUpdateUbos( castor3d::CpuUpdater & updater )
	{
		auto tslf = updater.tslf > 0_ms
			? updater.tslf
			: std::chrono::duration_cast< castor::Milliseconds >( m_timer.getElapsed() );
		m_configuration.time += float( tslf.count() ) / 1000.0f;
		m_ubo->cpuUpdate( m_configuration
			, m_oceanFFT->getConfig()
			, updater.camera->getParent()->getDerivedPosition() );
		RenderTechniquePass::doUpdateUbos( updater );
	}

	bool OceanRenderPass::doIsValidPass( castor3d::Pass const & pass )const
	{
		return pass.getRenderPassInfo()
			&& pass.getRenderPassInfo()->name == Type
			&& doAreValidPassFlags( pass.getPassFlags() );
	}

	void OceanRenderPass::doFillAdditionalBindings( castor3d::PipelineFlags const & flags
		, ashes::VkDescriptorSetLayoutBindingArray & bindings )const
	{
		bindings.emplace_back( getCuller().getScene().getLightCache().createLayoutBinding( WaveIdx::eLightBuffer ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eOceanUbo
			, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			, ( VK_SHADER_STAGE_VERTEX_BIT
				| VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
				| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
				| VK_SHADER_STAGE_FRAGMENT_BIT ) ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eHeightDisplacement
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, ( VK_SHADER_STAGE_FRAGMENT_BIT
				| VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT ) ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eGradientJacobian
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eNormals
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eSceneNormals
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eSceneDepth
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( WaveIdx::eSceneResult
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, VK_SHADER_STAGE_FRAGMENT_BIT ) );

		auto index = uint32_t( WaveIdx::eSceneResult ) + 1u;

		for ( uint32_t j = 0u; j < uint32_t( castor3d::LightType::eCount ); ++j )
		{
			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag( uint8_t( castor3d::SceneFlag::eShadowBegin ) << j ) ) )
			{
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// light type shadow depth
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// light type shadow variance
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			}
		}

		if ( checkFlag( flags.passFlags, castor3d::PassFlag::eReflection )
			|| checkFlag( flags.passFlags, castor3d::PassFlag::eRefraction ) )
		{
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// Env map
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		}

		if ( checkFlag( flags.passFlags, castor3d::PassFlag::eImageBasedLighting ) )
		{
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_mapBrdf
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_mapIrradiance
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_mapPrefiltered
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		}

		if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eVoxelConeTracing ) )
		{
			CU_Require( m_vctConfigUbo );
			CU_Require( m_vctFirstBounce );
			CU_Require( m_vctSecondaryBounce );
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// Voxel UBO
				, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_mapVoxelsFirstBounce
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_mapVoxelsSecondaryBounce
				, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
				, VK_SHADER_STAGE_FRAGMENT_BIT ) );
		}
		else
		{
			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLpvGI ) )
			{
				CU_Require( m_lpvConfigUbo );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// LPV Grid UBO
					, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			}

			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLayeredLpvGI ) )
			{
				CU_Require( m_llpvConfigUbo );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// LLPV Grid UBO
					, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			}

			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLpvGI ) )
			{
				CU_Require( m_lpvResult );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationR
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationG
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
				bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationB
					, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
					, VK_SHADER_STAGE_FRAGMENT_BIT ) );
			}

			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLayeredLpvGI ) )
			{
				CU_Require( m_llpvResult );

				for ( size_t i = 0u; i < m_llpvResult->size(); ++i )
				{
					bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationRn
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, VK_SHADER_STAGE_FRAGMENT_BIT ) );
					bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationGn
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, VK_SHADER_STAGE_FRAGMENT_BIT ) );
					bindings.emplace_back( castor3d::makeDescriptorSetLayoutBinding( index++	// c3d_lpvAccumulationBn
						, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
						, VK_SHADER_STAGE_FRAGMENT_BIT ) );
				}
			}
		}
	}

	ashes::PipelineDepthStencilStateCreateInfo OceanRenderPass::doCreateDepthStencilState( castor3d::PipelineFlags const & flags )const
	{
		return ashes::PipelineDepthStencilStateCreateInfo
		{
			0u,
			VK_TRUE,
			VK_TRUE,
		};
	}

	ashes::PipelineColorBlendStateCreateInfo OceanRenderPass::doCreateBlendState( castor3d::PipelineFlags const & flags )const
	{
		ashes::VkPipelineColorBlendAttachmentStateArray attachments
		{
			VkPipelineColorBlendAttachmentState
			{
				VK_TRUE,
				VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				castor3d::defaultColorWriteMask,
			},
		};
		return ashes::PipelineColorBlendStateCreateInfo
		{
			0u,
			VK_FALSE,
			VK_LOGIC_OP_COPY,
			std::move( attachments ),
		};
	}

	namespace
	{
		void fillAdditionalDescriptor( crg::RunnableGraph & graph
			, castor3d::RenderPipeline const & pipeline
			, ashes::WriteDescriptorSetArray & descriptorWrites
			, castor3d::Scene const & scene
			, castor3d::SceneNode const & sceneNode
			, castor3d::ShadowMapLightTypeArray const & shadowMaps
			, castor3d::LpvGridConfigUbo const * lpvConfigUbo
			, castor3d::LayeredLpvGridConfigUbo const * llpvConfigUbo
			, castor3d::VoxelizerUbo const * vctConfigUbo
			, castor3d::LightVolumePassResult const * lpvResult
			, castor3d::LightVolumePassResultArray const * llpvResult
			, castor3d::Texture const * vctFirstBounce
			, castor3d::Texture const * vctSecondaryBounce
			, OceanUbo const & OceanUbo
			, VkSampler linearWrapSampler
			, VkSampler pointClampSampler
			, VkImageView const & heightDisplView
			, VkImageView const & gradJacobView
			, VkImageView const & normals
			, VkImageView const & sceneNormals
			, VkImageView const & sceneDepth
			, VkImageView const & sceneColour )
		{
			auto & flags = pipeline.getFlags();
			descriptorWrites.push_back( scene.getLightCache().getBinding( WaveIdx::eLightBuffer ) );
			descriptorWrites.push_back( OceanUbo.getDescriptorWrite( WaveIdx::eOceanUbo ) );
			auto index = uint32_t( WaveIdx::eHeightDisplacement );
			bindTexture( heightDisplView, linearWrapSampler, descriptorWrites, index );
			bindTexture( gradJacobView, linearWrapSampler, descriptorWrites, index );
			bindTexture( normals, linearWrapSampler, descriptorWrites, index );
			bindTexture( sceneNormals, pointClampSampler, descriptorWrites, index );
			bindTexture( sceneDepth, pointClampSampler, descriptorWrites, index );
			bindTexture( sceneColour, pointClampSampler, descriptorWrites, index );

			castor3d::bindShadowMaps( graph
				, pipeline.getFlags()
				, shadowMaps
				, descriptorWrites
				, index );

			if ( checkFlag( flags.passFlags, castor3d::PassFlag::eReflection )
				|| checkFlag( flags.passFlags, castor3d::PassFlag::eRefraction ) )
			{
				auto & envMap = scene.getEnvironmentMap();
				auto envMapIndex = scene.getEnvironmentMapIndex( sceneNode );
				bindTexture( envMap.getColourView( envMapIndex )
					, *envMap.getColourId().sampler
					, descriptorWrites
					, index );
			}

			if ( checkFlag( flags.passFlags, castor3d::PassFlag::eImageBasedLighting ) )
			{
				auto & background = *scene.getBackground();

				if ( background.hasIbl() )
				{
					auto & ibl = background.getIbl();
					bindTexture( ibl.getPrefilteredBrdfTexture().wholeView
						, ibl.getPrefilteredBrdfSampler()
						, descriptorWrites
						, index );
					bindTexture( ibl.getIrradianceTexture().wholeView
						, ibl.getIrradianceSampler()
						, descriptorWrites
						, index );
					bindTexture( ibl.getPrefilteredEnvironmentTexture().wholeView
						, ibl.getPrefilteredEnvironmentSampler()
						, descriptorWrites
						, index );
				}
			}

			if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eVoxelConeTracing ) )
			{
				CU_Require( vctConfigUbo );
				CU_Require( vctFirstBounce );
				CU_Require( vctSecondaryBounce );
				descriptorWrites.push_back( vctConfigUbo->getDescriptorWrite( index++ ) );
				bindTexture( vctFirstBounce->wholeView
					, *vctFirstBounce->sampler
					, descriptorWrites
					, index );
				bindTexture( vctSecondaryBounce->wholeView
					, *vctSecondaryBounce->sampler
					, descriptorWrites
					, index );
			}
			else
			{
				if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLpvGI ) )
				{
					CU_Require( lpvConfigUbo );
					descriptorWrites.push_back( lpvConfigUbo->getDescriptorWrite( index++ ) );
				}

				if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLayeredLpvGI ) )
				{
					CU_Require( llpvConfigUbo );
					descriptorWrites.push_back( llpvConfigUbo->getDescriptorWrite( index++ ) );
				}

				if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLpvGI ) )
				{
					CU_Require( lpvResult );
					auto & lpv = *lpvResult;
					bindTexture( lpv[castor3d::LpvTexture::eR].wholeView
						, *lpv[castor3d::LpvTexture::eR].sampler
						, descriptorWrites
						, index );
					bindTexture( lpv[castor3d::LpvTexture::eG].wholeView
						, *lpv[castor3d::LpvTexture::eG].sampler
						, descriptorWrites
						, index );
					bindTexture( lpv[castor3d::LpvTexture::eB].wholeView
						, *lpv[castor3d::LpvTexture::eB].sampler
						, descriptorWrites
						, index );
				}

				if ( checkFlag( flags.sceneFlags, castor3d::SceneFlag::eLayeredLpvGI ) )
				{
					CU_Require( llpvResult );

					for ( auto & plpv : *llpvResult )
					{
						auto & lpv = *plpv;
						bindTexture( lpv[castor3d::LpvTexture::eR].wholeView
							, *lpv[castor3d::LpvTexture::eR].sampler
							, descriptorWrites
							, index );
						bindTexture( lpv[castor3d::LpvTexture::eG].wholeView
							, *lpv[castor3d::LpvTexture::eG].sampler
							, descriptorWrites
							, index );
						bindTexture( lpv[castor3d::LpvTexture::eB].wholeView
							, *lpv[castor3d::LpvTexture::eB].sampler
							, descriptorWrites
							, index );
					}
				}
			}
		}
	}

	void OceanRenderPass::doFillAdditionalDescriptor( castor3d::RenderPipeline const & pipeline
		, ashes::WriteDescriptorSetArray & descriptorWrites
		, castor3d::BillboardRenderNode & node
		, castor3d::ShadowMapLightTypeArray const & shadowMaps )
	{
		fillAdditionalDescriptor( m_graph
			, pipeline
			, descriptorWrites
			, m_scene
			, node.sceneNode
			, shadowMaps
			, m_lpvConfigUbo
			, m_llpvConfigUbo
			, m_vctConfigUbo
			, m_lpvResult
			, m_llpvResult
			, m_vctFirstBounce
			, m_vctSecondaryBounce
			, *m_ubo
			, *m_linearWrapSampler
			, *m_pointClampSampler
			, m_oceanFFT->getHeightDisplacement().sampledView
			, m_oceanFFT->getGradientJacobian().sampledView
			, m_oceanFFT->getNormals().sampledView
			, m_parent->getNormalTexture().sampledView
			, m_depthInput->sampledView
			, m_colourInput->sampledView );
	}

	void OceanRenderPass::doFillAdditionalDescriptor( castor3d::RenderPipeline const & pipeline
		, ashes::WriteDescriptorSetArray & descriptorWrites
		, castor3d::SubmeshRenderNode & node
		, castor3d::ShadowMapLightTypeArray const & shadowMaps )
	{
		fillAdditionalDescriptor( m_graph
			, pipeline
			, descriptorWrites
			, m_scene
			, node.sceneNode
			, shadowMaps
			, m_lpvConfigUbo
			, m_llpvConfigUbo
			, m_vctConfigUbo
			, m_lpvResult
			, m_llpvResult
			, m_vctFirstBounce
			, m_vctSecondaryBounce
			, *m_ubo
			, *m_linearWrapSampler
			, *m_pointClampSampler
			, m_oceanFFT->getHeightDisplacement().sampledView
			, m_oceanFFT->getGradientJacobian().sampledView
			, m_oceanFFT->getNormals().sampledView
			, m_parent->getNormalTexture().sampledView
			, m_depthInput->sampledView
			, m_colourInput->sampledView );
	}

	void OceanRenderPass::doUpdateFlags( castor3d::PipelineFlags & flags )const
	{
		addFlag( flags.programFlags, castor3d::ProgramFlag::eLighting );

		remFlag( flags.programFlags, castor3d::ProgramFlag::eMorphing );
		remFlag( flags.programFlags, castor3d::ProgramFlag::eInstantiation );
		remFlag( flags.programFlags, castor3d::ProgramFlag::eSecondaryUV );
		remFlag( flags.programFlags, castor3d::ProgramFlag::eForceTexCoords );

		remFlag( flags.passFlags, castor3d::PassFlag::eReflection );
		remFlag( flags.passFlags, castor3d::PassFlag::eRefraction );
		remFlag( flags.passFlags, castor3d::PassFlag::eParallaxOcclusionMappingOne );
		remFlag( flags.passFlags, castor3d::PassFlag::eParallaxOcclusionMappingRepeat );
		remFlag( flags.passFlags, castor3d::PassFlag::eDistanceBasedTransmittance );
		remFlag( flags.passFlags, castor3d::PassFlag::eSubsurfaceScattering );
		remFlag( flags.passFlags, castor3d::PassFlag::eAlphaBlending );
		remFlag( flags.passFlags, castor3d::PassFlag::eAlphaTest );
		remFlag( flags.passFlags, castor3d::PassFlag::eBlendAlphaTest );

#if !Ocean_DebugPixelShader
		addFlag( flags.programFlags, castor3d::ProgramFlag::eHasTessellation );
		flags.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		flags.patchVertices = OutputVertices;
#endif
	}

	void OceanRenderPass::doUpdatePipeline( castor3d::RenderPipeline & pipeline )
	{
	}

	castor3d::ShaderPtr OceanRenderPass::doGetVertexShaderSource( castor3d::PipelineFlags const & flags )const
	{
		using namespace sdw;
		using namespace castor3d;
		VertexWriter writer;

		// Since their vertex attribute locations overlap, we must not have both set at the same time.
		CU_Require( ( checkFlag( flags.programFlags, ProgramFlag::eInstantiation ) ? 1 : 0 )
			+ ( checkFlag( flags.programFlags, ProgramFlag::eMorphing ) ? 1 : 0 ) < 2
			&& "Can't have both instantiation and morphing yet." );
		auto textureFlags = filterTexturesFlags( flags.textures );

		UBO_MODEL( writer
			, uint32_t( NodeUboIdx::eModel )
			, RenderPipeline::eBuffers );

		UBO_MATRIX( writer
			, uint32_t( PassUboIdx::eMatrix )
			, RenderPipeline::eAdditional );
		UBO_SCENE( writer
			, uint32_t( PassUboIdx::eScene )
			, RenderPipeline::eAdditional );
		UBO_OCEAN( writer
			, WaveIdx::eOceanUbo
			, RenderPipeline::eAdditional );

		writer.implementMainT< castor3d::shader::VertexSurfaceT, PatchT >( sdw::VertexInT< castor3d::shader::VertexSurfaceT >{ writer
				, flags.programFlags
				, getShaderFlags()
				, textureFlags
				, flags.passFlags
				, false /* force no texcoords*/ }
		, sdw::VertexOutT< PatchT >{ writer }
		, [&]( VertexInT< castor3d::shader::VertexSurfaceT > in
			, VertexOutT< PatchT > out )
		{
			auto pos = writer.declLocale( "pos"
				, ( ( in.position.xz() / c3d_oceanData.patchSize ) + c3d_oceanData.blockOffset ) * c3d_oceanData.patchSize );
				out.vtx.position = vec4( pos.x(), 0.0_f, pos.y(), 1.0_f );
				out.worldPosition = out.vtx.position.xyz();
				out.material = c3d_modelData.getMaterialIndex( flags.programFlags
					, in.material );

#if Ocean_DebugPixelShader
				auto curMtxModel = writer.declLocale< Mat4 >( "curMtxModel"
					, c3d_modelData.getModelMtx() );
				out.worldPosition = curMtxModel * out.vtx.position;
				out.viewPosition = c3d_matrixData.worldToCurView( out.worldPosition );
				out.vtx.position = c3d_matrixData.viewToProj( out.viewPosition );
				auto mtxNormal = writer.declLocale< Mat3 >( "mtxNormal"
					, c3d_modelData.getNormalMtx( flags.programFlags, curMtxModel ) );
				auto v4Normal = writer.declLocale( "v4Normal"
					, vec4( in.normal, 0.0_f ) );
				auto v4Tangent = writer.declLocale( "v4Tangent"
					, vec4( in.tangent, 0.0_f ) );
				out.computeTangentSpace( flags.programFlags
					, c3d_sceneData.cameraPosition
					, out.worldPosition.xyz()
					, mtxNormal
					, v4Normal
					, v4Tangent );
#endif
			} );
		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
	}

	castor3d::ShaderPtr OceanRenderPass::doGetBillboardShaderSource( castor3d::PipelineFlags const & flags )const
	{
		using namespace sdw;
		using namespace castor3d;
		VertexWriter writer;
		auto textureFlags = filterTexturesFlags( flags.textures );

		// Shader inputs
		auto position = writer.declInput< Vec4 >( "position", 0u );
		auto uv = writer.declInput< Vec2 >( "uv", 1u );
		auto center = writer.declInput< Vec3 >( "center", 2u );

		UBO_MODEL( writer
			, uint32_t( NodeUboIdx::eModel )
			, RenderPipeline::eBuffers );
		UBO_BILLBOARD( writer
			, uint32_t( NodeUboIdx::eBillboard )
			, RenderPipeline::eBuffers );

		UBO_MATRIX( writer
			, uint32_t( PassUboIdx::eMatrix )
			, RenderPipeline::eAdditional );
		UBO_SCENE( writer
			, uint32_t( PassUboIdx::eScene )
			, RenderPipeline::eAdditional );
		UBO_OCEAN( writer
			, WaveIdx::eOceanUbo
			, RenderPipeline::eAdditional );

		writer.implementMainT< VoidT, PatchT >( sdw::VertexInT< sdw::VoidT >{ writer }
			, sdw::VertexOutT< PatchT >{ writer }
			, [&]( VertexInT< VoidT > in
				, VertexOutT< PatchT > out )
			{
				auto curBbcenter = writer.declLocale( "curBbcenter"
					, c3d_modelData.modelToCurWorld( vec4( center, 1.0_f ) ).xyz() );
				auto prvBbcenter = writer.declLocale( "prvBbcenter"
					, c3d_modelData.modelToPrvWorld( vec4( center, 1.0_f ) ).xyz() );
				auto curToCamera = writer.declLocale( "curToCamera"
					, c3d_sceneData.getPosToCamera( curBbcenter ) );
				curToCamera.y() = 0.0_f;
				curToCamera = normalize( curToCamera );

				auto right = writer.declLocale( "right"
					, c3d_billboardData.getCameraRight( flags.programFlags, c3d_matrixData ) );
				auto up = writer.declLocale( "up"
					, c3d_billboardData.getCameraUp( flags.programFlags, c3d_matrixData ) );
				auto width = writer.declLocale( "width"
					, c3d_billboardData.getWidth( flags.programFlags, c3d_sceneData ) );
				auto height = writer.declLocale( "height"
					, c3d_billboardData.getHeight( flags.programFlags, c3d_sceneData ) );
				auto scaledRight = writer.declLocale( "scaledRight"
					, right * position.x() * width );
				auto scaledUp = writer.declLocale( "scaledUp"
					, up * position.y() * height );
				auto worldPos = writer.declLocale( "worldPos"
					, ( curBbcenter + scaledRight + scaledUp ) );
				out.worldPosition = worldPos;
				out.material = c3d_modelData.getMaterialIndex();
				out.vtx.position = c3d_modelData.worldToModel( vec4( worldPos, 1.0_f ) );
			} );

		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
	}

	castor3d::ShaderPtr OceanRenderPass::doGetHullShaderSource( castor3d::PipelineFlags const & flags )const
	{
#if Ocean_DebugPixelShader
		return nullptr;
#else
		using namespace sdw;
		using namespace castor3d;
		TessellationControlWriter writer;
		auto textureFlags = filterTexturesFlags( flags.textures );

		UBO_SCENE( writer
			, uint32_t( PassUboIdx::eScene )
			, RenderPipeline::eAdditional );
		UBO_OCEAN( writer
			, WaveIdx::eOceanUbo
			, RenderPipeline::eAdditional );

		auto tessLevel1f = writer.implementFunction< Float >( "tessLevel1f"
			, [&]( Float lod
				, Vec2 maxTessLevel )
			{
				writer.returnStmt( maxTessLevel.y() * exp2( -lod ) );
			}
			, sdw::InFloat{ writer, "lod" }
			, sdw::InVec2{ writer, "maxTessLevel" } );

		auto tessLevel4f = writer.implementFunction< Vec4 >( "tessLevel4f"
			, [&]( Vec4 lod
				, Vec2 maxTessLevel )
			{
				writer.returnStmt( maxTessLevel.y() * exp2( -lod ) );
			}
			, sdw::InVec4{ writer, "lod" }
			, sdw::InVec2{ writer, "maxTessLevel" } );

		auto lodFactor = writer.implementFunction< Float >( "lodFactor"
			, [&]( Vec3 worldPos
				, Vec3 worldEye
				, Vec2 tileScale
				, Vec2 maxTessLevel
				, Float distanceMod )
			{
				worldPos.xz() *= tileScale;
				auto distToCam = writer.declLocale( "distToCam"
					, worldEye - worldPos );
				auto level = writer.declLocale( "level"
					, log2( ( length( distToCam ) + 0.0001_f ) * distanceMod ) );
				writer.returnStmt( clamp( level
					, 0.0_f
					, maxTessLevel.x() ) );
			}
			, sdw::InVec3{ writer, "worldPos" }
			, sdw::InVec3{ writer, "worldEye" }
			, sdw::InVec2{ writer, "tileScale" }
			, sdw::InVec2{ writer, "maxTessLevel" }
			, sdw::InFloat{ writer, "distanceMod" } );

		writer.implementPatchRoutineT< PatchT, OutputVertices, PatchT >( TessControlListInT< PatchT, OutputVertices >{ writer
				, false }
			, sdw::QuadsTessPatchOutT< PatchT >{ writer
				, 9u }
			, [&]( sdw::TessControlPatchRoutineIn in
				, sdw::TessControlListInT< PatchT, OutputVertices > listIn
				, sdw::QuadsTessPatchOutT< PatchT > patchOut )
			{
				auto patchSize = writer.declLocale( "patchSize"
					, vec3( c3d_oceanData.patchSize.x(), 0.0_f, c3d_oceanData.patchSize.y() ) );
				auto p0 = writer.declLocale( "p0"
					, listIn[0u].worldPosition );
				patchOut.patchPosBase = p0;

				auto l0 = writer.declLocale( "l0"
					, lodFactor( p0 + vec3( 0.0_f, 0.0f, 1.0f ) * patchSize
						, c3d_sceneData.cameraPosition
						, c3d_oceanData.tileScale
						, c3d_oceanData.maxTessLevel
						, c3d_oceanData.distanceMod ) );
				auto l1 = writer.declLocale( "l1"
					, lodFactor( p0 + vec3( 0.0_f, 0.0f, 0.0f ) * patchSize
						, c3d_sceneData.cameraPosition
						, c3d_oceanData.tileScale
						, c3d_oceanData.maxTessLevel
						, c3d_oceanData.distanceMod ) );
				auto l2 = writer.declLocale( "l2"
					, lodFactor( p0 + vec3( 1.0_f, 0.0f, 0.0f ) * patchSize
						, c3d_sceneData.cameraPosition
						, c3d_oceanData.tileScale
						, c3d_oceanData.maxTessLevel
						, c3d_oceanData.distanceMod ) );
				auto l3 = writer.declLocale( "l3"
					, lodFactor( p0 + vec3( 1.0_f, 0.0f, 1.0f ) * patchSize
						, c3d_sceneData.cameraPosition
						, c3d_oceanData.tileScale
						, c3d_oceanData.maxTessLevel
						, c3d_oceanData.distanceMod ) );

				auto lods = writer.declLocale( "lods"
					, vec4( l0, l1, l2, l3 ) );
				patchOut.patchLods = lods;
				patchOut.material = listIn[0u].material;

				auto outerLods = writer.declLocale( "outerLods"
					, min( lods.xyzw(), lods.yzwx() ) );
				auto tessLevels = writer.declLocale( "tessLevels"
					, tessLevel4f( outerLods, c3d_oceanData.maxTessLevel ) );
				auto innerLevel = writer.declLocale( "innerLevel"
					, max( max( tessLevels.x(), tessLevels.y() )
						, max( tessLevels.z(), tessLevels.w() ) ) );

				patchOut.tessLevelOuter[0] = tessLevels.x();
				patchOut.tessLevelOuter[1] = tessLevels.y();
				patchOut.tessLevelOuter[2] = tessLevels.z();
				patchOut.tessLevelOuter[3] = tessLevels.w();

				patchOut.tessLevelInner[0] = innerLevel;
				patchOut.tessLevelInner[1] = innerLevel;
			} );

		writer.implementMainT< PatchT, OutputVertices, VoidT >( TessControlListInT< PatchT, OutputVertices >{ writer
				, true }
			, TrianglesTessControlListOut{ writer
				, ast::type::Partitioning::eFractionalEven
				, ast::type::OutputTopology::eQuad
				, ast::type::PrimitiveOrdering::eCW
				, OutputVertices }
			, [&]( TessControlMainIn in
				, TessControlListInT< PatchT, OutputVertices > listIn
				, TrianglesTessControlListOut listOut )
			{
				listOut.vtx.position = listIn[in.invocationID].vtx.position;
			} );

		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
#endif
	}

	castor3d::ShaderPtr OceanRenderPass::doGetDomainShaderSource( castor3d::PipelineFlags const & flags )const
	{
#if Ocean_DebugPixelShader
		return nullptr;
#else
		using namespace sdw;
		using namespace castor3d;
		TessellationEvaluationWriter writer;
		auto textureFlags = filterTexturesFlags( flags.textures );

		castor3d::shader::Utils utils{ writer, *getEngine() };
		UBO_MODEL( writer
			, uint32_t( NodeUboIdx::eModel )
			, RenderPipeline::eBuffers );
		auto skinningData = SkinningUbo::declare( writer
			, uint32_t( NodeUboIdx::eSkinningUbo )
			, uint32_t( NodeUboIdx::eSkinningSsbo )
			, RenderPipeline::eBuffers
			, flags.programFlags );

		UBO_MATRIX( writer
			, uint32_t( PassUboIdx::eMatrix )
			, RenderPipeline::eAdditional );
		UBO_SCENE( writer
			, uint32_t( PassUboIdx::eScene )
			, RenderPipeline::eAdditional );
		UBO_OCEAN( writer
			, WaveIdx::eOceanUbo
			, RenderPipeline::eAdditional );

		auto c3d_heightDisplacementMap = writer.declSampledImage< sdw::SampledImage2DRgba32 >( "c3d_heightDisplacementMap"
			, WaveIdx::eHeightDisplacement
			, RenderPipeline::eAdditional );

		auto lerpVertex = writer.implementFunction< Vec2 >( "lerpVertex"
			, [&]( Vec3 patchPosBase
				, Vec2 tessCoord
				, Vec2 patchSize )
			{
				writer.returnStmt( fma( tessCoord, patchSize, patchPosBase.xz() ) );
			}
			, sdw::InVec3{ writer, "patchPosBase" }
			, sdw::InVec2{ writer, "tessCoord" }
			, sdw::InVec2{ writer, "patchSize" } );

		auto lodFactor = writer.implementFunction< Vec2 >( "lodFactor"
			, [&]( Vec2 tessCoord
				, Vec4 patchLods )
			{
				// Bilinear interpolation from patch corners.
				auto x = writer.declLocale( "x"
					, mix( patchLods.yx(), patchLods.zw(), tessCoord.xx() ) );
				auto level = writer.declLocale( "level"
					, mix( x.x(), x.y(), tessCoord.y() ) );

				auto floorLevel = writer.declLocale( "floorLevel"
					, floor( level ) );
				auto fractLevel = writer.declLocale( "fractLevel"
					, level - floorLevel );
				writer.returnStmt( vec2( floorLevel, fractLevel ) );
			}
			, sdw::InVec2{ writer, "tessCoord" }
			, sdw::InVec4{ writer, "patchLods" } );

		auto sampleHeightDisplacement = writer.implementFunction< Vec3 >( "sampleHeightDisplacement"
			, [&]( Vec2 uv
				, Vec2 off
				, Vec2 lod )
			{
				writer.returnStmt( mix( c3d_heightDisplacementMap.lod( uv + vec2( 0.5_f ) * off, lod.x() ).xyz()
					, c3d_heightDisplacementMap.lod( uv + vec2( 1.0_f ) * off, lod.x() + 1.0_f ).xyz()
					, vec3( lod.y() ) ) );
			}
			, sdw::InVec2{ writer, "uv" }
			, sdw::InVec2{ writer, "off" }
			, sdw::InVec2{ writer, "lod" } );

		writer.implementMainT< PatchT, OutputVertices, PatchT, PatchT >( TessEvalListInT< PatchT, OutputVertices >{ writer
				, ast::type::PatchDomain::eQuads
				, type::Partitioning::eFractionalEven
				, type::PrimitiveOrdering::eCW }
			, QuadsTessPatchInT< PatchT >{ writer, 9u }
			, TessEvalDataOutT< PatchT >{ writer }
			, [&]( TessEvalMainIn mainIn
				, TessEvalListInT< PatchT, OutputVertices > listIn
				, QuadsTessPatchInT< PatchT > patchIn
				, TessEvalDataOutT< PatchT > out )
			{
				auto tessCoord = writer.declLocale( "tessCoord"
					, patchIn.tessCoord.xy() );
				auto pos = writer.declLocale( "pos"
					, lerpVertex( patchIn.patchPosBase, tessCoord, c3d_oceanData.patchSize ) );
				auto lod = writer.declLocale( "lod"
					, lodFactor( tessCoord, patchIn.patchLods ) );

				auto tex = writer.declLocale( "tex"
					, pos * c3d_oceanData.invHeightmapSize.xy() );
				pos *= c3d_oceanData.tileScale;

				auto deltaMod = writer.declLocale( "deltaMod"
					, exp2( lod.x() ) );
				auto off = writer.declLocale( "off"
					, c3d_oceanData.invHeightmapSize.xy() * deltaMod );

				out.gradNormalTexcoord = vec4( tex + vec2( 0.5_f ) * c3d_oceanData.invHeightmapSize.xy()
					, tex * c3d_oceanData.normalScale );
				auto heightDisplacement = writer.declLocale( "heightDisplacement"
					, sampleHeightDisplacement( tex, off, lod ) );

				pos += heightDisplacement.yz();
				out.worldPosition = vec3( pos.x(), heightDisplacement.x(), pos.y() );

				out.material = patchIn.material;

				auto worldPosition = writer.declLocale( "worldPosition"
					, c3d_modelData.modelToWorld( vec4( out.worldPosition, 1.0_f ) ) );
				auto viewPosition = writer.declLocale( "viewPosition"
					, c3d_matrixData.worldToCurView( worldPosition ) );
				out.vtx.position = c3d_matrixData.viewToProj( viewPosition );
			} );

		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
#endif
	}

	castor3d::ShaderPtr OceanRenderPass::doGetGeometryShaderSource( castor3d::PipelineFlags const & flags )const
	{
		return nullptr;
	}

	castor3d::ShaderPtr OceanRenderPass::doGetPixelShaderSource( castor3d::PipelineFlags const & flags )const
	{
#if Ocean_Debug
#	define displayDebugData( OceanDataType, RGB, A )\
	IF( writer, c3d_oceanData.debug == sdw::UInt( OceanDataType ) )\
	{\
		pxl_colour = vec4( RGB, A );\
		writer.returnStmt();\
	}\
	FI
#else
#	define displayDebugData( OceanDataType, RGB, A )
#endif

		using namespace sdw;
		using namespace castor3d;
		FragmentWriter writer;

		auto & renderSystem = *getEngine()->getRenderSystem();
		auto textureFlags = filterTexturesFlags( flags.textures );
		bool hasTextures = !flags.textures.empty();
		bool hasDiffuseGI = checkFlag( flags.sceneFlags, SceneFlag::eVoxelConeTracing )
			|| checkFlag( flags.sceneFlags, SceneFlag::eLpvGI )
			|| checkFlag( flags.sceneFlags, SceneFlag::eLayeredLpvGI );

		shader::Utils utils{ writer, *getEngine() };
		shader::CookTorranceBRDF cookTorrance{ writer, utils };

		shader::Materials materials{ writer };
		materials.declare( renderSystem.getGpuInformations().hasShaderStorageBuffers()
			, uint32_t( NodeUboIdx::eMaterials )
			, RenderPipeline::eBuffers );
		shader::TextureConfigurations textureConfigs{ writer };
		shader::TextureAnimations textureAnims{ writer };

		if ( hasTextures )
		{
			textureConfigs.declare( renderSystem.getGpuInformations().hasShaderStorageBuffers()
				, uint32_t( NodeUboIdx::eTexConfigs )
				, RenderPipeline::eBuffers );
			textureAnims.declare( renderSystem.getGpuInformations().hasShaderStorageBuffers()
				, uint32_t( NodeUboIdx::eTexAnims )
				, RenderPipeline::eBuffers );
		}

		UBO_MODEL( writer
			, uint32_t( NodeUboIdx::eModel )
			, RenderPipeline::eBuffers );

		auto c3d_maps( writer.declSampledImageArray< FImg2DRgba32 >( "c3d_maps"
			, 0u
			, RenderPipeline::eTextures
			, std::max( 1u, uint32_t( flags.textures.size() ) )
			, hasTextures ) );

		UBO_MATRIX( writer
			, uint32_t( PassUboIdx::eMatrix )
			, RenderPipeline::eAdditional );
		UBO_SCENE( writer
			, uint32_t( PassUboIdx::eScene )
			, RenderPipeline::eAdditional );
		auto index = uint32_t( PassUboIdx::eCount );
		auto lightsIndex = index++;
		UBO_OCEAN( writer
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_heightDisplacementMap = writer.declSampledImage< sdw::SampledImage2DRgba16 >( "c3d_heightDisplacementMap"
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_gradientJacobianMap = writer.declSampledImage< sdw::SampledImage2DRgba16 >( "c3d_gradientJacobianMap"
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_normalsMap = writer.declSampledImage< sdw::SampledImage2DRg32 >( "c3d_normalsMap"
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_sceneNormals = writer.declSampledImage< FImg2DRgba32 >( "c3d_sceneNormals"
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_sceneDepth = writer.declSampledImage< FImg2DR32 >( "c3d_sceneDepth"
			, index++
			, RenderPipeline::eAdditional );
		auto c3d_sceneColour = writer.declSampledImage< FImg2DRgba32 >( "c3d_sceneColour"
			, index++
			, RenderPipeline::eAdditional );
		auto lightingModel = shader::LightingModel::createModel( utils
			, shader::getLightingModelName( *getEngine(), flags.passType )
			, lightsIndex
			, RenderPipeline::eAdditional
			, shader::ShadowOptions{ flags.sceneFlags, false }
			, index
			, RenderPipeline::eAdditional
			, false
			, renderSystem.getGpuInformations().hasShaderStorageBuffers() );
		auto reflections = lightingModel->getReflectionModel( flags.passFlags
			, index
			, uint32_t( RenderPipeline::eAdditional ) );
		shader::GlobalIllumination indirect{ writer, utils };
		indirect.declare( index
			, RenderPipeline::eAdditional
			, flags.sceneFlags );
		shader::Fog fog{ writer };

		// Fragment Outputs
		auto pxl_colour( writer.declOutput< Vec4 >( "pxl_colour", 0 ) );

		writer.implementMainT< PatchT, VoidT >( sdw::FragmentInT< PatchT >{ writer }
			, FragmentOut{ writer }
			, [&]( FragmentInT< PatchT > in
				, FragmentOut out )
			{
				auto hdrCoords = writer.declLocale( "hdrCoords"
					, in.fragCoord.xy() / c3d_sceneData.renderSize );
				auto gradJacobian = writer.declLocale( "gradJacobian"
					, c3d_gradientJacobianMap.sample( in.gradNormalTexcoord.xy() ).xyz() );
				displayDebugData( eGradJacobian, gradJacobian, 1.0_f );
				auto noiseGradient = writer.declLocale( "noiseGradient"
					, vec2( 0.3_f ) * c3d_normalsMap.sample( in.gradNormalTexcoord.zw() ) );
				displayDebugData( eNoiseGradient, vec3( noiseGradient, 0.0_f ), 1.0_f );

				auto jacobian = writer.declLocale( "jacobian"
					, gradJacobian.z() );
				displayDebugData( eJacobian, vec3( jacobian ), 1.0_f );
				auto turbulence = writer.declLocale( "turbulence"
					, max( 2.0_f - jacobian + dot( abs( noiseGradient ), vec2( 1.2_f ) )
						, 0.0_f ) );
				displayDebugData( eTurbulence, vec3( turbulence ), 1.0_f );

				// Add low frequency gradient from heightmap with gradient from high frequency noisemap.
				auto normal = writer.declLocale( "normal"
					, vec3( -gradJacobian.x(), 1.0, -gradJacobian.y() ) );
				normal.xz() -= noiseGradient;
				normal = normalize( normal );
				displayDebugData( eNormal, normal, 1.0_f );

				// Make water brighter based on how turbulent the water is.
				// This is rather "arbitrary", but looks pretty good in practice.
				auto colorMod = writer.declLocale( "colorMod"
					, 1.0_f + 3.0_f * smoothStep( 1.2_f, 1.8_f, turbulence ) );
				displayDebugData( eColorMod, vec3( colorMod ), 1.0_f );

				auto material = writer.declLocale( "material"
					, materials.getMaterial( in.material ) );
				auto emissive = writer.declLocale( "emissive"
					, vec3( material.emissive ) );
				auto worldEye = writer.declLocale( "worldEye"
					, c3d_sceneData.cameraPosition );
				auto lightMat = lightingModel->declMaterial( "lightMat" );
				lightMat->create( material );
				displayDebugData( eMatSpecular, lightMat->specular, 1.0_f );

				auto viewPosition = writer.declLocale( "viewPosition"
					, c3d_matrixData.worldToCurView( vec4( in.worldPosition, 1.0_f ) ) );

				if ( checkFlag( flags.passFlags, PassFlag::eLighting ) )
				{
					// Direct Lighting
					auto lightDiffuse = writer.declLocale( "lightDiffuse"
						, vec3( 0.0_f ) );
					auto lightSpecular = writer.declLocale( "lightSpecular"
						, vec3( 0.0_f ) );
					shader::OutputComponents output{ lightDiffuse, lightSpecular };
					auto surface = writer.declLocale< shader::Surface >( "surface" );
					surface.create( in.fragCoord.xy(), viewPosition.xyz(), in.worldPosition, normal );
					lightingModel->computeCombined( *lightMat
						, c3d_sceneData
						, surface
						, worldEye
						, c3d_modelData.isShadowReceiver()
						, output );
					lightMat->adjustDirectSpecular( lightSpecular );
					displayDebugData( eLightDiffuse, lightDiffuse, 1.0_f );
					displayDebugData( eLightSpecular, lightSpecular, 1.0_f );


					// Indirect Lighting
					auto indirectOcclusion = writer.declLocale( "indirectOcclusion"
						, 1.0_f );
					auto lightIndirectDiffuse = indirect.computeDiffuse( flags.sceneFlags
						, surface
						, indirectOcclusion );
					auto worldToCam = writer.declLocale( "worldToCam"
						, c3d_sceneData.getPosToCamera( surface.worldPosition ) );
					displayDebugData( eIndirectOcclusion, vec3( indirectOcclusion ), 1.0_f );
					displayDebugData( eLightIndirectDiffuse, lightIndirectDiffuse.xyz(), 1.0_f );
					auto lightIndirectSpecular = indirect.computeSpecular( flags.sceneFlags
						, worldEye
						, worldToCam
						, surface
						, lightMat->specular
						, lightMat->getRoughness()
						, indirectOcclusion
						, lightIndirectDiffuse.w() );
					displayDebugData( eLightIndirectSpecular, lightIndirectSpecular, 1.0_f );
					auto indirectAmbient = writer.declLocale( "indirectAmbient"
						, lightMat->getIndirectAmbient( indirect.computeAmbient( flags.sceneFlags, lightIndirectDiffuse.xyz() ) ) );
					displayDebugData( eIndirectAmbient, indirectAmbient, 1.0_f );
					auto indirectDiffuse = writer.declLocale( "indirectDiffuse"
						, ( hasDiffuseGI
							? cookTorrance.computeDiffuse( lightIndirectDiffuse.xyz()
								, c3d_sceneData.cameraPosition
								, normal
								, lightMat->specular
								, lightMat->getMetalness()
								, surface )
							: vec3( 0.0_f ) ) );
					displayDebugData( eIndirectDiffuse, indirectDiffuse, 1.0_f );


					// Reflection
					auto reflected = writer.declLocale( "reflected"
						, reflections->computeForward( *lightMat
							, surface
							, c3d_sceneData ) );
					displayDebugData( eRawBackgroundReflection, reflected, 1.0_f );
					auto NdotV = writer.declLocale( "NdotV"
						, dot( worldToCam, normal ) );
					reflected = utils.fresnelSchlick( NdotV, lightMat->specular ) * reflected;
					displayDebugData( eFresnelBackgroundReflection, reflected, 1.0_f );
					auto ssrResult = writer.declLocale( "ssrResult"
						, reflections->computeScreenSpace( c3d_matrixData
							, surface.viewPosition
							, normal
							, hdrCoords
							, c3d_oceanData.ssrSettings
							, c3d_sceneDepth
							, c3d_sceneNormals
							, c3d_sceneColour ) );
					displayDebugData( eSSRResult, ssrResult.xyz(), 1.0_f );
					displayDebugData( eSSRFactor, ssrResult.www(), 1.0_f );
					displayDebugData( eSSRResultFactor, ssrResult.xyz() * ssrResult.www(), 1.0_f );
					auto reflectionResult = writer.declLocale( "reflectionResult"
						, mix( reflected, ssrResult.xyz(), ssrResult.www() ) );
					displayDebugData( eCombinedReflection, reflectionResult, 1.0_f );


					// Wobbly refractions
					auto distortedTexCoord = writer.declLocale( "distortedTexCoord"
						, ( hdrCoords + ( ( normal.xz() + normal.xy() ) * 0.5 ) * c3d_oceanData.refractionDistortionFactor ) );
					auto distortedDepth = writer.declLocale( "distortedDepth"
						, c3d_sceneDepth.sample( distortedTexCoord ) );
					auto distortedPosition = writer.declLocale( "distortedPosition"
						, c3d_matrixData.curProjToWorld( utils, distortedTexCoord, distortedDepth ) );
					auto refractionTexCoord = writer.declLocale( "refractionTexCoord"
						, writer.ternary( distortedPosition.y() < in.worldPosition.y(), distortedTexCoord, hdrCoords ) );
					auto refractionResult = writer.declLocale( "refractionResult"
						, c3d_sceneColour.sample( refractionTexCoord ).rgb() * material.transmission );
					displayDebugData( eRefraction, refractionResult, 1.0_f );
					//  Retrieve non distorted scene colour.
					auto sceneDepth = writer.declLocale( "sceneDepth"
						, c3d_sceneDepth.sample( hdrCoords ) );
					auto scenePosition = writer.declLocale( "scenePosition"
						, c3d_matrixData.curProjToWorld( utils, hdrCoords, sceneDepth ) );
					// Depth softening, to fade the alpha of the water where it meets the scene geometry by some predetermined distance. 
					auto depthSoftenedAlpha = writer.declLocale( "depthSoftenedAlpha"
						, clamp( distance( scenePosition, in.worldPosition.xyz() ) / c3d_oceanData.depthSofteningDistance, 0.0_f, 1.0_f ) );
					displayDebugData( eDepthSoftenedAlpha, vec3( depthSoftenedAlpha ), 1.0_f );
					auto waterSurfacePosition = writer.declLocale( "waterSurfacePosition"
						, writer.ternary( distortedPosition.y() < in.worldPosition.y(), distortedPosition, scenePosition ) );
					auto waterTransmission = writer.declLocale( "waterTransmission"
						, material.transmission.rgb() * ( indirectAmbient + indirectDiffuse ) );
					auto heightFactor = writer.declLocale( "heightFactor"
						, c3d_oceanData.refractionHeightFactor * ( c3d_sceneData.farPlane - c3d_sceneData.nearPlane ) );
					refractionResult = mix( refractionResult
						, waterTransmission
						, vec3( clamp( ( in.worldPosition.y() - waterSurfacePosition.y() ) / heightFactor, 0.0_f, 1.0_f ) ) );
					displayDebugData( eHeightMixedRefraction, refractionResult, 1.0_f );
					auto distanceFactor = writer.declLocale( "distanceFactor"
						, c3d_oceanData.refractionDistanceFactor * ( c3d_sceneData.farPlane - c3d_sceneData.nearPlane ) );
					refractionResult = mix( refractionResult
						, waterTransmission
						, utils.saturate( vec3( utils.saturate( length( viewPosition ) / distanceFactor ) ) ) );
					displayDebugData( eDistanceMixedRefraction, refractionResult, 1.0_f );


					//Combine all that
					auto fresnelFactor = writer.declLocale( "fresnelFactor"
						, vec3( reflections->computeFresnel( *lightMat
							, surface
							, c3d_sceneData
							, c3d_oceanData.refractionRatio ) ) );
					displayDebugData( eFresnelFactor, vec3( fresnelFactor ), 1.0_f );
					reflectionResult *= fresnelFactor;
					displayDebugData( eFinalReflection, reflectionResult, 1.0_f );
					refractionResult *= vec3( 1.0_f ) - fresnelFactor;
					displayDebugData( eFinalRefraction, refractionResult, 1.0_f );
					pxl_colour = vec4( lightSpecular + lightIndirectSpecular
							+ emissive
							+ refractionResult * colorMod
							+ ( reflectionResult * colorMod * indirectAmbient )
						, depthSoftenedAlpha );
				}
				else
				{
					pxl_colour = vec4( lightMat->albedo, 1.0_f );
				}

				if ( getFogType( flags.sceneFlags ) != FogType::eDisabled )
				{
					pxl_colour = fog.apply( c3d_sceneData.getBackgroundColour( utils )
						, pxl_colour
						, length( viewPosition )
						, viewPosition.y()
						, c3d_sceneData );
				}
			} );

		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
	}
}
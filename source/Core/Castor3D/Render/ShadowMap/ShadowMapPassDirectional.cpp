#include "Castor3D/Render/ShadowMap/ShadowMapPassDirectional.hpp"

#include "Castor3D/DebugDefines.hpp"
#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/UniformBuffer.hpp"
#include "Castor3D/Buffer/PoolUniformBuffer.hpp"
#include "Castor3D/Cache/LightCache.hpp"
#include "Castor3D/Cache/MaterialCache.hpp"
#include "Castor3D/Material/Texture/TextureLayout.hpp"
#include "Castor3D/Material/Texture/TextureUnit.hpp"
#include "Castor3D/Material/Texture/TextureView.hpp"
#include "Castor3D/Render/RenderModule.hpp"
#include "Castor3D/Render/RenderPipeline.hpp"
#include "Castor3D/Render/RenderQueue.hpp"
#include "Castor3D/Render/RenderSystem.hpp"
#include "Castor3D/Render/Culling/InstantiatedFrustumCuller.hpp"
#include "Castor3D/Render/ShadowMap/ShadowMapDirectional.hpp"
#include "Castor3D/Render/Technique/RenderTechniquePass.hpp"
#include "Castor3D/Scene/Scene.hpp"
#include "Castor3D/Scene/Light/Light.hpp"
#include "Castor3D/Scene/Light/DirectionalLight.hpp"
#include "Castor3D/Shader/Program.hpp"
#include "Castor3D/Shader/ShaderBuffers/PassBuffer.hpp"
#include "Castor3D/Shader/ShaderBuffers/TextureConfigurationBuffer.hpp"
#include "Castor3D/Shader/Shaders/GlslLight.hpp"
#include "Castor3D/Shader/Shaders/GlslLighting.hpp"
#include "Castor3D/Shader/Shaders/GlslMaterial.hpp"
#include "Castor3D/Shader/Shaders/GlslOutputComponents.hpp"
#include "Castor3D/Shader/Shaders/GlslSurface.hpp"
#include "Castor3D/Shader/Shaders/GlslTextureAnimation.hpp"
#include "Castor3D/Shader/Shaders/GlslTextureConfiguration.hpp"
#include "Castor3D/Shader/Shaders/GlslUtils.hpp"
#include "Castor3D/Shader/Ubos/ModelInstancesUbo.hpp"
#include "Castor3D/Shader/Ubos/ModelDataUbo.hpp"
#include "Castor3D/Shader/Ubos/MorphingUbo.hpp"
#include "Castor3D/Shader/Ubos/SkinningUbo.hpp"
#include "Castor3D/Shader/Ubos/ShadowMapDirectionalUbo.hpp"
#include "Castor3D/Shader/Ubos/ShadowMapUbo.hpp"

#include <CastorUtils/Graphics/Image.hpp>

#include <ashespp/Descriptor/DescriptorSet.hpp>
#include <ashespp/Descriptor/DescriptorSetLayout.hpp>
#include <ashespp/RenderPass/RenderPassCreateInfo.hpp>

#include <ShaderWriter/Source.hpp>

using namespace castor;

namespace castor3d
{
	namespace
	{
		castor::String getPassName( uint32_t cascadeIndex )
		{
			return cuT( "DirectionalSMC" ) + string::toString( cascadeIndex );
		}
	}

	uint32_t const ShadowMapPassDirectional::TileSize = 2048u;
	uint32_t const ShadowMapPassDirectional::TileCountX = ShadowMapDirectionalTileCountX;
	uint32_t const ShadowMapPassDirectional::TileCountY = ShadowMapDirectionalTileCountY;
	castor::String const ShadowMapPassDirectional::Type = "c3d.shadows.directional";

	ShadowMapPassDirectional::ShadowMapPassDirectional( crg::FramePass const & pass
		, crg::GraphContext & context
		, crg::RunnableGraph & graph
		, RenderDevice const & device
		, MatrixUbo & matrixUbo
		, SceneCuller & culler
		, ShadowMap const & shadowMap
		, uint32_t cascadeCount )
#if C3D_UseTiledDirectionalShadowMap
		: ShadowMapPass{ pass
			, context
			, graph
			, device
			, Type
			, cuT( "DirectionalSM" )
			, matrixUbo
			, culler
			, shadowMap
			, createRenderPass( device, shadowMap, cuT( "DirectionalSM" ) )
			, cascadeCount }
#else
		: ShadowMapPass{ pass
			, context
			, graph
			, device
			, Type
			, getPassName( cascadeCount )
			, matrixUbo
			, culler
			, shadowMap
			, 1u }
#endif
		, m_shadowMapDirectionalUbo{ device }
	{
		log::trace << "Created " << m_name << std::endl;
	}

	ShadowMapPassDirectional::~ShadowMapPassDirectional()
	{
		getCuller().getCamera().detach();
	}

	void ShadowMapPassDirectional::update( CpuUpdater & updater )
	{
#if C3D_UseTiledDirectionalShadowMap
		auto & light = *updater.light;
		auto & camera = *updater.camera;
		auto & directional = *light.getDirectionalLight();
		auto & culler = static_cast< InstantiatedFrustumCuller & >( getCuller() );
		std::vector< Frustum > frustums;

		for ( uint32_t cascade = 0u; cascade < getInstanceMult(); ++cascade )
		{
			frustums.emplace_back( camera.getViewport() );
			frustums.back().update( directional.getProjMatrix( cascade )
				, directional.getViewMatrix( cascade ) );
		}

		culler.setFrustums( std::move( frustums ) );
		culler.compute();
		m_outOfDate = m_outOfDate
			|| culler.areAllChanged()
			|| culler.areCulledChanged();
#else
		getCuller().compute();
#endif
		m_outOfDate = true;
		RenderNodesPass::update( updater );
	}

	void ShadowMapPassDirectional::update( GpuUpdater & updater )
	{
		if ( m_initialised )
		{
			doUpdateNodes( m_renderQueue->getCulledRenderNodes() );
		}
	}

	void ShadowMapPassDirectional::doUpdateUbos( CpuUpdater & updater )
	{
		RenderNodesPass::doUpdateUbos( updater );
#if C3D_UseTiledDirectionalShadowMap
		m_shadowMapDirectionalUbo.update( *updater.light->getDirectionalLight() );
#else
		m_shadowMapUbo.update( *updater.light, updater.index );
#endif
	}

	void ShadowMapPassDirectional::doFillAdditionalBindings( ashes::VkDescriptorSetLayoutBindingArray & bindings )const
	{
		auto index = uint32_t( GlobalBuffersIdx::eCount );
		bindings.emplace_back( m_shadowMap.getScene().getLightCache().createLayoutBinding( index++ ) );
		bindings.emplace_back( makeDescriptorSetLayoutBinding( index++
			, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT ) );
		m_initialised = true;
	}

	void ShadowMapPassDirectional::doFillAdditionalDescriptor( ashes::WriteDescriptorSetArray & descriptorWrites
		, ShadowMapLightTypeArray const & shadowMaps )
	{
		auto index = uint32_t( GlobalBuffersIdx::eCount );
		descriptorWrites.push_back( getCuller().getScene().getLightCache().getBinding( index++ ) );
#if C3D_UseTiledDirectionalShadowMap
		descriptorWrites.push_back( m_shadowMapDirectionalUbo.getDescriptorWrite( index++ ) );
#else
		descriptorWrites.push_back( m_shadowMapUbo.getDescriptorWrite( index++ ) );
#endif
	}

	ashes::PipelineDepthStencilStateCreateInfo ShadowMapPassDirectional::doCreateDepthStencilState( PipelineFlags const & flags )const
	{
		return ashes::PipelineDepthStencilStateCreateInfo{ 0u, true, true };
	}

	ashes::PipelineColorBlendStateCreateInfo ShadowMapPassDirectional::doCreateBlendState( PipelineFlags const & flags )const
	{
		return RenderNodesPass::createBlendState( BlendMode::eNoBlend
			, BlendMode::eNoBlend
			, uint32_t( SmTexture::eCount ) - 1u );
	}

	void ShadowMapPassDirectional::doUpdateFlags( PipelineFlags & flags )const
	{
		addFlag( flags.programFlags, ProgramFlag::eLighting );
		remFlag( flags.programFlags, ProgramFlag::eInvertNormals );
		remFlag( flags.passFlags, PassFlag::eAlphaBlending );
		addFlag( flags.programFlags, ProgramFlag::eShadowMapDirectional );
#if C3D_UseTiledDirectionalShadowMap
		addFlag( flags.programFlags, ProgramFlag::eInstanceMult );
#endif
	}

	ShaderPtr ShadowMapPassDirectional::doGetVertexShaderSource( PipelineFlags const & flags )const
	{
		// Since their vertex attribute locations overlap, we must not have both set at the same time.
		CU_Require( ( checkFlag( flags.programFlags, ProgramFlag::eInstantiation ) ? 1 : 0 )
			+ ( checkFlag( flags.programFlags, ProgramFlag::eMorphing ) ? 1 : 0 ) < 2 );
		using namespace sdw;
		VertexWriter writer;
		auto textureFlags = filterTexturesFlags( flags.textures );
		bool hasTextures = !flags.textures.empty();

		C3D_ObjectIdsData( writer
			, GlobalBuffersIdx::eObjectsNodeID
			, RenderPipeline::ePass );
		C3D_ModelsData( writer
			, GlobalBuffersIdx::eModelsData
			, RenderPipeline::ePass );
		C3D_Morphing( writer
			, GlobalBuffersIdx::eMorphingData
			, RenderPipeline::ePass
			, flags.programFlags );
		auto index = uint32_t( GlobalBuffersIdx::eCount ) + 1u;
#if C3D_UseTiledDirectionalShadowMap
		UBO_SHADOWMAP_DIRECTIONAL( writer
			, index++
			, RenderPipeline::ePass );
#else
		UBO_SHADOWMAP( writer
			, index++
			, RenderPipeline::ePass );
#endif

		auto skinningData = SkinningUbo::declare( writer
			, uint32_t( NodeUboIdx::eSkinningSsbo )
			, uint32_t( NodeUboIdx::eSkinningBones )
			, RenderPipeline::eBuffers
			, flags.programFlags );
#if C3D_UseTiledDirectionalShadowMap
		UBO_MODEL_INSTANCES( writer
			, NodeUboIdx::eModelInstances
			, RenderPipeline::eBuffers );
#endif

		sdw::Pcb pcb{ writer, "DrawData" };
		auto pipelineID = pcb.declMember< sdw::UInt >( "pipelineID" );
		auto customDrawID = pcb.declMember< sdw::UInt >( "customDrawID" );
		pcb.end();

		writer.implementMainT< shader::VertexSurfaceT, shader::FragmentSurfaceT >( sdw::VertexInT< shader::VertexSurfaceT >{ writer
				, flags.programFlags
				, getShaderFlags()
				, textureFlags
				, flags.passFlags
				, hasTextures }
			, sdw::VertexOutT< shader::FragmentSurfaceT >{ writer
				, flags.programFlags
				, getShaderFlags()
				, textureFlags
				, flags.passFlags
				, hasTextures }
			, [&]( VertexInT< shader::VertexSurfaceT > in
			, VertexOutT< shader::FragmentSurfaceT > out )
			{
				auto curPosition = writer.declLocale( "curPosition"
					, in.position );
				auto v4Normal = writer.declLocale( "v4Normal"
					, vec4( in.normal, 0.0_f ) );
				auto v4Tangent = writer.declLocale( "v4Tangent"
					, vec4( in.tangent, 0.0_f ) );
				auto objectIdsData = writer.declLocale( "objectIdsData"
					, c3d_objectIdsData[pipelineID][customDrawID] );
				auto nodeId = writer.declLocale( "nodeId"
					, shader::ObjectsIds::getNodeId( objectIdsData ) );
				auto morphingId = writer.declLocale( "morphingId"
					, shader::ObjectsIds::getMorphingId( objectIdsData )
					, checkFlag( flags.programFlags, ProgramFlag::eMorphing ) );
				out.texture0 = in.texture0;
				auto morphingData = writer.declLocale( "morphingData"
					, c3d_morphingData[morphingId]
					, checkFlag( flags.programFlags, ProgramFlag::eMorphing ) );
				in.morph( morphingData
					, curPosition
					, v4Normal
					, v4Tangent
					, out.texture0 );
				auto modelData = writer.declLocale( "modelData"
					, c3d_modelsData[nodeId] );
				out.textures0 = modelData.getTextures0( flags.programFlags
					, in.textures0 );
				out.textures1 = modelData.getTextures1( flags.programFlags
					, in.textures1 );
				out.textures = modelData.getTextures( flags.programFlags
					, in.textures );
				out.material = modelData.getMaterialId( flags.programFlags
					, in.material );
				out.nodeId = writer.cast< sdw::Int >( nodeId );
				out.instanceId = writer.cast< UInt >( in.instanceIndex );

				auto mtxModel = writer.declLocale< Mat4 >( "mtxModel"
					, modelData.getCurModelMtx( flags.programFlags
						, skinningData
						, in.instanceIndex - in.baseInstance
						, in.vertexIndex - in.baseVertex ) );
				auto worldPos = writer.declLocale( "worldPos"
					, mtxModel * curPosition );
				out.worldPosition = worldPos;

#if C3D_UseTiledDirectionalShadowMap
				auto ti = writer.declLocale( "tileIndex"
					, c3d_shadowMapDirectionalData.getTileIndex( c3d_modelInstancesData, in ) );
				auto tileMin = writer.declLocale( "tileMin"
					, c3d_shadowMapDirectionalData.getTileMin( ti ) );
				auto tileMax = writer.declLocale( "tileMax"
					, c3d_shadowMapDirectionalData.getTileMax( tileMin ) );

				curPosition = c3d_shadowMapDirectionalData.worldToView( ti, worldPos );
				curPosition = c3d_shadowMapDirectionalData.viewToProj( ti, curPosition );
				out.vtx.position = curPosition;

				out.vtx.clipDistance[0] = dot( vec4( 1.0_f, 0.0_f, 0.0_f, -tileMin.x() ), curPosition );
				out.vtx.clipDistance[1] = dot( vec4( -1.0_f, 0.0_f, 0.0_f, tileMax.x() ), curPosition );
				out.vtx.clipDistance[2] = dot( vec4( 0.0_f, -1.0_f, 0.0_f, -tileMin.y() ), curPosition );
				out.vtx.clipDistance[3] = dot( vec4( 0.0_f, 1.0_f, 0.0_f, tileMax.y() ), curPosition );
#else
				curPosition = c3d_shadowMapData.worldToView( worldPos );
				out.vtx.position = c3d_shadowMapData.viewToProj( curPosition );
#endif

				auto mtxNormal = writer.declLocale< Mat3 >( "mtxNormal"
					, modelData.getNormalMtx( flags.programFlags, mtxModel ) );
				out.computeTangentSpace( flags.programFlags
					, vec3( 0.0_f )
					, worldPos.xyz()
					, mtxNormal
					, v4Normal
					, v4Tangent );
			} );
		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
	}

	ShaderPtr ShadowMapPassDirectional::doGetPixelShaderSource( PipelineFlags const & flags )const
	{
		auto & renderSystem = *getEngine()->getRenderSystem();

		using namespace sdw;
		FragmentWriter writer;
		auto textureFlags = filterTexturesFlags( flags.textures );
		bool hasTextures = !flags.textures.empty();

		shader::Utils utils{ writer, *renderSystem.getEngine() };

		shader::Materials materials{ writer
			, uint32_t( GlobalBuffersIdx::eMaterials )
			, RenderPipeline::ePass };
		shader::TextureConfigurations textureConfigs{ writer
			, uint32_t( GlobalBuffersIdx::eTexConfigs )
			, RenderPipeline::ePass
			, hasTextures };
		shader::TextureAnimations textureAnims{ writer
			, uint32_t( GlobalBuffersIdx::eTexAnims )
			, RenderPipeline::ePass
			, hasTextures };
		auto index = uint32_t( GlobalBuffersIdx::eCount );
		auto lightsIndex = index++;
#if C3D_UseTiledDirectionalShadowMap
		UBO_SHADOWMAP_DIRECTIONAL( writer
			, index++
			, RenderPipeline::ePass );
#else
		UBO_SHADOWMAP( writer
			, index++
			, RenderPipeline::ePass );
#endif
		auto lightingModel = shader::LightingModel::createModel( utils
			, shader::getLightingModelName( *getEngine(), flags.passType )
			, LightType::eDirectional
			, lightsIndex
			, RenderPipeline::ePass
			, false
			, shader::ShadowOptions{ SceneFlag::eNone, false }
			, index
			, RenderPipeline::ePass
			, renderSystem.getGpuInformations().hasShaderStorageBuffers() );


		auto c3d_maps( writer.declCombinedImgArray< FImg2DRgba32 >( "c3d_maps"
			, 0u
			, RenderPipeline::eTextures
			, hasTextures ) );

		// Fragment Outputs
		auto pxl_normalLinear( writer.declOutput< Vec4 >( "pxl_normalLinear", 0u ) );
		auto pxl_variance( writer.declOutput< Vec2 >( "pxl_variance", 1u ) );
		auto pxl_position( writer.declOutput< Vec4 >( "pxl_position", 2u ) );
		auto pxl_flux( writer.declOutput< Vec4 >( "pxl_flux", 3u ) );

		writer.implementMainT< shader::FragmentSurfaceT, VoidT >( sdw::FragmentInT< shader::FragmentSurfaceT >{ writer
				, flags.programFlags
				, getShaderFlags()
				, textureFlags
				, flags.passFlags
				, hasTextures }
			, FragmentOut{ writer }
			, [&]( FragmentInT< shader::FragmentSurfaceT > in
				, FragmentOut out )
			{
				pxl_normalLinear = vec4( 0.0_f );
				pxl_variance = vec2( 0.0_f );
				pxl_position = vec4( 0.0_f );
				pxl_flux = vec4( 0.0_f );
				auto texCoord = writer.declLocale( "texCoord"
					, in.texture0 );
				auto normal = writer.declLocale( "normal"
					, normalize( in.normal ) );
				auto tangent = writer.declLocale( "tangent"
					, normalize( in.tangent ) );
				auto bitangent = writer.declLocale( "bitangent"
					, normalize( in.bitangent ) );
				auto material = materials.getMaterial( in.material );
				auto emissive = writer.declLocale( "emissive"
					, vec3( material.emissive ) );
				auto alpha = writer.declLocale( "alpha"
					, material.opacity );
				auto alphaRef = writer.declLocale( "alphaRef"
					, material.alphaRef );
				auto lightMat = lightingModel->declMaterial( "lightMat" );
				lightMat->create( material );
				auto occlusion = writer.declLocale( "occlusion"
					, 1.0_f );
				auto transmittance = writer.declLocale( "transmittance"
					, 0.0_f );

				if ( hasTextures )
				{
					lightingModel->computeMapContributions( flags.passFlags
						, flags.textures
						, textureConfigs
						, textureAnims
						, c3d_maps
						, in.textures0
						, in.textures1
						, texCoord
						, normal
						, tangent
						, bitangent
						, emissive
						, alpha
						, occlusion
						, transmittance
						, *lightMat
						, in.tangentSpaceViewPosition
						, in.tangentSpaceFragPosition );
				}

				utils.applyAlphaFunc( flags.alphaFunc
					, alpha
					, alphaRef );

				auto lightDiffuse = writer.declLocale( "lightDiffuse"
					, vec3( 0.0_f ) );
				auto lightSpecular = writer.declLocale( "lightSpecular"
					, vec3( 0.0_f ) );
				shader::OutputComponents output{ lightDiffuse, lightSpecular };
#if C3D_UseTiledDirectionalShadowMap
				auto light = writer.declLocale( "light"
					, c3d_shadowMapDirectionalData.getDirectionalLight( *lightingModel ) );
#else
				auto light = writer.declLocale( "light"
					, c3d_shadowMapData.getDirectionalLight( *lightingModel ) );
#endif
				pxl_flux.rgb() = lightMat->albedo
					* light.m_lightBase.m_colour
					* light.m_lightBase.m_intensity.x()
					/** clamp( dot( normalize( light.m_direction ), normal ), 0.0_f, 1.0_f )*/;

				auto depth = writer.declLocale( "depth"
					, in.fragCoord.z() );
				pxl_normalLinear.w() = depth;
				pxl_normalLinear.xyz() = normal;
				pxl_position.xyz() = in.worldPosition.xyz();

				pxl_variance.x() = depth;
				pxl_variance.y() = depth * depth;

				auto dx = writer.declLocale( "dx"
					, dFdx( depth ) );
				auto dy = writer.declLocale( "dy"
					, dFdy( depth ) );
				pxl_variance.y() += 0.25_f * ( dx * dx + dy * dy );
			} );

		return std::make_unique< ast::Shader >( std::move( writer.getShader() ) );
	}
}

#include "Castor3D/Shader/Shaders/GlslPbrLighting.hpp"

#include "Castor3D/DebugDefines.hpp"
#include "Castor3D/Shader/Shaders/GlslLight.hpp"
#include "Castor3D/Shader/Shaders/GlslMaterial.hpp"
#include "Castor3D/Shader/Shaders/GlslOutputComponents.hpp"
#include "Castor3D/Shader/Shaders/GlslShadow.hpp"
#include "Castor3D/Shader/Shaders/GlslSurface.hpp"
#include "Castor3D/Shader/Shaders/GlslTextureConfiguration.hpp"
#include "Castor3D/Shader/Shaders/GlslUtils.hpp"
#include "Castor3D/Shader/Ubos/SceneUbo.hpp"

#include <ShaderWriter/Source.hpp>

using namespace castor;
using namespace sdw;

namespace castor3d
{
	namespace shader
	{
		const String PbrLightingModel::Name = cuT( "pbr" );

		PbrLightingModel::PbrLightingModel( ShaderWriter & writer
			, Utils & utils
			, ShadowOptions shadowOptions
			, bool isOpaqueProgram )
			: LightingModel{ writer
				, utils
				, std::move( shadowOptions )
				, isOpaqueProgram }
			, m_cookTorrance{ writer, utils }
		{
		}

		void PbrLightingModel::computeCombined( Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, SceneData const & sceneData
			, Surface surface
			, OutputComponents & parentOutput )const
		{
			auto begin = m_writer.declLocale( "begin"
				, 0_i );
			auto end = m_writer.declLocale( "end"
				, sceneData.getDirectionalLightCount() );

#if C3D_UseTiledDirectionalShadowMap
			FOR( m_writer, Int, dir, begin, dir < end, ++dir )
			{
				compute( getTiledDirectionalLight( dir )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface
					, parentOutput );
			}
			ROF;
#else
			FOR( m_writer, Int, dir, begin, dir < end, ++dir )
			{
				compute( getDirectionalLight( dir )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface
					, parentOutput );
			}
			ROF;
#endif

			begin = end;
			end += sceneData.getPointLightCount();

			FOR( m_writer, Int, point, begin, point < end, ++point )
			{
				compute( getPointLight( point )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface
					, parentOutput );
			}
			ROF;

			begin = end;
			end += sceneData.getSpotLightCount();

			FOR( m_writer, Int, spot, begin, spot < end, ++spot )
			{
				compute( getSpotLight( spot )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface
					, parentOutput );
			}
			ROF;
		}

		void PbrLightingModel::compute( TiledDirectionalLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface
			, OutputComponents & parentOutput )const
		{
			m_computeTiledDirectional( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface
				, parentOutput );
		}

		void PbrLightingModel::compute( DirectionalLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface
			, OutputComponents & parentOutput )const
		{
			m_computeDirectional( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface
				, parentOutput );
		}

		void PbrLightingModel::compute( PointLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface
			, OutputComponents & parentOutput )const
		{
			m_computePoint( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface
				, parentOutput );
		}

		void PbrLightingModel::compute( SpotLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface
			, OutputComponents & parentOutput )const
		{
			m_computeSpot( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface
				, parentOutput );
		}

		sdw::Vec3 PbrLightingModel::combine( sdw::Vec3 const & directDiffuse
			, sdw::Vec3 const & indirectDiffuse
			, sdw::Vec3 const & directSpecular
			, sdw::Vec3 const & indirectSpecular
			, sdw::Vec3 const & ambient
			, sdw::Vec3 const & indirectAmbient
			, sdw::Float const & ambientOcclusion
			, sdw::Vec3 const & emissive
			, sdw::Vec3 const & reflected
			, sdw::Vec3 const & refracted
			, sdw::Vec3 const & materialAlbedo )
		{
			return materialAlbedo * ( directDiffuse + ( indirectDiffuse * ambientOcclusion ) )
				+ directSpecular + ( indirectSpecular * ambientOcclusion )
				+ emissive
				+ refracted
				+ ( reflected * ambient * indirectAmbient * ambientOcclusion );
		}

		Vec3 PbrLightingModel::computeCombinedDiffuse( Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, SceneData const & sceneData
			, Surface surface )const
		{
			auto begin = m_writer.declLocale( "begin"
				, 0_i );
			auto end = m_writer.declLocale( "end"
				, sceneData.getDirectionalLightCount() );
			auto result = m_writer.declLocale( "result"
				, vec3( 0.0_f ) );

#if C3D_UseTiledDirectionalShadowMap
			FOR( m_writer, Int, dir, begin, dir < end, ++dir )
			{
				result += computeDiffuse( getTiledDirectionalLight( dir )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface );
			}
			ROF;
#else
			FOR( m_writer, Int, dir, begin, dir < end, ++dir )
			{
				result += computeDiffuse( getDirectionalLight( dir )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface );
			}
			ROF;
#endif

			begin = end;
			end += sceneData.getPointLightCount();

			FOR( m_writer, Int, point, begin, point < end, ++point )
			{
				result += computeDiffuse( getPointLight( point )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface );
			}
			ROF;

			begin = end;
			end += sceneData.getSpotLightCount();

			FOR( m_writer, Int, spot, begin, spot < end, ++spot )
			{
				result += computeDiffuse( getSpotLight( spot )
					, worldEye
					, specular
					, metalness
					, roughness
					, receivesShadows
					, surface );
			}
			ROF;

			return result;
		}

		Vec3 PbrLightingModel::computeDiffuse( TiledDirectionalLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface )const
		{
			return m_computeTiledDirectionalDiffuse( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface );
		}

		Vec3 PbrLightingModel::computeDiffuse( DirectionalLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface )const
		{
			return m_computeDirectionalDiffuse( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface );
		}

		Vec3 PbrLightingModel::computeDiffuse( PointLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface )const
		{
			return m_computePointDiffuse( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface );
		}

		Vec3 PbrLightingModel::computeDiffuse( SpotLight const & light
			, Vec3 const & worldEye
			, Vec3 const & specular
			, Float const & metalness
			, Float const & roughness
			, Int const & receivesShadows
			, Surface surface )const
		{
			return m_computeSpotDiffuse( light
				, worldEye
				, specular
				, metalness
				, roughness
				, receivesShadows
				, surface );
		}

		std::shared_ptr< PbrLightingModel > PbrLightingModel::createModel( sdw::ShaderWriter & writer
			, Utils & utils
			, uint32_t lightsBufBinding
			, uint32_t lightsBufSet
			, ShadowOptions const & shadows
			, uint32_t & shadowMapBinding
			, uint32_t shadowMapSet
			, bool isOpaqueProgram )
		{
			auto result = std::make_shared< PbrLightingModel >( writer
				, utils
				, shadows
				, isOpaqueProgram );
			result->declareModel( lightsBufBinding
				, lightsBufSet
				, shadowMapBinding
				, shadowMapSet );
			return result;
		}

		std::shared_ptr< PbrLightingModel > PbrLightingModel::createModel( sdw::ShaderWriter & writer
			, Utils & utils
			, LightType lightType
			, bool lightUbo
			, uint32_t lightBinding
			, uint32_t lightSet
			, ShadowOptions const & shadows
			, uint32_t & shadowMapBinding
			, uint32_t shadowMapSet )
		{
			auto result = std::make_shared< PbrLightingModel >( writer
				, utils
				, shadows
				, true );

			switch ( lightType )
			{
			case LightType::eDirectional:
				result->declareDirectionalModel( lightUbo
					, lightBinding
					, lightSet
					, shadowMapBinding
					, shadowMapSet );
				break;

			case LightType::ePoint:
				result->declarePointModel( lightUbo
					, lightBinding
					, lightSet
					, shadowMapBinding
					, shadowMapSet );
				break;

			case LightType::eSpot:
				result->declareSpotModel( lightUbo
					, lightBinding
					, lightSet
					, shadowMapBinding
					, shadowMapSet );
				break;

			default:
				CU_Failure( "Invalid light type" );
				break;
			}

			return result;
		}

		std::shared_ptr< PbrLightingModel > PbrLightingModel::createDiffuseModel( sdw::ShaderWriter & writer
			, Utils & utils
			, uint32_t lightsBufBinding
			, uint32_t lightsBufSet
			, ShadowOptions const & shadows
			, uint32_t & shadowMapBinding
			, uint32_t shadowMapSet
			, bool isOpaqueProgram )
		{
			auto result = std::make_shared< PbrLightingModel >( writer
				, utils
				, shadows
				, isOpaqueProgram );
			result->declareDiffuseModel( lightsBufBinding
				, lightsBufSet
				, shadowMapBinding
				, shadowMapSet );
			return result;
		}

		void PbrLightingModel::computeMapContributions( PassFlags const & passFlags
			, FilteredTextureFlags const & textures
			, sdw::Float const & gamma
			, TextureConfigurations const & textureConfigs
			, sdw::Array< sdw::SampledImage2DRgba32 > const & maps
			, sdw::Vec3 & texCoords
			, sdw::Vec3 & normal
			, sdw::Vec3 & tangent
			, sdw::Vec3 & bitangent
			, sdw::Vec3 & emissive
			, sdw::Float & opacity
			, sdw::Float & occlusion
			, sdw::Float & transmittance
			, sdw::Vec3 & albedo
			, sdw::Float & metalness
			, sdw::Float & roughness
			, sdw::Vec3 & tangentSpaceViewPosition
			, sdw::Vec3 & tangentSpaceFragPosition )
		{
			bool hasEmissive = false;
			m_utils.computeGeometryMapsContributions( textures
				, passFlags
				, textureConfigs
				, maps
				, texCoords
				, opacity
				, normal
				, tangent
				, bitangent
				, tangentSpaceViewPosition
				, tangentSpaceFragPosition );

			for ( auto & textureIt : textures )
			{
				if ( textureIt.second.flags != TextureFlag::eOpacity
					&& textureIt.second.flags != TextureFlag::eNormal
					&& textureIt.second.flags != TextureFlag::eHeight )
				{
					auto name = string::stringCast< char >( string::toString( textureIt.first, std::locale{ "C" } ) );
					auto config = m_writer.declLocale( "config" + name
						, textureConfigs.getTextureConfiguration( m_writer.cast< UInt >( textureIt.second.id ) ) );
					auto sampled = m_writer.declLocale( "sampled" + name
						, m_utils.computeCommonMapContribution( textureIt.second.flags
							, passFlags
							, name
							, config
							, maps[textureIt.first]
							, gamma
							, texCoords
							, emissive
							, opacity
							, occlusion
							, transmittance
							, tangentSpaceViewPosition
							, tangentSpaceFragPosition ) );

					if ( checkFlag( textureIt.second.flags, TextureFlag::eAlbedo ) )
					{
						albedo = config.getAlbedo( m_writer, sampled, albedo, gamma );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eMetalness ) )
					{
						metalness = config.getMetalness( m_writer, sampled, metalness );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eRoughness ) )
					{
						roughness = config.getGlossiness( m_writer, sampled, roughness );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eEmissive ) )
					{
						hasEmissive = true;
					}
				}
			}

			if ( checkFlag( passFlags, PassFlag::eLighting )
				&& !hasEmissive )
			{
				emissive *= albedo;
			}
		}

		void PbrLightingModel::computeMapVoxelContributions( PassFlags const & passFlags
			, FilteredTextureFlags const & textures
			, sdw::Float const & gamma
			, TextureConfigurations const & textureConfigs
			, sdw::Array< sdw::SampledImage2DRgba32 > const & maps
			, sdw::Vec3 const & texCoords
			, sdw::Vec3 & emissive
			, sdw::Float & opacity
			, sdw::Float & occlusion
			, sdw::Vec3 & albedo
			, sdw::Float & metalness
			, sdw::Float & roughness )
		{
			bool hasEmissive = false;

			for ( auto & textureIt : textures )
			{
				auto name = string::stringCast< char >( string::toString( textureIt.first, std::locale{ "C" } ) );
				auto config = m_writer.declLocale( "config" + name
					, textureConfigs.getTextureConfiguration( m_writer.cast< UInt >( textureIt.second.id ) ) );
				auto sampled = m_writer.declLocale( "sampled" + name
					, m_utils.computeCommonMapVoxelContribution( textureIt.second.flags
						, passFlags
						, name
						, config
						, maps[textureIt.first]
						, gamma
						, texCoords
						, emissive
						, opacity
						, occlusion ) );

				if ( checkFlag( textureIt.second.flags, TextureFlag::eAlbedo ) )
				{
					albedo = config.getAlbedo( m_writer, sampled, albedo, gamma );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eMetalness ) )
				{
					metalness = config.getMetalness( m_writer, sampled, metalness );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eRoughness ) )
				{
					roughness = config.getGlossiness( m_writer, sampled, roughness );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eEmissive ) )
				{
					hasEmissive = true;
				}
			}

			if ( checkFlag( passFlags, PassFlag::eLighting ) 
				&& !hasEmissive )
			{
				emissive *= albedo;
			}
		}

		void PbrLightingModel::computeMapContributions( PassFlags const & passFlags
			, FilteredTextureFlags const & textures
			, sdw::Float const & gamma
			, TextureConfigurations const & textureConfigs
			, sdw::Array< sdw::SampledImage2DRgba32 > const & maps
			, sdw::Vec3 & texCoords
			, sdw::Vec3 & normal
			, sdw::Vec3 & tangent
			, sdw::Vec3 & bitangent
			, sdw::Vec3 & emissive
			, sdw::Float & opacity
			, sdw::Float & occlusion
			, sdw::Float & transmittance
			, sdw::Vec3 & albedo
			, sdw::Vec3 & specular
			, sdw::Float & glossiness
			, sdw::Vec3 & tangentSpaceViewPosition
			, sdw::Vec3 & tangentSpaceFragPosition )
		{
			bool hasEmissive = false;
			m_utils.computeGeometryMapsContributions( textures
				, passFlags
				, textureConfigs
				, maps
				, texCoords
				, opacity
				, normal
				, tangent
				, bitangent
				, tangentSpaceViewPosition
				, tangentSpaceFragPosition );

			for ( auto & textureIt : textures )
			{
				if ( textureIt.second.flags != TextureFlag::eOpacity
					&& textureIt.second.flags != TextureFlag::eNormal
					&& textureIt.second.flags != TextureFlag::eHeight )
				{
					auto name = string::stringCast< char >( string::toString( textureIt.first, std::locale{ "C" } ) );
					auto config = m_writer.declLocale( "config" + name
						, textureConfigs.getTextureConfiguration( m_writer.cast< UInt >( textureIt.second.id ) ) );
					auto sampled = m_writer.declLocale( "sampled" + name
						, m_utils.computeCommonMapContribution( textureIt.second.flags
							, passFlags
							, name
							, config
							, maps[textureIt.first]
							, gamma
							, texCoords
							, emissive
							, opacity
							, occlusion
							, transmittance
							, tangentSpaceViewPosition
							, tangentSpaceFragPosition ) );

					if ( checkFlag( textureIt.second.flags, TextureFlag::eAlbedo ) )
					{
						albedo = config.getAlbedo( m_writer, sampled, albedo, gamma );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eSpecular ) )
					{
						specular = config.getSpecular( m_writer, sampled, specular );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eGlossiness ) )
					{
						glossiness = config.getGlossiness( m_writer, sampled, glossiness );
					}

					if ( checkFlag( textureIt.second.flags, TextureFlag::eEmissive ) )
					{
						hasEmissive = true;
					}
				}
			}

			if ( checkFlag( passFlags, PassFlag::eLighting )
				&& !hasEmissive )
			{
				emissive *= albedo;
			}
		}

		void PbrLightingModel::computeMapVoxelContributions( PassFlags const & passFlags
			, FilteredTextureFlags const & textures
			, sdw::Float const & gamma
			, TextureConfigurations const & textureConfigs
			, sdw::Array< sdw::SampledImage2DRgba32 > const & maps
			, sdw::Vec3 const & texCoords
			, sdw::Vec3 & emissive
			, sdw::Float & opacity
			, sdw::Float & occlusion
			, sdw::Vec3 & albedo
			, sdw::Vec3 & specular
			, sdw::Float & glossiness )
		{
			bool hasEmissive = false;

			for ( auto & textureIt : textures )
			{
				auto name = string::stringCast< char >( string::toString( textureIt.first, std::locale{ "C" } ) );
				auto config = m_writer.declLocale( "config" + name
					, textureConfigs.getTextureConfiguration( m_writer.cast< UInt >( textureIt.second.id ) ) );
				auto sampled = m_writer.declLocale( "sampled" + name
					, m_utils.computeCommonMapVoxelContribution( textureIt.second.flags
						, passFlags
						, name
						, config
						, maps[textureIt.first]
						, gamma
						, texCoords
						, emissive
						, opacity
						, occlusion ) );

				if ( checkFlag( textureIt.second.flags, TextureFlag::eAlbedo ) )
				{
					albedo = config.getAlbedo( m_writer, sampled, albedo, gamma );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eSpecular ) )
				{
					specular = config.getSpecular( m_writer, sampled, specular );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eGlossiness ) )
				{
					glossiness = config.getGlossiness( m_writer, sampled, glossiness );
				}

				if ( checkFlag( textureIt.second.flags, TextureFlag::eEmissive ) )
				{
					hasEmissive = true;
				}
			}

			if ( checkFlag( passFlags, PassFlag::eLighting ) 
				&& !hasEmissive )
			{
				emissive *= albedo;
			}
		}

		void PbrLightingModel::doDeclareModel()
		{
			m_cookTorrance.declare();
		}

		void PbrLightingModel::doDeclareComputeDirectionalLight()
		{
			OutputComponents output{ m_writer };
#if C3D_UseTiledDirectionalShadowMap
			m_computeTiledDirectional = m_writer.implementFunction< sdw::Void >( "computeDirectionalLight"
				, [this]( TiledDirectionalLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface
					, OutputComponents & parentOutput )
				{
					OutputComponents output
					{
						m_writer.declLocale( "lightDiffuse", vec3( 0.0_f ) ),
						m_writer.declLocale( "lightSpecular", vec3( 0.0_f ) )
					};
					PbrSGMaterials materials{ m_writer };
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto cascadeFactors = m_writer.declLocale( "cascadeFactors"
								, vec3( 0.0_f, 1.0_f, 0.0_f ) );
							auto cascadeIndex = m_writer.declLocale( "cascadeIndex"
								, 0_u );
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF ( m_writer, receivesShadows != 0_i )
							{
								auto c3d_maxCascadeCount = m_writer.getVariable< UInt >( "c3d_maxCascadeCount" );
								auto maxCount = m_writer.declLocale( "maxCount"
									, m_writer.cast< UInt >( clamp( light.m_cascadeCount, 1_u, c3d_maxCascadeCount ) - 1_u ) );

								// Get cascade index for the current fragment's view position
								FOR( m_writer, UInt, i, 0u, i < maxCount, ++i )
								{
									auto factors = m_writer.declLocale( "factors"
										, m_getTileFactors( Vec3{ surface.viewPosition }
											, light.m_splitDepths
											, i ) );

									IF( m_writer, factors.x() != 0.0_f )
									{
										cascadeFactors = factors;
									}
									FI;
								}
								ROF;

								cascadeIndex = m_writer.cast< UInt >( cascadeFactors.x() );
								shadowFactor = cascadeFactors.y()
									* max( 1.0_f - m_writer.cast< Float >( receivesShadows )
										, m_shadowModel->computeDirectional( light.m_lightBase
											, surface
											, light.m_transforms[cascadeIndex]
											, -lightDirection
											, cascadeIndex
											, light.m_cascadeCount ) );

								IF( m_writer, cascadeIndex > 0_u )
								{
									shadowFactor += cascadeFactors.z()
										* max( 1.0_f - m_writer.cast< Float >( receivesShadows )
											, m_shadowModel->computeDirectional( light.m_lightBase
												, surface
												, light.m_transforms[cascadeIndex - 1u]
												, -lightDirection
												, cascadeIndex - 1u
												, light.m_cascadeCount ) );
								}
								FI;
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								m_cookTorrance.compute( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, roughness
									, surface
									, output );
								output.m_diffuse *= shadowFactor;
								output.m_specular *= shadowFactor;
							}
							FI;

							if ( m_isOpaqueProgram )
							{
								IF( m_writer, light.m_lightBase.m_volumetricSteps != 0_u )
								{
									m_shadowModel->computeVolumetric( light.m_lightBase
										, surface
										, worldEye
										, light.m_transforms[cascadeIndex]
										, light.m_direction
										, cascadeIndex
										, light.m_cascadeCount
										, output );
								}
								FI;
							}

#if C3D_DebugCascades
							IF( m_writer, cascadeIndex == 0_u )
							{
								output.m_diffuse.rgb() *= vec3( 1.0_f, 0.25f, 0.25f );
								output.m_specular.rgb() *= vec3( 1.0_f, 0.25f, 0.25f );
							}
							ELSEIF( cascadeIndex == 1_u )
							{
								output.m_diffuse.rgb() *= vec3( 0.25_f, 1.0f, 0.25f );
								output.m_specular.rgb() *= vec3( 0.25_f, 1.0f, 0.25f );
							}
							ELSEIF( cascadeIndex == 2_u )
							{
								output.m_diffuse.rgb() *= vec3( 0.25_f, 0.25f, 1.0f );
								output.m_specular.rgb() *= vec3( 0.25_f, 0.25f, 1.0f );
							}
							ELSE
							{
								output.m_diffuse.rgb() *= vec3( 1.0_f, 1.0f, 0.25f );
								output.m_specular.rgb() *= vec3( 1.0_f, 1.0f, 0.25f );
							}
							FI;
#endif
						}
						ELSE
						{
							m_cookTorrance.compute( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, roughness
								, surface
								, output );
						}
						FI;
					}
					else
					{
						m_cookTorrance.compute( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, roughness
							, surface
							, output );
					}

					parentOutput.m_diffuse += max( vec3( 0.0_f ), output.m_diffuse );
					parentOutput.m_specular += max( vec3( 0.0_f ), output.m_specular );
				}
				, InTiledDirectionalLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" }
				, output );
#else
			m_computeDirectional = m_writer.implementFunction< sdw::Void >( "computeDirectionalLight"
				, [this]( DirectionalLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface
					, OutputComponents & parentOutput )
				{
					OutputComponents output
					{
						m_writer.declLocale( "lightDiffuse", vec3( 0.0_f ) ),
						m_writer.declLocale( "lightSpecular", vec3( 0.0_f ) )
					};
					PbrSGMaterials materials{ m_writer };
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto cascadeFactors = m_writer.declLocale( "cascadeFactors"
								, vec3( 0.0_f, 1.0_f, 0.0_f ) );
							auto cascadeIndex = m_writer.declLocale( "cascadeIndex"
								, 0_u );
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF ( m_writer, receivesShadows != 0_i )
							{
								auto c3d_maxCascadeCount = m_writer.getVariable< UInt >( "c3d_maxCascadeCount" );
								auto maxCount = m_writer.declLocale( "maxCount"
									, m_writer.cast< UInt >( clamp( light.m_cascadeCount, 1_u, c3d_maxCascadeCount ) - 1_u ) );

								// Get cascade index for the current fragment's view position
								FOR( m_writer, UInt, i, 0u, i < maxCount, ++i )
								{
									auto factors = m_writer.declLocale( "factors"
										, m_getCascadeFactors( Vec3{ surface.viewPosition }
											, light.m_splitDepths
											, i ) );

									IF( m_writer, factors.x() != 0.0_f )
									{
										cascadeFactors = factors;
									}
									FI;
								}
								ROF;

								cascadeIndex = m_writer.cast< UInt >( cascadeFactors.x() );
								shadowFactor = cascadeFactors.y()
									* max( 1.0_f - m_writer.cast< Float >( receivesShadows )
										, m_shadowModel->computeDirectional( light.m_lightBase
											, surface
											, light.m_transforms[cascadeIndex]
											, -lightDirection
											, cascadeIndex
											, light.m_cascadeCount ) );

								IF( m_writer, cascadeIndex > 0_u )
								{
									shadowFactor += cascadeFactors.z()
										* max( 1.0_f - m_writer.cast< Float >( receivesShadows )
											, m_shadowModel->computeDirectional( light.m_lightBase
												, surface
												, light.m_transforms[cascadeIndex - 1u]
												, -lightDirection
												, cascadeIndex - 1u
												, light.m_cascadeCount ) );
								}
								FI;
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								m_cookTorrance.compute( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, roughness
									, surface
									, output );
								output.m_diffuse *= shadowFactor;
								output.m_specular *= shadowFactor;
							}
							FI;

							if ( m_isOpaqueProgram )
							{
								IF( m_writer, light.m_lightBase.m_volumetricSteps != 0_u )
								{
									m_shadowModel->computeVolumetric( light.m_lightBase
										, surface
										, worldEye
										, light.m_transforms[cascadeIndex]
										, light.m_direction
										, cascadeIndex
										, light.m_cascadeCount
										, output );
								}
								FI;
							}

#if C3D_DebugCascades
							IF( m_writer, cascadeIndex == 0_u )
							{
								output.m_diffuse.rgb() *= vec3( 1.0_f, 0.25f, 0.25f );
								output.m_specular.rgb() *= vec3( 1.0_f, 0.25f, 0.25f );
							}
							ELSEIF( cascadeIndex == 1_u )
							{
								output.m_diffuse.rgb() *= vec3( 0.25_f, 1.0f, 0.25f );
								output.m_specular.rgb() *= vec3( 0.25_f, 1.0f, 0.25f );
							}
							ELSEIF( cascadeIndex == 2_u )
							{
								output.m_diffuse.rgb() *= vec3( 0.25_f, 0.25f, 1.0f );
								output.m_specular.rgb() *= vec3( 0.25_f, 0.25f, 1.0f );
							}
							ELSE
							{
								output.m_diffuse.rgb() *= vec3( 1.0_f, 1.0f, 0.25f );
								output.m_specular.rgb() *= vec3( 1.0_f, 1.0f, 0.25f );
							}
							FI;
#endif
						}
						ELSE
						{
							m_cookTorrance.compute( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, roughness
								, surface
								, output );
						}
						FI;
					}
					else
					{
						m_cookTorrance.compute( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, roughness
							, surface
							, output );
					}

					parentOutput.m_diffuse += max( vec3( 0.0_f ), output.m_diffuse );
					parentOutput.m_specular += max( vec3( 0.0_f ), output.m_specular );
				}
				, InDirectionalLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" }
				, output );
#endif
		}

		void PbrLightingModel::doDeclareComputePointLight()
		{
			OutputComponents output{ m_writer };
			m_computePoint = m_writer.implementFunction< sdw::Void >( "computePointLight"
				, [this]( PointLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface
					, OutputComponents & parentOutput )
				{
					OutputComponents output
					{
						m_writer.declLocale( "lightDiffuse", vec3( 0.0_f ) ),
						m_writer.declLocale( "lightSpecular", vec3( 0.0_f ) )
					};
					PbrSGMaterials materials{ m_writer };
					auto lightToVertex = m_writer.declLocale( "lightToVertex"
						, light.m_position.xyz() - surface.worldPosition );
					auto distance = m_writer.declLocale( "distance"
						, length( lightToVertex ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( lightToVertex ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF( m_writer, receivesShadows != 0_i )
							{
								shadowFactor = max( 1.0_f - m_writer.cast< Float >( receivesShadows )
									, m_shadowModel->computePoint( light.m_lightBase
										, surface
										, light.m_position.xyz() ) );
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								m_cookTorrance.compute( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, roughness
									, surface
									, output );
								output.m_diffuse *= shadowFactor;
								output.m_specular *= shadowFactor;
							}
							FI;
						}
						ELSE
						{
							m_cookTorrance.compute( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, roughness
								, surface
								, output );
						}
						FI;
					}
					else
					{
						m_cookTorrance.compute( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, roughness
							, surface
							, output );
					}

					auto attenuation = m_writer.declLocale( "attenuation"
						, sdw::fma( light.m_attenuation.z()
							, distance * distance
							, sdw::fma( light.m_attenuation.y()
								, distance
								, light.m_attenuation.x() ) ) );
					output.m_diffuse = output.m_diffuse / attenuation;
					output.m_specular = output.m_specular / attenuation;
					parentOutput.m_diffuse += max( vec3( 0.0_f ), output.m_diffuse );
					parentOutput.m_specular += max( vec3( 0.0_f ), output.m_specular );
				}
				, InPointLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" }
				, output );
		}

		void PbrLightingModel::doDeclareComputeSpotLight()
		{
			OutputComponents output{ m_writer };
			m_computeSpot = m_writer.implementFunction< sdw::Void >( "computeSpotLight"
				, [this]( SpotLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface
					, OutputComponents & parentOutput )
				{
					OutputComponents output
					{
						m_writer.declLocale( "lightDiffuse", vec3( 0.0_f ) ),
						m_writer.declLocale( "lightSpecular", vec3( 0.0_f ) )
					};
					PbrSGMaterials materials{ m_writer };
					auto lightToVertex = m_writer.declLocale( "lightToVertex"
						, light.m_position.xyz() - surface.worldPosition );
					auto distance = m_writer.declLocale( "distance"
						, length( lightToVertex ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( lightToVertex ) );
					auto spotFactor = m_writer.declLocale( "spotFactor"
						, dot( lightDirection, -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f - step( spotFactor, light.m_cutOff ) );
							shadowFactor *= max( 1.0_f - m_writer.cast< Float >( receivesShadows )
								, m_shadowModel->computeSpot( light.m_lightBase
									, surface
									, light.m_transform
									, -lightToVertex ) );

							IF( m_writer, shadowFactor )
							{
								m_cookTorrance.compute( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, roughness
									, surface
									, output );
								output.m_diffuse *= shadowFactor;
								output.m_specular *= shadowFactor;
							}
							FI;
						}
						ELSE
						{
							m_cookTorrance.compute( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, roughness
								, surface
								, output );
						}
						FI;
					}
					else
					{
						m_cookTorrance.compute( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, roughness
							, surface
							, output );
					}

					auto attenuation = m_writer.declLocale( "attenuation"
						, sdw::fma( light.m_attenuation.z()
							, distance * distance
							, sdw::fma( light.m_attenuation.y()
								, distance
								, light.m_attenuation.x() ) ) );
					spotFactor = sdw::fma( ( spotFactor - 1.0_f )
						, 1.0_f / ( 1.0_f - light.m_cutOff )
						, 1.0_f );
					output.m_diffuse = spotFactor * output.m_diffuse / attenuation;
					output.m_specular = spotFactor * output.m_specular / attenuation;
					parentOutput.m_diffuse += max( vec3( 0.0_f ), output.m_diffuse );
					parentOutput.m_specular += max( vec3( 0.0_f ), output.m_specular );
				}
				, InSpotLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" }
				, output );
		}

		void PbrLightingModel::doDeclareDiffuseModel()
		{
			m_cookTorrance.declareDiffuse();
		}

		void PbrLightingModel::doDeclareComputeDirectionalLightDiffuse()
		{
#if C3D_UseTiledDirectionalShadowMap
			m_computeTiledDirectionalDiffuse = m_writer.implementFunction< sdw::Vec3 >( "computeDirectionalLight"
				, [this]( TiledDirectionalLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface )
				{
					PbrSGMaterials materials{ m_writer };
					auto diffuse = m_writer.declLocale( "diffuse"
						, vec3( 0.0_f ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto cascadeFactors = m_writer.declLocale( "cascadeFactors"
								, vec3( 0.0_f, 1.0_f, 0.0_f ) );
							auto cascadeIndex = m_writer.declLocale( "cascadeIndex"
								, light.m_cascadeCount - 1_u );
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF ( m_writer, receivesShadows != 0_i )
							{
								shadowFactor = max( 1.0_f - m_writer.cast< Float >( receivesShadows )
									, m_shadowModel->computeDirectional( light.m_lightBase
										, surface
										, light.m_transforms[cascadeIndex]
										, -lightDirection
										, cascadeIndex
										, light.m_cascadeCount ) );
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								diffuse = shadowFactor * m_cookTorrance.computeDiffuse( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, surface );
							}
							FI;
						}
						ELSE
						{
							diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, surface );
						}
						FI;
					}
					else
					{
						diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, surface );
					}

					m_writer.returnStmt( max( vec3( 0.0_f ), diffuse ) );
				}
				, InTiledDirectionalLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" } );
#else
			m_computeDirectionalDiffuse = m_writer.implementFunction< sdw::Vec3 >( "computeDirectionalLight"
				, [this]( DirectionalLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface )
				{
					PbrSGMaterials materials{ m_writer };
					auto diffuse = m_writer.declLocale( "diffuse"
						, vec3( 0.0_f ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto cascadeFactors = m_writer.declLocale( "cascadeFactors"
								, vec3( 0.0_f, 1.0_f, 0.0_f ) );
							auto cascadeIndex = m_writer.declLocale( "cascadeIndex"
								, light.m_cascadeCount - 1_u );
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF ( m_writer, receivesShadows != 0_i )
							{
								shadowFactor = max( 1.0_f - m_writer.cast< Float >( receivesShadows )
									, m_shadowModel->computeDirectional( light.m_lightBase
										, surface
										, light.m_transforms[cascadeIndex]
										, -lightDirection
										, cascadeIndex
										, light.m_cascadeCount ) );
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								diffuse = shadowFactor * m_cookTorrance.computeDiffuse( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, surface );
							}
							FI;
						}
						ELSE
						{
							diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, surface );
						}
						FI;
					}
					else
					{
						diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, surface );
					}

					m_writer.returnStmt( max( vec3( 0.0_f ), diffuse ) );
				}
				, InDirectionalLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" } );
#endif
		}

		void PbrLightingModel::doDeclareComputePointLightDiffuse()
		{
			m_computePointDiffuse = m_writer.implementFunction< sdw::Vec3 >( "computePointLight"
				, [this]( PointLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface )
				{
					PbrSGMaterials materials{ m_writer };
					auto diffuse = m_writer.declLocale( "diffuse"
						, vec3( 0.0_f ) );
					auto lightToVertex = m_writer.declLocale( "lightToVertex"
						, light.m_position.xyz() - surface.worldPosition );
					auto distance = m_writer.declLocale( "distance"
						, length( lightToVertex ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( lightToVertex ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f );

							IF( m_writer, receivesShadows != 0_i )
							{
								shadowFactor = max( 1.0_f - m_writer.cast< Float >( receivesShadows )
									, m_shadowModel->computePoint( light.m_lightBase
										, surface
										, light.m_position.xyz() ) );
							}
							FI;

							IF( m_writer, shadowFactor )
							{
								diffuse = shadowFactor * m_cookTorrance.computeDiffuse( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, surface );
							}
							FI;
						}
						ELSE
						{
							diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, surface );
						}
						FI;
					}
					else
					{
						diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, surface );
					}

					auto attenuation = m_writer.declLocale( "attenuation"
						, sdw::fma( light.m_attenuation.z()
							, distance * distance
							, sdw::fma( light.m_attenuation.y()
								, distance
								, light.m_attenuation.x() ) ) );
					m_writer.returnStmt( max( vec3( 0.0_f ), diffuse / attenuation ) );
				}
				, InPointLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" } );
		}

		void PbrLightingModel::doDeclareComputeSpotLightDiffuse()
		{
			m_computeSpotDiffuse = m_writer.implementFunction< sdw::Vec3 >( "computeSpotLight"
				, [this]( SpotLight const & light
					, Vec3 const & worldEye
					, Vec3 const & specular
					, Float const & metalness
					, Float const & roughness
					, Int const & receivesShadows
					, Surface surface )
				{
					PbrSGMaterials materials{ m_writer };
					auto diffuse = m_writer.declLocale( "diffuse"
						, vec3( 0.0_f ) );
					auto lightToVertex = m_writer.declLocale( "lightToVertex"
						, light.m_position.xyz() - surface.worldPosition );
					auto distance = m_writer.declLocale( "distance"
						, length( lightToVertex ) );
					auto lightDirection = m_writer.declLocale( "lightDirection"
						, normalize( lightToVertex ) );
					auto spotFactor = m_writer.declLocale( "spotFactor"
						, dot( lightDirection, -light.m_direction ) );

					if ( m_shadowModel->isEnabled() )
					{
						IF( m_writer, light.m_lightBase.m_shadowType != Int( int( ShadowType::eNone ) ) )
						{
							auto shadowFactor = m_writer.declLocale( "shadowFactor"
								, 1.0_f - step( spotFactor, light.m_cutOff ) );
							shadowFactor *= max( 1.0_f - m_writer.cast< Float >( receivesShadows )
								, m_shadowModel->computeSpot( light.m_lightBase
									, surface
									, light.m_transform
									, -lightToVertex ) );

							IF( m_writer, shadowFactor )
							{
								diffuse = shadowFactor * m_cookTorrance.computeDiffuse( light.m_lightBase
									, worldEye
									, lightDirection
									, specular
									, metalness
									, surface );
							}
							FI;
						}
						ELSE
						{
							diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
								, worldEye
								, lightDirection
								, specular
								, metalness
								, surface );
						}
						FI;
					}
					else
					{
						diffuse = m_cookTorrance.computeDiffuse( light.m_lightBase
							, worldEye
							, lightDirection
							, specular
							, metalness
							, surface );
					}

					auto attenuation = m_writer.declLocale( "attenuation"
						, sdw::fma( light.m_attenuation.z()
							, distance * distance
							, sdw::fma( light.m_attenuation.y()
								, distance
								, light.m_attenuation.x() ) ) );
					spotFactor = sdw::fma( ( spotFactor - 1.0_f )
						, 1.0_f / ( 1.0_f - light.m_cutOff )
						, 1.0_f );
					m_writer.returnStmt( max( vec3( 0.0_f ), spotFactor * diffuse / attenuation ) );
				}
				, InSpotLight( m_writer, "light" )
				, InVec3( m_writer, "worldEye" )
				, InVec3( m_writer, "specular" )
				, InFloat( m_writer, "metalness" )
				, InFloat( m_writer, "roughness" )
				, InInt( m_writer, "receivesShadows" )
				, InSurface{ m_writer, "surface" } );
		}

		//***********************************************************************************************
	}
}
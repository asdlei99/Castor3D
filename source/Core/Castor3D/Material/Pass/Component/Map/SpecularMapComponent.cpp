#include "Castor3D/Material/Pass/Component/Map/SpecularMapComponent.hpp"

#include "Castor3D/Limits.hpp"
#include "Castor3D/Engine.hpp"
#include "Castor3D/Material/Pass/Pass.hpp"
#include "Castor3D/Material/Pass/PassFactory.hpp"
#include "Castor3D/Material/Pass/Component/Lighting/SpecularComponent.hpp"
#include "Castor3D/Material/Pass/PBR/PbrPass.hpp"
#include "Castor3D/Material/Pass/PassVisitor.hpp"
#include "Castor3D/Material/Texture/TextureConfiguration.hpp"
#include "Castor3D/Scene/SceneFileParser.hpp"
#include "Castor3D/Shader/Shaders/GlslBlendComponents.hpp"
#include "Castor3D/Shader/Shaders/GlslLighting.hpp"
#include "Castor3D/Shader/Shaders/GlslMaterial.hpp"
#include "Castor3D/Shader/Shaders/GlslTextureConfiguration.hpp"

#include <CastorUtils/FileParser/ParserParameter.hpp>

//*************************************************************************************************

namespace castor
{
	template<>
	class TextWriter< castor3d::SpecularMapComponent >
		: public TextWriterT< castor3d::SpecularMapComponent >
	{
	public:
		explicit TextWriter( String const & tabs
			, uint32_t mask )
			: TextWriterT< castor3d::SpecularMapComponent >{ tabs }
			, m_mask{ mask }
		{
		}

		bool operator()( StringStream & file )
		{
			return writeNamedSub( file, cuT( "specular_mask" ), m_mask );
		}

		bool operator()( castor3d::SpecularMapComponent const & object
			, StringStream & file )override
		{
			return writeNamedSub( file, cuT( "specular_mask" ), m_mask );
		}

	private:
		uint32_t m_mask;
	};
}

namespace castor3d
{
	//*********************************************************************************************

	namespace spccmp
	{
		static CU_ImplementAttributeParser( parserUnitSpecularMask )
		{
			auto & parsingContext = getParserContext( context );

			if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameter." ) );
			}
			else
			{
				auto & component = getPassComponent< SpecularMapComponent >( parsingContext );
				component.fillChannel( parsingContext.textureConfiguration
					, params[0]->get< uint32_t >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserTexRemapSpecular )
		{
			auto & parsingContext = getParserContext( context );
			parsingContext.sceneImportConfig.textureRemapIt = parsingContext.sceneImportConfig.textureRemaps.emplace( TextureFlag::eSpecular, TextureConfiguration{} ).first;
			parsingContext.sceneImportConfig.textureRemapIt->second = TextureConfiguration{};
		}
		CU_EndAttributePush( CSCNSection::eTextureRemapChannel )

		static CU_ImplementAttributeParser( parserTexRemapSpecularMask )
		{
			auto & parsingContext = getParserContext( context );

			if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameter." ) );
			}
			else
			{
				SpecularMapComponent::fillRemapMask( params[0]->get< uint32_t >()
					, parsingContext.sceneImportConfig.textureRemapIt->second );
			}
		}
		CU_EndAttribute()
	}

	//*********************************************************************************************

	void SpecularMapComponent::ComponentsShader::applyComponents( TextureFlags const & texturesFlags
		, PipelineFlags const * flags
		, shader::TextureConfigData const & config
		, sdw::Vec4 const & sampled
		, shader::BlendComponents const & components )const
	{
		if ( !components.hasMember( "specular" )
			|| !checkFlag( texturesFlags, TextureFlag::eSpecular ) )
		{
			return;
		}

		auto & writer{ *sampled.getWriter() };

		IF( writer, config.spcEnbl() != 0.0_f )
		{
			components.getMember< sdw::Vec3 >( "specular" ) *= config.getVec3( sampled, config.spcMask() );
		}
		FI;
	}

	//*********************************************************************************************

	castor::String const SpecularMapComponent::TypeName = C3D_MakePassMapComponentName( "specular" );

	SpecularMapComponent::SpecularMapComponent( Pass & pass )
		: PassMapComponent{ pass, TypeName }
	{
		if ( !pass.hasComponent< SpecularComponent >() )
		{
			pass.createComponent< SpecularComponent >();
		}
	}

	void SpecularMapComponent::createParsers( castor::AttributeParsers & parsers
		, ChannelFillers & channelFillers )
	{
		channelFillers.emplace( "specular", ChannelFiller{ uint32_t( TextureFlag::eSpecular )
			, []( SceneFileContext & parsingContext )
			{
				auto & component = getPassComponent< SpecularMapComponent >( parsingContext );
				component.fillChannel( parsingContext.textureConfiguration
					, 0x00FFFFFF );
			} } );

		Pass::addParserT( parsers
			, CSCNSection::eTextureUnit
			, cuT( "specular_mask" )
			, spccmp::parserUnitSpecularMask
			, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );

		Pass::addParserT( parsers
			, CSCNSection::eTextureRemap
			, cuT( "specular" )
			, spccmp::parserTexRemapSpecular );

		Pass::addParserT( parsers
			, CSCNSection::eTextureRemapChannel
			, cuT( "specular_mask" )
			, spccmp::parserTexRemapSpecularMask
			, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
	}

	void SpecularMapComponent::fillRemapMask( uint32_t maskValue
		, TextureConfiguration & configuration )
	{
		configuration.specularMask[0] = maskValue;
	}

	bool SpecularMapComponent::writeTextureConfig( TextureConfiguration const & configuration
		, castor::String const & tabs
		, castor::StringStream & file )
	{
		return castor::TextWriter< SpecularMapComponent >{ tabs, configuration.specularMask[0] }( file );
	}

	bool SpecularMapComponent::isComponentNeeded( TextureFlags const & textures
		, ComponentModeFlags const & filter )
	{
		return checkFlag( filter, ComponentModeFlag::eSpecularLighting )
			|| checkFlag( textures, TextureFlag::eSpecular );
	}

	bool SpecularMapComponent::needsMapComponent( TextureConfiguration const & configuration )
	{
		return configuration.specularMask[0] != 0u;
	}

	void SpecularMapComponent::createMapComponent( Pass & pass
		, std::vector< PassComponentUPtr > & result )
	{
		result.push_back( std::make_unique< SpecularMapComponent >( pass ) );
	}

	void SpecularMapComponent::updateComponent( TextureFlags const & texturesFlags
		, shader::BlendComponents const & components )
	{
		if ( checkFlag( texturesFlags, TextureFlag::eMetalness )
			&& !checkFlag( texturesFlags, TextureFlag::eSpecular ) )
		{
			auto specular = components.getMember< sdw::Vec3 >( "specular", true );
			auto colour = components.getMember< sdw::Vec3 >( "colour", vec3( 0.0_f ) );
			auto metalness = components.getMember< sdw::Float >( "metalness", 0.0_f );
			specular = shader::BlendComponents::computeF0( colour, metalness );
		}
	}

	void SpecularMapComponent::mergeImages( TextureUnitDataSet & result )
	{
		getOwner()->mergeImages( TextureFlag::eSpecular
			, offsetof( TextureConfiguration, specularMask )
			, 0x00FFFFFF
			, TextureFlag::eGlossiness
			, offsetof( TextureConfiguration, glossinessMask )
			, 0xFF000000
			, "SpcGls"
			, result );
	}

	void SpecularMapComponent::fillChannel( TextureConfiguration & configuration
		, uint32_t mask )
	{
		configuration.textureSpace |= TextureSpace::eNormalised;
		configuration.textureSpace |= TextureSpace::eColour;
		configuration.specularMask[0] = mask;
	}

	void SpecularMapComponent::fillConfig( TextureConfiguration & configuration
		, PassVisitorBase & vis )
	{
		vis.visit( cuT( "Specular" ), castor3d::TextureFlag::eSpecular, configuration.specularMask, 3u );
	}

	PassComponentUPtr SpecularMapComponent::doClone( Pass & pass )const
	{
		return std::make_unique< SpecularMapComponent >( pass );
	}

	//*********************************************************************************************
}

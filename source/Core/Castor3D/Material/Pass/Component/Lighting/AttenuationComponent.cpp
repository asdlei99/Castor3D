#include "Castor3D/Material/Pass/Component/Lighting/AttenuationComponent.hpp"

#include "Castor3D/Material/Pass/Pass.hpp"
#include "Castor3D/Material/Pass/PassVisitor.hpp"
#include "Castor3D/Material/Pass/Component/Lighting/TransmissionComponent.hpp"
#include "Castor3D/Scene/SceneFileParser.hpp"
#include "Castor3D/Shader/ShaderBuffers/PassBuffer.hpp"
#include "Castor3D/Shader/Shaders/GlslBlendComponents.hpp"
#include "Castor3D/Shader/Shaders/GlslLighting.hpp"
#include "Castor3D/Shader/Shaders/GlslMaterial.hpp"
#include "Castor3D/Shader/Shaders/GlslSurface.hpp"

#include <CastorUtils/FileParser/ParserParameter.hpp>
#include <CastorUtils/Data/Text/TextRgbColour.hpp>

namespace castor
{
	template<>
	class TextWriter< castor3d::AttenuationComponent >
		: public TextWriterT< castor3d::AttenuationComponent >
	{
	public:
		explicit TextWriter( String const & tabs )
			: TextWriterT< castor3d::AttenuationComponent >{ tabs }
		{
		}

		bool operator()( castor3d::AttenuationComponent const & object
			, StringStream & file )override
		{
			return writeNamedSubOpt( file, cuT( "attenuation_colour" ), object.getAttenuationColour(), castor::RgbColour{} )
				&& writeOpt( file, cuT( "attenuation_distance" ), object.getAttenuationDistance(), std::numeric_limits< float >::infinity() )
				&& writeOpt( file, cuT( "thickness_factor" ), object.getThicknessFactor(), 0.0f );
		}
	};
}

namespace castor3d
{
	//*********************************************************************************************

	namespace trsatt
	{
		static CU_ImplementAttributeParser( parserPassAttenuationColour )
		{
			auto & parsingContext = getParserContext( context );

			if ( !parsingContext.pass )
			{
				CU_ParsingError( cuT( "No Pass initialised." ) );
			}
			else if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameter." ) );
			}
			else
			{
				auto & component = getPassComponent< AttenuationComponent >( parsingContext );
				component.setAttenuationColour( params[0]->get< castor::RgbColour >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserPassAttenuationDistance )
		{
			auto & parsingContext = getParserContext( context );

			if ( !parsingContext.pass )
			{
				CU_ParsingError( cuT( "No Pass initialised." ) );
			}
			else if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameter." ) );
			}
			else
			{
				auto & component = getPassComponent< AttenuationComponent >( parsingContext );
				component.setAttenuationDistance( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserPassThicknessFactor )
		{
			auto & parsingContext = getParserContext( context );

			if ( !parsingContext.pass )
			{
				CU_ParsingError( cuT( "No Pass initialised." ) );
			}
			else if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameter." ) );
			}
			else
			{
				auto & component = getPassComponent< AttenuationComponent >( parsingContext );
				component.setThicknessFactor( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()
	}

	//*********************************************************************************************

	void AttenuationComponent::ComponentsShader::fillComponents( sdw::type::BaseStruct & components
		, shader::Materials const & materials
		, sdw::StructInstance const * surface )const
	{
		if ( !checkFlag( materials.getFilter(), ComponentModeFlag::eDiffuseLighting )
			&& !checkFlag( materials.getFilter(), ComponentModeFlag::eSpecularLighting ) )
		{
			return;
		}

		if ( !components.hasMember( "attenuationColour" ) )
		{
			components.declMember( "attenuationColour", sdw::type::Kind::eVec3F );
			components.declMember( "attenuationDistance", sdw::type::Kind::eFloat );
			components.declMember( "thicknessFactor", sdw::type::Kind::eFloat );
		}
	}

	void AttenuationComponent::ComponentsShader::fillComponentsInits( sdw::type::BaseStruct const & components
		, shader::Materials const & materials
		, shader::Material const * material
		, sdw::StructInstance const * surface
		, sdw::Vec4 const * clrCot
		, sdw::expr::ExprList & inits )const
	{
		if ( !components.hasMember( "attenuationColour" ) )
		{
			return;
		}

		if ( material )
		{
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Vec3 >( "attenuationColour" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "attenuationDistance" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "thicknessFactor" ) ) );
		}
		else
		{
			inits.emplace_back( sdw::makeExpr( vec3( 0.0_f ) ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
		}
	}

	void AttenuationComponent::ComponentsShader::blendComponents( shader::Materials const & materials
		, sdw::Float const & passMultiplier
		, shader::BlendComponents & res
		, shader::BlendComponents const & src )const
	{
		res.getMember< sdw::Vec3 >( "attenuationColour", true ) = src.getMember< sdw::Vec3 >( "attenuationColour", true ) * passMultiplier;
		res.getMember< sdw::Float >( "attenuationDistance", true ) = src.getMember< sdw::Float >( "attenuationDistance", true ) * passMultiplier;
		res.getMember< sdw::Float >( "thicknessFactor", true ) = src.getMember< sdw::Float >( "thicknessFactor", true ) * passMultiplier;
	}

	//*********************************************************************************************

	AttenuationComponent::MaterialShader::MaterialShader()
		: shader::PassMaterialShader{ 20u }
	{
	}

	void AttenuationComponent::MaterialShader::fillMaterialType( ast::type::BaseStruct & type
		, sdw::expr::ExprList & inits )const
	{
		if ( !type.hasMember( "attenuationColour" ) )
		{
			type.declMember( "attenuationColour", ast::type::Kind::eVec3F );
			type.declMember( "attenuationDistance", ast::type::Kind::eFloat );
			type.declMember( "thicknessFactor", ast::type::Kind::eFloat );
			inits.emplace_back( sdw::makeExpr( vec3( 0.0_f ) ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
		}
	}

	//*********************************************************************************************

	void AttenuationComponent::Plugin::createParsers( castor::AttributeParsers & parsers
		, ChannelFillers & channelFillers )const
	{
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "attenuation_colour" )
			, trsatt::parserPassAttenuationColour
			, { castor::makeParameter< castor::ParameterType::eRgbColour >() } );
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "attenuation_distance" )
			, trsatt::parserPassAttenuationDistance
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "thickness_factor" )
			, trsatt::parserPassThicknessFactor
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
	}

	void AttenuationComponent::Plugin::zeroBuffer( Pass const & pass
		, shader::PassMaterialShader const & materialShader
		, PassBuffer & buffer )const
	{
		auto data = buffer.getData( pass.getId() );
		VkDeviceSize offset{};
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
	}

	bool AttenuationComponent::Plugin::isComponentNeeded( TextureCombine const & textures
		, ComponentModeFlags const & filter )const
	{
		return checkFlag( filter, ComponentModeFlag::eDiffuseLighting )
			|| checkFlag( filter, ComponentModeFlag::eSpecularLighting );
	}

	//*********************************************************************************************

	castor::String const AttenuationComponent::TypeName = C3D_MakePassLightingComponentName( "attenuation" );

	AttenuationComponent::AttenuationComponent( Pass & pass )
		: BaseDataPassComponentT{ pass, TypeName }
	{
		if ( !pass.hasComponent< TransmissionComponent >() )
		{
			pass.createComponent< TransmissionComponent >( 1.0f );
		}
	}

	void AttenuationComponent::accept( PassVisitorBase & vis )
	{
		vis.visit( cuT( "Attenuation Colour" ), m_value.colour );
		vis.visit( cuT( "Attenuation Distance" ), m_value.distance );
		vis.visit( cuT( "Thickness Factor" ), m_value.thicknessFactor );
	}

	PassComponentUPtr AttenuationComponent::doClone( Pass & pass )const
	{
		auto result = std::make_unique< AttenuationComponent >( pass );
		result->setData( getData() );
		return result;
	}

	bool AttenuationComponent::doWriteText( castor::String const & tabs
		, castor::Path const & folder
		, castor::String const & subfolder
		, castor::StringStream & file )const
	{
		return castor::TextWriter< AttenuationComponent >{ tabs }( *this, file );
	}

	void AttenuationComponent::doFillBuffer( PassBuffer & buffer )const
	{
		auto data = buffer.getData( getOwner()->getId() );
		VkDeviceSize offset{};
		offset += data.write( m_materialShader->getMaterialChunk()
			, getAttenuationColour()
			, offset );
		offset += data.write( m_materialShader->getMaterialChunk()
			, getAttenuationDistance()
			, offset );
		data.write( m_materialShader->getMaterialChunk()
			, getThicknessFactor()
			, offset );
	}

	//*********************************************************************************************
}

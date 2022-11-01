#include "Castor3D/Material/Pass/Component/Lighting/IridescenceComponent.hpp"

#include "Castor3D/Material/Pass/Pass.hpp"
#include "Castor3D/Material/Pass/PassVisitor.hpp"
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
	class TextWriter< castor3d::IridescenceComponent >
		: public TextWriterT< castor3d::IridescenceComponent >
	{
	public:
		explicit TextWriter( String const & tabs )
			: TextWriterT< castor3d::IridescenceComponent >{ tabs }
		{
		}

		bool operator()( castor3d::IridescenceComponent const & object
			, StringStream & file )override
		{
			return write( file, cuT( "iridescence_factor" ), object.getFactor() )
				&& writeOpt( file, cuT( "iridescence_ior" ), object.getIor(), 1.3f )
				&& writeOpt( file, cuT( "iridescence_min_thickness" ), object.getMinThickness(), 100.0f )
				&& writeOpt( file, cuT( "iridescence_max_thickness" ), object.getMaxThickness(), 400.0f );
		}
	};
}

namespace castor3d
{
	//*********************************************************************************************

	namespace irid
	{
		static CU_ImplementAttributeParser( parserPassIridescenceFactor )
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
				auto & component = getPassComponent< IridescenceComponent >( parsingContext );
				component.setFactor( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserPassIridescenceIor )
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
				auto & component = getPassComponent< IridescenceComponent >( parsingContext );
				component.setIor( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserPassIridescenceMinThickness )
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
				auto & component = getPassComponent< IridescenceComponent >( parsingContext );
				component.setMinThickness( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserPassIridescenceMaxThickness )
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
				auto & component = getPassComponent< IridescenceComponent >( parsingContext );
				component.setMaxThickness( params[0]->get< float >() );
			}
		}
		CU_EndAttribute()
	}

	//*********************************************************************************************

	void IridescenceComponent::ComponentsShader::fillComponents( sdw::type::BaseStruct & components
		, shader::Materials const & materials
		, sdw::StructInstance const * surface )const
	{
		if ( !checkFlag( materials.getFilter(), ComponentModeFlag::eDiffuseLighting )
			&& !checkFlag( materials.getFilter(), ComponentModeFlag::eSpecularLighting ) )
		{
			return;
		}

		if ( !components.hasMember( "iridescenceFactor" ) )
		{
			components.declMember( "iridescenceFactor", sdw::type::Kind::eFloat );
			components.declMember( "iridescenceIor", sdw::type::Kind::eFloat );
			components.declMember( "iridescenceMinThickness", sdw::type::Kind::eFloat );
			components.declMember( "iridescenceMaxThickness", sdw::type::Kind::eFloat );
			components.declMember( "iridescenceThickness", sdw::type::Kind::eFloat );
			components.declMember( "iridescenceFresnel", sdw::type::Kind::eVec3F );
			components.declMember( "iridescenceF0", sdw::type::Kind::eVec3F );
		}
	}

	void IridescenceComponent::ComponentsShader::fillComponentsInits( sdw::type::BaseStruct const & components
		, shader::Materials const & materials
		, shader::Material const * material
		, sdw::StructInstance const * surface
		, sdw::Vec4 const * clrCot
		, sdw::expr::ExprList & inits )const
	{
		if ( !components.hasMember( "iridescenceFactor" ) )
		{
			return;
		}

		if ( material )
		{
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "iridescenceFactor" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "iridescenceIor" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "iridescenceMinThickness" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "iridescenceMaxThickness" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Float >( "iridescenceMaxThickness" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Vec3 >( "specular" ) ) );
			inits.emplace_back( sdw::makeExpr( material->getMember< sdw::Vec3 >( "specular" ) ) );
		}
		else
		{
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( vec3( 0.0_f ) ) );
			inits.emplace_back( sdw::makeExpr( vec3( 0.0_f ) ) );
		}
	}

	void IridescenceComponent::ComponentsShader::blendComponents( shader::Materials const & materials
		, sdw::Float const & passMultiplier
		, shader::BlendComponents & res
		, shader::BlendComponents const & src )const
	{
		res.getMember< sdw::Float >( "iridescenceFactor", true ) = src.getMember< sdw::Float >( "iridescenceFactor", true ) * passMultiplier;
		res.getMember< sdw::Float >( "iridescenceIor", true ) = src.getMember< sdw::Float >( "iridescenceIor", true ) * passMultiplier;
		res.getMember< sdw::Float >( "iridescenceMinThickness", true ) = src.getMember< sdw::Float >( "iridescenceMinThickness", true ) * passMultiplier;
		res.getMember< sdw::Float >( "iridescenceMaxThickness", true ) = src.getMember< sdw::Float >( "iridescenceMaxThickness", true ) * passMultiplier;
		res.getMember< sdw::Float >( "iridescenceThickness", true ) = src.getMember< sdw::Float >( "iridescenceThickness", true ) * passMultiplier;
		res.getMember< sdw::Vec3 >( "iridescenceFresnel", true ) = src.getMember< sdw::Vec3 >( "iridescenceFresnel", true ) * passMultiplier;
		res.getMember< sdw::Vec3 >( "iridescenceF0", true ) = src.getMember< sdw::Vec3 >( "iridescenceF0", true ) * passMultiplier;
	}

	//*********************************************************************************************

	IridescenceComponent::MaterialShader::MaterialShader()
		: shader::PassMaterialShader{ 16u }
	{
	}

	void IridescenceComponent::MaterialShader::fillMaterialType( ast::type::BaseStruct & type
		, sdw::expr::ExprList & inits )const
	{
		if ( !type.hasMember( "iridescenceFactor" ) )
		{
			type.declMember( "iridescenceFactor", ast::type::Kind::eFloat );
			type.declMember( "iridescenceIor", ast::type::Kind::eFloat );
			type.declMember( "iridescenceMinThickness", ast::type::Kind::eFloat );
			type.declMember( "iridescenceMaxThickness", ast::type::Kind::eFloat );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
			inits.emplace_back( sdw::makeExpr( 0.0_f ) );
		}
	}

	void IridescenceComponent::MaterialShader::updateMaterial( sdw::Vec3 const & albedo
		, sdw::Vec4 const & spcRgh
		, sdw::Vec4 const & colMtl
		, sdw::Vec4 const & crTsIr
		, sdw::Vec4 const & sheen
		, shader::Material & material )const
	{
		material.getMember< sdw::Float >( "iridescenceFactor", true ) = crTsIr.z();
		material.getMember< sdw::Float >( "iridescenceMinThickness", true ) = crTsIr.w();
		material.getMember< sdw::Float >( "iridescenceMaxThickness", true ) = crTsIr.w();
	}

	//*********************************************************************************************

	void IridescenceComponent::Plugin::createParsers( castor::AttributeParsers & parsers
		, ChannelFillers & channelFillers )const
	{
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "iridescence_factor" )
			, irid::parserPassIridescenceFactor
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "iridescence_ior" )
			, irid::parserPassIridescenceIor
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "iridescence_min_thickness" )
			, irid::parserPassIridescenceMinThickness
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "iridescence_max_thickness" )
			, irid::parserPassIridescenceMaxThickness
			, { castor::makeParameter< castor::ParameterType::eFloat >() } );
	}

	void IridescenceComponent::Plugin::zeroBuffer( Pass const & pass
		, shader::PassMaterialShader const & materialShader
		, PassBuffer & buffer )const
	{
		auto data = buffer.getData( pass.getId() );
		VkDeviceSize offset{};
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		offset += data.write( materialShader.getMaterialChunk(), 0.0f, offset );
		data.write( materialShader.getMaterialChunk(), 0.0f, offset );
	}

	bool IridescenceComponent::Plugin::isComponentNeeded( TextureCombine const & textures
		, ComponentModeFlags const & filter )const
	{
		return checkFlag( filter, ComponentModeFlag::eDiffuseLighting )
			|| checkFlag( filter, ComponentModeFlag::eSpecularLighting );
	}

	//*********************************************************************************************

	castor::String const IridescenceComponent::TypeName = C3D_MakePassLightingComponentName( "iridescence" );

	IridescenceComponent::IridescenceComponent( Pass & pass )
		: BaseDataPassComponentT{ pass, TypeName }
	{
	}

	void IridescenceComponent::accept( PassVisitorBase & vis )
	{
		vis.visit( cuT( "Iridescence Factor" ), m_value.factor );
		vis.visit( cuT( "Iridescence IOR" ), m_value.ior );
		vis.visit( cuT( "Iridescence Min Thickness" ), m_value.minThickness );
		vis.visit( cuT( "Iridescence Max Thickness" ), m_value.maxThickness );
	}

	PassComponentUPtr IridescenceComponent::doClone( Pass & pass )const
	{
		auto result = std::make_unique< IridescenceComponent >( pass );
		result->setData( getData() );
		return result;
	}

	bool IridescenceComponent::doWriteText( castor::String const & tabs
		, castor::Path const & folder
		, castor::String const & subfolder
		, castor::StringStream & file )const
	{
		return castor::TextWriter< IridescenceComponent >{ tabs }( *this, file );
	}

	void IridescenceComponent::doFillBuffer( PassBuffer & buffer )const
	{
		auto data = buffer.getData( getOwner()->getId() );
		VkDeviceSize offset{};
		offset += data.write( m_materialShader->getMaterialChunk()
			, getFactor()
			, offset );
		offset += data.write( m_materialShader->getMaterialChunk()
			, getIor()
			, offset );
		offset += data.write( m_materialShader->getMaterialChunk()
			, getMinThickness()
			, offset );
		data.write( m_materialShader->getMaterialChunk()
			, getMaxThickness()
			, offset );
	}

	//*********************************************************************************************
}

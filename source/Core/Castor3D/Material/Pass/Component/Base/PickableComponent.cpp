#include "Castor3D/Material/Pass/Component/Base/PickableComponent.hpp"

#include "Castor3D/Material/Pass/Pass.hpp"
#include "Castor3D/Material/Pass/PassVisitor.hpp"
#include "Castor3D/Material/Texture/TextureConfiguration.hpp"
#include "Castor3D/Scene/SceneFileParser.hpp"

#include <CastorUtils/FileParser/ParserParameter.hpp>

namespace castor
{
	template<>
	class TextWriter< castor3d::PickableComponent >
		: public TextWriterT< castor3d::PickableComponent >
	{
	public:
		explicit TextWriter( String const & tabs )
			: TextWriterT< castor3d::PickableComponent >{ tabs }
		{
		}

		bool operator()( castor3d::PickableComponent const & object
			, StringStream & file )override
		{
			return writeOpt( file, cuT( "two_sided" ), object.isPickable(), false );
		}
	};
}

namespace castor3d
{
	//*********************************************************************************************

	namespace tws
	{
		static CU_ImplementAttributeParser( parserPassPickable )
		{
			auto & parsingContext = getParserContext( context );

			if ( !parsingContext.pass )
			{
				CU_ParsingError( cuT( "No Pass initialised." ) );
			}
			else if ( !params.empty() )
			{
				bool value;
				params[0]->get( value );
				auto & component = getPassComponent< PickableComponent >( parsingContext );
				component.setPickable( value );
			}
		}
		CU_EndAttribute()
	}

	//*********************************************************************************************

	void PickableComponent::Plugin::createParsers( castor::AttributeParsers & parsers
		, ChannelFillers & channelFillers )const
	{
		Pass::addParserT( parsers
			, CSCNSection::ePass
			, cuT( "pickable" )
			, tws::parserPassPickable
			, { castor::makeParameter< castor::ParameterType::eBool >() } );
	}

	//*********************************************************************************************

	castor::String const PickableComponent::TypeName = C3D_MakePassBaseComponentName( "pickable" );

	PickableComponent::PickableComponent( Pass & pass, bool pickable )
		: BaseDataPassComponentT< castor::AtomicGroupChangeTracked< bool > >{ pass, TypeName, pickable }
	{
	}

	void PickableComponent::accept( PassVisitorBase & vis )
	{
		vis.visit( cuT( "Pickable" ), m_value );
	}

	PassComponentUPtr PickableComponent::doClone( Pass & pass )const
	{
		auto result = std::make_unique< PickableComponent >( pass );
		result->setData( getData() );
		return result;
	}

	bool PickableComponent::doWriteText( castor::String const & tabs
		, castor::Path const & folder
		, castor::String const & subfolder
		, castor::StringStream & file )const
	{
		return castor::TextWriter< PickableComponent >{ tabs }( *this, file );
	}

	//*********************************************************************************************
}
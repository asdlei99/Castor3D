#include "Castor3D/Material/Material.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Cache/MaterialCache.hpp"
#include "Castor3D/Miscellaneous/Logger.hpp"
#include "Castor3D/Material/Pass/Pass.hpp"
#include "Castor3D/Material/Pass/PassFactory.hpp"
#include "Castor3D/Render/RenderSystem.hpp"
#include "Castor3D/Scene/Scene.hpp"
#include "Castor3D/Scene/SceneFileParser.hpp"

#include <CastorUtils/FileParser/ParserParameter.hpp>

CU_ImplementSmartPtr( castor3d, Material )

namespace castor3d
{
	namespace mat
	{
		static CU_ImplementAttributeParser( parserPass )
		{
			auto & parsingContext = getParserContext( context );
			parsingContext.strName.clear();

			if ( parsingContext.material )
			{
				if ( parsingContext.createMaterial
					|| parsingContext.material->getPassCount() < parsingContext.passIndex )
				{
					parsingContext.pass = parsingContext.material->createPass();
					parsingContext.createPass = true;

				}
				else
				{
					parsingContext.pass = parsingContext.material->getPass( parsingContext.passIndex );
					parsingContext.createPass = false;
				}

				++parsingContext.passIndex;
				parsingContext.unitIndex = 0u;
			}
			else
			{
				CU_ParsingError( cuT( "Material not initialised" ) );
			}
		}
		CU_EndAttributePush( CSCNSection::ePass )

		static CU_ImplementAttributeParser( parserRenderPass )
		{
			auto & parsingContext = getParserContext( context );
			parsingContext.strName.clear();

			if ( !parsingContext.material )
			{
				CU_ParsingError( cuT( "Material not initialised" ) );
			}
			else if ( params.empty() )
			{
				CU_ParsingError( cuT( "Missing parameters" ) );
			}
			else
			{
				castor::String typeName;
				params[0]->get( typeName );
				parsingContext.material->setRenderPassInfo( parsingContext.parser->getEngine()->getRenderPassInfo( typeName ) );
			}
		}
		CU_EndAttribute()

		static CU_ImplementAttributeParser( parserEnd )
		{
			auto & parsingContext = getParserContext( context );

			if ( !parsingContext.ownMaterial
				&& !parsingContext.material )
			{
				CU_ParsingError( cuT( "Material not initialised" ) );
			}
			else if ( parsingContext.ownMaterial )
			{
				log::info << "Loaded material [" << parsingContext.material->getName() << "]" << std::endl;

				if ( parsingContext.scene )
				{
					parsingContext.scene->addMaterial( parsingContext.material->getName()
						, parsingContext.ownMaterial
						, true );
				}
				else
				{
					parsingContext.parser->getEngine()->addMaterial( parsingContext.material->getName()
						, parsingContext.ownMaterial
						, true );
				}
			}

			parsingContext.material = {};
			parsingContext.createMaterial = false;
			parsingContext.passIndex = 0u;
		}
		CU_EndAttributePop()
	}

	const castor::String Material::DefaultMaterialName = cuT( "C3D_DefaultMaterial" );

	Material::Material( castor::String const & name
		, Engine & engine
		, LightingModelID lightingModelId )
		: castor::Named{ name }
		, castor::OwnedBy< Engine >{ engine }
		, m_lightingModelId{ lightingModelId }
	{
	}

	void Material::initialise()
	{
		if ( m_initialised )
		{
			return;
		}

		log::debug << cuT( "Initialising material [" ) << getName() << cuT( "]" ) << std::endl;

		for ( auto & pass : m_passes )
		{
			pass->initialise();
		}

		m_initialised = true;
	}

	void Material::cleanup()
	{
		if ( !m_initialised )
		{
			return;
		}

		m_initialised = false;

		for ( auto & pass : m_passes )
		{
			pass->cleanup();
		}
	}

	PassRPtr Material::createPass( LightingModelID lightingModelId )
	{
		if ( m_passes.size() == MaxPassLayers )
		{
			log::error << cuT( "Can't create a new pass, max pass count reached" ) << std::endl;
			CU_Failure( "Can't create a new pass, max pass count reached" );
			return nullptr;
		}

		auto result = getEngine()->getPassFactory().create( *this
			, lightingModelId );
		CU_Require( result );
		auto ret = result.get();
		m_passListeners.emplace( ret
			, result->onChanged.connect( [this]( Pass const & p
				, PassComponentCombineID CU_UnusedParam( oldCombineID )
				, PassComponentCombineID CU_UnusedParam( newCombineID ) )
			{
				onPassChanged( p );
			} ) );
		m_passes.emplace_back( std::move( result ) );
		onChanged( *this );
		return ret;
	}

	PassRPtr Material::createPass()
	{
		return createPass( m_lightingModelId );
	}

	void Material::addPass( Pass const & pass )
	{
		if ( m_passes.size() == MaxPassLayers )
		{
			log::error << cuT( "Can't create a new pass, max pass count reached" ) << std::endl;
			CU_Failure( "Can't create a new pass, max pass count reached" );
			return;
		}

		auto newPass = getEngine()->getPassFactory().create( *this
			, pass );
		CU_Require( newPass );
		m_passListeners.emplace( newPass.get()
			, newPass->onChanged.connect( [this]( Pass const & p
				, PassComponentCombineID CU_UnusedParam( oldCombineID )
				, PassComponentCombineID CU_UnusedParam( newCombineID ) )
			{
				onPassChanged( p );
			} ) );
		m_passes.emplace_back( std::move( newPass ) );
		onChanged( *this );
	}

	void Material::removePass( PassRPtr pass )
	{
		auto it = std::find_if( m_passes.begin()
			, m_passes.end()
			, [pass]( PassUPtr const & lookup )
			{
				return lookup.get() == pass;
			} );

		if ( it != m_passes.end() )
		{
			m_passListeners.erase( it->get() );
			m_passes.erase( it );
			onChanged( *this );
		}
	}

	PassRPtr Material::getPass( uint32_t index )const
	{
		CU_Require( index < m_passes.size() );
		return m_passes[index].get();
	}

	void Material::destroyPass( uint32_t index )
	{
		CU_Require( index < m_passes.size() );
		m_passListeners.erase( ( m_passes.begin() + index )->get() );
		m_passes.erase( m_passes.begin() + index );
		onChanged( *this );
	}

	bool Material::hasAlphaBlending()const
	{
		return m_passes.end() == std::find_if( m_passes.begin()
			, m_passes.end()
			, []( PassUPtr const & pass )
			{
				return !pass->hasAlphaBlending();
			} );
	}

	bool Material::hasEnvironmentMapping()const
	{
		return m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, []( PassUPtr const & pass )
			{
				return pass->hasEnvironmentMapping();
			} );
	}

	bool Material::hasSubsurfaceScattering()const
	{
		return m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, []( PassUPtr const & pass )
			{
				return pass->hasSubsurfaceScattering();
			} );
	}

	void Material::onPassChanged( Pass const & pass )
	{
		onChanged( *this );
	}

	void Material::addParsers( castor::AttributeParsers & result )
	{
		using namespace castor;
		addParser( result, uint32_t( CSCNSection::eMaterial ), cuT( "pass" ), mat::parserPass );
		addParser( result, uint32_t( CSCNSection::eMaterial ), cuT( "render_pass" ), mat::parserRenderPass, { makeParameter< ParameterType::eText >() } );
		addParser( result, uint32_t( CSCNSection::eMaterial ), cuT( "}" ), mat::parserEnd );
	}
}

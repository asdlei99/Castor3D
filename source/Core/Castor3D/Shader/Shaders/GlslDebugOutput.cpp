#include "Castor3D/Shader/Shaders/GlslDebugOutput.hpp"

#include <CastorUtils/Data/Path.hpp>

#include <ShaderWriter/Source.hpp>

namespace castor3d::shader
{
	namespace debug
	{
		static castor::String normaliseName( castor::String const & name )
		{
			auto result = name;
			castor::string::replace( result, "\\", "" );
			castor::string::replace( result, "/", "" );
			castor::string::replace( result, ":", "" );
			castor::string::replace( result, "*", "" );
			castor::string::replace( result, "?", "" );
			castor::string::replace( result, "\"", "" );
			castor::string::replace( result, "<", "" );
			castor::string::replace( result, ">", "" );
			castor::string::replace( result, "|", "" );
			castor::string::replace( result, ".", "" );
			castor::string::replace( result, ",", "" );
			castor::string::replace( result, "\t", "" );
			castor::string::replace( result, "\r", "" );
			castor::string::replace( result, "\n", "" );
			castor::string::replace( result, " ", "" );
			return result;
		}
	}

	DebugOutput::DebugOutput( DebugConfig & config
		, castor::String category
		, sdw::UInt const index
		, sdw::Vec4 const output
		, bool enable )
		: m_config{ config }
		, m_category{ std::move( category ) }
		, m_index{ index }
		, m_output{ output }
		, m_enable{ enable }
	{
	}

	DebugOutput::~DebugOutput()noexcept
	{
		if ( m_enable )
		{
			auto & writer = sdw::findWriterMandat( m_index );

			for ( auto & entry : m_values )
			{
				IF( writer, m_index == sdw::UInt( entry.first ) )
				{
					m_output.xyz() = entry.second.xyz();
				}
				FI;
			}
		}
	}

	void DebugOutput::registerOutput( castor::String category
		, castor::String name
		, sdw::Vec4 const value )
	{
		auto & writer = sdw::findWriterMandat( value );
		auto index = m_config.registerValue( category, name );
		m_values.emplace_back( index
			, writer.declLocale( "debug" + castor::string::toString( index ) + debug::normaliseName( name )
				, value ) );
	}

	void DebugOutput::registerOutput( castor::String category
		, castor::String name
		, sdw::Vec3 const value )
	{
		registerOutput( category, name, vec4( value, 1.0_f ) );
	}

	void DebugOutput::registerOutput( castor::String category
		, castor::String name
		, sdw::Vec2 const value )
	{
		registerOutput( category, name, vec3( value, 0.0_f ) );
	}

	void DebugOutput::registerOutput( castor::String category
		, castor::String name
		, sdw::Float const value )
	{
		registerOutput( category, name, vec3( value ) );
	}
}

#include "Castor3D/Shader/Shaders/GlslVoxel.hpp"

#include <ShaderWriter/Source.hpp>

namespace castor3d
{
	namespace shader
	{
		Voxel::Voxel( sdw::ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled )
			: StructInstance{ writer, std::move( expr ), enabled }
			, colorMask{ getMember< sdw::UInt >( "colorMask" ) }
			, normalMask{ getMember< sdw::UInt >( "normalMask" ) }
		{
		}

		ast::type::BaseStructPtr Voxel::makeType( ast::type::TypesCache & cache )
		{
			auto result = cache.getStruct( ast::type::MemoryLayout::eStd430
				, "C3D_Voxel" );

			if ( result->empty() )
			{
				result->declMember( "colorMask", ast::type::Kind::eUInt );
				result->declMember( "normalMask", ast::type::Kind::eUInt );
			}

			return result;
		}

		std::unique_ptr< sdw::Struct > Voxel::declare( sdw::ShaderWriter & writer )
		{
			return std::make_unique< sdw::Struct >( writer
				, makeType( writer.getTypesCache() ) );
		}
	}
}

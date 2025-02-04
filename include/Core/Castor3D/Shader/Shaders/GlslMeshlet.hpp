/*
See LICENSE file in root folder
*/
#ifndef ___C3D_GlslMeshlet_H___
#define ___C3D_GlslMeshlet_H___

#include "SdwModule.hpp"

#include <ShaderWriter/BaseTypes/Array.hpp>
#include <ShaderWriter/BaseTypes/UInt.hpp>
#include <ShaderWriter/VecTypes/Vec4.hpp>
#include <ShaderWriter/CompositeTypes/StructInstance.hpp>

namespace castor3d
{
	namespace shader
	{
		struct Meshlet
			: public sdw::StructInstance
		{
			C3D_API Meshlet( sdw::ShaderWriter & writer
				, sdw::expr::ExprPtr expr
				, bool enabled = true );

			SDW_DeclStructInstance( C3D_API, Meshlet );

			C3D_API static sdw::type::BaseStructPtr makeType( sdw::type::TypesCache & cache );

			sdw::Array< sdw::UInt > vertices;
			sdw::Array< sdw::UInt8 > indices;

		private:
			sdw::UVec4 m_counts;

		public:
			sdw::UInt vertexCount;
			sdw::UInt triangleCount;
		};

		Writer_Parameter( Meshlet );
	}
}

#endif

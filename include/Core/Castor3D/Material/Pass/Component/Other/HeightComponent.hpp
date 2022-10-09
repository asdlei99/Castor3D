/*
See LICENSE file in root folder
*/
#ifndef ___C3D_HeightComponent_H___
#define ___C3D_HeightComponent_H___

#include "Castor3D/Material/Pass/Component/BaseDataPassComponent.hpp"

#include <CastorUtils/Design/GroupChangeTracked.hpp>
#include <CastorUtils/FileParser/FileParserModule.hpp>

namespace castor3d
{
	struct HeightComponent
		: public BaseDataPassComponentT< castor::AtomicGroupChangeTracked< ParallaxOcclusionMode > >
	{
		struct ComponentsShader
			: shader::PassComponentsShader
		{
			C3D_API void fillComponents( sdw::type::BaseStruct & components
				, shader::Materials const & materials
				, shader::Material const * material
				, sdw::StructInstance const * surface )const override;
			C3D_API void fillComponentsInits( sdw::type::BaseStruct & components
				, shader::Materials const & materials
				, shader::Material const * material
				, sdw::StructInstance const * surface
				, sdw::expr::ExprList & inits )const override;
			C3D_API void blendComponents( shader::Materials const & materials
				, sdw::Float const & passMultiplier
				, shader::BlendComponents const & res
				, shader::BlendComponents const & src )const override;
		};

		C3D_API explicit HeightComponent( Pass & pass );

		C3D_API static void createParsers( castor::AttributeParsers & parsers
			, ChannelFillers & channelFillers );
		C3D_API static bool isComponentNeeded( TextureFlags const & textures
			, ComponentModeFlags const & filter );

		C3D_API static shader::PassComponentsShaderPtr createComponentsShader()
		{
			return std::make_unique< ComponentsShader >();
		}

		C3D_API void accept( PassVisitorBase & vis )override;

		C3D_API PassFlags getPassFlags()const override
		{
			return ( ( getParallaxOcclusion() == ParallaxOcclusionMode::eOne )
				? PassFlag::eParallaxOcclusionMappingOne
				: ( ( getParallaxOcclusion() == ParallaxOcclusionMode::eRepeat )
					? PassFlag::eParallaxOcclusionMappingRepeat
					: PassFlag::eNone ) );
		}

		void setParallaxOcclusion( ParallaxOcclusionMode v )
		{
			setData( v );
		}

		ParallaxOcclusionMode getParallaxOcclusion()const
		{
			return getData();
		}

		bool hasParallaxOcclusion()const
		{
			return getParallaxOcclusion() != ParallaxOcclusionMode::eNone;
		}

		C3D_API static castor::String const TypeName;

	private:
		PassComponentUPtr doClone( Pass & pass )const override;
		bool doWriteText( castor::String const & tabs
			, castor::Path const & folder
			, castor::String const & subfolder
			, castor::StringStream & file )const override;
	};
}

#endif

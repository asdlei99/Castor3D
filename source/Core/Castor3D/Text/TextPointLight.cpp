#include "Castor3D/Text/TextPointLight.hpp"

#include "Castor3D/Miscellaneous/Logger.hpp"
#include "Castor3D/Scene/Light/Light.hpp"
#include "Castor3D/Scene/SceneNode.hpp"
#include "Castor3D/Text/TextShadow.hpp"

#include <CastorUtils/Data/Text/TextPoint.hpp>
#include <CastorUtils/Data/Text/TextRgbColour.hpp>

using namespace castor3d;

namespace castor
{
	TextWriter< PointLight >::TextWriter( String const & tabs )
		: TextWriterT< PointLight >{ tabs }
	{
	}

	bool TextWriter< PointLight >::operator()( PointLight const & light
		, TextFile & file )
	{
		log::info << tabs() << cuT( "Writing PointLight " ) << light.getLight().getName() << std::endl;
		bool result{ false };

		if ( auto block = beginBlock( file, "light", light.getLight().getName() ) )
		{
			result = writeName( file, "parent", light.getLight().getParent()->getName() )
				&& write( file, cuT( "type" ), castor3d::getName( light.getLightType() ) )
				&& write( file, cuT( "colour" ), light.getColour() )
				&& write( file, cuT( "intensity" ), light.getIntensity() )
				&& write( file, cuT( "attenuation" ), light.getAttenuation() )
				&& write( file, cuT( "shadow_producer" ), light.getLight().isShadowProducer() )
				&& write( file, light.getShadowConfig() );
		}

		return result;
	}
}

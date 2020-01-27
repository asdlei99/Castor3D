#include "FireworksParticle/FireworksParticle.hpp"

#include <Castor3D/Engine.hpp>
#include <Castor3D/Plugin/ParticlePlugin.hpp>
#include <Castor3D/Render/RenderSystem.hpp>

#include <CastorUtils/Log/Logger.hpp>

#ifndef CU_PlatformWindows
#	define C3D_Fireworks_API
#else
#	ifdef castor3dFireworksParticle_EXPORTS
#		define C3D_Fireworks_API __declspec( dllexport )
#	else
#		define C3D_Fireworks_API __declspec( dllimport )
#	endif
#endif

extern "C"
{
	C3D_Fireworks_API void getRequiredVersion( castor3d::Version * p_version )
	{
		*p_version = castor3d::Version();
	}

	C3D_Fireworks_API void getType( castor3d::PluginType * p_type )
	{
		*p_type = castor3d::PluginType::ePostEffect;
	}

	C3D_Fireworks_API void getName( char const ** p_name )
	{
		*p_name = Fireworks::ParticleSystem::Name.c_str();
	}

	C3D_Fireworks_API void OnLoad( castor3d::Engine * engine, castor3d::Plugin * p_plugin )
	{
		engine->getParticleFactory().registerType( Fireworks::ParticleSystem::Type
			, &Fireworks::ParticleSystem::create );
	}

	C3D_Fireworks_API void OnUnload( castor3d::Engine * engine )
	{
		engine->getParticleFactory().unregisterType( Fireworks::ParticleSystem::Type );
	}
}

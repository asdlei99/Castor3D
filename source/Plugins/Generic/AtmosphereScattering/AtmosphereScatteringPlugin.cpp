#include "AtmosphereScattering/AtmosphereScatteringPrerequisites.hpp"

#include "AtmosphereScattering/AtmosphereBackground.hpp"
#include "AtmosphereScattering/AtmosphereBackgroundModel.hpp"
#include "AtmosphereScattering/AtmosphereLightingModel.hpp"
#include "AtmosphereScattering/AtmosphereScattering_Parsers.hpp"

#include <Castor3D/Engine.hpp>
#include <Castor3D/Material/Pass/PassFactory.hpp>
#include <Castor3D/Material/Pass/PbrPass.hpp>
#include <Castor3D/Material/Pass/PhongPass.hpp>
#include <Castor3D/Model/Mesh/MeshFactory.hpp>
#include <Castor3D/Scene/SceneFileParser.hpp>

#include <CastorUtils/FileParser/ParserParameter.hpp>

namespace atmosphere_scattering
{
	namespace parser
	{
		static castor::AttributeParsers createParsers()
		{
			castor::AttributeParsers result;

			addParser( result
				, uint32_t( castor3d::CSCNSection::eScene )
				, cuT( "atmospheric_scattering" )
				, &parserAtmosphereScattering );
			addParser( result, uint32_t( AtmosphereSection::eRoot )
				, cuT( "}" )
				, &parserAtmosphereScatteringEnd );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "sunNode" )
				, &parserSunNode
				, { castor::makeParameter< castor::ParameterType::eName >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "planetNode" )
				, &parserPlanetNode
				, { castor::makeParameter< castor::ParameterType::eName >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "transmittanceResolution" )
				, &parserTransmittanceResolution
				, { castor::makeParameter< castor::ParameterType::ePoint2U >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "multiScatterResolution" )
				, &parserMultiScatterResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "atmosphereVolumeResolution" )
				, &parserAtmosphereVolumeResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "skyViewResolution" )
				, &parserSkyViewResolution
				, { castor::makeParameter< castor::ParameterType::ePoint2U >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "sunIlluminance" )
				, &parserSunIlluminance
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "sunIlluminanceScale" )
				, &parserSunIlluminanceScale
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "rayMarchMinSPP" )
				, &parserRayMarchMinSPP
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "rayMarchMaxSPP" )
				, &parserRayMarchMaxSPP
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "multipleScatteringFactor" )
				, &parserMultipleScatteringFactor
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "solarIrradiance" )
				, &parserSolarIrradiance
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "sunAngularRadius" )
				, &parserSunAngularRadius
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "absorptionExtinction" )
				, &parserAbsorptionExtinction
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "maxSunZenithAngle" )
				, &parserMaxSunZenithAngle
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "rayleighScattering" )
				, &parserRayleighScattering
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "mieScattering" )
				, &parserMieScattering
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "miePhaseFunctionG" )
				, &parserMiePhaseFunctionG
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "mieExtinction" )
				, &parserMieExtinction
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "bottomRadius" )
				, &parserBottomRadius
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "topRadius" )
				, &parserTopRadius
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "groundAlbedo" )
				, &parserGroundAlbedo
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "minRayleighDensity" )
				, &parserMinRayleighDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "maxRayleighDensity" )
				, &parserMaxRayleighDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "minMieDensity" )
				, &parserMinMieDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "maxMieDensity" )
				, &parserMaxMieDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "minAbsorptionDensity" )
				, &parserMinAbsorptionDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "maxAbsorptionDensity" )
				, &parserMaxAbsorptionDensity );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "weather" )
				, &parserWeather );
			addParser( result
				, uint32_t( AtmosphereSection::eRoot )
				, cuT( "clouds" )
				, &parserClouds );
			addParser( result
				, uint32_t( AtmosphereSection::eDensity )
				, cuT( "layerWidth" )
				, &parserDensityLayerWidth
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eDensity )
				, cuT( "expTerm" )
				, &parserDensityExpTerm
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eDensity )
				, cuT( "expScale" )
				, &parserDensityExpScale
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eDensity )
				, cuT( "linearTerm" )
				, &parserDensityLinearTerm
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eDensity )
				, cuT( "constantTerm" )
				, &parserDensityConstantTerm
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "worleyResolution" )
				, &parserWorleyResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "perlinWorleyResolution" )
				, &parserPerlinWorleyResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "weatherResolution" )
				, &parserWeatherResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "curlResolution" )
				, &parserCurlResolution
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "amplitude" )
				, &parserWeatherAmplitude
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "frequency" )
				, &parserWeatherFrequency
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "scale" )
				, &parserWeatherScale
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eWeather )
				, cuT( "octaves" )
				, &parserWeatherOctaves
				, { castor::makeParameter< castor::ParameterType::eUInt32 >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "windDirection" )
				, &parserCloudsWindDirection
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "speed" )
				, &parserCloudsSpeed
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "coverage" )
				, &parserCloudsCoverage
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "crispiness" )
				, &parserCloudsCrispiness
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "curliness" )
				, &parserCloudsCurliness
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "density" )
				, &parserCloudsDensity
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "absorption" )
				, &parserCloudsAbsorption
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "innerRadius" )
				, &parserCloudsInnerRadius
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "outerRadius" )
				, &parserCloudsOuterRadius
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "topColour" )
				, &parserCloudsTopColour
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "bottomColour" )
				, &parserCloudsBottomColour
				, { castor::makeParameter< castor::ParameterType::ePoint3F >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "enablePowder" )
				, &parserCloudsEnablePowder
				, { castor::makeParameter< castor::ParameterType::eBool >() } );
			addParser( result
				, uint32_t( AtmosphereSection::eClouds )
				, cuT( "topOffset" )
				, &parserCloudsTopOffset
				, { castor::makeParameter< castor::ParameterType::eFloat >() } );

			return result;
		}

		static castor::StrUInt32Map createSections()
		{
			return
			{
				{ uint32_t( AtmosphereSection::eRoot ), PluginType },
				{ uint32_t( AtmosphereSection::eDensity ), "density" },
				{ uint32_t( AtmosphereSection::eWeather ), "weather" },
				{ uint32_t( AtmosphereSection::eClouds ), "clouds" },
			};
		}

		static void * createContext( castor::FileParserContext & context )
		{
			auto userContext = new ParserContext;
			userContext->engine = static_cast< castor3d::SceneFileParser * >( context.parser )->getEngine();
			return userContext;
		}
	}
}

#ifndef CU_PlatformWindows
#	define C3D_AtmosphereScattering_API
#else
#	ifdef AtmosphereScattering_EXPORTS
#		define C3D_AtmosphereScattering_API __declspec( dllexport )
#	else
#		define C3D_AtmosphereScattering_API __declspec( dllimport )
#	endif
#endif

extern "C"
{
	C3D_AtmosphereScattering_API void getRequiredVersion( castor3d::Version * version );
	C3D_AtmosphereScattering_API void getType( castor3d::PluginType * type );
	C3D_AtmosphereScattering_API void getName( char const ** name );
	C3D_AtmosphereScattering_API void OnLoad( castor3d::Engine * engine, castor3d::Plugin * plugin );
	C3D_AtmosphereScattering_API void OnUnload( castor3d::Engine * engine );

	C3D_AtmosphereScattering_API void getRequiredVersion( castor3d::Version * version )
	{
		*version = castor3d::Version();
	}

	C3D_AtmosphereScattering_API void getType( castor3d::PluginType * type )
	{
		*type = castor3d::PluginType::eGeneric;
	}

	C3D_AtmosphereScattering_API void getName( char const ** name )
	{
		*name = atmosphere_scattering::PluginName.c_str();
	}

	C3D_AtmosphereScattering_API void OnLoad( castor3d::Engine * engine, castor3d::Plugin * plugin )
	{
		auto backgroundModelId = engine->registerBackgroundModel( atmosphere_scattering::AtmosphereBackgroundModel::Name
			, atmosphere_scattering::AtmosphereBackgroundModel::create );
		engine->registerPassModel( backgroundModelId
			, { atmosphere_scattering::AtmospherePhongLightingModel::getName()
				, castor3d::PhongPass::create
				, &atmosphere_scattering::AtmospherePhongLightingModel::create
				, false } );
		engine->registerPassModel( backgroundModelId
			, { atmosphere_scattering::AtmospherePbrLightingModel::getName()
				, castor3d::PbrPass::create
				, &atmosphere_scattering::AtmospherePbrLightingModel::create
				, true } );
		engine->registerParsers( atmosphere_scattering::PluginType
			, atmosphere_scattering::parser::createParsers()
			, atmosphere_scattering::parser::createSections()
			, &atmosphere_scattering::parser::createContext );
	}

	C3D_AtmosphereScattering_API void OnUnload( castor3d::Engine * engine )
	{
		auto backgroundModelId = engine->unregisterBackgroundModel( atmosphere_scattering::AtmosphereBackgroundModel::Name );
		engine->unregisterParsers( atmosphere_scattering::PluginType );
		engine->unregisterPassModel( backgroundModelId
			, engine->getPassFactory().getNameId( atmosphere_scattering::AtmospherePhongLightingModel::getName() ) );
		engine->unregisterPassModel( backgroundModelId
			, engine->getPassFactory().getNameId( atmosphere_scattering::AtmospherePbrLightingModel::getName() ) );
	}
}

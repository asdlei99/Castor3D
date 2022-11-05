/*
See LICENSE file in root folder
*/
#ifndef ___C3DWR_OceanUbo_H___
#define ___C3DWR_OceanUbo_H___

#include <Castor3D/Castor3DModule.hpp>
#include <Castor3D/Buffer/UniformBufferOffset.hpp>
#include <CastorUtils/Math/RangedValue.hpp>

#include <ShaderWriter/BaseTypes/Array.hpp>
#include <ShaderWriter/BaseTypes/Int.hpp>
#include <ShaderWriter/CompositeTypes/StructInstance.hpp>
#include <ShaderWriter/VecTypes/Vec4.hpp>

#define Ocean_Debug 1

namespace ocean
{
#if Ocean_Debug
	enum OceanDisplayData : uint32_t
	{
#pragma clang push
#pragma clang diagnostic ignored "-Wduplicate-enum"
		eResult,
		eFinalNormal,
		eMatSpecular,
		eLightDiffuse,
		eLightSpecular,
		eLightScattering,
		eNoisedSpecular,
		eSpecularNoise,
		eIndirectOcclusion,
		eLightIndirectDiffuse,
		eLightIndirectSpecular,
		eIndirectAmbient,
		eIndirectDiffuse,
		eBackgroundReflection,
		eSSRResult,
		eSSRFactor,
		eSSRResultFactor,
		eReflection,
		eRefraction,
		eDepthSoftenedAlpha,
		eHeightMixedRefraction,
		eDistanceMixedRefraction,
		eFresnelFactor,
		eFinalReflection,
		eFinalRefraction,
		eWaterBaseColour,
		CU_EnumBounds( OceanDisplayData, eResult )
#pragma clang pop
	};
	castor::StringArray const & getOceanDisplayDataNames();
#endif

	static uint32_t constexpr MaxWaves = 8u;

	struct WaveConfigurationData
	{
		castor::Point4f direction;
		float steepness;
		float length;
		float amplitude;
		float speed;
	};

	struct OceanUboConfiguration
	{
		float tessellationFactor{ 7.0f };
		uint32_t numWaves{ 2u };
		float time{ 0.0f };
		float dampeningFactor{ 5.0f };

		float refractionRatio{ 1.2f };
		float refractionDistortionFactor{ 0.04f };
		float refractionHeightFactor{ 2.5f };
		float refractionDistanceFactor{ 15.0f };

		float foamHeightStart{ 0.8f };
		float foamFadeDistance{ 0.4f };
		float foamTiling{ 2.0f };
		float foamAngleExponent{ 80.0f };

		castor::Point2f normalMapScrollSpeed{ 0.01f, 0.01f };
		float foamBrightness{ 4.0f };
		float depthSofteningDistance{ 0.5f };

		castor::Point4f normalMapScroll{ 1.0f, 0.0f, 0.0f, 1.0f };
		float ssrStepSize{ 0.5f };
		float ssrForwardStepsCount{ 20.0f };
		float ssrBackwardStepsCount{ 10.0f };
		float ssrDepthMult{ 20.0f };
#if Ocean_Debug
		uint32_t debug{ eResult };
		uint32_t dummy[3]{};
#endif
		std::array< WaveConfigurationData, MaxWaves > waves{ WaveConfigurationData{ castor::Point4f{ 0.3f, 0.0f, -0.7f }, 1.79f, 3.75f, 0.85f, 1.21f }
			, WaveConfigurationData{ castor::Point4f{ 0.5f, 0.0f, -0.2f }, 1.79f, 4.1f, 0.52f, 1.03f }
			, WaveConfigurationData{}
			, WaveConfigurationData{}
			, WaveConfigurationData{}
			, WaveConfigurationData{}
			, WaveConfigurationData{}
			, WaveConfigurationData{} };
	};

	struct Wave
		: sdw::StructInstanceHelperT< "Wave"
			, sdw::type::MemoryLayout::eStd140
			, sdw::Vec4Field< "direction" >
			, sdw::FloatField< "steepness" >
			, sdw::FloatField< "length" >
			, sdw::FloatField< "amplitude" >
			, sdw::FloatField< "speed" > >
	{
		Wave( sdw::ShaderWriter & writer
			, sdw::expr::ExprPtr expr
			, bool enabled = true )
			: StructInstanceHelperT{ writer, std::move( expr ), enabled }
		{
		}

		auto direction()const { return getMember< "direction" >().xyz(); }
		auto steepness()const { return getMember< "steepness" >(); }
		auto length()const { return getMember< "length" >(); }
		auto amplitude()const { return getMember< "amplitude" >(); }
		auto speed()const { return getMember< "speed" >(); }
	};

	Writer_Parameter( Wave );

	struct OceanData
		: public sdw::StructInstanceHelperT< "WavesUbo"
			, sdw::type::MemoryLayout::eStd140
			, sdw::FloatField< "tessellationFactor" >
			, sdw::UIntField< "numWaves" >
			, sdw::FloatField< "time" >
			, sdw::FloatField< "dampeningFactor" >
			, sdw::FloatField< "refractionRatio" >
			, sdw::FloatField< "refractionDistortionFactor" >
			, sdw::FloatField< "refractionHeightFactor" >
			, sdw::FloatField< "refractionDistanceFactor" >
			, sdw::FloatField< "foamHeightStart" >
			, sdw::FloatField< "foamFadeDistance" >
			, sdw::FloatField< "foamTiling" >
			, sdw::FloatField< "foamAngleExponent" >
			, sdw::Vec2Field< "normalMapScrollSpeed" >
			, sdw::FloatField< "foamBrightness" >
			, sdw::FloatField< "depthSofteningDistance" >
			, sdw::Vec4Field< "normalMapScroll" >
			, sdw::FloatField< "ssrStepSize" >
			, sdw::FloatField< "ssrForwardStepsCount" >
			, sdw::FloatField< "ssrBackwardStepsCount" >
			, sdw::FloatField< "ssrDepthMult" >
			, sdw::UVec4Field< "debug" >
			, sdw::StructFieldArrayT< Wave, "waves", MaxWaves > >
	{
		OceanData( sdw::ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled )
			: StructInstanceHelperT{ writer, std::move( expr ), enabled }
		{
		}

		auto normalMapScroll()const { return getMember< "normalMapScroll" >(); }
		auto ssrStepSize()const { return getMember< "ssrStepSize" >(); }
		auto ssrForwardStepsCount()const { return getMember< "ssrForwardStepsCount" >(); }
		auto ssrBackwardStepsCount()const { return getMember< "ssrBackwardStepsCount" >(); }
		auto ssrDepthMult()const { return getMember< "ssrDepthMult" >(); }
		auto debug()const { return getMember< "debug" >(); }
		auto waves()const { return getMember< "waves" >(); }
		auto tessellationFactor()const { return getMember< "tessellationFactor" >(); }
		auto numWaves()const { return getMember< "numWaves" >(); }
		auto time()const { return getMember< "time" >(); }
		auto dampeningFactor()const { return getMember< "dampeningFactor" >(); }
		auto refractionRatio()const { return getMember< "refractionRatio" >(); }
		auto refractionDistortionFactor()const { return getMember< "refractionDistortionFactor" >(); }
		auto refractionHeightFactor()const { return getMember< "refractionHeightFactor" >(); }
		auto refractionDistanceFactor()const { return getMember< "refractionDistanceFactor" >(); }
		auto foamHeightStart()const { return getMember< "foamHeightStart" >(); }
		auto foamFadeDistance()const { return getMember< "foamFadeDistance" >(); }
		auto foamTiling()const { return getMember< "foamTiling" >(); }
		auto foamAngleExponent()const { return getMember< "foamAngleExponent" >(); }
		auto foamBrightness()const { return getMember< "foamBrightness" >(); }
		auto depthSofteningDistance()const { return getMember< "depthSofteningDistance" >(); }
		auto normalMapScrollSpeed()const { return getMember< "normalMapScrollSpeed" >(); }
	};

	class OceanUbo
	{
	private:
		using Configuration = OceanUboConfiguration;

	public:
		explicit OceanUbo( castor3d::RenderDevice const & device );
		~OceanUbo();
		void cpuUpdate( OceanUboConfiguration const & config );

		void createPassBinding( crg::FramePass & pass
			, uint32_t binding )const
		{
			m_ubo.createPassBinding( pass, "WaveCfg", binding );
		}

		void createSizedBinding( ashes::DescriptorSet & descriptorSet
			, VkDescriptorSetLayoutBinding const & layoutBinding )const
		{
			return m_ubo.createSizedBinding( descriptorSet, layoutBinding );
		}

		ashes::WriteDescriptorSet getDescriptorWrite( uint32_t dstBinding
			, uint32_t dstArrayElement = 0u )const
		{
			return m_ubo.getDescriptorWrite( dstBinding, dstArrayElement );
		}

		castor3d::UniformBufferOffsetT< Configuration > const & getUbo()const
		{
			return m_ubo;
		}

		castor3d::UniformBufferOffsetT< Configuration > & getUbo()
		{
			return m_ubo;
		}

	public:
		static const castor::String Buffer;
		static const castor::String Data;

	private:
		castor3d::RenderDevice const & m_device;
		castor3d::UniformBufferOffsetT< Configuration > m_ubo;
	};
}

#define C3D_Ocean( writer, binding, set )\
	auto oceanBuffer = writer.declUniformBuffer( ocean::OceanUbo::Buffer, binding, set );\
	auto c3d_oceanData = oceanBuffer.declMember< ocean::OceanData >( ocean::OceanUbo::Data );\
	oceanBuffer.end()

#endif

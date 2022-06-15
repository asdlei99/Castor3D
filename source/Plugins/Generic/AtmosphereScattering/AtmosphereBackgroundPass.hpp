/*
See LICENSE file in root folder
*/
#ifndef ___C3DAS_AtmosphereBackgroundPass_H___
#define ___C3DAS_AtmosphereBackgroundPass_H___

#include "AtmosphereScatteringPrerequisites.hpp"

#include <Castor3D/Render/Passes/BackgroundPassBase.hpp>
#include <Castor3D/Scene/Background/Background.hpp>
#include <Castor3D/Shader/ShaderModule.hpp>

namespace atmosphere_scattering
{
	class AtmosphereBackgroundPass
		: public castor3d::BackgroundPassBase
	{
	public:
		enum Bindings : uint32_t
		{
			eTransmittance = castor3d::SceneBackground::SkyBoxImgIdx,
			eMultiScatter,
			eAtmosphere,
			eCount,
		};

	public:
		AtmosphereBackgroundPass( crg::FramePass const & pass
			, crg::GraphContext & context
			, crg::RunnableGraph & graph
			, castor3d::RenderDevice const & device
			, AtmosphereBackground & background
			, VkExtent2D const & size
			, bool usesDepth );

	private:
		void doInitialiseVertexBuffer()override;
		ashes::PipelineShaderStageCreateInfoArray doInitialiseShader()override;
		void doFillDescriptorBindings()override;
		void doCreatePipeline()override;

	private:
		AtmosphereBackground & m_atmosBackground;
		castor3d::ShaderModule m_vertexModule;
		castor3d::ShaderModule m_pixelModule;
	};
}

#endif
/*
See LICENSE file in root folder
*/
#ifndef ___C3D_OverlayVertexBufferPool_H___
#define ___C3D_OverlayVertexBufferPool_H___

#include "Castor3D/Render/Overlays/OverlaysModule.hpp"

#include "Castor3D/Buffer/GpuBuffer.hpp"
#include "Castor3D/Render/Overlays/OverlayTextBufferPool.hpp"
#include "Castor3D/Shader/Ubos/UbosModule.hpp"

#include <CastorUtils/Design/ArrayView.hpp>

#include <ashespp/Buffer/Buffer.hpp>
#include <ashespp/Descriptor/DescriptorSetPool.hpp>
#include <ashespp/Pipeline/PipelineVertexInputStateCreateInfo.hpp>

namespace castor3d
{
	template< typename VertexT, uint32_t CountT >
	struct OverlayVertexBufferPoolT
	{
		using Quad = std::array< VertexT, CountT >;
		static bool constexpr isPanel = std::is_same_v< VertexT, OverlayCategory::Vertex > && ( CountT == 6u );
		static bool constexpr isBorder = std::is_same_v< VertexT, OverlayCategory::Vertex > && ( CountT == 48u );
		static bool constexpr isText = !isPanel && !isBorder;
		static bool constexpr isCpuFilled = isText;

		OverlayVertexBufferPoolT( Engine & engine
			, std::string const & debugName
			, RenderDevice const & device
			, CameraUbo const & cameraUbo
			, ashes::DescriptorSetLayout const & descriptorLayout
			, uint32_t count
			, OverlayTextBufferPoolUPtr textBuf = nullptr );
		template< typename OverlayT >
		bool fill( castor::Size const & renderSize
			, OverlayT const & overlay
			, OverlayDrawData & data
			, bool secondary
			, FontTexture const * fontTexture );
		void upload( UploadData & uploader );

		void clearDrawPipelineData( FontTexture const * fontTexture );
		OverlayPipelineData & getDrawPipelineData( OverlayDrawPipeline const & pipeline
			, FontTexture const * fontTexture
			, ashes::DescriptorSet const * textDescriptorSet );
		void fillComputeDescriptorSet( FontTexture const * fontTexture
			, ashes::DescriptorSetLayout const & descriptorLayout
			, ashes::DescriptorSet & descriptorSet )const;
		OverlayTextBuffer const * getTextBuffer( FontTexture const & fontTexture )const;

		Engine & engine;
		RenderDevice const & device;
		CameraUbo const & cameraUbo;
		ashes::DescriptorSetLayout const & descriptorLayout;
		std::string name;
		ashes::BufferPtr< OverlayUboConfiguration > overlaysData;
		castor::ArrayView< OverlayUboConfiguration > overlaysBuffer;
		GpuBufferBase vertexBuffer;
		uint32_t allocated{};
		uint32_t index{};
		ashes::DescriptorSetPoolPtr descriptorPool;
		OverlayTextBufferPoolUPtr textBuffer;

	private:
		using PipelineDataMap = std::map< OverlayDrawPipeline const *, OverlayPipelineData >;
		std::map< FontTexture const *, PipelineDataMap > m_pipelines;
		std::vector< OverlayPipelineData > m_retired;

	private:
		ashes::DescriptorSetPtr doCreateDescriptorSet( std::string debugName
			, FontTexture const * fontTexture
			, ashes::BufferBase const & idsBuffer )const;
	};
}

#include "OverlayVertexBufferPool.inl"

#endif

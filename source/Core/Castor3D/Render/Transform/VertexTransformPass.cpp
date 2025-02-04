#include "Castor3D/Render/Transform/VertexTransformPass.hpp"

#include "Castor3D/Buffer/GpuBuffer.hpp"
#include "Castor3D/Buffer/GpuBufferPool.hpp"
#include "Castor3D/Buffer/ObjectBufferOffset.hpp"
#include "Castor3D/Render/RenderDevice.hpp"
#include "Castor3D/Render/Node/SubmeshRenderNode.hpp"
#include "Castor3D/Render/Transform/VertexTransforming.hpp"
#include "Castor3D/Scene/Geometry.hpp"
#include "Castor3D/Scene/Animation/AnimatedMesh.hpp"
#include "Castor3D/Scene/Animation/AnimatedSkeleton.hpp"

#include <RenderGraph/RecordContext.hpp>

#include <ashespp/Descriptor/DescriptorSetPool.hpp>
#include <ashespp/Pipeline/ComputePipeline.hpp>
#include <ashespp/Pipeline/PipelineLayout.hpp>

CU_ImplementSmartPtr( castor3d, VertexTransformPass )

namespace castor3d
{
	//*********************************************************************************************

	namespace vtxtrs
	{
		static ashes::DescriptorSetPtr createDescriptorSet( TransformPipeline const & pipeline
			, ObjectBufferOffset const & input
			, ObjectBufferOffset const & output
			, ashes::Buffer< ModelBufferConfiguration > const & modelsBuffer
			, GpuBufferOffsetT< castor::Point4f > const & morphTargets
			, GpuBufferOffsetT< MorphingWeightsConfiguration > const & morphingWeights
			, GpuBufferOffsetT< SkinningTransformsConfiguration > const & skinTransforms )
		{
			ashes::WriteDescriptorSetArray writes;
			CU_Require( morphTargets || skinTransforms );
			writes.push_back( ashes::WriteDescriptorSet{ VertexTransformPass::eModelsData
				, 0u
				, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
				, { VkDescriptorBufferInfo{ modelsBuffer.getBuffer(), 0u, ashes::WholeSize } } } );

			if ( morphTargets )
			{
				writes.push_back( morphTargets.getStorageBinding( VertexTransformPass::eMorphTargets ) );
			}

			if ( morphingWeights )
			{
				writes.push_back( morphingWeights.getStorageBinding( VertexTransformPass::eMorphingWeights ) );
			}

			if ( skinTransforms )
			{
				writes.push_back( skinTransforms.getStorageBinding( VertexTransformPass::eSkinTransforms ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::ePositions ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::ePositions
					, VertexTransformPass::eInPosition ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::ePositions
					, VertexTransformPass::eOutPosition ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eNormals ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eNormals
					, VertexTransformPass::eInNormal ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eNormals
					, VertexTransformPass::eOutNormal ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eTangents ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eTangents
					, VertexTransformPass::eInTangent ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eTangents
					, VertexTransformPass::eOutTangent ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eBitangents ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eBitangents
					, VertexTransformPass::eInBitangent ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eBitangents
					, VertexTransformPass::eOutBitangent ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eTexcoords0 ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eTexcoords0
					, VertexTransformPass::eInTexcoord0 ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eTexcoords0
					, VertexTransformPass::eOutTexcoord0 ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eTexcoords1 ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eTexcoords1
					, VertexTransformPass::eInTexcoord1 ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eTexcoords1
					, VertexTransformPass::eOutTexcoord1 ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eTexcoords2 ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eTexcoords2
					, VertexTransformPass::eInTexcoord2 ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eTexcoords2
					, VertexTransformPass::eOutTexcoord2 ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eTexcoords3 ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eTexcoords3
					, VertexTransformPass::eInTexcoord3 ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eTexcoords3
					, VertexTransformPass::eOutTexcoord3 ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eColours ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eColours
					, VertexTransformPass::eInColour ) );
				writes.push_back( output.getStorageBinding( SubmeshFlag::eColours
					, VertexTransformPass::eOutColour ) );
			}

			if ( checkFlag( pipeline.submeshFlags, SubmeshFlag::eSkin ) )
			{
				writes.push_back( input.getStorageBinding( SubmeshFlag::eSkin
					, VertexTransformPass::eInSkin ) );
			}

			writes.push_back( output.getStorageBinding( SubmeshFlag::eVelocity
				, VertexTransformPass::eOutVelocity ) );

			auto descriptorSet = pipeline.descriptorSetPool->createDescriptorSet( pipeline.getName() );
			descriptorSet->setBindings( writes );
			descriptorSet->update();
			return descriptorSet;
		}
	}

	//*********************************************************************************************

	VertexTransformPass::VertexTransformPass( RenderDevice const & device
		, SubmeshRenderNode const & node
		, TransformPipeline const & pipeline
		, ObjectBufferOffset const & input
		, ObjectBufferOffset const & output
		, ashes::Buffer< ModelBufferConfiguration > const & modelsBuffer
		, GpuBufferOffsetT< castor::Point4f > const & morphTargets
		, GpuBufferOffsetT< MorphingWeightsConfiguration > const & morphingWeights
		, GpuBufferOffsetT< SkinningTransformsConfiguration > const & skinTransforms )
		: m_device{ device }
		, m_pipeline{ pipeline }
		, m_input{ input }
		, m_output{ output }
		, m_morphTargets{ morphTargets }
		, m_morphingWeights{ morphingWeights }
		, m_skinTransforms{ skinTransforms }
		, m_descriptorSet{ vtxtrs::createDescriptorSet( pipeline
			, m_input
			, m_output
			, modelsBuffer
			, morphTargets
			, morphingWeights
			, skinTransforms ) }
	{
		m_objectIds.nodeId = node.instance.getId( *node.pass, node.data ) - 1u;
		m_objectIds.morphingId = node.mesh ? node.mesh->getId( node.data ) - 1u : 0u;
		m_objectIds.skinningId = node.skeleton ? node.skeleton->getId() - 1u : 0u;
	}

	void VertexTransformPass::recordInto( crg::RecordContext & context
		, VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		if ( m_morphTargets )
		{
			context.memoryBarrier( commandBuffer
				, m_morphTargets.buffer->getBuffer()
				, { m_morphTargets.chunk.offset, m_morphTargets.chunk.size }
				, VK_ACCESS_HOST_WRITE_BIT
				, VK_PIPELINE_STAGE_HOST_BIT
				, { VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
		}

		if ( m_morphingWeights )
		{
			context.memoryBarrier( commandBuffer
				, m_morphingWeights.buffer->getBuffer()
				, { m_morphingWeights.chunk.offset, m_morphingWeights.chunk.size }
				, VK_ACCESS_HOST_WRITE_BIT
				, VK_PIPELINE_STAGE_HOST_BIT
				, { VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
		}

		if ( m_skinTransforms )
		{
			context.memoryBarrier( commandBuffer
				, m_skinTransforms.buffer->getBuffer()
				, { m_skinTransforms.chunk.offset, m_skinTransforms.chunk.size }
				, VK_ACCESS_HOST_WRITE_BIT
				, VK_PIPELINE_STAGE_HOST_BIT
				, { VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
		}

		auto itInput = m_input.buffers.begin();
		auto itOutput = m_output.buffers.begin();

		while ( itInput != m_input.buffers.end() )
		{
			if ( itInput->buffer && itOutput->buffer )
			{
				context.memoryBarrier( commandBuffer
					, itInput->buffer->getBuffer()
					, { itInput->chunk.offset, itInput->chunk.size }
					, VK_ACCESS_HOST_WRITE_BIT
					, VK_PIPELINE_STAGE_HOST_BIT
					, { VK_ACCESS_SHADER_READ_BIT
						, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
				context.memoryBarrier( commandBuffer
					, itOutput->buffer->getBuffer()
					, { itOutput->chunk.offset, itOutput->chunk.size }
					, VK_ACCESS_HOST_WRITE_BIT
					, VK_PIPELINE_STAGE_HOST_BIT
					, { VK_ACCESS_SHADER_WRITE_BIT
						, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
			}

			++itInput;
			++itOutput;
		}

		context.getContext().vkCmdBindPipeline( commandBuffer
			, VK_PIPELINE_BIND_POINT_COMPUTE
			, *m_pipeline.pipeline );
		context.getContext().vkCmdPushConstants( commandBuffer
			, *m_pipeline.pipelineLayout
			, VK_SHADER_STAGE_COMPUTE_BIT
			, 0u
			, sizeof( ObjectIdsConfiguration )
			, &m_objectIds );
		VkDescriptorSet set = *m_descriptorSet;
		context.getContext().vkCmdBindDescriptorSets( commandBuffer
			, VK_PIPELINE_BIND_POINT_COMPUTE
			, *m_pipeline.pipelineLayout
			, 0u
			, 1u
			, &set
			, 0u
			, nullptr );
		auto size = uint32_t( m_device.properties.limits.nonCoherentAtomSize );
		context.getContext().vkCmdDispatch( commandBuffer
			, uint32_t( ashes::getAlignedSize( m_input.getCount< castor::Point4f >( SubmeshFlag::ePositions ), size ) ) / size
			, 1u
			, 1u );

		itInput = m_input.buffers.begin();
		itOutput = m_output.buffers.begin();

		while ( itInput != m_input.buffers.end() )
		{
			if ( itInput->buffer && itOutput->buffer )
			{
				context.memoryBarrier( commandBuffer
					, itOutput->buffer->getBuffer()
					, { itOutput->chunk.offset, itOutput->chunk.size }
					, VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					, { VK_ACCESS_HOST_WRITE_BIT
						, VK_PIPELINE_STAGE_HOST_BIT } );

				if ( m_pipeline.meshletsBounds )
				{
					context.memoryBarrier( commandBuffer
						, itInput->buffer->getBuffer()
						, { itInput->chunk.offset, itInput->chunk.size }
						, VK_ACCESS_SHADER_WRITE_BIT
						, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
						, { VK_ACCESS_SHADER_READ_BIT
							, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT } );
				}
				else
				{
					context.memoryBarrier( commandBuffer
						, itInput->buffer->getBuffer()
						, { itInput->chunk.offset, itInput->chunk.size }
						, VK_ACCESS_SHADER_WRITE_BIT
						, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
						, { VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
							, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT } );
				}
			}

			++itInput;
			++itOutput;
		}

		if ( !m_pipeline.meshletsBounds )
		{
			if ( m_skinTransforms )
			{
				context.memoryBarrier( commandBuffer
					, m_skinTransforms.buffer->getBuffer()
					, { m_skinTransforms.chunk.offset, m_skinTransforms.chunk.size }
					, VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					, { VK_ACCESS_HOST_WRITE_BIT
						, VK_PIPELINE_STAGE_HOST_BIT } );
			}

			if ( m_morphTargets )
			{
				context.memoryBarrier( commandBuffer
					, m_morphTargets.buffer->getBuffer()
					, { m_morphTargets.chunk.offset, m_morphTargets.chunk.size }
					, VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					, { VK_ACCESS_HOST_WRITE_BIT
						, VK_PIPELINE_STAGE_HOST_BIT } );
			}

			if ( m_morphingWeights )
			{
				context.memoryBarrier( commandBuffer
					, m_morphingWeights.buffer->getBuffer()
					, { m_morphingWeights.chunk.offset, m_morphingWeights.chunk.size }
					, VK_ACCESS_SHADER_READ_BIT
					, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					, { VK_ACCESS_HOST_WRITE_BIT
						, VK_PIPELINE_STAGE_HOST_BIT } );
			}
		}
	}

	//*********************************************************************************************
}

#include "Castor3D/Model/Mesh/Submesh/Component/BonesComponent.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/GpuBuffer.hpp"
#include "Castor3D/Buffer/GpuBufferPool.hpp"
#include "Castor3D/Miscellaneous/makeVkType.hpp"
#include "Castor3D/Miscellaneous/StagingData.hpp"
#include "Castor3D/Model/Mesh/Submesh/Submesh.hpp"
#include "Castor3D/Model/Skeleton/BonedVertex.hpp"
#include "Castor3D/Render/RenderDevice.hpp"
#include "Castor3D/Render/RenderNodesPass.hpp"
#include "Castor3D/Render/RenderSystem.hpp"
#include "Castor3D/Scene/Scene.hpp"
#include "Castor3D/Shader/ShaderBuffer.hpp"

#include <CastorUtils/Miscellaneous/Hash.hpp>

namespace castor3d
{
	//*********************************************************************************************

	namespace smshcompskin
	{
		static ashes::PipelineVertexInputStateCreateInfo doCreateVertexLayout( ShaderFlags shaderFlags
			, uint32_t & currentLocation )
		{
			ashes::VkVertexInputBindingDescriptionArray bindings{ { BonesComponent::BindingPoint
				, sizeof( VertexBoneData ), VK_VERTEX_INPUT_RATE_VERTEX } };

			ashes::VkVertexInputAttributeDescriptionArray attributes;
			attributes.push_back( { currentLocation++
				, BonesComponent::BindingPoint
				, VK_FORMAT_R32G32B32A32_UINT
				, offsetof( VertexBoneData, m_ids ) + offsetof( VertexBoneData::Ids::ids, id0 ) } );
			attributes.push_back( { currentLocation++
				, BonesComponent::BindingPoint
				, VK_FORMAT_R32G32B32A32_UINT
				, offsetof( VertexBoneData, m_ids ) + offsetof( VertexBoneData::Ids::ids, id1 ) } );
			attributes.push_back( { currentLocation++
				, BonesComponent::BindingPoint
				, VK_FORMAT_R32G32B32A32_SFLOAT
				, offsetof( VertexBoneData, m_weights ) + offsetof( VertexBoneData::Weights::weights, weight0 ) } );
			attributes.push_back( { currentLocation++
				, BonesComponent::BindingPoint
				, VK_FORMAT_R32G32B32A32_SFLOAT
				, offsetof( VertexBoneData, m_weights ) + offsetof( VertexBoneData::Weights::weights, weight1 ) } );

			return ashes::PipelineVertexInputStateCreateInfo{ 0u, bindings, attributes };
		}
	}

	//*********************************************************************************************

	castor::String const BonesComponent::Name = cuT( "bones" );

	BonesComponent::BonesComponent( Submesh & submesh )
		: SubmeshComponent{ submesh, Name, BindingPoint }
	{
	}

	void BonesComponent::addBoneDatas( VertexBoneData const * const begin
		, VertexBoneData const * const end )
	{
		m_bones.insert( m_bones.end(), begin, end );
	}

	SkeletonRPtr BonesComponent::getSkeleton()const
	{
		return getOwner()->getParent().getSkeleton();
	}

	void BonesComponent::gather( ShaderFlags const & shaderFlags
		, SubmeshFlags const & submeshFlags
		, MaterialRPtr material
		, ashes::BufferCRefArray & buffers
		, std::vector< uint64_t > & offsets
		, ashes::PipelineVertexInputStateCreateInfoCRefArray & layouts
		, TextureFlagsArray const & mask
		, uint32_t & currentLocation )
	{
		if ( checkFlag( submeshFlags, SubmeshFlag::eSkinning ) )
		{
			auto hash = std::hash< ShaderFlags::BaseType >{}( shaderFlags );
			hash = castor::hashCombine( hash, mask.empty() );
			hash = castor::hashCombine( hash, currentLocation );
			auto layoutIt = m_bonesLayouts.find( hash );

			if ( layoutIt == m_bonesLayouts.end() )
			{
				layoutIt = m_bonesLayouts.emplace( hash
					, smshcompskin::doCreateVertexLayout( shaderFlags, currentLocation ) ).first;
			}
			else
			{
				currentLocation = layoutIt->second.vertexAttributeDescriptions.back().location + 1u;
			}

			buffers.emplace_back( getOwner()->getBufferOffsets().getBonesBuffer() );
			offsets.emplace_back( 0u );
			layouts.emplace_back( layoutIt->second );
		}
	}

	SubmeshComponentSPtr BonesComponent::clone( Submesh & submesh )const
	{
		auto result = std::make_shared< BonesComponent >( submesh );
		result->m_bones = m_bones;
		return std::static_pointer_cast< SubmeshComponent >( result );
	}

	void BonesComponent::addBoneDatas( std::vector< VertexBoneData > const & boneData )
	{
		addBoneDatas( boneData.data(), boneData.data() + boneData.size() );
	}

	bool BonesComponent::doInitialise( RenderDevice const & device )
	{
		return true;
	}

	void BonesComponent::doCleanup( RenderDevice const & device )
	{
	}

	void BonesComponent::doUpload()
	{
		auto count = uint32_t( m_bones.size() );
		auto & offsets = getOwner()->getBufferOffsets();

		if ( count && offsets.hasBones() )
		{
			std::copy( m_bones.begin()
				, m_bones.end()
				, offsets.getBoneData().begin() );
			offsets.markBonesDirty();
		}
	}

	//*********************************************************************************************
}

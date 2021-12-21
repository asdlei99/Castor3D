#include "Castor3D/Model/Mesh/Submesh/Submesh.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/GeometryBuffers.hpp"
#include "Castor3D/Buffer/GpuBuffer.hpp"
#include "Castor3D/Cache/MaterialCache.hpp"
#include "Castor3D/Material/Material.hpp"
#include "Castor3D/Render/RenderPass.hpp"
#include "Castor3D/Scene/Scene.hpp"

#include <CastorUtils/Miscellaneous/Hash.hpp>

namespace castor3d
{
	namespace
	{
		size_t hash( MaterialRPtr material
			, ShaderFlags const & shaderFlags
			, ProgramFlags const & programFlags
			, TextureFlagsArray const & mask
			, bool forceTexcoords )
		{
			auto result = std::hash< Material * >{}( material );

			for ( auto const & flagId : mask )
			{
				result = castor::hashCombine( result, flagId.id );
				result = castor::hashCombine( result, flagId.flags.value() );
			}

			result = castor::hashCombine( result, shaderFlags.value() );
			result = castor::hashCombine( result, programFlags.value() );
			result = castor::hashCombine( result, forceTexcoords );
			return result;
		}

		bool fix( castor::Point3f & value
			, castor::Point3f const & defaultValue )
		{
			bool result = false;

			if ( std::isnan( value->x ) )
			{
				value->x = defaultValue->x;
				result = true;
			}

			if ( std::isnan( value->y ) )
			{
				value->z = defaultValue->y;
				result = true;
			}

			if ( std::isnan( value->z ) )
			{
				value->z = defaultValue->z;
				result = true;
			}

			return result;
		}

		bool fixNml( castor::Point3f & value )
		{
			static castor::Point3f const defaultValue{ 0.0f, 1.0f, 0.0f };
			auto result = fix( value, defaultValue );

			if ( castor::point::length( value ) < std::numeric_limits< float >::epsilon() )
			{
				value = defaultValue;
				result = true;
			}

			return result;
		}

		bool fixPos( castor::Point3f & value )
		{
			static castor::Point3f const defaultValue{ 0.0f, 0.0f, 0.0f };
			return fix( value, defaultValue );
		}

		bool fixTex( castor::Point3f & value )
		{
			static castor::Point3f const defaultValue{ 0.0f, 0.0f, 0.0f };
			return fix( value, defaultValue );
		}

		ashes::PipelineVertexInputStateCreateInfo doCreateVertexLayout( ShaderFlags const & flags
			, bool hasTextures
			, uint32_t & currentLocation )
		{
			ashes::VkVertexInputBindingDescriptionArray bindings{ { 0u
				, sizeof( InterleavedVertex ), VK_VERTEX_INPUT_RATE_VERTEX } };

			ashes::VkVertexInputAttributeDescriptionArray attributes{ 1u, { currentLocation++
				, 0u
				, VK_FORMAT_R32G32B32_SFLOAT
				, offsetof( InterleavedVertex, pos ) } };

			if ( checkFlag( flags, ShaderFlag::eNormal ) )
			{
				attributes.push_back( { currentLocation++
					, 0u
					, VK_FORMAT_R32G32B32_SFLOAT
					, offsetof( InterleavedVertex, nml ) } );
			}

			if ( checkFlag( flags, ShaderFlag::eTangentSpace ) )
			{
				attributes.push_back( { currentLocation++
					, 0u
					, VK_FORMAT_R32G32B32_SFLOAT
					, offsetof( InterleavedVertex, tan ) } );
			}

			if ( hasTextures )
			{
				attributes.push_back( { currentLocation++
					, 0u
					, VK_FORMAT_R32G32B32_SFLOAT
					, offsetof( InterleavedVertex, tex ) } );
			}

			return ashes::PipelineVertexInputStateCreateInfo{ 0u, bindings, attributes };
		}
	}

	Submesh::Submesh( Mesh & mesh, uint32_t id )
		: OwnedBy< Mesh >{ mesh }
		, m_id{ id }
		, m_defaultMaterial{ mesh.getScene()->getEngine()->getMaterialCache().getDefaultMaterial() }
	{
		addComponent( std::make_shared< InstantiationComponent >( *this, 2u ) );
	}

	Submesh::~Submesh()
	{
		CU_Assert( !m_initialised, "Did you forget to call Submesh::cleanup ?" );
	}

	void Submesh::initialise( RenderDevice const & device )
	{
		if ( !m_generated )
		{
			doGenerateVertexBuffer( device );

			if ( m_indexMapping )
			{
				m_indexBuffer = makeBuffer< uint32_t >( device
					, VkDeviceSize( m_indexMapping->getCount() ) * m_indexMapping->getComponentsCount()
					, VK_BUFFER_USAGE_INDEX_BUFFER_BIT
					, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
					, getParent().getName() + "Submesh" + castor::string::toString( m_id ) + "IndexBuffer" );
			}

			for ( auto & component : m_components )
			{
				component.second->initialise( device );
				component.second->upload();
			}

			m_generated = true;
		}

		if ( !m_initialised )
		{
			m_initialised = true;

			if ( m_instantiation )
			{
				// Make sure instantiation is initialised, for the components that need it.
				m_instantiation->initialise( device );
			}

			for ( auto & component : m_components )
			{
				m_initialised = m_initialised && component.second->initialise( device );
			}

			m_dirty = !m_initialised;
		}
	}

	void Submesh::cleanup()
	{
		m_initialised = false;

		for ( auto & component : m_components )
		{
			component.second->cleanup();
		}

		m_vertexBuffer.reset();
		m_indexBuffer.reset();
		m_vertexLayouts.clear();
		m_points.clear();
	}

	void Submesh::update()
	{
		if ( m_dirty && m_vertexBuffer )
		{
			auto size = uint32_t( m_points.size() );

			if ( auto buffer = m_vertexBuffer->lock( 0u
				, size
				, 0u ) )
			{
				std::copy( m_points.begin(), m_points.end(), buffer );
				m_vertexBuffer->flush( 0u, size );
				m_vertexBuffer->unlock();
			}

			m_dirty = false;
		}

		for ( auto & component : m_components )
		{
			component.second->upload();
		}
	}

	void Submesh::computeContainers()
	{
		if ( !m_points.empty() )
		{
			castor::Point3f min = m_points[0].pos;
			castor::Point3f max = m_points[0].pos;
			uint32_t nbVertex = getPointsCount();

			for ( uint32_t i = 1; i < nbVertex; i++ )
			{
				castor::Point3f cur = m_points[i].pos;
				max[0] = std::max( cur[0], max[0] );
				max[1] = std::max( cur[1], max[1] );
				max[2] = std::max( cur[2], max[2] );
				min[0] = std::min( cur[0], min[0] );
				min[1] = std::min( cur[1], min[1] );
				min[2] = std::min( cur[2], min[2] );
			}

			m_box.load( min, max );
			m_sphere.load( m_box );

			if ( m_needsNormalsCompute )
			{
				computeNormals( false );
			}
		}
	}

	void Submesh::updateContainers( castor::BoundingBox const & boundingBox )
	{
		m_box = boundingBox;
		m_sphere.load( m_box );
	}

	uint32_t Submesh::getFaceCount()const
	{
		uint32_t result = 0u;
		
		if ( m_indexMapping )
		{
			result = m_indexMapping->getCount();
		}
		else
		{
			result = getPointsCount();

			switch ( getTopology() )
			{
			case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
				break;

			case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
				result /= 2;
				break;

			case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
				result -= 1u;
				break;

			case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
				result /= 3u;
				break;

			case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
				result -= 2u;
				break;

			default:
				break;
			}
		}

		return result;
	}

	uint32_t Submesh::getPointsCount()const
	{
		return std::max< uint32_t >( uint32_t( m_points.size() )
			, ( m_vertexBuffer ? uint32_t( m_vertexBuffer->getCount() ) : 0u ) );
	}

	int Submesh::isInMyPoints( castor::Point3f const & vertex
		, double precision )
	{
		int result = -1;
		int index = 0;

		for ( auto it = m_points.begin(); it != m_points.end() && result == -1; ++it )
		{
			if ( castor::point::distanceSquared( vertex, it->pos ) < precision )
			{
				result = index;
			}

			index++;
		}

		return result;
	}

	InterleavedVertex Submesh::addPoint( float x, float y, float z )
	{
		InterleavedVertex result;
		result.pos = castor::Point3f{ x, y, z };
		addPoint( result );
		return result;
	}

	InterleavedVertex Submesh::addPoint( castor::Point3f const & value )
	{
		return addPoint( value[0], value[1], value[2] );
	}

	InterleavedVertex Submesh::addPoint( float * value )
	{
		return addPoint( value[0], value[1], value[2] );
	}

	void Submesh::addPoint( InterleavedVertex const & vertex )
	{
		auto point = vertex;
		m_needsNormalsCompute = fixNml( point.nml );
		m_needsNormalsCompute = fixPos( point.pos ) || m_needsNormalsCompute;
		m_needsNormalsCompute = fixTex( point.tex ) || m_needsNormalsCompute;
		m_points.push_back( point );
	}

	void Submesh::addPoints( InterleavedVertex const * const begin
		, InterleavedVertex const * const end )
	{
		m_points.reserve( size_t( m_points.size() + end - begin ) );

		for ( auto & point : castor::makeArrayView( begin, end ) )
		{
			addPoint( point );
		}
	}

	void Submesh::computeNormals( bool reverted )
	{
		if ( m_indexMapping )
		{
			m_indexMapping->computeNormals( reverted );
			m_needsNormalsCompute = false;
		}
	}

	void Submesh::sortByDistance( castor::Point3f const & cameraPosition )
	{
		if ( m_indexMapping )
		{
			m_indexMapping->sortByDistance( cameraPosition );
		}
	}

	ProgramFlags Submesh::getProgramFlags( MaterialRPtr material )const
	{
		auto result = m_programFlags;

		for ( auto & component : m_components )
		{
			result |= component.second->getProgramFlags( material );
		}

		return result;
	}

	void Submesh::setMaterial( MaterialRPtr oldMaterial
		, MaterialRPtr newMaterial
		, bool update )
	{
		if ( oldMaterial != newMaterial )
		{
			if ( m_instantiation )
			{
				m_instantiation->unref( oldMaterial );
				m_instantiation->ref( newMaterial );
			}

			if ( !m_disableSceneUpdate )
			{
				getOwner()->getScene()->setChanged();
			}
		}
	}

	GeometryBuffers const & Submesh::getGeometryBuffers( ShaderFlags const & shaderFlags
		, ProgramFlags const & programFlags
		, MaterialRPtr material
		, uint32_t instanceMult
		, TextureFlagsArray const & mask
		, bool forceTexcoords )const
	{
		auto key = hash( material, shaderFlags, programFlags, mask, forceTexcoords );
		auto it = m_geometryBuffers.find( key );
		bool hasTextures = forceTexcoords || ( !mask.empty() );

		if ( it == m_geometryBuffers.end() )
		{
			ashes::BufferCRefArray buffers;
			ashes::UInt64Array offsets;
			ashes::PipelineVertexInputStateCreateInfoCRefArray layouts;
			buffers.emplace_back( m_vertexBuffer->getBuffer() );
			offsets.emplace_back( 0u );
			auto hash = std::hash< ShaderFlags::BaseType >{}( shaderFlags );
			hash = castor::hashCombine( hash, hasTextures );

			auto layoutIt = m_vertexLayouts.find( hash );
			uint32_t currentLocation = 0u;

			if ( layoutIt == m_vertexLayouts.end() )
			{
				layoutIt = m_vertexLayouts.emplace( hash
					, doCreateVertexLayout( shaderFlags, hasTextures, currentLocation ) ).first;
			}
			else
			{
				currentLocation = layoutIt->second.vertexAttributeDescriptions.back().location + 1u;
			}

			layouts.push_back( layoutIt->second );

			for ( auto & component : m_components )
			{
				component.second->gather( shaderFlags
					, programFlags
					, material
					, buffers
					, offsets
					, layouts
					, instanceMult
					, mask
					, currentLocation );
			}

			GeometryBuffers result;
			result.vbo = buffers;
			result.vboOffsets = offsets;
			result.layouts = layouts;
			result.vtxCount = uint32_t( m_vertexBuffer->getCount() );

			if ( m_indexBuffer )
			{
				result.ibo = &m_indexBuffer->getBuffer();
				result.iboOffset = 0u;
				result.idxCount = uint32_t( m_indexBuffer->getCount() );
			}

			it = m_geometryBuffers.emplace( key, std::move( result ) ).first;
		}

		return it->second;
	}

	void Submesh::enableSceneUpdate( bool updateScene )
	{
		m_disableSceneUpdate = false;

		if ( updateScene )
		{
			getOwner()->getScene()->setChanged();
		}
	}

	void Submesh::doGenerateVertexBuffer( RenderDevice const & device )
	{
		uint32_t size = uint32_t( m_points.size() );

		if ( size )
		{
			if ( !m_vertexBuffer
				|| size != m_vertexBuffer->getCount() )
			{
				m_vertexBuffer = makeVertexBuffer< InterleavedVertex >( device
					, size
					, 0u
					, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
					, getParent().getName() + "Submesh" + castor::string::toString( m_id ) + "VertexBuffer" );
			}

			if ( auto buffer = m_vertexBuffer->getBuffer().lock( 0u
				, ~( 0ull )
				, 0u ) )
			{
				std::memcpy( buffer, m_points.data(), m_points.size() * sizeof( InterleavedVertex ) );
				m_vertexBuffer->getBuffer().flush( 0u, ~( 0ull ) );
				m_vertexBuffer->getBuffer().unlock();
			}

			//m_points.clear();
		}
	}
}

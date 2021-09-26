#include "EnemySpawner.hpp"

#include "Game.hpp"

#include <Castor3D/Cache/GeometryCache.hpp>
#include <Castor3D/Cache/LightCache.hpp>
#include <Castor3D/Cache/ObjectCache.hpp>
#include <Castor3D/Model/Mesh/Mesh.hpp>
#include <Castor3D/Scene/Geometry.hpp>
#include <Castor3D/Scene/Scene.hpp>
#include <Castor3D/Scene/SceneNode.hpp>
#include <Castor3D/Scene/Light/Light.hpp>
#include <Castor3D/Scene/Light/PointLight.hpp>

using namespace castor;
using namespace castor3d;

namespace castortd
{
	EnemySpawner::EnemySpawner()
	{
		Reset();
	}

	void EnemySpawner::Reset()
	{
		m_category = BaseEnemy{};

		m_totalsWaves = 0u;
		m_totalSpawned = 0u;
		m_timeSinceLastSpawn = Milliseconds{};
		m_count = 0u;
	}

	void EnemySpawner::StartWave( uint32_t p_count )
	{
		m_category.Upgrade();
		m_count = p_count;
		++m_totalsWaves;
		m_timeBetweenTwoSpawns = Milliseconds( 1000 );
		m_timeSinceLastSpawn = m_timeBetweenTwoSpawns;
	}

	bool EnemySpawner::CanSpawn( Milliseconds const & p_elapsed )
	{
		m_timeSinceLastSpawn += p_elapsed;
		return m_count && m_timeSinceLastSpawn >= m_timeBetweenTwoSpawns;
	}

	EnemyPtr EnemySpawner::Spawn( Game const & p_game, Path const & p_path )
	{
		--m_count;
		m_timeSinceLastSpawn = Milliseconds{};
		++m_totalSpawned;
		auto & pathNode = *p_path.begin();
		auto & cell = p_game.getCell( Point2i{ pathNode.m_x, pathNode.m_y } );
		EnemyPtr result;

		if ( m_enemiesCache.empty() )
		{
			String name = cuT( "EnemyCube_" ) + std::to_string( m_totalSpawned );
			auto baseNode = p_game.getScene().getSceneNodeCache().add( name + cuT( "_Base" )
				, *p_game.getScene().getObjectRootNode()
				, p_game.getScene() ).lock();
			baseNode->setPosition( p_game.convert( Point2i{ cell.m_x, cell.m_y - 1 } ) + castor::Point3f{ 0, p_game.getCellHeight(), 0 } );
			baseNode->attachTo( *p_game.getMapNode() );
			auto node = p_game.getScene().getSceneNodeCache().add( name
				, *p_game.getScene().getObjectRootNode()
				, p_game.getScene() ).lock();
			node->setOrientation( Quaternion::fromAxisAngle( castor::Point3f{ 1, 0, 1 }, 45.0_degrees ) );
			node->attachTo( *baseNode );
			auto geometry = p_game.getScene().getGeometryCache().add( name
				, p_game.getScene()
				, *node
				, p_game.getEnemyMesh() ).lock();

			for ( auto submesh : *geometry->getMesh().lock() )
			{
				geometry->setMaterial( *submesh, p_game.getEnemyMaterial() );
			}

			auto light = p_game.getScene().getLightCache().add( name
				, p_game.getScene()
				, *node
				, p_game.getScene().getLightsFactory()
				, LightType::ePoint ).lock();
			light->setColour( RgbColour::fromPredefined( PredefinedRgbColour::eRed ) );
			light->setIntensity( 0.8f, 1.0f );
			light->getPointLight()->setAttenuation( Point3f{ 1.0f, 0.1f, 0.0f } );
			result = std::make_shared< Enemy >( *baseNode, p_game, p_path, m_category );
		}
		else
		{
			result = m_enemiesCache.front();
			m_enemiesCache.erase( m_enemiesCache.begin() );
			result->getNode().setPosition( p_game.convert( Point2i{ cell.m_x, cell.m_y - 1 } ) + castor::Point3f{ 0, p_game.getCellHeight(), 0 } );
			result->load( p_game );
		}

		return result;
	}

	void EnemySpawner::KillEnemy( Game & p_game, EnemyPtr && p_enemy )
	{
		p_enemy->getNode().setPosition( castor::Point3f{ 0, -1000, 0 } );
		m_enemiesCache.push_back( std::move( p_enemy ) );
	}
}

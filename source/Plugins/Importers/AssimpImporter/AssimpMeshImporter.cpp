#include "AssimpImporter/AssimpMeshImporter.hpp"

#include "AssimpImporter/AssimpMaterialImporter.hpp"

#include <Castor3D/Engine.hpp>
#include <Castor3D/Miscellaneous/Logger.hpp>
#include <Castor3D/Model/Mesh/Mesh.hpp>
#include <Castor3D/Model/Mesh/Submesh/Submesh.hpp>
#include <Castor3D/Model/Mesh/Submesh/Component/BaseDataComponent.hpp>
#include <Castor3D/Model/Mesh/Submesh/Component/BonesComponent.hpp>
#include <Castor3D/Model/Mesh/Submesh/Component/MorphComponent.hpp>
#include <Castor3D/Model/Skeleton/BoneNode.hpp>
#include <Castor3D/Model/Skeleton/Skeleton.hpp>
#include <Castor3D/Scene/Scene.hpp>

namespace c3d_assimp
{
	namespace meshes
	{
		static castor::Matrix4x4f getTranslation( aiMatrix4x4 const & transform )
		{
			aiQuaternion quat;
			aiVector3D tran;
			transform.DecomposeNoScaling( quat, tran );

			castor::Matrix4x4f result;
			castor::matrix::setTranslate( result, fromAssimp( tran ) );
			return result;
		}

		static void applyMorphTarget( float weight
			, castor3d::SubmeshAnimationBuffer const & target
			, castor::Point3fArray & positions
			, castor::Point3fArray & normals
			, castor::Point3fArray & tangents
			, castor::Point3fArray & texcoords0
			, castor::Point3fArray & texcoords1
			, castor::Point3fArray & texcoords2
			, castor::Point3fArray & texcoords3
			, castor::Point3fArray & colours )
		{
			if ( weight != 0.0f )
			{
				if ( !positions.empty() )
				{
					auto posIt = positions.begin();
					auto bufferIt = target.positions.begin();

					while ( bufferIt != target.positions.end() )
					{
						auto & buf = *bufferIt;
						*posIt += buf * weight;
						++posIt;
						++bufferIt;
					}
				}

				if ( !normals.empty() )
				{
					auto nmlIt = normals.begin();
					auto bufferIt = target.normals.begin();

					while ( bufferIt != target.normals.end() )
					{
						auto & buf = *bufferIt;
						*nmlIt += buf * weight;
						++nmlIt;
						++bufferIt;
					}
				}

				if ( !tangents.empty() )
				{
					auto tanIt = tangents.begin();
					auto bufferIt = target.tangents.begin();

					while ( bufferIt != target.tangents.end() )
					{
						auto & buf = *bufferIt;
						*tanIt += buf * weight;
						++tanIt;
						++bufferIt;
					}
				}

				if ( !texcoords0.empty() )
				{
					auto texIt = texcoords0.begin();
					auto bufferIt = target.texcoords0.begin();

					while ( bufferIt != target.texcoords0.end() )
					{
						auto & buf = *bufferIt;
						*texIt += buf * weight;
						++texIt;
						++bufferIt;
					}
				}

				if ( !texcoords1.empty() )
				{
					auto texIt = texcoords1.begin();
					auto bufferIt = target.texcoords1.begin();

					while ( bufferIt != target.texcoords1.end() )
					{
						auto & buf = *bufferIt;
						*texIt += buf * weight;
						++texIt;
						++bufferIt;
					}
				}

				if ( !texcoords2.empty() )
				{
					auto texIt = texcoords2.begin();
					auto bufferIt = target.texcoords2.begin();

					while ( bufferIt != target.texcoords2.end() )
					{
						auto & buf = *bufferIt;
						*texIt += buf * weight;
						++texIt;
						++bufferIt;
					}
				}

				if ( !texcoords3.empty() )
				{
					auto texIt = texcoords3.begin();
					auto bufferIt = target.texcoords3.begin();

					while ( bufferIt != target.texcoords3.end() )
					{
						auto & buf = *bufferIt;
						*texIt += buf * weight;
						++texIt;
						++bufferIt;
					}
				}

				if ( !colours.empty() )
				{
					auto texIt = colours.begin();
					auto bufferIt = target.colours.begin();

					while ( bufferIt != target.colours.end() )
					{
						auto & buf = *bufferIt;
						*texIt += buf * weight;
						++texIt;
						++bufferIt;
					}
				}
			}
		}

		castor3d::MorphFlags computeMorphFlags( castor3d::SubmeshAnimationBuffer const & buffer )
		{
			castor3d::MorphFlags result{};

			if ( !buffer.positions.empty() )
			{
				result |= castor3d::MorphFlag::ePositions;
			}

			if ( !buffer.normals.empty() )
			{
				result |= castor3d::MorphFlag::eNormals;
			}

			if ( !buffer.tangents.empty() )
			{
				result |= castor3d::MorphFlag::eTangents;
			}

			if ( !buffer.texcoords0.empty() )
			{
				result |= castor3d::MorphFlag::eTexcoords0;
			}

			if ( !buffer.texcoords1.empty() )
			{
				result |= castor3d::MorphFlag::eTexcoords1;
			}

			if ( !buffer.texcoords2.empty() )
			{
				result |= castor3d::MorphFlag::eTexcoords2;
			}

			if ( !buffer.texcoords3.empty() )
			{
				result |= castor3d::MorphFlag::eTexcoords3;
			}

			if ( !buffer.colours.empty() )
			{
				result |= castor3d::MorphFlag::eColours;
			}

			return result;
		}
	}

	AssimpMeshImporter::AssimpMeshImporter( castor3d::Engine & engine )
		: castor3d::MeshImporter{ engine }
	{
	}

	bool AssimpMeshImporter::doImportMesh( castor3d::Mesh & mesh )
	{
		auto & file = static_cast< AssimpImporterFile const & >( *m_file );

		if ( file.getListedMeshes().empty() )
		{
			doImportSingleMesh( mesh );
			return true;
		}

		return doImportSceneMesh( mesh );
	}

	void AssimpMeshImporter::doImportSingleMesh( castor3d::Mesh & mesh )
	{
		auto & file = static_cast< AssimpImporterFile & >( *m_file );
		auto & aiScene = file.getScene();
		auto & scene = *mesh.getScene();
		uint32_t meshIndex{};

		for ( auto aiMesh : castor::makeArrayView( aiScene.mMeshes, aiScene.mNumMeshes ) )
		{
			if ( isValidMesh( *aiMesh ) )
			{
				auto matName = file.getMaterialName( aiMesh->mMaterialIndex );
				auto materialRes = scene.tryFindMaterial( matName );
				castor3d::MaterialRPtr material{};

				if ( !materialRes.lock() )
				{
					AssimpMaterialImporter importer{ *scene.getEngine() };
					auto mat = getOwner()->createMaterial( matName
						, *getOwner()
						, getOwner()->getPassesType() );

					if ( importer.import( *mat
						, &file
						, castor3d::Parameters{}
						, std::map< castor3d::TextureFlag, castor3d::TextureConfiguration >{} ) )
					{
						material = mat.get();
						scene.getMaterialView().add( matName, mat, true );
					}
					else
					{
						material = scene.getEngine()->getDefaultMaterial();
					}
				}
				else
				{
					material = &( *materialRes.lock() );
				}

				doProcessMesh( aiScene
					, *aiMesh
					, meshIndex
					, mesh
					, *mesh.createSubmesh() );
			}

			++meshIndex;
		}

		doTransformMesh( *aiScene.mRootNode
			, mesh );
	}

	bool AssimpMeshImporter::doImportSceneMesh( castor3d::Mesh & mesh )
	{
		auto & file = static_cast< AssimpImporterFile const & >( *m_file );
		auto name = mesh.getName();
		auto it = file.getMeshes().find( name );

		if ( it == file.getMeshes().end() )
		{
			return false;
		}

		auto & aiScene = file.getScene();

		for ( auto submesh : it->second.submeshes )
		{
			doProcessMesh( aiScene
				, *submesh.mesh
				, submesh.meshIndex
				, mesh
				, *mesh.createSubmesh() );
		}

		return true;
	}

	void AssimpMeshImporter::doProcessMesh( aiScene const & aiScene
		, aiMesh const & aiMesh
		, uint32_t aiMeshIndex
		, castor3d::Mesh & mesh
		, castor3d::Submesh & submesh )
	{
		auto & file = static_cast< AssimpImporterFile & >( *m_file );
		auto & scene = *mesh.getScene();
		auto materialRes = scene.tryFindMaterial( file.getMaterialName( aiMesh.mMaterialIndex ) );
		castor3d::MaterialRPtr material{};

		if ( !materialRes.lock() )
		{
			material = scene.getEngine()->getDefaultMaterial();
		}
		else
		{
			material = &( *materialRes.lock() );
		}

		castor3d::log::info << cuT( "  Mesh found: [" ) << file.getInternalName( aiMesh.mName ) << cuT( "]" ) << std::endl;
		submesh.setDefaultMaterial( material );

		auto positions = submesh.createComponent< castor3d::PositionsComponent >();
		auto normals = submesh.createComponent< castor3d::NormalsComponent >();
		castor::Point3fArray tan;
		castor::Point3fArray tex0;
		castor::Point3fArray tex1;
		castor::Point3fArray tex2;
		castor::Point3fArray tex3;
		castor::Point3fArray col;
		castor::Point3fArray * tangents = &tan;
		castor::Point3fArray * texcoords0 = &tex0;
		castor::Point3fArray * texcoords1 = &tex1;
		castor::Point3fArray * texcoords2 = &tex2;
		castor::Point3fArray * texcoords3 = &tex3;
		castor::Point3fArray * colours = &col;

		if ( aiMesh.HasTextureCoords( 0u ) )
		{
			auto tanComp = submesh.createComponent< castor3d::TangentsComponent >();
			auto texComp = submesh.createComponent< castor3d::Texcoords0Component >();
			tangents = &tanComp->getData();
			texcoords0 = &texComp->getData();
		}

		if ( aiMesh.HasTextureCoords( 1u ) )
		{
			auto texComp = submesh.createComponent< castor3d::Texcoords1Component >();
			texcoords1 = &texComp->getData();
		}

		if ( aiMesh.HasTextureCoords( 2u ) )
		{
			auto texComp = submesh.createComponent< castor3d::Texcoords2Component >();
			texcoords2 = &texComp->getData();
		}

		if ( aiMesh.HasTextureCoords( 3u ) )
		{
			auto texComp = submesh.createComponent< castor3d::Texcoords3Component >();
			texcoords3 = &texComp->getData();
		}

		if ( aiMesh.HasVertexColors( 0u ) )
		{
			auto colComp = submesh.createComponent< castor3d::ColoursComponent >();
			colours = &colComp->getData();
		}

		createVertexBuffer( aiMesh
			, positions->getData()
			, normals->getData()
			, *tangents
			, *texcoords0
			, *texcoords1
			, *texcoords2
			, *texcoords3
			, *colours );
		auto & animBuffers = file.addMeshAnimBuffers( &aiMesh
			, gatherMeshAnimBuffers( positions->getData()
				, normals->getData()
				, *tangents
				, *texcoords0
				, *texcoords1
				, *texcoords2
				, *texcoords3
				, *colours
				, castor::makeArrayView( aiMesh.mAnimMeshes, aiMesh.mNumAnimMeshes ) ) );

		if ( !animBuffers.empty() )
		{
			castor3d::log::debug << cuT( "    Morph targets found: [" ) << uint32_t( animBuffers.size() ) << cuT( "]" ) << std::endl;
			auto component = submesh.hasComponent( castor3d::MorphComponent::Name )
				? submesh.getComponent< castor3d::MorphComponent >()
				: submesh.createComponent< castor3d::MorphComponent >( meshes::computeMorphFlags( animBuffers.front() ) );

			for ( auto & animBuffer : animBuffers )
			{
				component->addMorphTarget( animBuffer );
			}
		}

		if ( aiMesh.HasBones() )
		{
			std::vector< castor3d::VertexBoneData > bonesData( aiMesh.mNumVertices );
			auto meshNode = findMeshNode( aiMeshIndex, *aiScene.mRootNode );
			auto rootNode = findRootSkeletonNode( *aiScene.mRootNode
				, castor::makeArrayView( aiMesh.mBones, aiMesh.mNumBones )
				, meshNode );
			auto skelName = normalizeName( file.getInternalName( rootNode->mName ) );
			auto skelIt = std::find_if( scene.getSkeletonCache().begin()
				, scene.getSkeletonCache().end()
				, [&skelName]( auto const & lookup )
				{
					return lookup.second->getRootNode()->getName() == skelName;
				} );

			if ( skelIt != scene.getSkeletonCache().end() )
			{
				auto & skeleton = *skelIt->second;

				for ( auto aiBone : castor::makeArrayView( aiMesh.mBones, aiMesh.mNumBones ) )
				{
					castor::String boneName = file.getInternalName( aiBone->mName );
					auto node = skeleton.findNode( boneName );
					CU_Require( node && node->getType() == castor3d::SkeletonNodeType::eBone );
					auto bone = &static_cast< castor3d::BoneNode & >( *node );

					for ( auto weight : castor::makeArrayView( aiBone->mWeights, aiBone->mNumWeights ) )
					{
						bonesData[weight.mVertexId].addBoneData( bone->getId(), weight.mWeight );
					}
				}

				auto bones = submesh.createComponent< castor3d::BonesComponent >();
				bones->addBoneDatas( bonesData );
			}
		}

		auto mapping = submesh.createComponent< castor3d::TriFaceMapping >();
		auto faces = castor::makeArrayView( aiMesh.mFaces, aiMesh.mNumFaces );

		for ( auto face : faces )
		{
			if ( face.mNumIndices == 3 )
			{
				mapping->addFace( face.mIndices[0], face.mIndices[2], face.mIndices[1] );
			}
			else if ( face.mNumIndices == 4 )
			{
				mapping->addQuadFace( face.mIndices[0], face.mIndices[2], face.mIndices[1], face.mIndices[2] );
			}
		}

		if ( !aiMesh.HasNormals() )
		{
			mapping->computeNormals( true );
		}
		else if ( !aiMesh.HasTangentsAndBitangents() )
		{
			mapping->computeTangentsFromNormals();
		}
	}

	void AssimpMeshImporter::doTransformMesh( aiNode const & aiNode
		, castor3d::Mesh & mesh
		, aiMatrix4x4 transformAcc )
	{
		transformAcc = transformAcc * aiNode.mTransformation;

		for ( auto aiMeshIndex : castor::makeArrayView( aiNode.mMeshes, aiNode.mNumMeshes ) )
		{
			if ( aiMeshIndex < mesh.getSubmeshCount() )
			{
				auto submesh = mesh.getSubmesh( aiMeshIndex );
				auto transform = ( submesh->hasComponent( castor3d::BonesComponent::Name )
					? meshes::getTranslation( transformAcc )
					: fromAssimp( transformAcc ) );

				for ( auto & vertex : submesh->getPositions() )
				{
					vertex = transform * vertex;
				}

				auto indexMapping = submesh->getIndexMapping();
				indexMapping->computeNormals( true );
			}
		}

		for ( auto child : castor::makeArrayView( aiNode.mChildren, aiNode.mNumChildren ) )
		{
			doTransformMesh( *child, mesh, transformAcc );
		}
	}
}

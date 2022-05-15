/*
See LICENSE file in root folder
*/
#ifndef ___C3DAssimp_SkeletonImporter___
#define ___C3DAssimp_SkeletonImporter___

#include <Castor3D/Castor3DPrerequisites.hpp>
#include <Castor3D/Model/Mesh/Importer.hpp>

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 4619 )
#include <assimp/scene.h> // Output data structure
#pragma warning( pop )

namespace c3d_assimp
{
	class AssimpImporter;
	using SkeletonAnimationKeyFrameMap = std::map< castor::Milliseconds, castor3d::SkeletonAnimationKeyFrameUPtr >;
	using SkeletonAnimationObjectSet = std::set< castor3d::SkeletonAnimationObjectSPtr >;

	struct BoneData
	{
		castor::Matrix4x4f inverseTransform;
	};

	class SkeletonImporter
	{
	public:
		explicit SkeletonImporter( AssimpImporter & importer );

		void import( castor::Path const & fileName
			, aiScene const & aiScene
			, castor3d::Scene & scene
			, castor3d::SkeletonRPtr skeleton
			, castor3d::MeshImporter::Mode mode );
		bool isBoneNode( aiNode const & aiNode )const;
		castor3d::SkeletonRPtr getSkeleton( aiMesh const & aiMesh
			, uint32_t aiMeshIndex )const;

		bool needsAnimsReparse()const
		{
			return m_needsAnimsReparse;
		}

	private:
		void doProcessSkeletons( aiScene const & aiScene
			, castor3d::Scene & scene
			, castor3d::SkeletonRPtr skeleton
			, castor::ArrayView< aiMesh * > aiMeshes
			, castor3d::MeshImporter::Mode mode );
		void doProcessSkeletonAnimation( castor::String const & name
			, castor3d::Skeleton & skeleton
			, aiNode const & aiNode
			, aiAnimation const & aiAnimation );
		void doProcessSkeletonAnimationNodes( castor3d::SkeletonAnimation & animation
			, castor::Milliseconds maxTime
			, int64_t ticksPerSecond
			, castor3d::Skeleton & skeleton
			, aiNode const & aiNode
			, aiAnimation const & aiAnimation
			, SkeletonAnimationKeyFrameMap & keyFrames
			, SkeletonAnimationObjectSet & notAnimated );
		void doProcessAnimationNodeKeys( aiNodeAnim const & aiNodeAnim
			, int64_t ticksPerSecond
			, castor3d::SkeletonAnimationObject & object
			, castor3d::SkeletonAnimation & animation
			, SkeletonAnimationKeyFrameMap & keyframes );

	private:
		AssimpImporter & m_importer;
		castor::Path m_fileName;
		bool m_needsAnimsReparse{};
		bool m_replaceInverseTransforms{};
		std::map< uint32_t, aiNode * > m_meshesNodes;
		std::map< castor::String, BoneData > m_bonesNodes;
		std::map< castor::String, castor3d::SkeletonRPtr > m_skeletons;
		std::map< castor::String, castor3d::SkeletonRPtr > m_meshSkeletons;
		std::map< castor::String, castor3d::ObjectTransform > m_nodeTransforms;
	};
}

#endif

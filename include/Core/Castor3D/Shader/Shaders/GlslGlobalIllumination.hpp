/*
See LICENSE file in root folder
*/
#ifndef ___C3D_GlslGlobalIllumination_H___
#define ___C3D_GlslGlobalIllumination_H___

#include "SdwModule.hpp"

#include "Castor3D/Scene/SceneModule.hpp"
#include "Castor3D/Shader/Ubos/LayeredLpvGridConfigUbo.hpp"
#include "Castor3D/Shader/Ubos/LpvGridConfigUbo.hpp"

#include <ShaderWriter/Intrinsics/Intrinsics.hpp>

namespace castor3d
{
	namespace shader
	{
		class GlobalIllumination
		{
		public:
			C3D_API explicit GlobalIllumination( sdw::ShaderWriter & writer
				, Utils & utils
				, bool deferred = false );
			// Forward only
			C3D_API void declare( uint32_t & bindingIndex
				, uint32_t setIndex
				, SceneFlags sceneFlags );
			// Deferred only
			C3D_API void declare( uint32_t vctUboBindingIndex
				, uint32_t lpvUboBindingIndex
				, uint32_t llpvUboBindingIndex
				, uint32_t & texBindingIndex
				, uint32_t texSetIndex
				, SceneFlags sceneFlags );
			C3D_API void declareTraceConeRadiance();
			C3D_API void declareVct( uint32_t & uboBindingIndex
				, uint32_t & texBindingIndex
				, uint32_t uboSetIndex = 0u
				, uint32_t texSetIndex = 0u );
			C3D_API void declareLpv( uint32_t & uboBindingIndex
				, uint32_t & texBindingIndex
				, uint32_t uboSetIndex = 0u
				, uint32_t texSetIndex = 0u
				, bool declUbo = true );
			C3D_API void declareLayeredLpv( uint32_t & uboBindingIndex
				, uint32_t & texBindingIndex
				, uint32_t uboSetIndex = 0u
				, uint32_t texSetIndex = 0u
				, bool declUbo = true );
			C3D_API sdw::Vec3 computeLPVRadiance( Surface surface
				, LpvGridData lpvGridData );
			C3D_API sdw::Vec3 computeLLPVRadiance( Surface surface
				, LayeredLpvGridData llpvGridData );

			C3D_API sdw::Float computeOcclusion( SceneFlags sceneFlags
				, LightType lightType
				, Surface surface );
			C3D_API sdw::Vec3 computeDiffuse( SceneFlags sceneFlags
				, Surface surface
				, sdw::Float indirectOcclusion );
			C3D_API sdw::Vec3 computeSpecular( SceneFlags sceneFlags
				, sdw::Vec3 wsCamera
				, Surface surface
				, sdw::Float roughness
				, sdw::Float indirectOcclusion );
			sdw::Vec4 traceConeRadiance( sdw::SampledImage3DRgba32 const & voxels
				, Surface surface
				, VoxelData const & voxelData )const;

		private:
			void declareTraceCone();
			void declareTraceConeReflection();
			void declareTraceConeOcclusion();
			sdw::Vec4 traceConeReflection( sdw::SampledImage3DRgba32 const & voxels
				, Surface surface
				, sdw::Vec3 const & wsViewVector
				, sdw::Float const & roughness
				, VoxelData const & voxelData )const;
			sdw::Float traceConeOcclusion( sdw::SampledImage3DRgba32 const & voxels
				, Surface surface
				, sdw::Vec3 const & wsConeDirection
				, VoxelData const & voxelData )const;

		private:
			sdw::ShaderWriter & m_writer;
			Utils & m_utils;
			bool m_deferred;
			sdw::Function< sdw::Vec4
				, sdw::InVec3 > m_evalSH;
			sdw::Function< sdw::Vec3
				, InSurface
				, InLpvGridData > m_computeLPVRadiance;
			sdw::Function< sdw::Vec3
				, InSurface
				, InLayeredLpvGridData > m_computeLLPVRadiance;
			sdw::Function< sdw::Vec4
				, sdw::InSampledImage3DRgba32
				, InSurface
				, sdw::InVec3
				, sdw::InFloat
				, InVoxelData > m_traceCone;
			sdw::Function< sdw::Vec4
				, sdw::InSampledImage3DRgba32
				, InSurface
				, InVoxelData > m_traceConeRadiance;
			sdw::Function< sdw::Vec4
				, sdw::InSampledImage3DRgba32
				, InSurface
				, sdw::InVec3
				, sdw::InFloat
				, InVoxelData > m_traceConeReflection;
			sdw::Function< sdw::Float
				, sdw::InSampledImage3DRgba32
				, InSurface
				, sdw::InVec3
				, InVoxelData > m_traceConeOcclusion;
		};

	}
}

#endif

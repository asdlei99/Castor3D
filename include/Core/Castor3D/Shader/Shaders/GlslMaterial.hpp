﻿/*
See LICENSE file in root folder
*/
#ifndef ___C3D_GlslMaterials_H___
#define ___C3D_GlslMaterials_H___

#include "SdwModule.hpp"

#include <ShaderWriter/MatTypes/Mat4.hpp>
#include <ShaderWriter/CompositeTypes/StructInstance.hpp>

namespace castor3d::shader
{
	castor::String const PassBufferName = cuT( "Materials" );

	struct Material
		: public sdw::StructInstance
	{
		friend class Materials;
		C3D_API Material( sdw::ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled );
		SDW_DeclStructInstance( C3D_API, Material );

		C3D_API sdw::Vec3 colour()const;
		/**
		 *\~english
		 *\brief		Writes the alpha function in GLSL.
		 *\param		alphaFunc	The alpha function.
		 *\param[in]	opacity		The alpha TypeEnum.
		 *\param[in]	passMasks	The subpasses masks.
		 *\param[in]	opaque		\p true for opaque nodes, \p false for transparent ones.
		 *\~french
		 *\brief		Ecrit la fonction d'opacité en GLSL.
		 *\param		alphaFunc	La fonction d'opacité.
		 *\param[in]	opacity		La valeur d'opacité.
		 *\param[in]	passMasks	Les masques de subpasses.
		 *\param[in]	opaque		\p true pour les noeuds opaques, \p false pour les transparents.
		 */
		C3D_API void applyAlphaFunc( VkCompareOp alphaFunc
			, sdw::Float & opacity
			, sdw::Float const & passMultiplier
			, bool opaque = true );
		/**
		 *\~english
		 *\brief		Writes the alpha function in GLSL.
		 *\param		alphaFunc	The alpha function.
		 *\param[in]	opacity		The opacity value.
		 *\param[in]	alphaRef	The alpha comparison reference value.
		 *\param[in]	passMasks	The subpasses masks.
		 *\param[in]	opaque		\p true for opaque nodes, \p false for transparent ones.
		 *\~french
		 *\brief		Ecrit la fonction d'opacité en GLSL.
		 *\param		alphaFunc	La fonction d'opacité.
		 *\param[in]	opacity		La valeur d'opacité.
		 *\param[in]	alphaRef	La valeur de référence pour la comparaison alpha.
		 *\param[in]	passMasks	Les masques de subpasses.
		 *\param[in]	opaque		\p true pour les noeuds opaques, \p false pour les transparents.
		 */
		C3D_API void applyAlphaFunc( VkCompareOp alphaFunc
			, sdw::Float & opacity
			, sdw::Float const & alphaRef
			, sdw::Float const & passMultiplier
			, bool opaque = true );
		/**
		 *\~english
		 *\brief		Writes the alpha function in GLSL.
		 *\param		alphaFunc	The alpha function.
		 *\param[in]	opacity		The opacity value.
		 *\param[in]	alphaRef	The alpha comparison reference value.
		 *\param[in]	passMasks	The subpasses masks.
		 *\param[in]	opaque		\p true for opaque nodes, \p false for transparent ones.
		 *\~french
		 *\brief		Ecrit la fonction d'opacité en GLSL.
		 *\param		alphaFunc	La fonction d'opacité.
		 *\param[in]	opacity		La valeur d'opacité.
		 *\param[in]	alphaRef	La valeur de référence pour la comparaison alpha.
		 *\param[in]	passMasks	Les masques de subpasses.
		 *\param[in]	opaque		\p true pour les noeuds opaques, \p false pour les transparents.
		 */
		C3D_API static void applyAlphaFunc( sdw::ShaderWriter & writer
			, VkCompareOp alphaFunc
			, sdw::Float & opacity
			, sdw::Float const & alphaRef
			, sdw::Float const & passMultiplier
			, bool opaque = true );
		/**
		 *\~english
		 *\brief		Writes the alpha function in GLSL.
		 *\param		alphaFunc	The alpha function.
		 *\param[in]	opacity		The alpha TypeEnum.
		 *\param[in]	passMasks	The subpasses masks.
		 *\param[in]	opaque		\p true for opaque nodes, \p false for transparent ones.
		 *\~french
		 *\brief		Ecrit la fonction d'opacité en GLSL.
		 *\param		alphaFunc	La fonction d'opacité.
		 *\param[in]	opacity		La valeur d'opacité.
		 *\param[in]	passMasks	Les masques de subpasses.
		 *\param[in]	opaque		\p true pour les noeuds opaques, \p false pour les transparents.
		 */
		C3D_API void getPassMultipliers( SubmeshFlags submeshFlags
			, sdw::UInt const & passCount
			, sdw::UVec4 const & passMasks
			, sdw::Array< sdw::Vec4 > & passMultipliers )const;
		C3D_API sdw::UInt getTexture( uint32_t index )const;

		C3D_API static ast::type::BaseStructPtr makeType( ast::type::TypesCache & cache );

	protected:
		using sdw::StructInstance::getMember;
		using sdw::StructInstance::getMemberArray;

	public:
		sdw::Vec4 colourDiv;
		sdw::Vec4 specDiv;
		sdw::Float edgeWidth;
		sdw::Float depthFactor;
		sdw::Float normalFactor;
		sdw::Float objectFactor;
		sdw::Vec4 edgeColour;
		sdw::Vec4 specific;
		sdw::UInt index;
		sdw::Float emissive;
		sdw::Float alphaRef;
		sdw::UInt sssProfileIndex;
		sdw::Vec3 transmission;
		sdw::Float opacity;
		sdw::Float refractionRatio;
		sdw::Int hasRefraction;
		sdw::Int hasReflection;
		sdw::Float bwAccumulationOperator;
		sdw::UVec4 textures0;
		sdw::UVec4 textures1;
		sdw::Int textures;
	};

	struct OpacityBlendComponents
	{
		sdw::Vec3 texCoord0;
		sdw::Float opacity;
	};

	struct GeometryBlendComponents
	{
		sdw::Vec3 texCoord0;
		sdw::Vec3 texCoord1;
		sdw::Vec3 texCoord2;
		sdw::Vec3 texCoord3;
		sdw::Float opacity;
		sdw::Vec3 normal;
		sdw::Vec3 tangent;
		sdw::Vec3 bitangent;
		sdw::Vec3 tangentSpaceViewPosition;
		sdw::Vec3 tangentSpaceFragPosition;
	};

	struct OpaqueBlendComponents
	{
		sdw::Vec3 texCoord0;
		sdw::Vec3 texCoord1;
		sdw::Vec3 texCoord2;
		sdw::Vec3 texCoord3;
		sdw::Float opacity;
		sdw::Float occlusion;
		sdw::Float transmittance;
		sdw::Vec3 emissive;
		sdw::Vec3 normal;
		sdw::Vec3 tangent;
		sdw::Vec3 bitangent;
		sdw::Vec3 tangentSpaceViewPosition;
		sdw::Vec3 tangentSpaceFragPosition;
	};

	struct LightingBlendComponents
	{
		sdw::Vec3 texCoord0;
		sdw::Vec3 texCoord1;
		sdw::Vec3 texCoord2;
		sdw::Vec3 texCoord3;
		sdw::Float opacity;
		sdw::Float occlusion;
		sdw::Float transmittance;
		sdw::Vec3 transmission;
		sdw::Vec3 emissive;
		sdw::Float refractionRatio;
		sdw::Int hasRefraction;
		sdw::Int hasReflection;
		sdw::Float bwAccumulationOperator;
		sdw::Vec3 normal;
		sdw::Vec3 tangent;
		sdw::Vec3 bitangent;
		sdw::Vec3 tangentSpaceViewPosition;
		sdw::Vec3 tangentSpaceFragPosition;
	};

	CU_DeclareSmartPtr( Material );

	class Materials
	{
	public:
		C3D_API explicit Materials( sdw::ShaderWriter & writer );
		C3D_API explicit Materials( sdw::ShaderWriter & writer
			, uint32_t binding
			, uint32_t set
			, bool enable = true );
		C3D_API void declare( uint32_t binding
			, uint32_t set );
		C3D_API Material getMaterial( sdw::UInt const & index )const;

		// Use by picking pass (opacity only)
		C3D_API void blendMaterials( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, SubmeshFlags const & submeshFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::UInt const & passCount
			, sdw::Array< sdw::Vec4 > const & passMultipliers
			, OpacityBlendComponents & output )const;
		// Use by depth pass (opacity and tangent space only)
		C3D_API void blendMaterials( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, SubmeshFlags const & submeshFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::UInt const & passCount
			, sdw::Array< sdw::Vec4 > const & passMultipliers
			, GeometryBlendComponents & output )const;
		// Use by shadow passes
		C3D_API std::unique_ptr< LightMaterial > blendMaterials( Utils & utils
			, bool needsRsm
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, SubmeshFlags const & submeshFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::UInt const & passCount
			, sdw::Array< sdw::Vec4 > const & passMultipliers
			, sdw::Vec3 const & vertexColour
			, OpaqueBlendComponents & output )const;
		// Use by opaque pass
		C3D_API std::unique_ptr< LightMaterial > blendMaterials( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, SubmeshFlags const & submeshFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::UInt const & passCount
			, sdw::Array< sdw::Vec4 > const & passMultipliers
			, sdw::Vec3 const & vertexColour
			, OpaqueBlendComponents & output )const;
		// Use by forward passes
		C3D_API std::unique_ptr< LightMaterial > blendMaterials( Utils & utils
			, bool opaque
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, SubmeshFlags const & submeshFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::UInt const & passCount
			, sdw::Array< sdw::Vec4 > const & passMultipliers
			, sdw::Vec3 const & vertexColour
			, LightingBlendComponents & output )const;

	private:
		Material applyMaterial( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, OpacityBlendComponents & output )const;
		Material applyMaterial( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, GeometryBlendComponents & output )const;
		std::pair< Material, std::unique_ptr< LightMaterial > > applyMaterial( Utils & utils
			, bool needsRsm
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::Vec3 const & vertexColour
			, OpaqueBlendComponents & output )const;
		std::pair< Material, std::unique_ptr< LightMaterial > > applyMaterial( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::Vec3 const & vertexColour
			, OpaqueBlendComponents & output )const;
		std::pair< Material, std::unique_ptr< LightMaterial > > applyMaterial( Utils & utils
			, VkCompareOp alphaFunc
			, PassFlags const & passFlags
			, TextureFlagsArray const & textures
			, bool hasTextures
			, shader::TextureConfigurations const & textureConfigs
			, shader::TextureAnimations const & textureAnims
			, shader::LightingModel & lightingModel
			, sdw::Array< sdw::CombinedImage2DRgba32 > const & maps
			, sdw::UInt const & materialId
			, sdw::Vec3 const & vertexColour
			, LightingBlendComponents & output )const;

	private:
		sdw::ShaderWriter & m_writer;
		std::unique_ptr< sdw::ArraySsboT< Material > > m_ssbo;
	};

	struct SssProfile
		: public sdw::StructInstance
	{
		friend class SssProfiles;
		C3D_API SssProfile( sdw::ShaderWriter & writer
			, ast::expr::ExprPtr expr
			, bool enabled );
		SDW_DeclStructInstance( C3D_API, SssProfile );

		C3D_API static ast::type::BaseStructPtr makeType( ast::type::TypesCache & cache );

	protected:
		using sdw::StructInstance::getMember;
		using sdw::StructInstance::getMemberArray;

	public:
		sdw::Int transmittanceProfileSize;
		sdw::Float gaussianWidth;
		sdw::Float subsurfaceScatteringStrength;
		sdw::Float pad;
		sdw::Array< sdw::Vec4 > transmittanceProfile;
	};

	CU_DeclareSmartPtr( SssProfile );

	class SssProfiles
	{
	public:
		C3D_API explicit SssProfiles( sdw::ShaderWriter & writer );
		C3D_API explicit SssProfiles( sdw::ShaderWriter & writer
			, uint32_t binding
			, uint32_t set
			, bool enable = true );
		C3D_API void declare( uint32_t binding
			, uint32_t set );
		C3D_API SssProfile getProfile( sdw::UInt const & index )const;

	protected:
		sdw::ShaderWriter & m_writer;
		std::unique_ptr< sdw::ArraySsboT< SssProfile > > m_ssbo;
	};
}

#endif

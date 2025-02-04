/*
See LICENSE file in root folder
*/
#ifndef ___C3D_RenderQuad_H___
#define ___C3D_RenderQuad_H___

#include "PassesModule.hpp"
#include "Castor3D/Buffer/UniformBufferOffset.hpp"
#include "Castor3D/Material/Texture/Sampler.hpp"
#include "Castor3D/Render/Passes/CommandsSemaphore.hpp"

#include <CastorUtils/Design/Named.hpp>
#include <CastorUtils/Design/Resource.hpp>

#include <ashespp/Buffer/VertexBuffer.hpp>
#include <ashespp/Command/CommandBuffer.hpp>
#include <ashespp/Descriptor/DescriptorSet.hpp>
#include <ashespp/Descriptor/DescriptorSetLayout.hpp>
#include <ashespp/Descriptor/DescriptorSetPool.hpp>
#include <ashespp/Pipeline/GraphicsPipeline.hpp>
#include <ashespp/Pipeline/PipelineLayout.hpp>
#include <ashespp/Pipeline/PipelineVertexInputStateCreateInfo.hpp>
#include <ashespp/Pipeline/PipelineViewportStateCreateInfo.hpp>

#include <type_traits>

namespace castor3d
{
	namespace rq
	{
		/**
		*\~english
		*\brief
		*	Tells how the texture coordinates from the vertex buffer are built.
		*\~french
		*\brief
		*	Définit comment sont construites les coordonnées de texture du vertex buffer.
		*/
		struct Texcoord
		{
			/*
			*\~english
			*\brief
			*	Tells if the U coordinate of UV must be inverted, thus mirroring vertically the resulting image.
			*\~french
			*	Dit si la coordonnée U de l'UV doit être inversée, rendant ainsi un miroir vertical de l'image.
			*/
			bool invertU{ false };
			/*
			*\~english
			*\brief
			*	Tells if the U coordinate of UV must be inverted, thus mirroring horizontally the resulting image.
			*\~french
			*	Dit si la coordonnée V de l'UV doit être inversée, rendant ainsi un miroir horizontal de l'image.
			*/
			bool invertV{ false };
		};

		struct BindingDescription
		{
			BindingDescription( VkDescriptorType descriptorType
				, ashes::Optional< VkImageViewType > viewType
				, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT )
				: descriptorType{ descriptorType }
				, viewType{ viewType }
				, stageFlags{ stageFlags }
			{
			}

			VkDescriptorType descriptorType;
			ashes::Optional< VkImageViewType > viewType;
			VkShaderStageFlags stageFlags;
		};
		using BindingDescriptionArray = std::vector< BindingDescription >;

		template< template< typename ValueT > typename WrapperT >
		struct ConfigT
		{
			WrapperT< BindingDescriptionArray > bindings;
			WrapperT< VkImageSubresourceRange > range;
			WrapperT< Texcoord > texcoordConfig;
			WrapperT< BlendMode > blendMode;
			WrapperT< IntermediateView > tex3DResult;
		};

		template< typename TypeT >
		struct RawTyperT
		{
			using Type = TypeT;
		};

		template< typename TypeT >
		using RawTypeT = typename RawTyperT< TypeT >::Type;

		using Config = ConfigT< ashes::Optional >;
		using ConfigData = ConfigT< RawTypeT >;
	}

	class RenderQuad
		: public castor::Named
	{
		template< typename ConfigT, typename BuilderT >
		friend class RenderQuadBuilderT;

	protected:
		/**
		*\~english
		*\brief
		*	Constructor.
		*\param[in] device
		*	The RenderDevice.
		*\param[in] name
		*	The pass name.
		*\param[in] samplerFilter
		*	The sampler filter for the source texture.
		*\param[in] config
		*	The configuration.
		*\~french
		*\brief
		*	Constructeur.
		*\param[in] device
		*	Le RenderDevice.
		*\param[in] name
		*	Le nom de la passe.
		*\param[in] samplerFilter
		*	Le filtre d'échantillonnage pour la texture source.
		*\param[in] config
		*	La configuration.
		*/
		C3D_API RenderQuad( RenderDevice const & device
			, castor::String const & name
			, VkFilter samplerFilter
			, rq::Config config );

	public:
		C3D_API virtual ~RenderQuad();
		C3D_API explicit RenderQuad( RenderQuad && rhs )noexcept;
		/**
		*\~english
		*\brief
		*	Cleans up GPU objects.
		*\~french
		*\brief
		*	Nettoie les objets GPU.
		*/
		C3D_API void cleanup();
		/**
		*\~english
		*\brief
		*	Creates the rendering pipeline.
		*\param[in] size
		*	The render size.
		*\param[in] position
		*	The render position.
		*\param[in] program
		*	The shader program.
		*\param[in] renderPass
		*	The render pass to use.
		*\param[in] pushRanges
		*	The push constant ranges.
		*\param[in] dsState
		*	The depth and stencil state.
		*\~french
		*\brief
		*	Crée le pipeline de rendu.
		*\param[in] size
		*	Les dimensions de rendu.
		*\param[in] position
		*	La position du rendu.
		*\param[in] program
		*	Le programme shader.
		*\param[in] renderPass
		*	La passe de rendu à utiliser.
		*\param[in] pushRanges
		*	Les intervalles de push constants.
		*\param[in] dsState
		*	L'état de profondeur et stencil.
		*/
		C3D_API void createPipeline( VkExtent2D const & size
			, castor::Position const & position
			, ashes::PipelineShaderStageCreateInfoArray const & program
			, ashes::RenderPass const & renderPass
			, ashes::VkPushConstantRangeArray const & pushRanges = ashes::VkPushConstantRangeArray{}
			, ashes::PipelineDepthStencilStateCreateInfo dsState = ashes::PipelineDepthStencilStateCreateInfo{ 0u, VK_FALSE, VK_FALSE } );
		/**
		*\~english
		*\brief
		*	Creates the entries for one pass.
		*\param[in] writes
		*	The pass descriptor writes.
		*\param[in] invertY
		*	\p true to Y invert the sampled image.
		*\~french
		*\brief
		*	Crée les entrées pour une passe.
		*\param[in] writes
		*	Les descriptor writes de la passe.
		*\param[in] invertY
		*	\p true pour inverser le Y de l'image échantillonnée.
		*/
		C3D_API void registerPassInputs( ashes::WriteDescriptorSetArray const & writes
			, bool invertY = false );
		/**
		*\~english
		*\brief
		*	Initialises the descriptor sets for the registered pass at given index.
		*\~french
		*\brief
		*	Crée les descriptor sets pour la passe enregistrée à l'indice donné.
		*/
		C3D_API void initialisePass( uint32_t passIndex );
		/**
		*\~english
		*\brief
		*	Initialises the descriptor sets for all registered passes.
		*\~french
		*\brief
		*	Crée les descriptor sets pour toute les passes enregistrées.
		*/
		C3D_API void initialisePasses();
		/**
		*\~english
		*\brief
		*	Creates the rendering pipeline and initialises the quad for one pass.
		*\param[in] size
		*	The render size.
		*\param[in] position
		*	The render position.
		*\param[in] program
		*	The shader program.
		*\param[in] renderPass
		*	The render pass to use.
		*\param[in] writes
		*	The pass descriptor writes.
		*\param[in] pushRanges
		*	The push constant ranges.
		*\param[in] dsState
		*	The depth stencil state to use.
		*\~french
		*\brief
		*	Crée le pipeline de rendu et initialise le quad pour une passe.
		*\param[in] size
		*	Les dimensions de rendu.
		*\param[in] position
		*	La position du rendu.
		*\param[in] program
		*	Le programme shader.
		*\param[in] renderPass
		*	La passe de rendu à utiliser.
		*\param[in] writes
		*	Les descriptor writes de la passe.
		*\param[in] pushRanges
		*	Les intervalles de push constants.
		*\param[in] dsState
		*	L'état de profondeur et stencil à utiliser.
		*/
		C3D_API void createPipelineAndPass( VkExtent2D const & size
			, castor::Position const & position
			, ashes::PipelineShaderStageCreateInfoArray const & program
			, ashes::RenderPass const & renderPass
			, ashes::WriteDescriptorSetArray const & writes
			, ashes::VkPushConstantRangeArray const & pushRanges = ashes::VkPushConstantRangeArray{}
			, ashes::PipelineDepthStencilStateCreateInfo dsState = ashes::PipelineDepthStencilStateCreateInfo{ 0u, false, false } );
		/**
		*\~english
		*\brief
		*	Prpares the commands to render the quad, inside given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*\param[in] descriptorSetIndex
		*	The render descriptor set index.
		*\~french
		*\brief
		*	Prépare les commandes de dessin du quad, dans le tampon de commandes donné.
		*\param[in,out] commandBuffer
		*	Le tampon de commandes.
		*\param[in] descriptorSetIndex
		*	L'indice du descriptor set.
		*/
		C3D_API void registerPass( ashes::CommandBuffer & commandBuffer
			, uint32_t descriptorSetIndex )const;
		/**
		*\~english
		*\brief
		*	Prpares the commands to render the quad, inside given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*\~french
		*\brief
		*	Prépare les commandes de dessin du quad, dans le tampon de commandes donné.
		*\param[in,out] commandBuffer
		*	Le tampon de commandes.
		*/
		void registerPass( ashes::CommandBuffer & commandBuffer )const
		{
			registerPass( commandBuffer, 0u );
		}

		RenderSystem * getRenderSystem()const
		{
			return &m_renderSystem;
		}

		RenderDevice const & getDevice()const
		{
			return m_device;
		}

		Sampler const & getSampler()const
		{
			return *m_sampler;
		}

	private:
		C3D_API virtual void doRegisterPass( ashes::CommandBuffer & commandBuffer )const;

	protected:
		RenderSystem & m_renderSystem;
		RenderDevice const & m_device;
		SamplerObs m_sampler{};
		rq::ConfigData m_config;

	private:
		bool m_useTexCoord{ true };
		ashes::DescriptorSetLayoutPtr m_descriptorSetLayout;
		ashes::PipelineLayoutPtr m_pipelineLayout;
		ashes::GraphicsPipelinePtr m_pipeline;
		ashes::DescriptorSetPoolPtr m_descriptorSetPool;
		std::vector< ashes::WriteDescriptorSetArray > m_passes;
		std::vector< ashes::DescriptorSetPtr > m_descriptorSets;
		std::vector< bool > m_invertY;
		ashes::VertexBufferPtr< TexturedQuad::Vertex > m_vertexBuffer;
		ashes::VertexBufferPtr< TexturedQuad::Vertex > m_uvInvVertexBuffer;
	};

	template< typename ConfigT, typename BuilderT >
	class RenderQuadBuilderT
	{
		static_assert( std::is_same_v< ConfigT, rq::Config >
			|| std::is_base_of_v< rq::Config, ConfigT >
			, "RenderQuadBuilderT::ConfigT must derive from castor3d::rq::Config" );

	public:
		RenderQuadBuilderT()
		{
		}
		/**
		*\~english
		*\param[in] config
		*	The texture coordinates configuration.
		*\~french
		*\param[in] config
		*	La configuration des coordonnées de texture.
		*/
		BuilderT & texcoordConfig( rq::Texcoord const & config )
		{
			m_config.texcoordConfig = config;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*\param[in] range
		*	Contains mip levels for the sampler.
		*\~french
		*\param[in] range
		*	Contient les mip levels, pour l'échantillonneur.
		*/
		BuilderT & range( VkImageSubresourceRange const & range )
		{
			m_config.range = range;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*\param[in] blendMode
		*	Contains blendMode to destination status.
		*\~french
		*\param[in] blendMode
		*	Contient le statut de mélange à la destination.
		*/
		BuilderT & blendMode( BlendMode blendMode )
		{
			m_config.blendMode = blendMode;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*\param[in] bindings
		*	Contains the inputs bindings.
		*\~french
		*\param[in] bindings
		*	Contient les bindings en entrée.
		*/
		BuilderT & bindings( rq::BindingDescriptionArray const & bindings )
		{
			m_config.bindings = bindings;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*	Adds an image.
		*\param[in] binding
		*	Contains the binding to add.
		*\~french
		*	Ajoute un binding.
		*\param[in] binding
		*	Contient le binding à ajouter.
		*/
		BuilderT & binding( rq::BindingDescription const & binding )
		{
			if ( !m_config.bindings )
			{
				m_config.bindings = rq::BindingDescriptionArray{};
			}

			m_config.bindings.value().push_back( binding );
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*	Adds an image.
		*\param[in] descriptor
		*	The descriptor type.
		*\param[in] stageFlags
		*	The descriptor stage flags.
		*\~french
		*	Ajoute un binding.
		*\param[in] descriptor
		*	Le type de descripteur.
		*\param[in] stageFlags
		*	Les flags de shader du descripteur.
		*/
		BuilderT & binding( VkDescriptorType descriptor
			, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT )
		{
			return binding( rq::BindingDescription{ descriptor, ashes::nullopt, stageFlags } );
		}
		/**
		*\~english
		*	Adds an image binding.
		*\param[in] descriptor
		*	The descriptor type.
		*\param[in] view
		*	The image view.
		*\param[in] stageFlags
		*	The descriptor stage flags.
		*\~french
		*	Ajoute un binding d'image.
		*\param[in] descriptor
		*	Le type de descripteur.
		*\param[in] view
		*	L'image view.
		*\param[in] stageFlags
		*	Les flags de shader du descripteur.
		*/
		BuilderT & binding( VkDescriptorType descriptor
			, VkImageViewType view
			, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT )
		{
			return binding( rq::BindingDescription{ descriptor, view, stageFlags } );
		}
		/**
		*\~english
		*	Sets the result used for for 3D texture inputs.
		*\remarks
		*	The 3D textures must be preprocessed externally to this result.
		*\param[in] result
		*	The result.
		*\~french
		*	Définit le résultat utilisé pour les textures 3D en entrée.
		*\remarks
		*	Les textures 3D doivent être traitées en externe dans ce résultat.
		*\param[in] result
		*	Le résultat.
		*/
		BuilderT & tex3DResult( IntermediateView result )
		{
			m_config.tex3DResult = result;
			return static_cast< BuilderT & >( *this );
		}
		/**
		*\~english
		*	Creates the RenderQuad.
		*\param[in] device
		*	The RenderDevice.
		*\param[in] name
		*	The pass name.
		*\param[in] samplerFilter
		*	The sampler filter for the source texture.
		*\~french
		*	Crée le RenderQuad.
		*\param[in] device
		*	Le RenderDevice.
		*\param[in] name
		*	Le nom de la passe.
		*\param[in] samplerFilter
		*	Le filtre d'échantillonnage pour la texture source.
		*/
		RenderQuadUPtr build( RenderDevice const & device
			, castor::String const & name
			, VkFilter samplerFilter )
		{
			return castor::UniquePtr< RenderQuad >( new RenderQuad{ device
				, name
				, samplerFilter
				, m_config } );
		}

	protected:
		ConfigT m_config;
	};

	class RenderQuadBuilder
		: public RenderQuadBuilderT< rq::Config, RenderQuadBuilder >
	{
	};
}

#endif

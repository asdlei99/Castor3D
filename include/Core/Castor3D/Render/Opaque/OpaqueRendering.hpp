/*
See LICENSE file in root folder
*/
#ifndef ___C3D_OpaqueRendering_H___
#define ___C3D_OpaqueRendering_H___

#include "OpaqueModule.hpp"

#include "Castor3D/Material/Texture/TextureUnit.hpp"
#include "Castor3D/Miscellaneous/MiscellaneousModule.hpp"
#include "Castor3D/Render/GlobalIllumination/LightPropagationVolumes/LightPropagationVolumesModule.hpp"
#include "Castor3D/Render/GlobalIllumination/VoxelConeTracing/VoxelizeModule.hpp"
#include "Castor3D/Render/Passes/CommandsSemaphore.hpp"
#include "Castor3D/Render/Prepass/PrepassModule.hpp"
#include "Castor3D/Render/ShadowMap/ShadowMap.hpp"
#include "Castor3D/Render/Opaque/OpaqueModule.hpp"
#include "Castor3D/Render/Ssao/SsaoConfig.hpp"
#include "Castor3D/Render/Ssao/SsaoPass.hpp"
#include "Castor3D/Render/Transparent/TransparentModule.hpp"
#include "Castor3D/Scene/Background/BackgroundModule.hpp"
#include "Castor3D/Shader/Ubos/GpInfoUbo.hpp"
#include "Castor3D/Shader/Ubos/LayeredLpvGridConfigUbo.hpp"
#include "Castor3D/Shader/Ubos/LpvGridConfigUbo.hpp"
#include "Castor3D/Shader/Ubos/MatrixUbo.hpp"
#include "Castor3D/Shader/Ubos/SceneUbo.hpp"
#include "Castor3D/Shader/Ubos/VoxelizerUbo.hpp"

#include <CastorUtils/Design/DelayedInitialiser.hpp>
#include <CastorUtils/Design/Named.hpp>

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FramePass.hpp>

namespace castor3d
{
	class OpaqueRendering
		: public castor::OwnedBy< RenderTechnique >
	{
	public:
		/**
		 *\~english
		 *\brief		Constructor
		 *\param[in]	parent			The parent technique.
		 *\param[in]	device			The GPU device.
		 *\param[in]	queueData		The queue receiving the GPU commands.
		 *\param[in]	parameters		The technique parameters.
		 *\param[in]	ssaoConfig		The SSAO configuration.
		 *\param[in]	progress		The optional progress bar.
		 *\~french
		 *\brief		Constructeur
		 *\param[in]	parent			La technique parente.
		 *\param[in]	device			Le device GPU.
		 *\param[in]	queueData		La queue recevant les commandes GPU.
		 *\param[in]	parameters		Les paramètres de la technique.
		 *\param[in]	ssaoConfig		La configuration du SSAO.
		 *\param[in]	progress		La barre de progression optionnelle.
		 */
		C3D_API OpaqueRendering( RenderTechnique & parent
			, RenderDevice const & device
			, QueueData const & queueData
			, PrepassRendering const & previous
			, crg::FramePassArray const & previousPasses
			, SsaoConfig const & ssaoConfig
			, ProgressBar * progress
			, TexturePtr normal
			, bool deferred );
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		C3D_API ~OpaqueRendering();
		/**
		 *\~english
		 *\return		The number of steps needed for initialisation, to show progression.
		 *\~french
		 *\return		Le nombre d'étapes nécessaires à l'initialisation, pour en montrer la progression.
		 */
		C3D_API static uint32_t countInitialisationSteps();
		/**
		 *\~english
		 *\brief		Lists the intermediate view used by the whole technique.
		 *\param[out]	intermediates	Receives the intermediate views.
		 *\~french
		 *\brief		Liste les vues intermédiaires utilisées par toute la technique.
		 *\param[out]	intermediates	Reçoit les vues intermédiaires.
		 */
		C3D_API void listIntermediates( std::vector< IntermediateView > & intermediates );
		/**
		 *\~english
		 *\brief			Updates the render pass, CPU wise.
		 *\param[in, out]	updater	The update data.
		 *\~french
		 *\brief			Met à jour la passe de rendu, au niveau CPU.
		 *\param[in, out]	updater	Les données d'update.
		 */
		C3D_API void update( CpuUpdater & updater );
		/**
		 *\~english
		 *\brief			Updates the render pass, GPU wise.
		 *\param[in, out]	updater	The update data.
		 *\~french
		 *\brief			Met à jour la passe de rendu, au niveau GPU.
		 *\param[in, out]	updater	Les données d'update.
		 */
		C3D_API void update( GpuUpdater & updater );
		/**
		*\~english
		*\brief
		*	Visitor acceptance function.
		*\param visitor
		*	The ... visitor.
		*\~french
		*\brief
		*	Fonction d'acceptation de visiteur.
		*\param visitor
		*	Le ... visiteur.
		*/
		C3D_API void accept( RenderTechniqueVisitor & visitor );
		/**
		*\~english
		*name
		*	Getters.
		*\~french
		*name
		*	Accesseurs.
		*/
		/**@{*/
		C3D_API Engine * getEngine()const;
		C3D_API crg::FramePass const & getLastPass()const;
		C3D_API Texture const & getSsaoResult()const;
		C3D_API crg::ImageViewId const & getLightDepthImgView()const;
		C3D_API Texture const & getDiffuseLightingResult()const;
		C3D_API Texture const & getBaseColourResult()const;

		OpaquePassResult const & getOpaqueResult()const
		{
			return *m_opaquePassResult;
		}

		ashes::Buffer< uint32_t > const & getMaterialsCounts()const
		{
			return *m_materialsCounts1;
		}

		ashes::Buffer< uint32_t > const & getMaterialsStarts()const
		{
			return *m_materialsStarts;
		}

		ashes::Buffer< castor::Point2ui > const & getPixelXY()const
		{
			return *m_pixelsXY;
		}
		/**@}*/

	public:
		using ShadowMapArray = std::vector< ShadowMapUPtr >;

	private:
		SsaoPassUPtr doCreateSsaoPass( ProgressBar * progress
			, crg::FramePass const & lastPass
			, crg::FramePassArray previousPasses )const;
		crg::FramePass & doCreateVisibilityResolve( ProgressBar * progress
			, Texture const & coin
			, VisibilityPass const & visibilityPass
			, crg::FramePassArray const & previousPasses );
		crg::FramePass & doCreateForwardOpaquePass( ProgressBar * progress
			, crg::FramePass const & lastPass
			, crg::FramePassArray const & previousPasses );
		crg::FramePass & doCreateDeferredOpaquePass( ProgressBar * progress
			, crg::FramePass const & lastPass
			, crg::FramePassArray const & previousPasses );

	private:
		RenderDevice const & m_device;
		crg::FramePassGroup & m_graph;
		OpaquePassResultUPtr m_opaquePassResult;
		ashes::BufferPtr< uint32_t > m_materialsCounts1;
		ashes::BufferPtr< uint32_t > m_materialsCounts2;
		ashes::BufferPtr< uint32_t > m_materialsStarts;
		ashes::BufferPtr< castor::Point2ui > m_pixelsXY;
		ShaderBufferUPtr m_visibilityPipelinesIds;
		VisibilityReorderPassUPtr m_visibilityReorder;
		crg::FramePass * m_visibilityResolveDesc{};
		VisibilityResolvePass * m_visibilityResolve{};
		SsaoPassUPtr m_ssao;
		crg::FramePass * m_opaquePassDesc{};
		RenderTechniquePass * m_opaquePass{};
		DeferredRenderingUPtr m_deferredRendering;
	};
}

#endif
/*
See LICENSE file in root folder
*/
#ifndef ___C3D_PostEffect_H___
#define ___C3D_PostEffect_H___

#include "PostEffectModule.hpp"

#include "Castor3D/Material/Texture/TextureUnit.hpp"
#include "Castor3D/Miscellaneous/ConfigurationVisitor.hpp"
#include "Castor3D/Render/RenderTarget.hpp"
#include "Castor3D/Render/Passes/CommandsSemaphore.hpp"

#include <CastorUtils/Design/Named.hpp>

namespace castor3d
{
	class PostEffect
		: public castor::OwnedBy< RenderSystem >
		, public castor::Named
	{
	public:
		enum class Kind
		{
			eHDR,
			eSRGB,
			eOverlay, // TODO: Unsupported yet.
		};

	protected:
		/**
		 *\~english
		 *\brief		Constructor.
		 *\param[in]	name			The effect name.
		 *\param[in]	groupName		The effect passes group name.
		 *\param[in]	fullName		The effect full (fancy) name.
		 *\param[in]	renderTarget	The render target to which is attached this effect.
		 *\param[in]	renderSystem	The render system.
		 *\param[in]	parameters		The optional parameters.
		 *\param[in]	passesCount		The number of passes for this post effect.
		 *\param[in]	kind			The post effect kind.
		 *\~french
		 *\brief		Constructeur.
		 *\param[in]	name			Le nom de l'effet.
		 *\param[in]	groupName		Le nom du groupe de passes de l'effet.
		 *\param[in]	fullName		Le nom complet (et joli) de l'effet.
		 *\param[in]	renderTarget	La cible de rendu sur laquelle cet effet s'applique.
		 *\param[in]	renderSystem	Le render system.
		 *\param[in]	parameters		Les paramètres optionnels.
		 *\param[in]	passesCount		Le nombre de passes pour cet effet.
		 *\param[in]	kind			Le type d'effet.
		 */
		C3D_API PostEffect( castor::String const & name
			, castor::String const & groupName
			, castor::String const & fullName
			, RenderTarget & renderTarget
			, RenderSystem & renderSystem
			, Parameters const & parameters
			, uint32_t passesCount = 1u
			, Kind kind = Kind::eHDR );

	public:
		/**
		 *\~english
		 *\brief		Destructor.
		 *\~french
		 *\brief		Destructeur.
		 */
		C3D_API virtual ~PostEffect();
		/**
		 *\~english
		 *\brief		Writes the effect into a text file.
		 *\param[in]	file	The file.
		 *\param[in]	tabs	The current indentation.
		 *\~french
		 *\brief		Ecrit l'effet dans un fichier texte.
		 *\param[in]	file	Le fichier.
		 *\param[in]	tabs	L'indentation actuelle.
		 */
		C3D_API bool writeInto( castor::StringStream & file
			, castor::String const & tabs );
		/**
		 *\~english
		 *\brief		Initialisation function.
		 *\param[in]	device			The GPU device.
		 *\param[in]	source			The source texture.
		 *\param[in]	target			The target texture.
		 *\param[in]	previousPass	The previous frame pass.
		 *\return		\p true if ok.
		 *\~french
		 *\brief		Fonction d'initialisation.
		 *\param[in]	device			Le device GPU.
		 *\param[in]	source			La texture source.
		 *\param[in]	target			La texture cible.
		 *\param[in]	previousPass	La frame pass précédente.
		 *\return		\p true if ok.
		 */
		C3D_API bool initialise( castor3d::RenderDevice const & device
			, Texture const & source
			, Texture const & target
			, crg::FramePass const & previousPass );
		/**
		 *\~english
		 *\brief		Cleanup function.
		 *\param[in]	device	The GPU device.
		 *\~french
		 *\brief		Fonction de nettoyage.
		 *\param[in]	device	Le device GPU.
		 */
		C3D_API void cleanup( castor3d::RenderDevice const & device );
		/**
		 *\~english
		 *\param[in, out]	updater	The update data.
		 *\param[in]		source	The current source image.
		 *\~french
		 *\param[in, out]	updater	Les données d'update.
		 *\param[in]		source	L'image source courante.
		 */
		C3D_API bool update( CpuUpdater & updater
			, Texture const & source );
		/**
		 *\~english
		 *\param[in, out]	updater	The update data.
		 *\~french
		 *\param[in, out]	updater	Les données d'update.
		 */
		C3D_API void update( GpuUpdater & updater );
		/**
		 *\~english
		 *\brief		Visitor acceptance function.
		 *\param		visitor	The ... visitor.
		 *\~french
		 *\brief		Fonction d'acceptation de visiteur.
		 *\param		visitor	Le ... visiteur.
		 */
		C3D_API virtual void accept( ConfigurationVisitorBase & visitor ) = 0;
		/**
		*\~english
		*name
		*	Mutators.
		*\~french
		*name
		*	Mutateurs.
		**/
		/**@{*/
		void enable( bool value )
		{
			m_enabled = value;
		}
		/**@}*/
		/**
		*\~english
		*name
		*	Getters.
		*\~french
		*name
		*	Accesseurs.
		**/
		/**@{*/
		C3D_API virtual crg::FramePass const & getPass()const = 0;
		C3D_API virtual void setParameters( Parameters parameters ) = 0;

		bool isAfterToneMapping()const
		{
			return m_kind == Kind::eSRGB;
		}

		castor::String const & getFullName()const
		{
			return m_fullName;
		}

		bool const & isEnabled()const
		{
			return m_enabled;
		}
		/**@}*/

	private:
		/**
		 *\~english
		 *\brief		Initialisation function.
		 *\param[in]	device			The GPU device.
		 *\param[in]	source			The initial source image.
		 *\param[in]	target			The initial target image.
		 *\param[in]	previousPass	The previous frame pass.
		 *\return		\p false on failure.
		 *\~french
		 *\brief		Fonction d'initialisation.
		 *\param[in]	device			Le device GPU.
		 *\param[in]	source			L'image source initiale.
		 *\param[in]	target			L'image cible initiale.
		 *\param[in]	previousPass	La frame pass précédente.
		 *\return		\p false en cas d'échec.
		 */
		C3D_API virtual bool doInitialise( castor3d::RenderDevice const & device
			, Texture const & source
			, Texture const & target
			, crg::FramePass const & previousPass ) = 0;
		/**
		 *\~english
		 *\brief		Cleanup function.
		 *\param[in]	device	The GPU device.
		 *\~french
		 *\brief		Fonction de nettoyage.
		 *\param[in]	device	Le device GPU.
		 */
		C3D_API virtual void doCleanup( castor3d::RenderDevice const & device ) = 0;
		/**
		 *\~english
		 *\brief			Updates the render pass, CPU wise.
		 *\param[in, out]	updater	The update data.
		 *\~french
		 *\brief			Met à jour la passe de rendu, au niveau CPU.
		 *\param[in, out]	updater	Les données d'update.
		 */
		C3D_API virtual void doCpuUpdate( CpuUpdater & updater );
		/**
		 *\~english
		 *\brief			Updates the render pass, GPU wise.
		 *\param[in, out]	updater	The update data.
		 *\~french
		 *\brief			Met à jour la passe de rendu, au niveau GPU.
		 *\param[in, out]	updater	Les données d'update.
		 */
		C3D_API virtual void doGpuUpdate( GpuUpdater & updater );
		/**
		 *\~english
		 *\brief		Writes the effect into a text file.
		 *\param[in]	file	The file.
		 *\param[in]	tabs	The current indentation.
		 *\~french
		 *\brief		Ecrit l'effet dans un fichier texte.
		 *\param[in]	file	Le fichier.
		 *\param[in]	tabs	L'indentation actuelle.
		 */
		C3D_API virtual bool doWriteInto( castor::StringStream & file
			, castor::String const & tabs ) = 0;

	protected:
		castor::String m_fullName;
		RenderTarget & m_renderTarget;
		crg::FramePassGroup & m_graph;
		uint32_t m_passesCount{ 1u };
		Kind m_kind{ Kind::eHDR };
		bool m_enabled;
		uint32_t m_passIndex{};
		Texture const * m_source{};
	};
}

#endif

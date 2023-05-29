/*
See LICENSE file in root folder
*/
#ifndef ___C3D_LIGHT_CATEGORY_H___
#define ___C3D_LIGHT_CATEGORY_H___

#include "LightModule.hpp"
#include "Castor3D/Render/GlobalIllumination/LightPropagationVolumes/LightPropagationVolumesModule.hpp"
#include "Castor3D/Shader/ShaderBuffers/LightBuffer.hpp"

#include <CastorUtils/Data/TextWriter.hpp>
#include <CastorUtils/Graphics/BoundingBox.hpp>

namespace castor3d
{
	class LightCategory
	{
	private:
		friend class Light;

	protected:
		/**
		 *\~english
		 *\brief		Constructor.
		 *\param[in]	lightType	The light category type.
		 *\param[in]	light		The parent Light.
		 *\~french
		 *\brief		Constructeur.
		 *\param[in]	lightType	Le type de catégorie de lumière.
		 *\param[in]	light		La Light parente.
		 */
		C3D_API explicit LightCategory( LightType lightType, Light & light, uint32_t componentCount );

	public:
		struct ShadowData
			: ShaderBufferTypes
		{
			Float1 shadowMapIndex;
			Float1 shadowType;
			Float1 pcfFilterSize;
			Float1 pcfSampleCount;
			Float2 rawShadowsOffsets;
			Float2 pcfShadowsOffsets;
			Float1 vsmMinVariance;
			Float1 vsmLightBleedingReduction;
			Float1 volumetricSteps;
			Float1 volumetricScattering;
		};

		struct LightData
			: ShaderBufferTypes
		{
			Float3 colour;
			Float1 farPlane;
			Float2 intensity;
			Float2 pad0;
			ShadowData shadows;
		};

	public:
		/**
		 *\~english
		 *\brief		Destructor
		 *\~french
		 *\brief		Destructeur
		 */
		C3D_API virtual ~LightCategory() = default;
		/**
		 *\~english
		 *\brief		Updates the light.
		 *\~french
		 *\brief		Met la source à jour.
		 */
		C3D_API virtual void update() = 0;
		/**
		 *\~english
		 *\brief		Puts the light into the given texture.
		 *\param[out]	data	Receives the light's data.
		 *\~french
		 *\brief		Met la lumière dans la texture donnée.
		 *\param[out]	data	Reçoit les données de la source lumineuse.
		 */
		C3D_API void fillBuffer( castor::Point4f * data )const;
		/**
		*\~english
		*name
		*	Getters.
		*\~french
		*name
		*	Accesseurs.
		*/
		/**@{*/
		C3D_API uint32_t getVolumetricSteps()const;
		C3D_API float getVolumetricScatteringFactor()const;
		C3D_API castor::Point2f const & getShadowRawOffsets()const;
		C3D_API castor::Point2f const & getShadowPcfOffsets()const;
		C3D_API float getVsmMinVariance()const;
		C3D_API float getVsmLightBleedingReduction()const;
		C3D_API castor::RangedValue< uint32_t > getShadowPcfFilterSize()const;
		C3D_API castor::RangedValue< uint32_t > getShadowPcfSampleCount()const;
		C3D_API ShadowConfig const & getShadowConfig()const;
		C3D_API RsmConfig const & getRsmConfig()const;
		C3D_API LpvConfig const & getLpvConfig()const;

		LightType getLightType()const
		{
			return m_lightType;
		}

		uint32_t getComponentCount()const
		{
			return m_componentCount;
		}

		float getDiffuseIntensity()const
		{
			return m_intensity[0];
		}

		float getSpecularIntensity()const
		{
			return m_intensity[1];
		}

		castor::Point2f const & getIntensity()const
		{
			return m_intensity;
		}

		float getFarPlane()const
		{
			return m_farPlane;
		}

		castor::Point3f const & getColour()const
		{
			return m_colour;
		}

		Light const & getLight()const
		{
			return m_light;
		}

		castor::BoundingBox const & getBoundingBox()const
		{
			return m_cubeBox;
		}
		/**@}*/
		/**
		*\~english
		*name
		*	Mutators.
		*\~french
		*name
		*	Mutateurs.
		*/
		/**@{*/
		C3D_API void setVolumetricSteps( uint32_t value );
		C3D_API void setVolumetricScatteringFactor( float value );
		C3D_API void setRawMinOffset( float value );
		C3D_API void setRawMaxSlopeOffset( float value );
		C3D_API void setPcfMinOffset( float value );
		C3D_API void setPcfMaxSlopeOffset( float value );
		C3D_API void setPcfFilterSize( uint32_t value );
		C3D_API void setPcfSampleCount( uint32_t value );
		C3D_API void setVsmMinVariance( float value );
		C3D_API void setVsmLightBleedingReduction( float value );
		C3D_API void setColour( castor::Point3f const & value );
		C3D_API void setIntensity( castor::Point2f const & value );
		C3D_API void setDiffuseIntensity( float value );
		C3D_API void setSpecularIntensity( float value );

		Light & getLight()
		{
			return m_light;
		}

		castor::Point3f & getColour()
		{
			return m_colour;
		}

		castor::Point2f & getIntensity()
		{
			return m_intensity;
		}
		/**@}*/

	private:
		/**
		 *\~english
		 *\brief		Puts the light into the given texture.
		 *\param[out]	data	Receives the light's data.
		 *\~french
		 *\brief		Met la lumière dans la texture donnée.
		 *\param[out]	data	Reçoit les données de la source lumineuse.
		 */
		C3D_API virtual void doFillBuffer( castor::Point4f * data )const = 0;

	protected:
		//!\~english	The cube box for the light volume of effect.
		//!\~french		La cube box pour le volume d'effet de la lumière.
		castor::BoundingBox m_cubeBox;
		//!\~english	The far plane's depth.
		//!\~french		La profondeur du plan éloigné.
		float m_farPlane{ 1.0f };

	private:
		LightType m_lightType;
		Light & m_light;
		uint32_t m_componentCount{};
		castor::Point3f m_colour{ 1.0, 1.0, 1.0 };
		castor::Point2f m_intensity{ 1.0, 1.0 };
	};
}

#endif

/*
See LICENSE file in root folder
*/
#ifndef ___C3D_FaceInfos_H___
#define ___C3D_FaceInfos_H___

#include "ComponentModule.hpp"

namespace castor3d
{
	struct FaceInfos
	{
		/**
		\~english
		\brief		Holds vertex informations.
		\~french
		\brief		Contient les informations d'un sommet.
		\remark
		*/
		struct Vertex
		{
			//!\~english	Vertex index.
			//!\~french		Indice du sommet.
			int m_index{ -1 };
			//!\~english	Vertex normal.
			//!\~french		Normale du sommet.
			std::array< float, 3 > m_fNormal{ { 0.0f, 0.0f, 0.0f } };
			//!\~english	Vertex UV.
			//!\~french		UV du sommet.
			std::array< float, 2 > m_fTexCoords{ { 0.0f, 0.0f } };
			//!\~english	Tells the vertex has a normal.
			//!\~french		Dit que le sommet a une normale.
			bool m_bHasNormal{ false };
			//!\~english	Tells the vertex has UV.
			//!\~french		Dit que le sommet a un UV.
			bool m_bHasTexCoords{ false };
		};
		//!\~english	The 3 vertex informations.
		//!\~french		Les informations des 3 sommets.
		std::array< Vertex, 3 > m_vertex;
	};
}

#endif

/*
See LICENSE file in root folder
*/
#ifndef ___C3D_DummyCuller_H___
#define ___C3D_DummyCuller_H___

#include "Castor3D/Render/Culling/SceneCuller.hpp"

#include <optional>

namespace castor3d
{
	class DummyCuller
		: public SceneCuller
	{
	public:
		C3D_API explicit DummyCuller( Scene & scene
			, Camera * camera = nullptr
			, std::optional< bool > isStatic = std::nullopt );

	private:
		bool isSubmeshVisible( SubmeshRenderNode const & node )const override;
		bool isBillboardVisible( BillboardRenderNode const & node )const override;
	};
}

#endif

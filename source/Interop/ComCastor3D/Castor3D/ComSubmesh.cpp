#include "ComCastor3D/Castor3D/ComSubmesh.hpp"
#include "ComCastor3D/CastorUtils/ComVector3D.hpp"

namespace CastorCom
{
	namespace submesh
	{
		static const tstring ERROR_UNINITIALISED = _T( "The submesh must be initialised" );
	}

	STDMETHODIMP CSubmesh::AddPoint( /* [in] */ IVector3D * val )noexcept
	{
		HRESULT hr = E_POINTER;

		if ( m_internal )
		{
			m_internal->addPoint( static_cast< CVector3D * >( val )->getInternal() );
			hr = S_OK;
		}
		else
		{
			hr = CComError::dispatchError(
				E_FAIL,							// This represents the error
				IID_ISubmesh,					// This is the GUID of PixelComponents throwing error
				_T( "addPoint" ),				// This is generally displayed as the title
				submesh::ERROR_UNINITIALISED.c_str(),	// This is the description
				0,								// This is the context in the help file
				nullptr );
		}

		return hr;
	}
}

/*
See LICENSE file in root folder
*/
#ifndef ___C3D_ControlStyle_H___
#define ___C3D_ControlStyle_H___

#include "Castor3D/Gui/GuiModule.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Cache/MaterialCache.hpp"
#include "Castor3D/Event/UserInput/EventHandler.hpp"

#include <CastorUtils/Design/Named.hpp>
#include <CastorUtils/Graphics/Pixel.hpp>
#include <CastorUtils/Graphics/Position.hpp>
#include <CastorUtils/Graphics/Rectangle.hpp>
#include <CastorUtils/Graphics/Size.hpp>

namespace castor3d
{
	class ControlStyle
		: public castor::Named
	{
	public:
		virtual ~ControlStyle() = default;

		ControlStyle( ControlType type
			, castor::String const & name
			, Engine & engine
			, MouseCursor cursor = MouseCursor::eHand )
			: castor::Named{ name }
			, m_engine{ engine }
			, m_cursor{ cursor }
			, m_backgroundMaterial{ getEngine().findMaterial( cuT( "Black" ) ).lock().get() }
			, m_foregroundMaterial{ getEngine().findMaterial( cuT( "White" ) ).lock().get() }
		{
		}

		void setBackgroundMaterial( MaterialRPtr value )
		{
			m_backgroundMaterial = value;
			doUpdateBackgroundMaterial();
		}

		void setForegroundMaterial( MaterialRPtr value )
		{
			m_foregroundMaterial = value;
			doUpdateForegroundMaterial();
		}

		void setCursor( MouseCursor value )
		{
			m_cursor = value;
		}

		MaterialRPtr getBackgroundMaterial()const
		{
			return m_backgroundMaterial;
		}

		MaterialRPtr getForegroundMaterial()const
		{
			return m_foregroundMaterial;
		}

		MouseCursor getCursor()const
		{
			return m_cursor;
		}

		Engine & getEngine()const
		{
			return m_engine;
		}

	protected:
		MaterialRPtr doCreateMaterial( MaterialRPtr material
			, float offset
			, castor::String const & suffix )
		{
			auto colour = getMaterialColour( *material->getPass( 0u ) );
			colour.red() = float( colour.red() ) + offset;
			colour.green() = float( colour.green() ) + offset;
			colour.blue() = float( colour.blue() ) + offset;
			return createMaterial( getEngine(), material->getName() + suffix, colour );
		}

	private:
		virtual void doUpdateBackgroundMaterial() = 0;
		virtual void doUpdateForegroundMaterial() = 0;

	private:
		Engine & m_engine;
		MouseCursor m_cursor;
		MaterialRPtr m_backgroundMaterial{};
		MaterialRPtr m_foregroundMaterial{};
	};
}

#endif
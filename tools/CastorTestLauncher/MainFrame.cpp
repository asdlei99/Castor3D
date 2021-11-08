#include "CastorTestLauncher/MainFrame.hpp"

#include "CastorTestLauncher/CastorTestLauncher.hpp"

#include <Castor3D/Engine.hpp>
#include <Castor3D/Material/Material.hpp>
#include <Castor3D/Render/RenderTarget.hpp>
#include <Castor3D/Render/RenderLoop.hpp>
#include <Castor3D/Render/RenderWindow.hpp>
#include <Castor3D/Scene/Scene.hpp>
#include <Castor3D/Scene/SceneFileParser.hpp>

#include <CastorUtils/Graphics/PixelBufferBase.hpp>

#include <ashespp/Core/PlatformWindowHandle.hpp>
#include <ashespp/Core/WindowHandle.hpp>

#pragma warning( push )
#pragma warning( disable: 4127 )
#pragma warning( disable: 4365 )
#pragma warning( disable: 4371 )
#include <wx/display.h>
#include <wx/mstream.h>
#include <wx/renderer.h>
#include <wx/rawbmp.h>
#pragma warning( pop )

#if defined( CU_PlatformApple )
#	include "MetalLayer.h"
#elif defined( CU_PlatformLinux )
#	include <gdk/gdkx.h>
#	include <gtk/gtk.h>
#	include <GL/glx.h>
#	undef None
#	undef Bool
#	undef Always
using Bool = int;
#endif

#include <castor.xpm>

using namespace castor3d;
using namespace castor;

namespace test_launcher
{
	namespace
	{
		RenderTargetSPtr doLoadScene( Engine & engine
			, Path const & fileName )
		{
			RenderTargetSPtr result;

			if ( File::fileExists( fileName ) )
			{
				Logger::logInfo( cuT( "Loading scene file : " ) + fileName );

				if ( fileName.getExtension() == cuT( "cscn" ) || fileName.getExtension() == cuT( "zip" ) )
				{
					try
					{
						SceneFileParser parser( engine );

						if ( parser.parseFile( fileName ) )
						{
							result = parser.getRenderWindow().renderTarget;
						}
						else
						{
							Logger::logWarning( cuT( "Can't read scene file" ) );
						}
					}
					catch ( std::exception & exc )
					{
						Logger::logError( makeStringStream() << cuT( "Failed to parse the scene file, with following error: " ) << exc.what() );
					}
				}
				else
				{
					Logger::logError( cuT( "Unsupported scene file type: " ) + fileName.getFileName() );
				}
			}
			else
			{
				Logger::logError( cuT( "Scene file doesn't exist: " ) + fileName );
			}

			return result;
		}

		void doCreateBitmapFromBuffer( uint8_t const * input
			, uint32_t width
			, uint32_t height
			, bool flip
			, wxBitmap & output )
		{
			output.Create( int( width ), int( height ), 24 );
			wxNativePixelData data( output );

			if ( output.IsOk()
				&& uint32_t( data.GetWidth() ) == width
				&& uint32_t( data.GetHeight() ) == height )
			{
				wxNativePixelData::Iterator it( data );

				try
				{
					if ( flip )
					{
						uint32_t pitch = width * 4;
						uint8_t const * buffer = input + ( height - 1ull ) * pitch;

						for ( uint32_t i = 0; i < height && it.IsOk(); i++ )
						{
							uint8_t const * line = buffer;
#if defined( CU_PlatformWindows )
							wxNativePixelData::Iterator rowStart = it;
#endif

							for ( uint32_t j = 0; j < width && it.IsOk(); j++ )
							{
								it.Red() = *line;
								line++;
								it.Green() = *line;
								line++;
								it.Blue() = *line;
								line++;
								// don't write the alpha.
								line++;
								it++;
							}

							buffer -= pitch;

#if defined( CU_PlatformWindows )
							it = rowStart;
							it.OffsetY( data, 1 );
#endif
						}
					}
					else
					{
						uint8_t const * buffer = input;

						for ( uint32_t i = 0; i < height && it.IsOk(); i++ )
						{
#if defined( CU_PlatformWindows )
							wxNativePixelData::Iterator rowStart = it;
#endif

							for ( uint32_t j = 0; j < width && it.IsOk(); j++ )
							{
								it.Red() = *buffer;
								buffer++;
								it.Green() = *buffer;
								buffer++;
								it.Blue() = *buffer;
								buffer++;
								// don't write the alpha.
								buffer++;
								it++;
							}

#if defined( CU_PlatformWindows )
							it = rowStart;
							it.OffsetY( data, 1 );
#endif
						}
					}
				}
				catch ( ... )
				{
					Logger::logWarning( cuT( "doCreateBitmapFromBuffer encountered an exception" ) );
				}
			}
		}

		void doCreateBitmapFromBuffer( PxBufferBaseSPtr input
			, bool flip
			, wxBitmap & output )
		{
			PxBufferBaseSPtr buffer;

			if ( input->getFormat() != PixelFormat::eR8G8B8A8_UNORM )
			{
				buffer = PxBufferBase::create( Size( input->getWidth(), input->getHeight() )
					, PixelFormat::eR8G8B8A8_UNORM
					, input->getConstPtr()
					, input->getFormat() );
			}
			else
			{
				buffer = input;
			}

			doCreateBitmapFromBuffer( buffer->getConstPtr()
				, buffer->getWidth()
				, buffer->getHeight()
				, flip
				, output );
		}

		ashes::WindowHandle makeWindowHandle( wxWindow * window )
		{
#if defined( CU_PlatformWindows )

			return ashes::WindowHandle( std::make_unique< ashes::IMswWindowHandle >( ::GetModuleHandle( nullptr )
				, window->GetHandle() ) );

#elif defined( CU_PlatformApple )

			auto handle = window->GetHandle();
			makeViewMetalCompatible( handle );
			return ashes::WindowHandle( std::make_unique< ashes::IMacOsWindowHandle >( handle ) );

#elif defined( CU_PlatformLinux )

			GtkWidget * gtkWidget = window->GetHandle();
			auto gdkWindow = gtk_widget_get_window( gtkWidget );
			GLXDrawable drawable = 0;
			Display * display = nullptr;

			if ( gtkWidget && gdkWindow )
			{
				drawable = GDK_WINDOW_XID( gdkWindow );
				GdkDisplay * gtkDisplay = gtk_widget_get_display( gtkWidget );

				if ( gtkDisplay )
				{
					display = gdk_x11_display_get_xdisplay( gtkDisplay );
				}
			}

			return ashes::WindowHandle( std::make_unique< ashes::IXWindowHandle >( drawable, display ) );

#endif
		}
	}

	MainFrame::MainFrame( castor3d::Engine & engine )
		: wxFrame{ nullptr, wxID_ANY, wxT( "Castor3D Test Launcher" ), wxDefaultPosition, wxSize( 800, 700 ) }
		, m_engine{ engine }
	{
		SetClientSize( 800, 600 );
	}

	bool MainFrame::initialise()
	{
		wxIcon icon = wxIcon( castor_xpm );
		SetIcon( icon );
		bool result = true;
		Logger::logInfo( cuT( "Initialising Castor3D" ) );

		try
		{
			m_engine.initialise( 60, false );
			Logger::logInfo( cuT( "Castor3D Initialised" ) );
		}
		catch ( std::exception & exc )
		{
			Logger::logError( makeStringStream() << cuT( "Problem occured while initialising Castor3D: " ) << exc.what() );
			result = false;
		}
		catch ( ... )
		{
			Logger::logError( cuT( "Problem occured while initialising Castor3D." ) );
			result = false;
		}

		return result;
	}

	bool MainFrame::loadScene( wxString const & fileName )
	{
		if ( !fileName.empty() )
		{
			m_filePath = Path{ static_cast< wxChar const * >( fileName.c_str() ) };
		}

		if ( !m_filePath.empty() )
		{
			auto sizewx = GetClientSize();
			castor::Size sizeWnd{ uint32_t( sizewx.GetWidth() ), uint32_t( sizewx.GetHeight() ) };
			m_renderWindow = std::make_unique< RenderWindow >( "CastorTest"
				, m_engine
				, sizeWnd
				, makeWindowHandle( this ) );
			auto target = doLoadScene( m_engine, m_filePath );

			if ( !target )
			{
				Logger::logError( cuT( "Can't initialise the render window." ) );
			}
			else
			{
				m_renderWindow->initialise( target );
			}
		}
		else
		{
			Logger::logError( cuT( "Can't open a scene file : empty file name." ) );
		}

		return m_renderWindow != nullptr;
	}

	FrameTimes MainFrame::saveFrame( castor::String const & suffix )
	{
		FrameTimes result;

		if ( m_renderWindow )
		{
			wxBitmap bitmap;

			// Prerender 10 frames, for environment maps.
			for ( auto i = 0; i <= 10; ++i )
			{
				m_engine.getRenderLoop().renderSyncFrame( 25_ms );
			}

			result.avg = std::chrono::duration_cast< castor::Microseconds >( m_engine.getRenderLoop().getAvgFrameTime() );
			result.last = m_engine.getRenderLoop().getLastFrameTime();
			m_renderWindow->enableSaveFrame();
			m_engine.getRenderLoop().renderSyncFrame( 25_ms );
			auto buffer = m_renderWindow->getSavedFrame();
			doCreateBitmapFromBuffer( buffer
				, false
				, bitmap );
			auto image = bitmap.ConvertToImage();
			auto folder = m_filePath.getPath() / cuT( "Compare" );

			if ( !castor::File::directoryExists( folder ) )
			{
				castor::File::directoryCreate( folder );
			}

			Path outputPath = folder / ( m_filePath.getFileName() + cuT( "_" ) + suffix + cuT( ".png" ) );
			image.SaveFile( wxString( outputPath ) );
		}

		return result;
	}

	void MainFrame::cleanup()
	{
		try
		{
			m_renderWindow->cleanup();
		}
		catch ( ... )
		{
		}

		m_renderWindow.reset();
		m_engine.cleanup();
	}
}

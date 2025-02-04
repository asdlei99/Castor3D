#include "Castor3D/Render/RenderLoopAsync.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Render/RenderSystem.hpp"

#include <CastorUtils/Miscellaneous/PreciseTimer.hpp>
#include <CastorUtils/Design/ScopeGuard.hpp>
#include <CastorUtils/Design/BlockGuard.hpp>

namespace castor3d
{
	namespace rendlpasnc
	{
		static const char * CALL_RENDER_SYNC_FRAME = "Can't call renderSyncFrame in threaded render loop";
		static const char * CALL_PAUSE_RENDERING = "Can't call Pause on a paused render loop";
		static const char * CALL_RESUME_RENDERING = "Can't call Resume on a non paused render loop";
	}

	RenderLoopAsync::RenderLoopAsync( Engine & engine, uint32_t wantedFPS )
		: RenderLoop{ engine, wantedFPS, true }
		, m_mainLoopThread{ nullptr }
		, m_ended{ false }
		, m_rendering{ false }
		, m_paused{ false }
		, m_interrupted{ false }
	{
		m_mainLoopThread = std::make_unique< std::thread >( [this]()
		{
			doMainLoop();
		} );
	}

	RenderLoopAsync::~RenderLoopAsync()
	{
		// We wait for the main loop to end (it makes a final render to clean the render system)
		m_interrupted = true;
		m_mainLoopThread->join();
		m_mainLoopThread.reset();
	}

	bool RenderLoopAsync::isRendering()const
	{
		return m_rendering;
	}

	bool RenderLoopAsync::isInterrupted()const
	{
		return m_interrupted;
	}

	bool RenderLoopAsync::isEnded()const
	{
		return m_ended;
	}

	bool RenderLoopAsync::isPaused()const
	{
		return m_paused;
	}

	void RenderLoopAsync::enableVSync( bool enable )
	{
		using type = decltype( m_savedTime );
		static auto zero = type::zero();

		if ( enable && m_savedTime != zero )
		{
			m_frameTime = m_savedTime;
			m_savedTime = zero;
		}
		else if ( m_savedTime == zero )
		{
			m_savedTime = m_frameTime;
			m_frameTime = type{ 1 };
		}
	}

	void RenderLoopAsync::beginRendering()
	{
		m_rendering = true;
	}

	void RenderLoopAsync::renderSyncFrame( castor::Milliseconds tslf )
	{
		if ( !m_paused )
		{
			CU_Exception( rendlpasnc::CALL_RENDER_SYNC_FRAME );
		}

		if ( m_rendering )
		{
			doRenderFrame( tslf );
		}
	}

	void RenderLoopAsync::pause()
	{
		if ( m_paused )
		{
			CU_Exception( rendlpasnc::CALL_PAUSE_RENDERING );
		}

		m_paused = true;

		while ( !m_frameEnded )
		{
			castor::System::sleep( 5 );
		}
	}

	void RenderLoopAsync::resume()
	{
		if ( !m_paused )
		{
			CU_Exception( rendlpasnc::CALL_RESUME_RENDERING );
		}

		m_paused = false;
	}

	void RenderLoopAsync::endRendering()
	{
		m_rendering = false;

		while ( !isEnded() )
		{
			castor::System::sleep( 5 );
		}
	}

	void RenderLoopAsync::doMainLoop()
	{
		castor::PreciseTimer timer;
		m_frameEnded = true;
		auto scopeGuard{ castor::makeScopeGuard( [this]()
		{
			cleanup();
		} ) };

#if defined( NDEBUG )

		auto tslf = 0_ms;

#else

		auto tslf = 25_ms;

#endif

		try
		{
			// Tant qu'on n'est pas interrompu, on continue
			while ( !isInterrupted() )
			{
				// Tant qu'on n'a pas demandé le début du rendu, on attend.
				while ( !isInterrupted() && !isRendering() )
				{
					castor::System::sleep( 10 );
				}

				// Le rendu est en cours
				while ( !isInterrupted() && isRendering() )
				{
					timer.getElapsed();

					if ( !isPaused() )
					{
						m_frameEnded = false;
						doRenderFrame( tslf );
						m_frameEnded = true;
					}

					auto endTime = std::chrono::duration_cast< castor::Milliseconds >( timer.getElapsed() );
					std::this_thread::sleep_for( std::max( 0_ms, getFrameTime() - endTime ) );
				}

				m_ended = true;
			}
		}
		catch ( castor::Exception & exc )
		{
			log::error << cuT( "RenderLoop - " ) << exc.getFullDescription() << std::endl;
			m_frameEnded = true;
			m_ended = true;
		}
		catch ( std::exception & exc )
		{
			log::error << std::string( "RenderLoop - " ) + exc.what() << std::endl;
			m_frameEnded = true;
			m_ended = true;
		}
	}
}

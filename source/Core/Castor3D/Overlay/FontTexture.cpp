#include "Castor3D/Overlay/FontTexture.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/GpuBuffer.hpp"
#include "Castor3D/Event/Frame/CpuFunctorEvent.hpp"
#include "Castor3D/Event/Frame/GpuFunctorEvent.hpp"
#include "Castor3D/Material/Texture/Sampler.hpp"
#include "Castor3D/Material/Texture/TextureLayout.hpp"
#include "Castor3D/Shader/ShaderBuffers/FontGlyphBuffer.hpp"

#include <CastorUtils/Design/ResourceCache.hpp>
#include <CastorUtils/Graphics/Font.hpp>
#include <CastorUtils/Graphics/Image.hpp>

CU_ImplementSmartPtr( castor3d, FontTexture )

namespace castor3d
{
	//*********************************************************************************************

	namespace fonttex
	{
		static TextureLayoutUPtr createTexture( Engine & engine, castor::FontResPtr font )
		{
			if ( !font )
			{
				CU_Exception( "No Font given to FontTexture" );
			}

			uint32_t const maxWidth = font->getMaxWidth();
			uint32_t const maxHeight = font->getMaxHeight();
			uint32_t const count = castor::divRoundUp( uint32_t( std::distance( font->begin(), font->end() ) ), 16u );

			ashes::ImageCreateInfo image{ 0u
				, VK_IMAGE_TYPE_2D
				, VK_FORMAT_R8_UNORM
				, { maxWidth * 16, maxHeight * count, 1u }
				, 1u
				, 1u
				, VK_SAMPLE_COUNT_1_BIT
				, VK_IMAGE_TILING_OPTIMAL
				, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT };
			return castor::makeUnique< TextureLayout >( *engine.getRenderSystem()
				, image
				, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				, cuT( "FontTexture_" ) + font->getFaceName() + castor::string::toString( font->getHeight() ) );
		}
	}

	//*********************************************************************************************

	void postPreRenderGpuEvent( Engine & engine
		, std::function< void( RenderDevice const &, QueueData const & ) > event )
	{
		engine.postEvent( makeGpuFunctorEvent( GpuEventType::ePreUpload, event ) );
	}

	void postQueueRenderCpuEvent( Engine & engine
		, std::function< void() > event )
	{
		engine.postEvent( makeCpuFunctorEvent( CpuEventType::ePreCpuStep, event ) );
	}

	//*********************************************************************************************

	FontTexture::FontTexture( Engine & engine, castor::FontResPtr font )
		: DoubleBufferedTextureLayout{ engine
			, fonttex::createTexture( engine, font )
			, fonttex::createTexture( engine, font ) }
		, m_font( font )
		, m_buffer{ castor::makeUnique< FontGlyphBuffer >( engine
			, *engine.getRenderDevice()
			, *this
			, MaxCharsPerBuffer ) }
	{
		if ( !m_font )
		{
			CU_Exception( "No Font given to FontTexture" );
		}

		if ( auto sampler = getEngine()->addNewSampler( m_font->getName(), *getEngine() ) )
		{
			sampler->setWrapS( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
			sampler->setWrapT( VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
			sampler->setMinFilter( VK_FILTER_LINEAR );
			sampler->setMagFilter( VK_FILTER_LINEAR );
			m_sampler = sampler;
		}
	}

	FontTexture::~FontTexture()noexcept
	{
	}

	void FontTexture::initialise( RenderDevice const & device
		, QueueData const & queueData )
	{
		doInitialise( device, queueData );
		onResourceChanged( *this );
	}

	void FontTexture::cleanup( RenderDevice const & device )
	{
		doCleanup();
	}

	void FontTexture::upload( UploadData & uploader )
	{
		auto & resource = doGetResource();

		if ( resource.needsUpload )
		{
			resource.resource->upload( uploader );
			resource.needsUpload = false;
		}

		m_buffer->update( uploader );
	}

	castor::UInt32Array FontTexture::convert( castor::U32String const & text )const
	{
		castor::UInt32Array result;
		result.resize( text.size() );
		auto defaultIt = m_charIndices.find( U'?' );

		if ( defaultIt != m_charIndices.end() )
		{
			auto dst = result.begin();
			for ( auto c : text )
			{
				auto it = m_charIndices.find( c );
				*dst = ( it != m_charIndices.end()
					? it->second
					: defaultIt->second );
				++dst;
			}
		}

		return result;
	}

	castor::String const & FontTexture::getFontName()const
	{
		return getFont()->getName();
	}

	castor::Position const & FontTexture::getGlyphPosition( char32_t glyphChar )const
	{
		auto it = m_frontGlyphsPositions.find( glyphChar );

		if ( it == m_frontGlyphsPositions.end() )
		{
			return getGlyphPosition( U'?' );
		}

		return it->second;
	}

	void FontTexture::initialiseResource( Resource & resource
		, RenderDevice const & device
		, QueueData const & queueData )
	{
		resource.resource->initialise( device );
	}

	void FontTexture::cleanupResource( Resource & resource )
	{
		resource.resource->cleanup();
	}

	void FontTexture::updateResource( Resource & resource
		, bool front )
	{
		if ( auto font = getFont() )
		{
			auto & glyphPositions = front ? m_frontGlyphsPositions : m_backGlyphsPositions;
			uint32_t const maxWidth = font->getMaxWidth();
			uint32_t const maxHeight = font->getMaxHeight();
			uint32_t const count = castor::divRoundUp( uint32_t( std::distance( font->begin(), font->end() ) ), 16u );
			castor::Size size{ maxWidth * 16, maxHeight * count };
			m_buffer->setMaxHeight( font->getMaxHeight() );
			m_buffer->setImgWidth( size.getWidth() );
			m_buffer->setImgHeight( size.getHeight() );
			resource.resource->setSource( castor::PxBufferBase::create( castor::Size( maxWidth * 16, maxHeight * count )
				, castor::PixelFormat::eR8_UNORM ), true );
			auto & image = resource.resource->getImage();

			auto it = font->begin();
			castor::Size const & sizeImg = size;
			uint32_t const imgLineSize = sizeImg.getWidth();
			uint32_t offY = sizeImg.getHeight() - maxHeight;
			auto buffer = image.getBuffer();
			uint8_t * dstBuffer = buffer.data();

			for ( uint32_t y = 0; y < count && it != font->end(); ++y )
			{
				uint32_t offX = 0;

				for ( uint32_t x = 0; x < 16 && it != font->end(); ++x )
				{
					castor::Glyph const & glyph = *it;
					castor::Size const & glyphSize = glyph.getSize();
					auto srcGlyphBuffer = glyph.getBitmap().data();
					uint32_t dstGlyphIndex = ( imgLineSize * offY ) + offX;
					uint8_t * dstGlyphBuffer = &dstBuffer[dstGlyphIndex];

					for ( uint32_t i = 0; i < glyphSize.getHeight(); ++i )
					{
						CU_Ensure( size_t( dstGlyphIndex ) + glyphSize.getWidth() <= buffer.size() );
						std::memcpy( dstGlyphBuffer, srcGlyphBuffer, glyphSize.getWidth() );
						dstGlyphBuffer += imgLineSize;
						dstGlyphIndex += imgLineSize;
						srcGlyphBuffer += glyphSize.getWidth();
					}

					glyphPositions[glyph.getCharacter()] = castor::Position( int32_t( offX ), int32_t( offY ) );
					offX += maxWidth;
					++it;

					auto ires = m_charIndices.emplace( glyph.getCharacter(), uint32_t( m_charIndices.size() ) );

					if ( ires.second )
					{
						m_buffer->add( glyph );
					}
				}

				offY -= maxHeight;
			}

			resource.needsUpload = true;
		}
	}

	void FontTexture::swapResources()
	{
		std::swap( m_frontGlyphsPositions, m_backGlyphsPositions );
	}

	//*********************************************************************************************
}

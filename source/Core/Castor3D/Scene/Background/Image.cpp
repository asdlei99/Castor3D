#include "Castor3D/Scene/Background/Image.hpp"

#include "Castor3D/Engine.hpp"
#include "Castor3D/Buffer/DirectUploadData.hpp"
#include "Castor3D/Buffer/InstantUploadData.hpp"
#include "Castor3D/Buffer/UploadData.hpp"
#include "Castor3D/Material/Texture/TextureLayout.hpp"
#include "Castor3D/Scene/Camera.hpp"
#include "Castor3D/Scene/Scene.hpp"
#include "Castor3D/Scene/SceneNode.hpp"
#include "Castor3D/Scene/Background/Visitor.hpp"
#include "Castor3D/Scene/Background/Shaders/GlslImgBackground.hpp"

#include <CastorUtils/Data/TextWriter.hpp>

CU_ImplementSmartPtr( castor3d, ImageBackground )

namespace castor
{
	using namespace castor3d;

	template<>
	class TextWriter< ImageBackground >
		: public TextWriterT< ImageBackground >
	{
	public:
		explicit TextWriter( String const & tabs
			, Path const & folder )
			: TextWriterT< ImageBackground >{ tabs }
			, m_folder{ folder }
		{
		}

		bool operator()( ImageBackground const & background
			, StringStream & file )override
		{
			log::info << tabs() << cuT( "Writing ImageBackground" ) << std::endl;
			return writeFile( file
				, "background_image"
				, background.getImagePath()
				, m_folder
				, cuT( "Textures" ) );
		}

	private:
		Path const & m_folder;
	};
}

namespace castor3d
{
	//************************************************************************************************

	namespace bgimage
	{
		static ashes::ImageCreateInfo doGetImageCreate( VkFormat format
			, castor::Size const & dimensions
			, bool attachment
			, uint32_t mipLevel = 1u )
		{
			return ashes::ImageCreateInfo
			{
				VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
				VK_IMAGE_TYPE_2D,
				format,
				{ dimensions.getWidth(), dimensions.getHeight(), 1u },
				mipLevel,
				6u,
				VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_TILING_OPTIMAL,
				( VK_IMAGE_USAGE_SAMPLED_BIT
					| VK_IMAGE_USAGE_TRANSFER_DST_BIT
					| VkImageUsageFlags( attachment
						? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						: VkImageUsageFlagBits( 0u ) ) ),
			};
		}
	}

	//************************************************************************************************

	ImageBackground::ImageBackground( Engine & engine
		, Scene & scene
		, castor::String const & name )
		: SceneBackground{ engine, scene, name + cuT( "Image" ), cuT( "image" ), false }
	{
		m_texture = castor::makeUnique< TextureLayout >( *engine.getRenderSystem()
			, bgimage::doGetImageCreate( VK_FORMAT_R8G8B8A8_UNORM, { 16u, 16u }, false )
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, cuT( "ImageBackground_Dummy" ) );
	}

	void ImageBackground::accept( BackgroundVisitor & visitor )
	{
		visitor.visit( *this );
	}

	void ImageBackground::accept( ConfigurationVisitorBase & visitor )
	{
	}

	bool ImageBackground::write( castor::String const & tabs
		, castor::Path const & folder
		, castor::StringStream & stream )const
	{
		return castor::TextWriter< ImageBackground >{ tabs, folder }( *this, stream );
	}

	bool ImageBackground::setImage( castor::Path const & folder, castor::Path const & relative )
	{
		bool result = false;

		try
		{
			ashes::ImageCreateInfo image{ 0u
				, VK_IMAGE_TYPE_2D
				, VK_FORMAT_UNDEFINED
				, { 1u, 1u, 1u }
				, 1u
				, 1u
				, VK_SAMPLE_COUNT_1_BIT
				, VK_IMAGE_TILING_OPTIMAL
				, ( VK_IMAGE_USAGE_TRANSFER_SRC_BIT
					| VK_IMAGE_USAGE_TRANSFER_DST_BIT ) };
			m_2dTexture = castor::makeUnique< TextureLayout >( *getScene().getEngine()->getRenderSystem()
				, std::move( image )
				, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				, cuT( "SkyboxBackground2D" ) );
			m_2dTexture->setSource( folder, relative, { false, false, false } );

			m_2dTexturePath = folder / relative;
			notifyChanged();
			result = true;
		}
		catch ( castor::Exception & exc )
		{
			log::error << exc.what() << std::endl;
		}

		return result;
	}

	castor::String const & ImageBackground::getModelName()const
	{
		return shader::ImgBackgroundModel::Name;
	}

	bool ImageBackground::doInitialise( RenderDevice const & device )
	{
		doInitialise2DTexture( device );
		m_hdr = m_texture->getPixelFormat() == VK_FORMAT_R32_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R32G32_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R32G32B32_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R32G32B32A32_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R16_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R16G16_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R16G16B16_SFLOAT
			|| m_texture->getPixelFormat() == VK_FORMAT_R16G16B16A16_SFLOAT;
		m_srgb = isSRGBFormat( convert( m_texture->getPixelFormat() ) );
		return m_texture->initialise( device );
	}

	void ImageBackground::doCleanup()
	{
	}

	void ImageBackground::doCpuUpdate( CpuUpdater & updater )const
	{
		auto & viewport = *updater.viewport;
		viewport.resize( updater.camera->getSize() );
		viewport.setOrtho( -1.0f
			, 1.0f
			, -m_ratio
			, m_ratio
			, 0.1f
			, 2.0f );
		viewport.update();
		auto node = updater.camera->getParent();
		castor::Matrix4x4f view;
		castor::matrix::lookAt( view
			, node->getDerivedPosition()
			, node->getDerivedPosition() + castor::Point3f{ 0.0f, 0.0f, 1.0f }
			, castor::Point3f{ 0.0f, 1.0f, 0.0f } );
		updater.bgMtxView = view;
		updater.bgMtxProj = updater.isSafeBanded
			? viewport.getSafeBandedProjection()
			: viewport.getProjection();
	}

	void ImageBackground::doGpuUpdate( GpuUpdater & updater )const
	{
	}

	void ImageBackground::doUpload( UploadData & uploader )
	{
	}

	void ImageBackground::doAddPassBindings( crg::FramePass & pass
		, crg::ImageViewIdArray const & targetImage
		, uint32_t & index )const
	{
		pass.addSampledView( m_textureId.wholeViewId
			, index++
			, crg::SamplerDesc{ VK_FILTER_LINEAR
				, VK_FILTER_LINEAR
				, VK_SAMPLER_MIPMAP_MODE_LINEAR } );
	}

	void ImageBackground::doAddBindings( ashes::VkDescriptorSetLayoutBindingArray & bindings
		, VkShaderStageFlags shaderStages
		, uint32_t & index )const
	{
		bindings.emplace_back( makeDescriptorSetLayoutBinding( index++
			, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			, shaderStages ) );	// c3d_mapBackground
	}

	void ImageBackground::doAddDescriptors( ashes::WriteDescriptorSetArray & descriptorWrites
		, crg::ImageViewIdArray const & targetImage
		, uint32_t & index )const
	{
		bindTexture( m_textureId.wholeView
			, *m_textureId.sampler
			, descriptorWrites
			, index );
	}

	void ImageBackground::doInitialise2DTexture( RenderDevice const & device )
	{
		auto data = device.graphicsData();
		auto & queueData = *data;
		m_2dTexture->initialise( device );
		{
			auto & image = m_2dTexture->getImage();
			auto & texture = m_2dTexture->getTexture();
			InstantDirectUploadData upload{ *queueData.queue
				, device
				, image.getName()
				, *queueData.commandPool };
			upload->pushUpload( image.getPxBuffer().getConstPtr()
				, image.getPxBuffer().getSize()
				, texture
				, image.getLayout()
				, { VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, image.getLayout().depthLayers() }
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, VK_PIPELINE_STAGE_TRANSFER_BIT );
		}

		VkExtent3D extent{ m_2dTexture->getWidth(), m_2dTexture->getHeight(), 1u };
		auto dim = std::max( extent.width, extent.height );

		// create the cube texture if needed.
		if ( m_texture->getDimensions().width != dim
			|| m_texture->getDimensions().height != dim )
		{
			m_ratio = float( extent.height ) / float( extent.width );
			m_textureId = Texture{ device
				, getScene().getResources()
				, cuT( "ImageBackgroundCube" )
				, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
				, { dim, dim, 1u }
				, 6u
				, 1u
				, m_2dTexture->getPixelFormat()
				, ( VK_IMAGE_USAGE_SAMPLED_BIT
					| VK_IMAGE_USAGE_TRANSFER_DST_BIT ) };
			m_textureId.create();
			m_texture = castor::makeUnique< TextureLayout >( device.renderSystem
				, cuT( "ImageBackgroundCube" )
				, *m_textureId.image
				, m_textureId.wholeViewId );
		}

		auto xOffset = ( dim - extent.width ) / 2u;
		auto yOffset = ( dim - extent.height ) / 2u;
		VkOffset3D const srcOffset{ 0, 0, 0 };
		VkOffset3D const dstOffset{ int32_t( xOffset ), int32_t( yOffset ), 0 };
		VkImageSubresourceLayers srcSubresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		VkImageSubresourceLayers dstSubresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

		VkImageCopy copyInfos[6];
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::ePositiveX );
		copyInfos[uint32_t( CubeMapFace::ePositiveX )].dstOffset = dstOffset;

		copyInfos[uint32_t( CubeMapFace::eNegativeX )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::eNegativeX )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeX )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::eNegativeX )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeX )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::eNegativeX );
		copyInfos[uint32_t( CubeMapFace::eNegativeX )].dstOffset = dstOffset;

		copyInfos[uint32_t( CubeMapFace::ePositiveY )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::ePositiveY )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveY )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::ePositiveY )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveY )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::ePositiveY );
		copyInfos[uint32_t( CubeMapFace::ePositiveY )].dstOffset = dstOffset;

		copyInfos[uint32_t( CubeMapFace::eNegativeY )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::eNegativeY )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeY )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::eNegativeY )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeY )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::eNegativeY );
		copyInfos[uint32_t( CubeMapFace::eNegativeY )].dstOffset = dstOffset;

		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::ePositiveZ );
		copyInfos[uint32_t( CubeMapFace::ePositiveZ )].dstOffset = dstOffset;

		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].extent = extent;
		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].srcSubresource = srcSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].srcOffset = srcOffset;
		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].dstSubresource = dstSubresource;
		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].dstSubresource.baseArrayLayer = uint32_t( CubeMapFace::eNegativeZ );
		copyInfos[uint32_t( CubeMapFace::eNegativeZ )].dstOffset = dstOffset;

		auto commandBuffer = queueData.commandPool->createCommandBuffer( "ImageBackground" );
		commandBuffer->begin();
		uint32_t index{ 0u };

		for ( auto & copyInfo : copyInfos )
		{
			commandBuffer->memoryBarrier( VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
				, VK_PIPELINE_STAGE_TRANSFER_BIT
				, m_texture->getLayerCubeFaceTargetView( 0, CubeMapFace( index ) ).makeTransferDestination( VK_IMAGE_LAYOUT_UNDEFINED ) );
			commandBuffer->copyImage( copyInfo
				, m_2dTexture->getTexture()
				, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				, m_texture->getTexture()
				, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
			commandBuffer->memoryBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT
				, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				, m_texture->getLayerCubeFaceTargetView( 0, CubeMapFace( index ) ).makeShaderInputResource( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ) );
			++index;
		}

		commandBuffer->end();

		queueData.queue->submit( *commandBuffer, nullptr );
		queueData.queue->waitIdle();

		m_2dTexture->cleanup();
	}

	//************************************************************************************************
}

#include <fwog/Common.h>
#include <fwog/detail/ApiToEnum.h>
#include <fwog/detail/SamplerCache.h>
#include <fwog/Texture.h>
#include <utility>
#include <array>

#define MAX_NAME_LEN 256

namespace Fwog
{
  // static objects
  // TODO: move initialization
  namespace
  {
    detail::SamplerCache sSamplerCache;
  }

  namespace // detail
  {
    void subImage(uint32_t texture, const TextureUpdateInfo& info)
    {
      // TODO: safety checks

      switch (info.dimension)
      {
      case UploadDimension::ONE:
        glTextureSubImage1D(texture, 
          info.level, 
          info.offset.width, 
          info.size.width, 
          detail::UploadFormatToGL(info.format), 
          detail::UploadTypeToGL(info.type), info.pixels);
        break;
      case UploadDimension::TWO:
        glTextureSubImage2D(texture, 
          info.level, 
          info.offset.width, 
          info.offset.height, 
          info.size.width, 
          info.size.height, 
          detail::UploadFormatToGL(info.format), 
          detail::UploadTypeToGL(info.type), 
          info.pixels);
        break;
      case UploadDimension::THREE:
        glTextureSubImage3D(texture, 
          info.level, 
          info.offset.width, 
          info.offset.height, 
          info.offset.depth, 
          info.size.width, 
          info.size.height, 
          info.size.depth, 
          detail::UploadFormatToGL(info.format), 
          detail::UploadTypeToGL(info.type), 
          info.pixels);
        break;
      }
    }

    void clearImage(uint32_t texture, const TextureClearInfo& info)
    {
      glClearTexSubImage(texture,
        info.level,
        info.offset.width,
        info.offset.height,
        info.offset.depth,
        info.size.width,
        info.size.height,
        info.size.depth,
        detail::UploadFormatToGL(info.format),
        detail::UploadTypeToGL(info.type),
        info.data);
    }
  }



  std::optional<Texture> Texture::Create(const TextureCreateInfo& createInfo, std::string_view name)
  {
    Texture texture;
    texture.createInfo_ = createInfo;
    glCreateTextures(detail::ImageTypeToGL(createInfo.imageType), 1, &texture.id_);

    switch (createInfo.imageType)
    {
    case ImageType::TEX_1D:
      glTextureStorage1D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width);
      break;
    case ImageType::TEX_2D:
      glTextureStorage2D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height);
      break;
    case ImageType::TEX_3D:
      glTextureStorage3D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth);
      break;
    case ImageType::TEX_1D_ARRAY:
      glTextureStorage2D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.arrayLayers);
      break;
    case ImageType::TEX_2D_ARRAY:
      glTextureStorage3D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height, createInfo.arrayLayers);
      break;
    case ImageType::TEX_CUBEMAP:
      glTextureStorage2D(texture.id_, createInfo.mipLevels, detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height);
      break;
      //case ImageType::TEX_CUBEMAP_ARRAY:
      //  ASSERT(false);
      //  break;
    case ImageType::TEX_2D_MULTISAMPLE:
      glTextureStorage2DMultisample(texture.id_, detail::SampleCountToGL(createInfo.sampleCount), detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height, GL_FALSE);
      break;
    case ImageType::TEX_2D_MULTISAMPLE_ARRAY:
      glTextureStorage3DMultisample(texture.id_, detail::SampleCountToGL(createInfo.sampleCount), detail::FormatToGL(createInfo.format), createInfo.extent.width, createInfo.extent.height, createInfo.arrayLayers, GL_FALSE);
      break;
    default:
      break;
    }

    if (!name.empty())
    {
      glObjectLabel(GL_TEXTURE, texture.id_, static_cast<GLsizei>(name.length()), name.data());
    }
    return texture;
  }

  Texture::Texture(Texture&& old) noexcept
  {
    id_ = std::exchange(old.id_, 0);
    createInfo_ = old.createInfo_;
  }

  Texture& Texture::operator=(Texture&& old) noexcept
  {
    if (&old == this) return *this;
    this->~Texture();
    id_ = std::exchange(old.id_, 0);
    createInfo_ = old.createInfo_;
    return *this;
  }

  Texture::~Texture()
  {
    glDeleteTextures(1, &id_);
  }

  std::optional<TextureView> Texture::View() const
  {
    return TextureView::Create(*this);
  }

  std::optional<TextureView> Texture::MipView(uint32_t level) const
  {
    TextureViewCreateInfo createInfo
    {
      .viewType = createInfo_.imageType,
      .format = createInfo_.format,
      .minLevel = level,
      .numLevels = 1,
      .minLayer = 0,
      .numLayers = createInfo_.arrayLayers
    };
    return TextureView::Create(createInfo, *this);
  }

  void Texture::SubImage(const TextureUpdateInfo& info)
  {
    subImage(id_, info);
  }

  void Texture::ClearImage(const TextureClearInfo& info)
  {
    clearImage(id_, info);
  }

  void Texture::GenMipmaps()
  {
    glGenerateTextureMipmap(id_);
  }



  std::optional<TextureView> TextureView::Create(const TextureViewCreateInfo& viewInfo, const Texture& texture, std::string_view name)
  {
    TextureView view;
    view.createInfo_ = texture.createInfo_;
    view.viewInfo_ = viewInfo;
    glGenTextures(1, &view.id_); // glCreateTextures does not work here
    glTextureView(view.id_, detail::ImageTypeToGL(viewInfo.viewType), texture.id_,
      detail::FormatToGL(viewInfo.format), viewInfo.minLevel,
      viewInfo.numLevels, viewInfo.minLayer,
      viewInfo.numLayers);
    if (!name.empty())
    {
      glObjectLabel(GL_TEXTURE, view.id_, static_cast<GLsizei>(name.length()), name.data());
    }
    return view;
  }

  std::optional<TextureView> TextureView::Create(const TextureViewCreateInfo& viewInfo, const TextureView& textureView, std::string_view name)
  {
    auto view = Create(viewInfo, textureView, name);
    view->createInfo_ = TextureCreateInfo{
      .imageType = textureView.viewInfo_.viewType,
      .format = textureView.viewInfo_.format,
      .extent = textureView.createInfo_.extent,
      .mipLevels = textureView.viewInfo_.numLevels,
      .arrayLayers = textureView.viewInfo_.numLayers
    };
    return view;
  }

  std::optional<TextureView> TextureView::Create(const Texture& texture, std::string_view name)
  {
    TextureViewCreateInfo createInfo
    {
      .viewType = texture.createInfo_.imageType,
      .format = texture.createInfo_.format,
      .minLevel = 0,
      .numLevels = texture.createInfo_.mipLevels,
      .minLayer = 0,
      .numLayers = texture.createInfo_.arrayLayers
    };
    return Create(createInfo, texture, name);
  }

  TextureView::TextureView(TextureView&& old) noexcept
    : Texture(std::move(old))
  {
    viewInfo_ = old.viewInfo_;
  }

  TextureView& TextureView::operator=(TextureView&& old) noexcept
  {
    if (&old == this) return *this;
    this->~TextureView();
    viewInfo_ = old.viewInfo_;
    return static_cast<TextureView&>(Texture::operator=(std::move(old)));
  }

  TextureView::~TextureView()
  {
    glDeleteTextures(1, &id_);
  }



  std::optional<Sampler> Sampler::Create(const SamplerState& samplerState)
  {
    return sSamplerCache.CreateOrGetCachedTextureSampler(samplerState);
  }



  std::optional<Texture> CreateTexture2D(Extent2D size, Format format, std::string_view name)
  {
    TextureCreateInfo createInfo
    {
      .imageType = ImageType::TEX_2D,
      .format = format,
      .extent = { size.width, size.height, 1 },
      .mipLevels = 1,
      .arrayLayers = 1,
      .sampleCount = SampleCount::SAMPLES_1
    };
    return Texture::Create(createInfo, name);
  }

  std::optional<Texture> CreateTexture2DMip(Extent2D size, Format format, uint32_t mipLevels, std::string_view name)
  {
    TextureCreateInfo createInfo
    {
      .imageType = ImageType::TEX_2D,
      .format = format,
      .extent = { size.width, size.height, 1 },
      .mipLevels = mipLevels,
      .arrayLayers = 1,
      .sampleCount = SampleCount::SAMPLES_1
    };
    return Texture::Create(createInfo, name);
  }
}
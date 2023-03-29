#pragma once
#include <Fwog/Config.h>
#include <Fwog/BasicTypes.h>
#include <cstdint>
#include <string_view>

namespace Fwog
{
  namespace detail
  {
    class SamplerCache;
  }

  class TextureView;
  class Sampler;

  /// @brief Parameters for the constructor of Texture
  struct TextureCreateInfo
  {
    ImageType imageType = {};
    Format format = {};
    Extent3D extent = {};
    uint32_t mipLevels = 0;
    uint32_t arrayLayers = 0;
    SampleCount sampleCount = {};

    bool operator==(const TextureCreateInfo&) const noexcept = default;
  };

  /// @brief Parameters for the constructor of TextureView
  struct TextureViewCreateInfo
  {
    /// @note Must be an image type compatible with the base texture as defined by table 8.21 in the OpenGL spec
    ImageType viewType = {};
    /// @note Must be a format compatible with the base texture as defined by table 8.22 in the OpenGL spec
    Format format = {};
    uint32_t minLevel = 0;
    uint32_t numLevels = 0;
    uint32_t minLayer = 0;
    uint32_t numLayers = 0;
  };

  /// @brief Parameters for Texture::SubImage
  struct TextureUpdateInfo
  {
    UploadDimension dimension = {};
    uint32_t level = 0;
    Extent3D offset = {};
    Extent3D size = {};
    UploadFormat format = {};
    UploadType type = {};
    const void* pixels = nullptr;
  };

  /// @brief Parameters for Texture::ClearImage
  struct TextureClearInfo
  {
    uint32_t level = 0;
    Extent3D offset = {};
    Extent3D size = {};
    UploadFormat format = {};
    UploadType type = {};
    /// @brief If null, then the subresource will be cleared with zeroes
    const void* data = nullptr;
  };

  /// @brief Parameters for the constructor of Sampler
  struct SamplerState
  {
    bool operator==(const SamplerState& rhs) const noexcept = default;

    float lodBias{0};
    float minLod{-1000};
    float maxLod{1000};

    Filter minFilter = Filter::LINEAR;
    Filter magFilter = Filter::LINEAR;
    Filter mipmapFilter = Filter::NONE;
    AddressMode addressModeU = AddressMode::CLAMP_TO_EDGE;
    AddressMode addressModeV = AddressMode::CLAMP_TO_EDGE;
    AddressMode addressModeW = AddressMode::CLAMP_TO_EDGE;
    BorderColor borderColor = BorderColor::FLOAT_OPAQUE_WHITE;
    SampleCount anisotropy = SampleCount::SAMPLES_1;
    bool compareEnable = false;
    CompareOp compareOp = CompareOp::NEVER;
  };

  /// @brief Encapsulates an immutable OpenGL texture
  class Texture
  {
  public:
    /// @brief Constructs the texture
    /// @param createInfo Parameters to construct the texture
    /// @param name An optional name for viewing the resource in a graphics debugger
    explicit Texture(const TextureCreateInfo& createInfo, std::string_view name = "");
    Texture(Texture&& old) noexcept;
    Texture& operator=(Texture&& old) noexcept;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    virtual ~Texture();

    /// @todo Remove
    bool operator==(const Texture&) const noexcept = default;

    /// @brief Updates a subresource of the image
    /// @param info The subresource and data to upload
    void SubImage(const TextureUpdateInfo& info);

    /// @brief Clears a subresource of the image to a specified value
    /// @param info The subresource and value to clear it with
    void ClearImage(const TextureClearInfo& info);

    /// @brief Automatically generates LoDs of the image. All mip levels beyond 0 are filled with the generated LoDs
    void GenMipmaps();

    /// @brief Creates a view of a single mip level of the image
    [[nodiscard]] TextureView CreateSingleMipView(uint32_t level) const;

    /// @brief Creates a view of a single array layer of the image
    [[nodiscard]] TextureView CreateSingleLayerView(uint32_t layer) const;

    /// @brief Reinterpret the data of this texture
    /// @param newFormat The format to reinterpret the data as
    /// @return A new texture view
    [[nodiscard]] TextureView CreateFormatView(Format newFormat) const;

    /// @brief Generates and makes resident a bindless handle from the image and a sampler. Only available if GL_ARB_bindless_texture is supported
    /// @param sampler The sampler to bind to the texture
    /// @return A bindless texture handle that can be placed in a buffer and used to construct a combined texture sampler in a shader
    /// @todo Improve this
    [[nodiscard]] uint64_t GetBindlessHandle(Sampler sampler);

    [[nodiscard]] const TextureCreateInfo& CreateInfo() const
    {
      return createInfo_;
    }

    [[nodiscard]] Extent3D Extent() const
    {
      return createInfo_.extent;
    }

    /// @brief Gets the handle of the underlying OpenGL texture object
    /// @return The texture
    [[nodiscard]] uint32_t Handle() const
    {
      return id_;
    }

  protected:
    Texture();
    uint32_t id_{};
    TextureCreateInfo createInfo_{};
    uint64_t bindlessHandle_ = 0;
  };

  // TODO: implement
  //class ColorTexture : public Texture
  //{
  //public:
  //  // Should this constructor take a version of TextureCreateInfo that uses a more constrained format enum?
  //  explicit ColorTexture()
  //};

  //class DepthStencilTexture : public Texture
  //{
  //public:
  //  // See comment for above class' constructor
  //  explicit DepthStencilTexture()
  //};

  /// @brief Encapsulates an OpenGL texture view
  class TextureView : public Texture
  {
  public:
    /// @brief Constructs the texture view with explicit parameters
    /// @param viewInfo Parameters to construct the texture
    /// @param texture A texture of which to construct a view
    /// @param name An optional name for viewing the resource in a graphics debugger
    explicit TextureView(const TextureViewCreateInfo& viewInfo, const Texture& texture, std::string_view name = "");
    explicit TextureView(const TextureViewCreateInfo& viewInfo,
                         const TextureView& textureView,
                         std::string_view name = "");

    // make a texture view with automatic parameters (view of whole texture, same type)
    explicit TextureView(const Texture& texture, std::string_view name = "");

    TextureView(TextureView&& old) noexcept;
    TextureView& operator=(TextureView&& old) noexcept;
    TextureView(const TextureView& other) = delete;
    TextureView& operator=(const TextureView& other) = delete;
    ~TextureView();

    [[nodiscard]] TextureViewCreateInfo ViewInfo() const
    {
      return viewInfo_;
    }

  private:
    TextureView();
    TextureViewCreateInfo viewInfo_{};
  };

  /// @brief Encapsulates an OpenGL sampler
  class Sampler
  {
  public:
    explicit Sampler(const SamplerState& samplerState);

    /// @brief Gets the handle of the underlying OpenGL sampler object
    /// @return The sampler
    [[nodiscard]] uint32_t Handle() const
    {
      return id_;
    }

  private:
    friend class detail::SamplerCache;
    Sampler(){}; // you cannot create samplers out of thin air
    explicit Sampler(uint32_t id) : id_(id){};

    uint32_t id_{};
  };

  // convenience functions
  Texture CreateTexture2D(Extent2D size, Format format, std::string_view name = "");
  Texture CreateTexture2DMip(Extent2D size, Format format, uint32_t mipLevels, std::string_view name = "");
} // namespace Fwog
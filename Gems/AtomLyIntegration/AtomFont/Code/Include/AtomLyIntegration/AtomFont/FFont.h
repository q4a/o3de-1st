/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Font class.


#pragma once


#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/Cry_Color.h>
#include <CryCommon/CryString.h>
#include "AtomFont.h"

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/containers/map.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/Image.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystemInterface.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <Atom/Bootstrap/DefaultWindowBus.h>
#include <Atom/Bootstrap/BootstrapNotificationBus.h>

struct ISystem;

namespace AZ
{
    class FontTexture;

    struct FontDeleter
    {
        void operator () (const AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>* ptr) const;
    };

    //! FFont is the implementation of IFFont used to draw text with a particular font (e.g. Consolas Italic)
    //! FFont manages creation of a gpu texture to cache the font and generates draw commands that use that texture.
    //! FFont's are managed by AtomFont as either individual font instances or a font family
    //! that collects all the variations (italic, bold, bold italic, normal).
    class FFont
        : public IFFont
        , public AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>
        , private AZ::Render::Bootstrap::NotificationBus::Handler
    {
        using ref_count = AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>;
        friend FontDeleter;
    public:
        using TextDrawContext = STextDrawContext;
        //! Determines how characters of different sizes should be handled during render.
        enum class SizeBehavior
        {
            Scale,      //!< Default behavior; glyphs rendered at different sizes are rendered on scaled geometry
            Rerender    //!< Similar to Scale, but the glyph in the font texture is re-rendered to match the target
                        //!< size, as long as the size isn't greater than the maximum glyph/slot resolution as
                        //!< configured for the font texture in the font XML.
        };

        //! The hinting visual algorithm to be used (when hinting is enabled)
        enum class HintStyle
        {
            Normal, //!< Default hinting behavior provided by font renderer
            Light   //!< Produces fuzzier glyphs but more accurately tracks glyph shape
        };

        //! Chooses whether hinting info should be obtained from the font, turned off entirely, or automatically generated
        enum class HintBehavior
        {
            Default,    //!< Obtain hinting data from font itself
            AutoHint,   //!< Procedurally derive hinting information from glyph
            NoHinting,  //!< Disable hinting entirely
        };

        //! Simple struct used to communicate font hinting parameters to font renderer.
        struct FontHintParams
        {
            FontHintParams()
                : hintStyle(HintStyle::Normal)
                , hintBehavior(HintBehavior::Default) { }

            HintStyle hintStyle;
            HintBehavior hintBehavior;
        };

        struct FontRenderingPass
        {
            ColorB m_color;
            Vec2 m_posOffset;
            int m_blendSrc;
            int m_blendDest;

            FontRenderingPass()
                : m_color(255, 255, 255, 255)
                , m_posOffset(0, 0)
                , m_blendSrc(GS_BLSRC_SRCALPHA)
                , m_blendDest(GS_BLDST_ONEMINUSSRCALPHA)
            {
            }

            void GetMemoryUsage([[maybe_unused]] ICrySizer* sizer) const {}
        };

        struct FontEffect
        {
            string m_name;
            std::vector<FontRenderingPass> m_passes;

            FontEffect(const char* name)
                : m_name(name)
            {
                assert(name);
            }

            FontRenderingPass* AddPass()
            {
                m_passes.push_back(FontRenderingPass());
                return &m_passes[m_passes.size() - 1];
            }

            void ClearPasses()
            {
                m_passes.resize(0);
            }

            void GetMemoryUsage([[maybe_unused]] ICrySizer* sizer) const {}
        };

        typedef std::vector<FontEffect> FontEffects;
        typedef FontEffects::iterator FontEffectsIterator;

        struct FontPipelineStateMapKey
        {
            AZ::RPI::SceneId m_sceneId;         // which scene pipeline state is attached to (via Render Pipeline)
            AZ::RHI::DrawListTag m_drawListTag; // which render pass this pipeline draws in by default

            bool operator<(const FontPipelineStateMapKey& other) const
            {
                return m_sceneId < other.m_sceneId
                       ||      (m_sceneId == other.m_sceneId && m_drawListTag < other.m_drawListTag);
            }
        };

        struct FontShaderData
        {
            const char* m_fontShaderFilepath;
            AZ::Data::Instance<AZ::RPI::Shader> m_fontShader;
            AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> m_perDrawSrgAsset;
            AZ::RPI::ShaderVariantKey m_shaderVariantKeyFallback;
            AZ::RHI::ShaderInputImageIndex m_imageInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;

            AZ::RPI::ShaderVariantStableId m_fontVariantStableId;

            AZ::RHI::DrawListTag m_drawListTag;
            AZStd::map<FontPipelineStateMapKey, AZ::RHI::ConstPtr<AZ::RHI::PipelineState>> m_pipelineStates;
        };


    public:
        /////////////////////////////////////////////////////////////////////////////////////////////////
        // IFFont interface
        int32_t AddRef() override;
        int32_t Release() override;
        bool Load(const char* fontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio) override;
        bool Load(const char* xmlFile) override;
        void Free() override;
        void DrawString(float x, float y, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        void DrawString(float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        Vec2 GetTextSize(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        size_t GetTextLength(const char* str, const bool asciiMultiLine) const override;
        void WrapText(string& result, float maxWidth, const char* str, const TextDrawContext& ctx) override;
        void GetMemoryUsage([[maybe_unused]] ICrySizer* sizer) const override {};
        void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const override;
        unsigned int GetEffectId(const char* effectName) const override;
        unsigned int GetNumEffects() const override;
        const char* GetEffectName(unsigned int effectId) const override;
        Vec2 GetMaxEffectOffset(unsigned int effectId) const override;
        bool DoesEffectHaveTransparency(unsigned int effectId) const override;
        void AddCharsToFontTexture(const char* chars, int glyphSizeX = ICryFont::defaultGlyphSizeX, int glyphSizeY = ICryFont::defaultGlyphSizeY) override;
        Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const TextDrawContext& ctx) const override;
        float GetAscender(const TextDrawContext& ctx) const override;
        float GetBaseline(const TextDrawContext& ctx) const override;
        float GetSizeRatio() const override;

        uint32_t GetNumQuadsForText(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        uint32_t WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16_t* indices, uint32_t maxQuads, float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx) override;
        int GetFontTextureId() override {return -1;} // old Cry Interface, disable
        uint32_t GetFontTextureVersion() override;
        /////////////////////////////////////////////////////////////////////////////////////////////////////


    public:
        FFont(AtomFont* atomFont, const char* fontName);

        FontTexture* GetFontTexture() const { return m_fontTexture; }
        const string& GetName() const { return m_name; }

        FontEffect* AddEffect(const char* effectName);
        FontEffect* GetDefaultEffect();

    private:
        virtual ~FFont();
        bool InitFont();
        bool InitTexture();
        bool InitCache();

        void Prepare(const char* str, bool updateTexture, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize);
        void DrawStringUInternal(float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx);
        Vec2 GetTextSizeUInternal(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx);

        // returns true if add operation was successful, false otherwise
        using AddFunction = AZStd::function<bool(const Vec3&, const Vec3&, const Vec3&, const Vec3&, const Vec2&, const Vec2&, const Vec2&, const Vec2&, uint32_t)>;

        //! This function is used by both DrawStringUInternal and WriteTextQuadsToBuffers
        //! To do this is takes a function pointer that implement the appropriate AddQuad behavior
        int CreateQuadsForText(
            float x,
            float y,
            float z,
            const char* str,
            const bool asciiMultiLine,
            const TextDrawContext& ctx,
            AddFunction AddQuad);

        struct TextScaleInfoInternal
        {
            TextScaleInfoInternal(const Vec2& _scale, float _rcpCellWidth)
                : scale(_scale)
                , rcpCellWidth(_rcpCellWidth) { }

            Vec2 scale;
            float rcpCellWidth;
        };

        TextScaleInfoInternal CalculateScaleInternal(const TextDrawContext& ctx) const;

        Vec2 GetRestoredFontSize(const TextDrawContext& ctx) const;

        bool UpdateTexture();

        void LoadShader(const char* shaderFilepath);

        void CommitDrawGeometry();
        void UpdateVertexBuffer();
        void MapVertexBuffer();
        void UpdateIndexBuffer();
        void MapIndexBuffer();

        void ScaleCoord(float& x, float& y) const;

        void InitWindowContext();
        void InitViewportContext();

        AZ::RHI::ConstPtr<AZ::RHI::PipelineState> GetPipelineState(const AZ::RPI::Scene* scene, AZ::RHI::DrawListTag drawListTag);

        void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;

    private:
        static constexpr uint32_t NumBuffers = 2;
        static constexpr float WindowScaleWidth = 800.0f;
        static constexpr float WindowScaleHeight = 600.0f;
        string m_name;
        string m_curPath;

        FontTexture* m_fontTexture = nullptr;

        size_t m_fontBufferSize = 0;
        unsigned char* m_fontBuffer = nullptr;

        AZStd::shared_ptr<RPI::WindowContext> m_windowContext;
        AZStd::shared_ptr<AZ::RPI::ViewportContext> m_viewportContext;

        AZ::Data::Instance<AZ::RPI::StreamingImage> m_fontStreamingImage;
        AZ::RHI::Ptr<AZ::RHI::Image>     m_fontImage;
        AZ::RHI::Ptr<const AZ::RHI::ImageView> m_fontImageView;
        uint32_t m_fontImageVersion = 0;

        AtomFont* m_atomFont;

        bool m_fontTexDirty = false;
        bool m_fontInitialized = false;

        FontEffects m_effects;

        // Atom data
        AZStd::mutex                        m_vertexDataMutex;
        AZ::RHI::Ptr<AZ::RHI::Buffer>       m_vertexBuffer[NumBuffers];
        AZ::RHI::StreamBufferView           m_streamBufferView[NumBuffers];
        SVF_P3F_C4B_T2F*                    m_mappedVertexPtr = nullptr;
        uint16_t                            m_vertexCount = 0;

        AZ::RHI::Ptr<AZ::RHI::Buffer>       m_indexBuffer[NumBuffers];
        AZ::RHI::IndexBufferView            m_indexBufferView[NumBuffers];
        uint16_t*                           m_mappedIndexPtr = nullptr;
        uint16_t                            m_indexCount = 0;

        AZ::RHI::Ptr<AZ::RHI::BufferPool>   m_inputAssemblyPool;

        AZStd::mutex                        m_pipelineStateCacheMutex;
        FontShaderData                      m_fontShaderData;
        AZ::RHI::DrawListTag                m_2DPassDrawListTag;

        AZ::RPI::DynamicDrawPreRenderNotificationHandler m_preRenderNotificationHandler;

        bool m_monospacedFont = false; //!< True if this font is fixed/monospaced, false otherwise (obtained from FreeType)

        float m_sizeRatio = IFFontConstants::defaultSizeRatio;
        SizeBehavior m_sizeBehavior = SizeBehavior::Scale;   //!< Changes how glyphs rendered at different sizes are rendered.
        FontHintParams m_fontHintParams; //!< How the font should be hinted when its loaded and rendered to the font texture

        AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_processSrgs[NumBuffers]; // remember srg's for 2 frames
        uint m_activeIndex = 0; // controls which vertex buffer & view, index buffer & view, and process srgs list is active

        static constexpr char LogName[] = "AtomFont::FFont";
    };

    inline float FFont::GetSizeRatio() const
    {
        return m_sizeRatio;
    }

    inline void FontDeleter::operator () (const AZStd::intrusive_refcount<AZStd::atomic_uint, FontDeleter>* ptr) const
    {
        /// Recover the mutable parent object pointer from the refcount base class.
        FFont* font = const_cast<FFont*>(static_cast<const FFont*>(ptr));
        if (font && font->m_atomFont)
        {
            font->m_atomFont->UnregisterFont(font->m_name);
        }

        delete font;
    }
}

inline void AZ::FFont::InitWindowContext()
{
    if (!m_windowContext)
    {
        // font is created before window & viewport in the editor so need to do late init
        // TODO need to deal with multiple windows, such as the editor
        AZ::Render::Bootstrap::DefaultWindowBus::BroadcastResult(m_windowContext, &AZ::Render::Bootstrap::DefaultWindowInterface::GetDefaultWindowContext);
        AZ_Assert(m_windowContext, "Unable to get the main window context");
    }
}

inline void AZ::FFont::InitViewportContext()
{
    if (!m_viewportContext)
    {
        // font is created before window & viewport in the editor so need to do late init
        auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        m_viewportContext = viewContextManager->GetViewportContextByName(viewContextManager->GetDefaultViewportContextName());
        AZ_Assert(m_viewportContext, "Unable to get the viewport context");
    }
}

#endif

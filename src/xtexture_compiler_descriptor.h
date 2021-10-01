namespace xtexture_compiler
{
    struct descriptor
    {
        enum class compression : std::uint8_t
        { DONT_COMPRESS
        , LEVEL_1
        , LEVEL_2
        , LEVEL_Z
        };

        enum class type : std::uint8_t
        { OTHER
        , COLOR         // Color 
        , CHANNEL       // Color Channels MASKs, Should be linear
        , NORMAL        // DXT5 compress (R,G,A), B set to black, should be linear
        , INTENSITY     // Things like MASKs (Data expected to be in R) Keep other channels black for better results. Should be linear.
        };

        enum class force_compression_format : std::uint8_t
        { INVALID
        , DXT1
        , DXT1_A
        , DXT3
        , DXT5
        };

        struct sources
        {
            struct paths
            {
                xcore::cstring      m_Color{};
                xcore::cstring      m_Alpha{};
            };

            xcore::vector<paths>        m_lPaths                {};
            type                        m_Type                  { type::COLOR };
            bool                        m_isCubeMap             { false };
            bool                        m_LinearSpace           { false };
            bool                        m_TextureCanWrapU       { false };
            bool                        m_TextureCanWrapV       { false };
        };

        struct manipulations
        {
            bool                        m_PackSrcTexturesAsOne  { false };
            bool                        m_bRemoveAlpha          { false };
        };

        struct quality
        {
            compression                 m_Compression             { compression::LEVEL_Z };
            force_compression_format    m_ForceCompressionFormat  { force_compression_format::INVALID };
            bool                        m_bForceAlphaTo1Bit       { false };
            bool                        m_GenerateMips            { true };
            bool                        m_bEnablePerceptualMetrics{ true };
            float                       m_LODBias                 { 0 };
        };

        sources         m_Source;
        manipulations   m_Manipulations;
        quality         m_Quality;
    };
}
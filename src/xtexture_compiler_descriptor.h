namespace xtexture_compiler
{
    struct descriptor : xresource_pipeline::descriptor::base
    {
        using parent = xresource_pipeline::descriptor::base;

        enum class compression : std::uint8_t
        { DONT_COMPRESS
        , LEVEL_1
        , LEVEL_2
        , LEVEL_Z
        };

        constexpr static auto compression_string_v = std::array
        { xcore::string::constant{"DONT_COMPRESS"}
        , xcore::string::constant{"LEVEL_1"}
        , xcore::string::constant{"LEVEL_2"}
        , xcore::string::constant{"LEVEL_Z"}
        };

        enum class type : std::uint8_t
        { OTHER
        , COLOR         // Color 
        , CHANNEL       // Color Channels MASKs, Should be linear
        , NORMAL        // DXT5 compress (R,G,A), B set to black, should be linear
        , INTENSITY     // Things like MASKs (Data expected to be in R) Keep other channels black for better results. Should be linear.
        };

        constexpr static auto type_string_v = std::array
        { xcore::string::constant{"OTHER"}
        , xcore::string::constant{"COLOR"}
        , xcore::string::constant{"CHANNEL"}
        , xcore::string::constant{"NORMAL"}
        , xcore::string::constant{"INTENSITY"}
        };

        enum class force_compression_format : std::uint8_t
        { INVALID
        , DXT1
        , DXT1_A
        , DXT3
        , DXT5
        };

        constexpr static auto force_compression_format_string_v = std::array
        { xcore::string::constant{"INVALID"}
        , xcore::string::constant{"DXT1"}
        , xcore::string::constant{"DXT1_A"}
        , xcore::string::constant{"DXT3"}
        , xcore::string::constant{"DXT5"}
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

        descriptor()
        {
            parent::m_Version.m_Major = 1;
            parent::m_Version.m_Minor = 0;
        }
        inline static xcore::err Serialize(descriptor& Options, std::string_view FilePath, bool isRead) noexcept;

        sources         m_Source;
        manipulations   m_Manipulations;
        quality         m_Quality;
    };

    //-------------------------------------------------------------------------------------------------------

    xcore::err descriptor::Serialize( descriptor& Options, std::string_view FilePath, bool isRead ) noexcept
    {
        xcore::textfile::stream Stream;
        xcore::err              Error;

        // Open file
        if( auto Err = Stream.Open(isRead, FilePath, xcore::textfile::file_type::TEXT, xcore::textfile::flags{ .m_isWriteFloats = true, .m_isWriteEndianSwap = false } ); Err )
            return Err;

        if( auto Err = Options.parent::Serialize( Stream, isRead ); Err )
            return Err;

        if (Stream.Record(Error, "Sources"
            , [&](std::size_t, xcore::err& Err)
            {   
                int  nPaths = static_cast<int>(Options.m_Source.m_lPaths.size());
                auto Type   = xcore::cstring(type_string_v[ (int)Options.m_Source.m_Type ] );
                0
                || (Err = Stream.Field("nPaths",                nPaths))
                || (Err = Stream.Field("Type",                  Type  ))
                || (Err = Stream.Field("isCubeMap",             Options.m_Source.m_isCubeMap))
                || (Err = Stream.Field("isLinearSpace",         Options.m_Source.m_LinearSpace))
                || (Err = Stream.Field("isTextureCanWrapU",     Options.m_Source.m_TextureCanWrapU))
                || (Err = Stream.Field("isTextureCanWrapV",     Options.m_Source.m_TextureCanWrapV))
                ;

                if( isRead )
                {
                    for( auto i=0u; i < type_string_v.size(); ++i )
                    {
                        if( Type == type_string_v[i] )
                        {
                            Options.m_Source.m_Type = (descriptor::type)i;
                            break;
                        }
                    }

                    Options.m_Source.m_lPaths.appendList(nPaths);
                }

            })) return Error;

        if( Stream.Record( Error, "Sources-Files"
            , [&]( std::size_t& Count, xcore::err& Err )
            {
                if( isRead ) xassert( Count == Options.m_Source.m_lPaths.size());
                else         Count = Options.m_Source.m_lPaths.size();
            }
            , [&]( std::size_t I, xcore::err& Err )
            {
                0
                || (Err = Stream.Field( "ColorSourcePath", Options.m_Source.m_lPaths[I].m_Color ))
                || (Err = Stream.Field( "AlphaSourcePath", Options.m_Source.m_lPaths[I].m_Alpha ))
                ;
            }) ) return Error;


        if (Stream.Record(Error, "Manipulations"
            , [&](std::size_t, xcore::err& Err)
            {
                0
                || (Err = Stream.Field("PackSrcTexturesAsOne",          Options.m_Manipulations.m_PackSrcTexturesAsOne))
                || (Err = Stream.Field("bRemoveAlpha",          Options.m_Manipulations.m_bRemoveAlpha))
                ;
            })) return Error;


        if (Stream.Record(Error, "Quality"
            , [&](std::size_t, xcore::err& Err)
            {
                auto ForceString        = xcore::cstring(force_compression_format_string_v[(int)Options.m_Quality.m_ForceCompressionFormat]);
                auto CompressionString  = xcore::cstring(compression_string_v[(int)Options.m_Quality.m_Compression]);

                0
                || (Err = Stream.Field("Compression",               CompressionString))
                || (Err = Stream.Field("ForceCompressionFormat",    ForceString))
                || (Err = Stream.Field("bForceAlphaTo1Bit",         Options.m_Quality.m_bForceAlphaTo1Bit))
                || (Err = Stream.Field("GenerateMips",              Options.m_Quality.m_GenerateMips))
                || (Err = Stream.Field("bEnablePerceptualMetrics",  Options.m_Quality.m_bEnablePerceptualMetrics))
                || (Err = Stream.Field("LODBias",                   Options.m_Quality.m_LODBias))
                ;

                if (isRead)
                {
                    
                    for(int i = 0; auto& E : force_compression_format_string_v)
                    {
                        if(ForceString == E )
                        {
                            Options.m_Quality.m_ForceCompressionFormat = static_cast<force_compression_format>(i);
                            break;
                        }
                        i++;
                    }

                    for (int i = 0; auto& E : compression_string_v)
                    {
                        if (CompressionString == E)
                        {
                            Options.m_Quality.m_Compression = static_cast<compression>(i);
                            break;
                        }
                        i++;
                    }
                }

            })) return Error;

        return {};
    }

}
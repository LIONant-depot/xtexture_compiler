
#include "../../dependencies/xbmp_tools/src/xbmp_tools.h"
#include "../../dependencies/crunch/inc/crnlib.h"

namespace xtexture_compiler {


//---------------------------------------------------------------------------------------------

struct implementation : instance
{
    //---------------------------------------------------------------------------------------------

    void LoadTexture( xcore::bitmap& Bitmap, const xcore::cstring& FilePath )
    {
        if( xcore::string::FindStrI( FilePath, ".dds" ) == 0 )
        {
            if( auto Err = xbmp::tools::loader::LoadDSS( Bitmap, FilePath ); Err )
                throw( std::runtime_error( xbmp::tools::getErrorMsg(Err) ) );
        }
        else
        {
            if( auto Err = xbmp::tools::loader::LoadSTDImage( Bitmap, FilePath ); Err )
                throw(std::runtime_error(xbmp::tools::getErrorMsg(Err)));
        }
    }

    //---------------------------------------------------------------------------------------------

    virtual void LoadImages( const xcore::cstring& AssetPath, const descriptor& Descriptor ) override
    {
        //
        // Load all images
        //
        for( auto& SrcPath : Descriptor.m_Source.m_lPaths )
        {
            auto& Bitmap = m_lSrcBitmaps.append();
            bool  bRead  = false;

            if (SrcPath.m_Color.empty() == false)
            {
                LoadTexture(Bitmap.m_SrcBitmap, xcore::string::Fmt("%s/%s", AssetPath.data(), SrcPath.m_Color.data()));
                bRead = true;
            }

            if( SrcPath.m_Alpha.empty() == false )
            {
                if( bRead == false ) throw(std::runtime_error("Source set without color is trying to read an alpha texture" ));
                LoadTexture(Bitmap.m_SrcAlpha, xcore::string::Fmt("%s/%s", AssetPath.data(), SrcPath.m_Alpha.data()));
            }

            if( bRead == false ) m_lSrcBitmaps.eraseCollapse( m_lSrcBitmaps.getIndexByEntry(Bitmap) );
        }

        if( m_lSrcBitmaps.size() == 0 ) 
            throw(std::runtime_error("Unable to load any textures"));
    }

    //---------------------------------------------------------------------------------------------

    void NormalizeBitmap( xcore::bitmap& Bitmap )
    {
        if( Bitmap.getFormat() == xcore::bitmap::format::R8G8B8A8 ) return;

        if( Bitmap.getFormat() != xcore::bitmap::format::R8G8B8 && Bitmap.getFormat() != xcore::bitmap::format::R5G6B5 )
            throw(std::runtime_error("Source texture has a strange format"));

        auto        ColorFmt        = xcore::color::format{ static_cast<xcore::color::format::type>(Bitmap.getFormat()) };
        auto&       Descriptor      = ColorFmt.getDescriptor();
        const auto  BypePerPixel    = Descriptor.m_TB / 8;
        std::byte*  pBitmapData     = Bitmap.getMip<std::byte>(0).data();
        auto        Data            = std::make_unique<xcore::icolor[]>( 1 + Bitmap.getHeight() * Bitmap.getWidth() );
        auto        pData           = &Data[1];

        Data[0].m_Value = 0;

        for( int y=0, end_y = Bitmap.getHeight(); y < end_y; ++y )
        for( int x=0, end_x = Bitmap.getWidth();  x < end_x; ++x )
        {
            const std::uint32_t D = *reinterpret_cast<const std::uint32_t*>(pBitmapData);
            *pData = xcore::icolor{ D, ColorFmt };

            pData++;
            pBitmapData += BypePerPixel;
        }   

        //
        // Setup the bitmap again
        //
        Bitmap.setup
        ( Bitmap.getWidth()
        , Bitmap.getHeight()
        , xcore::bitmap::format::R8G8B8A8
        , sizeof(xcore::icolor) * (Bitmap.getHeight() * Bitmap.getWidth())
        , { reinterpret_cast<std::byte*>(Data.release()), sizeof(xcore::icolor) * (1 + Bitmap.getHeight() * Bitmap.getWidth()) }
        , true
        , 1
        , 1
        );
    }

    //---------------------------------------------------------------------------------------------

    void NormalizeSrcTextures()
    {
        //
        // Make sure that all textures are the same size
        //
        {
            const auto W = m_lSrcBitmaps[0].m_SrcBitmap.getWidth();
            const auto H = m_lSrcBitmaps[0].m_SrcBitmap.getHeight();

            for (auto& SrcBitmap : m_lSrcBitmaps)
            {
                if( W != SrcBitmap.m_SrcBitmap.getWidth() || H != SrcBitmap.m_SrcBitmap.getHeight() )
                    throw(std::runtime_error("All color input textures should be the same size"));

                if( SrcBitmap.m_SrcAlpha.getFormat() != xcore::bitmap::format::INVALID )
                {
                    if (W != SrcBitmap.m_SrcAlpha.getWidth() || H != SrcBitmap.m_SrcAlpha.getHeight())
                        throw(std::runtime_error("All alpha input textures should be the same size as color"));
                }
            }
        }

        //
        // Make all texture the same format
        //
        for( auto& SrcBitmap : m_lSrcBitmaps )
        {
            NormalizeBitmap(SrcBitmap.m_SrcBitmap);
            if( SrcBitmap.m_SrcAlpha.getFormat() != xcore::bitmap::format::INVALID ) NormalizeBitmap(SrcBitmap.m_SrcAlpha);
        }

        //
        // Merge Alpha textures
        //
        for (auto& SrcBitmap : m_lSrcBitmaps)
        {
            if (SrcBitmap.m_SrcAlpha.getFormat() == xcore::bitmap::format::INVALID)
                continue;

            xcore::icolor* pDest = SrcBitmap.m_SrcBitmap.getMip<xcore::icolor>(0).data();
            xcore::icolor* pSrc = SrcBitmap.m_SrcAlpha.getMip<xcore::icolor>(0).data();
            for( int y=0, end_y = SrcBitmap.m_SrcBitmap.getHeight(); y < end_y; ++y )
            for( int x=0, end_x = SrcBitmap.m_SrcBitmap.getWidth();  x < end_x; ++x )
            {
                pDest->m_A = pSrc->m_A;
                pDest++;
                pSrc++;
            }

            // Free the alpha texture
            SrcBitmap.m_SrcAlpha.Kill();
        }
    }

    //---------------------------------------------------------------------------------------------

    void PackTextures()
    {
        xcore::icolor* pDst = m_lSrcBitmaps[0].m_SrcBitmap.getMip<xcore::icolor>(0).data();

        for( int i=1; i< m_lSrcBitmaps.size(); ++i )
        {
            const int Index = 4 - i;
            xcore::icolor* pSrc = m_lSrcBitmaps[i].m_SrcBitmap.getMip<xcore::icolor>(0).data();
            for( int y=0, end_y = m_lSrcBitmaps[0].m_SrcBitmap.getHeight(); y < end_y; ++y )
            for( int x=0, end_x = m_lSrcBitmaps[0].m_SrcBitmap.getWidth();  x < end_x; ++x )
            {
                pDst->m_A = (pDst->m_A & (0xff<< Index)) | ((pSrc->m_Value&0xff) << (Index * 8) );
            }
        }
    }

    //---------------------------------------------------------------------------------------------

    crn_format MatchForceCompressionFormat(descriptor::force_compression_format Fmt )
    {
        return crn_format::cCRNFmtDXT1;
    }

    //---------------------------------------------------------------------------------------------

    void CompressBitmaps( const descriptor::quality& CompressionOpt, const descriptor::type Type, xresource_pipeline::compiler::base::optimization_type Optimization)
    {
        if( CompressionOpt.m_Compression == descriptor::compression::DONT_COMPRESS )
        {
            return;
        }

        // Crunch the image data and return a pointer to the crunched result array
        crn_comp_params Params;
        Params.clear();
        Params.m_alpha_component        = m_lSrcBitmaps[0].m_SrcBitmap.hasAlphaChannel() && m_lSrcBitmaps[0].m_SrcBitmap.hasAlphaInfo();
        Params.m_format                 = CompressionOpt.m_ForceCompressionFormat != descriptor::force_compression_format::INVALID 
                                            ? MatchForceCompressionFormat(CompressionOpt.m_ForceCompressionFormat)
                                            : Params.m_alpha_component 
                                                ? CompressionOpt.m_bForceAlphaTo1Bit 
                                                    ? crn_format::cCRNFmtDXT1
                                                    : crn_format::cCRNFmtDXT3
                                                : cCRNFmtDXT1;
        Params.m_dxt_quality            = Optimization == xresource_pipeline::compiler::base::optimization_type::O0
                                            ? crn_dxt_quality::cCRNDXTQualityFast
                                            : Optimization == xresource_pipeline::compiler::base::optimization_type::O1
                                                ? crn_dxt_quality::cCRNDXTQualityBetter
                                                : Optimization == xresource_pipeline::compiler::base::optimization_type::Oz
                                                    ? crn_dxt_quality::cCRNDXTQualityUber
                                                    : crn_dxt_quality::cCRNDXTQualityFast;

        Params.m_width                  = m_lSrcBitmaps[0].m_SrcBitmap.getWidth();
        Params.m_height                 = m_lSrcBitmaps[0].m_SrcBitmap.getHeight();
        Params.m_file_type              = crn_file_type::cCRNFileTypeDDS;
        Params.m_num_helper_threads     = (CompressionOpt.m_Compression == descriptor::compression::LEVEL_2 
                                          || CompressionOpt.m_Compression == descriptor::compression::LEVEL_Z) 
                                              ? std::thread::hardware_concurrency() : std::thread::hardware_concurrency()/2;

        Params.m_faces = static_cast<std::uint32_t>(m_lSrcBitmaps.size());
        for (int i = 0; i < m_lSrcBitmaps.size(); ++i)
        {
            Params.m_pImages[i][0] = m_lSrcBitmaps[i].m_SrcBitmap.getMip<std::uint32_t>(0).data();
        }

        if(Type == descriptor::type::NORMAL)    Params.m_flags &= ~crn_comp_flags::cCRNCompFlagPerceptual;
        else if(CompressionOpt.m_bEnablePerceptualMetrics) Params.m_flags |= crn_comp_flags::cCRNCompFlagPerceptual;

        if(Type == descriptor::type::INTENSITY) Params.m_flags |= crn_comp_flags::cCRNCompFlagGrayscaleSampling;

        if( Params.check() == false )
            throw(std::runtime_error("Parameters for the compressor (crunch) failed."));

        crn_mipmap_params Mipmaps;
        Mipmaps.clear();
        {
            crn_uint32  CompressSize;
            crn_uint32  Actual_quality_level;   // Print stats
            float       Actual_bitrate;

            auto pData = crn_compress
            ( Params
            , Mipmaps
            , CompressSize
            , &Actual_quality_level
            , &Actual_bitrate
            );

            if( pData == nullptr )
                throw(std::runtime_error("The compressor (crunch) failed."));

            // Convert from DDS format to xcore::bitmap
            if( auto Err = xbmp::tools::loader::LoadDSS(m_FinalBitmap, {reinterpret_cast<std::byte*>(pData), CompressSize } ); Err )
                throw(std::runtime_error(xbmp::tools::getErrorMsg(Err)));
        }
    }

    //---------------------------------------------------------------------------------------------

    virtual void Compile( const descriptor& Descriptor, xresource_pipeline::compiler::base::optimization_type Optimization) override
    {
        NormalizeSrcTextures();

        // Check if infact is a cube map
        if (Descriptor.m_Source.m_isCubeMap )
        {
            if( m_lSrcBitmaps.size() != 6 )
            {
                if(m_lSrcBitmaps.size() > 6 ) throw(std::runtime_error("You specified you wanted a cube map but you added too many images"));
                else                          throw(std::runtime_error("You specified you wanted a cube map but you added too few images"));
            }
        }

        if (Descriptor.m_Manipulations.m_PackSrcTexturesAsOne )
        {
            if( m_lSrcBitmaps.size() > 4 ) 
                throw(std::runtime_error("We can not pack more than 4 textures into a single texture"));
            PackTextures();
        }

        CompressBitmaps(Descriptor.m_Quality, Descriptor.m_Source.m_Type, Optimization );

        m_FinalBitmap.setColorSpace( Descriptor.m_Source.m_LinearSpace ? xcore::bitmap::color_space::LINEAR : xcore::bitmap::color_space::SRGB );
        m_FinalBitmap.setCubemap( Descriptor.m_Source.m_isCubeMap );
    }

    //---------------------------------------------------------------------------------------------

    virtual void Serialize( const std::string_view FilePath ) override
    {
        if( auto Err = m_FinalBitmap.SerializeSave( xcore::string::To<wchar_t>(FilePath), false ); Err )
            throw( std::runtime_error(Err.getCode().m_pString));
    }

    //---------------------------------------------------------------------------------------------

    struct src_bitmaps
    {
        xcore::bitmap   m_SrcBitmap;
        xcore::bitmap   m_SrcAlpha;
    };

    xcore::vector<src_bitmaps>  m_lSrcBitmaps;
    xcore::bitmap               m_FinalBitmap;
};









//---------------------------------------------------------------------------------------------

std::unique_ptr<instance> MakeInstance()
{
    return std::make_unique<implementation>();
}


} // end of namespace xtexture_compiler
#include "../../Src/xtexture_compiler.h"

//---------------------------------------------------------------------------------

struct geom_pipeline_compiler : xresource_pipeline::compiler::base
{
    static constexpr xcore::guid::rcfull<> full_guid_v
    { .m_Type = xcore::guid::rctype<>         { "resource.pipeline", "plugin" }
    , .m_Instance = xcore::guid::rcinstance<> { "xtexture" }
    };

    virtual xcore::guid::rcfull<> getResourcePipelineFullGuid() const noexcept override
    {
        return full_guid_v;
    }

    virtual xcore::err onCompile(void) noexcept override
    {
        //if (auto Err = xtexture_compiler::descriptor::Serialize(m_CompilerOptions, m_ResourceDescriptorPathFile.data(), true); Err)
        //    return Err;

        m_Compiler->LoadImages( m_AssetsRootPath, m_CompilerOptions );
        m_Compiler->Compile(m_CompilerOptions);
        for (auto& T : m_Target)
        {
            if (T.m_bValid) m_Compiler->Serialize(T.m_DataPath.data());
        }
        return {};
    }

    xtexture_compiler::descriptor                  m_CompilerOptions{};
    std::unique_ptr<xtexture_compiler::instance>   m_Compiler       = xtexture_compiler::MakeInstance();
};

//---------------------------------------------------------------------------------

int main( int argc, const char* argv[] )
{
    xcore::Init("xtexture_compiler");

    auto GeomCompilerPipeline = std::make_unique<geom_pipeline_compiler>();

    //
    // Parse parameters
    //
    if( auto Err = GeomCompilerPipeline->Parse( argc, argv ); Err )
    {
        printf( "%s\nERROR: Fail to compile\n", Err.getCode().m_pString );
        return -1;
    }

    //
    // Start compilation
    //
    if( auto Err = GeomCompilerPipeline->Compile(); Err )
    {
        printf("%s\nERROR: Fail to compile(2)\n", Err.getCode().m_pString);
        return -1;
    }
    
    return 0;
}



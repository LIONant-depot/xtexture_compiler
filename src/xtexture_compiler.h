#ifndef _TEXTURE_COMPILER_H
#define _TEXTURE_COMPILER_H
#pragma once

#include "xresource_pipeline.h"

namespace xtexture_compiler
{
    enum class error : std::uint32_t
    { GUID      = xcore::guid::unit<32>{"xtexture_compiler"}.m_Value
    , SUCCESS   = 0
    , FAILURE
    };

    static constexpr xcore::guid::rcfull<> full_guid_v
    { .m_Type = xcore::guid::rctype<>         { "resource.pipeline", "plugin" }
    , .m_Instance = xcore::guid::rcinstance<> { "xtexture" }
    };
}

#include "xtexture_compiler_descriptor.h"
#include "xtexture_compiler_instance.h"

#endif

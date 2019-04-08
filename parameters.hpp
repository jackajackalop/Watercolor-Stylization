#pragma once
namespace Parameters{
//Art-directable parameters
    #define DO_PARAMETER(type, name, value, hint) \
        extern type name
    #include "do_parameters.hpp"
    #undef DO_PARAMETER
}

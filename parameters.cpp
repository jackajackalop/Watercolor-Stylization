#include "parameters.hpp"

namespace Parameters{
//Art-directable parameters
    #define DO_PARAMETER(type, name, value, hint) \
        type name = value
    #include "do_parameters.hpp"
    #undef DO_PARAMETER
}

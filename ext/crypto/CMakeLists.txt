cmake_minimum_required(VERSION 3.10)

add_library(crypto SHARED crypto.cpp)
target_link_libraries(crypto OpenSSL::Crypto)

set_target_properties(crypto PROPERTIES
    OUTPUT_NAME crypto PREFIX "" SUFFIX ".so"
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(crypto PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif()

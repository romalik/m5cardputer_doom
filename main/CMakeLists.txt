

file(GLOB_RECURSE APPS_SRCS
    ./doom/*.c
    ./doom/*.cpp
)

file(GLOB_RECURSE HAL_SRCS
    ./hal/*.c
    ./hal/*.cpp
)



idf_component_register(SRCS "cardputer.cpp" ${APPS_SRCS} ${HAL_SRCS}
                    INCLUDE_DIRS "." "./hal" "./doom/include")



idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=unused-value" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=uninitialized" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=switch" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=unused-but-set-parameter" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=implicit-function-declaration" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=misleading-indentation" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=stringop-truncation" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=format-overflow=" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=parentheses" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Wno-error=pointer-sign" APPEND)

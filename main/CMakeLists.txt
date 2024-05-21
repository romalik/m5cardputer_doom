

file(GLOB_RECURSE DOOM_SRCS
    ./doom/*.c
    ./doom/*.cpp
)

file(GLOB_RECURSE HAL_SRCS
    ./hal/*.c
    ./hal/*.cpp
)

list(APPEND DOOM_SRCS "mmap_test.c")

idf_component_register(SRCS "cardputer_doom.cpp" ${DOOM_SRCS} ${HAL_SRCS}
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

#idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_panic_handler" APPEND)
#idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=spi_flash_check_and_flush_cache" APPEND)

#idf_build_set_property(C_COMPILE_OPTIONS "-fsanitize=kernel-address" APPEND)

set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS -fsanitize=kernel-address)

#set_source_files_properties(mmap_test.c PROPERTIES COMPILE_FLAGS -fsanitize-recover=address)
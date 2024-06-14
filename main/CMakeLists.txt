

file(GLOB_RECURSE DOOM_SRCS
    ./doom/*.c
    ./doom/*.cpp
)
file(GLOB_RECURSE FS_SRCS
    ./fs/*.c
)

file(GLOB_RECURSE HAL_SRCS
    ./hal/*.c
    ./hal/*.cpp
)

list(APPEND DOOM_SRCS "libc_wrap.c")
list(APPEND DOOM_SRCS "mmap.c")
list(APPEND DOOM_SRCS "spi_cache.c")

idf_component_register(SRCS "cardputer_doom.cpp" ${DOOM_SRCS} ${HAL_SRCS} ${FS_SRCS}
                    INCLUDE_DIRS "." "./fs" "./hal" "./doom/include")



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


set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS "-fplugin=${CMAKE_CURRENT_LIST_DIR}/../gcc_plugin_wrap_mem_access/wrap_memaccess.so -Dstrncmp=__wrap_strncmp -Dmemcmp=__wrap_memcmp -Dmemset=__wrap_memset -Dmemcpy=__wrap_memcpy -Dstrcpy=__wrap_strcpy -Dstrncpy=__wrap_strncpy ")
set_source_files_properties(cardputer_doom.cpp PROPERTIES COMPILE_FLAGS "-O3")
#set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS "-fplugin=${CMAKE_CURRENT_LIST_DIR}/../gcc_plugin_wrap_mem_access/wrap_memaccess.so  -Wl,--wrap=strncmp -Wl,--wrap=strcpy -Wl,--wrap=strncpy -Wl,--wrap=memcpy")



#idf_build_set_property(LINK_OPTIONS  APPEND)
#idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=strncmp" APPEND)
#idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=memcpy" APPEND)
#set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS "-Wl,--wrap=strcpy")
#set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS "-Wl,--wrap=strncpy")
#set_source_files_properties(${DOOM_SRCS} PROPERTIES COMPILE_FLAGS "-Wl,--wrap=memcpy")



#idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=spi_flash_check_and_flush_cache" APPEND)

#idf_build_set_property(C_COMPILE_OPTIONS "-fsanitize=kernel-address" APPEND)


#set_source_files_properties(mmap_test.c PROPERTIES COMPILE_FLAGS -fsanitize-recover=address)
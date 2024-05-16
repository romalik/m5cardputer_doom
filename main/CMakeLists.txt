

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



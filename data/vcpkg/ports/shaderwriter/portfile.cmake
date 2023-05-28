vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH
    REPO DragonJoker/ShaderWriter
    REF 6aa2e9bb7f51c80df3ebf5f3d7ea5936d87e671f
    HEAD_REF development
    SHA512 04c9973c513ff4b912283d4db93e255455e2ed909fa1ebfc09759d39e79bd7e156abf65f5686a8ffa311f7441b9c802c8a346ec41b02eb4e42564b53431042b8
)

vcpkg_from_github(OUT_SOURCE_PATH CMAKE_SOURCE_PATH
    REPO DragonJoker/CMakeUtils
    REF 3818effff171f863d0c23e6fbbf79911f03cc6d3
    HEAD_REF master
    SHA512 3a13b371adf24f530fdef6005d3a9105185eca131c9b8aba68615a593f76d6aac2c3c7f0ecf6ce430e2b854afca4abd99f1400a1e6665478c9daa34a8dac6f5b
)

file(REMOVE_RECURSE "${SOURCE_PATH}/CMake")
file(COPY "${CMAKE_SOURCE_PATH}/" DESTINATION "${SOURCE_PATH}/CMake")

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BUILD_STATIC)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        spirv SDW_BUILD_EXPORTER_SPIRV
        glsl  SDW_BUILD_EXPORTER_HLSL
        hlsl  SDW_BUILD_EXPORTER_GLSL
)

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DPROJECTS_USE_PRECOMPILED_HEADERS=OFF
        -DSDW_GENERATE_SOURCE=OFF
        -DSDW_BUILD_VULKAN_LAYER=OFF
        -DSDW_BUILD_TESTS=OFF
        -DSDW_BUILD_STATIC_SDW=${BUILD_STATIC}
        -DSDW_BUILD_STATIC_SDAST=${BUILD_STATIC}
        -DSDW_UNITY_BUILD=ON
        ${FEATURE_OPTIONS}
)

vcpkg_copy_pdbs()
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ShaderWriter CONFIG_PATH lib/cmake/shaderwriter)

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)

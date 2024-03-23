cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH RHINO_REPOSITORY_ROOT)

set(InterfaceHeaderFiles
        include/SCARVersion.h
        include/SCAR.h
        include/SCARUnarchiver.h
)

set(HeaderFiles
        source/pch.h

        source/CompilationChain.h
        source/ICompilationPipeline.h

        source/steps/HlslToDxilStep.h
        source/steps/DxilToMetallibStep.h
        source/steps/HlslToSpirvStep.h
        source/steps/DXCCommon.hpp

        source/pipelines/ILComputeCompilationPipeline.h

        source/archiver/PSOArchiver.h
        source/serializers/SerializeRootSignature.h
        )

set(SourceFiles
        source/libMain.cpp

        source/CompilationChain.cpp

        source/steps/HlslToDxilStep.cpp
        source/steps/DxilToMetallibStep.cpp
        source/steps/HlslToSpirvStep.cpp

        source/archiver/PSOArchiver.cpp

        source/pipelines/ILComputeCompilationPipeline.cpp

        source/serializers/SerializeRootSignature.cpp
)

if(APPLE)
    add_library(SCARs STATIC ${InterfaceHeaderFiles} ${HeaderFiles} ${SourceFiles})

    add_custom_command(TARGET SCARs POST_BUILD
            COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path" $<TARGET_FILE:SCARs> || echo)

    # DXC
    target_include_directories(SCARs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/osx/include)
    target_link_libraries(SCARs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/osx/lib/libdxcompiler.dylib)
    target_compile_definitions(SCARs PRIVATE __EMULATE_UUID)

    # metal-shaderconverter
    target_include_directories(SCARs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/metal-shaderconverter/osx/include)
    target_link_libraries(SCARs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/metal-shaderconverter/osx/lib/libmetalirconverter.dylib)

    # Install
    install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/osx/lib/libdxcompiler.3.7.dylib
            TYPE BIN
    )
else()
    add_library(SCARs STATIC ${InterfaceHeaderFiles} ${HeaderFiles} ${SourceFiles})

    # DXC
    target_include_directories(SCARs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/win32/inc)
    target_link_libraries(SCARs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/win32/lib/dxcompiler.lib)

    # Install
    install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/win32/bin/dxcompiler.dll
            ${CMAKE_CURRENT_SOURCE_DIR}/external/DXC/win32/bin/dxil.dll
            ${CMAKE_CURRENT_SOURCE_DIR}/external/FXC/bin/d3dcompiler_47.dll
            TYPE BIN)
endif()

target_include_directories(SCARs PUBLIC include)
target_include_directories(SCARs PUBLIC ${RHINO_REPOSITORY_ROOT}/Runtime/include)
target_include_directories(SCARs PRIVATE source)
target_precompile_headers(SCARs PRIVATE source/pch.h)

# SCAR CLI
add_executable(SCAR EXCLUDE_FROM_ALL cli/SCARCLI.cpp)
target_link_libraries(SCAR PRIVATE SCARs)
target_include_directories(SCAR PRIVATE external/include)

# SCAR Dump
add_executable(SCARDump EXCLUDE_FROM_ALL cli/SCARDump.cpp)
target_link_libraries(SCARDump PRIVATE SCARs)
target_include_directories(SCARDump PRIVATE external/include)

# SCAR Reflect
add_executable(SCARReflect EXCLUDE_FROM_ALL cli/SCARReflect.cpp)
target_link_libraries(SCARReflect PRIVATE SCARs)
target_include_directories(SCARReflect PRIVATE external/include)

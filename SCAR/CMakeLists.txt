cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH RHINO_REPOSITORY_ROOT)

set(InterfaceHeaderFiles
        include/SCARVersion.h
        include/SCAR.h
        include/SCARUnarchiver.h
)

set(HeaderFiles
        source/archiver/PSOArchiver.h
        source/pch.h
        )

set(SourceFiles
        source/libMain.cpp
        source/archiver/PSOArchiver.cpp
)

if(APPLE)

else()
    add_library(SCARs STATIC ${InterfaceHeaderFiles} ${HeaderFiles} ${SourceFiles})
endif()

target_include_directories(SCARs PUBLIC include)
target_include_directories(SCARs PUBLIC ${RHINO_REPOSITORY_ROOT}/Runtime/include)
target_include_directories(SCARs PRIVATE source)
target_precompile_headers(SCARs PRIVATE source/pch.h)

# SCAR CLI
add_executable(SCAR EXCLUDE_FROM_ALL cli/SCARCLI.cpp)
target_link_libraries(SCAR PRIVATE SCARs)

# SCAR Reflect
add_executable(SCARReflect EXCLUDE_FROM_ALL cli/SCARReflect.cpp)
target_link_libraries(SCARReflect PRIVATE SCARs)
target_include_directories(SCARReflect PRIVATE external/include)

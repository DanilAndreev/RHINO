set(InterfaceHeaderFiles
        include/SCARVersion.h
        include/SCAR.h
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
    add_executable(SCAR EXCLUDE_FROM_ALL source/cliMain.cpp)
    target_link_libraries(SCAR PRIVATE SCARs)
endif()

target_include_directories(SCARs PUBLIC include)
target_include_directories(SCARs PRIVATE source)
target_precompile_headers(SCARs PRIVATE source/pch.h)

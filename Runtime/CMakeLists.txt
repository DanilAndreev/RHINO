cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH RHINO_REPOSITORY_ROOT)

set(InterfaceHeaderFiles
        include/RHINO.h
        include/RHINOTypes.h)

set(HeaderFiles
        source/pch.h

        source/RHINOInterfaceImplBase.h
        source/RHINOTypesImpl.h
        source/Utils/Common.h
        source/Utils/PlatformBase.h

        source/DebugLayer/DebugLayer.h

        source/Vulkan/VulkanBackend.h
        source/Vulkan/VulkanBackendTypes.h
        source/Vulkan/VulkanConverters.h
        source/Vulkan/VulkanUtils.h
        source/Vulkan/VulkanAPI.h
        source/Vulkan/VulkanDescriptorHeap.h
        source/Vulkan/VulkanCommandList.h
        source/Vulkan/VulkanSwapchain.h

        source/D3D12/D3D12Backend.h
        source/D3D12/D3D12BackendTypes.h
        source/D3D12/D3D12Converters.h
        source/D3D12/D3D12Utils.h
        source/D3D12/D3D12DescriptorHeap.h
        source/D3D12/D3D12CommandList.h
        source/D3D12/D3D12Swapchain.h
        source/D3D12/D3D12GarbageCollector.h

        source/SCARTools/SCARComputePSOArchiveView.h
        source/SCARTools/SCARRTPSOArchiveView.h
)

set(ObjCHeaderFiles
        source/Metal/MetalBackend.h
        source/Metal/MetalConverters.h
        source/Metal/AllocateMetalBackend.h
        source/Metal/MetalBackendTypes.h
        source/Metal/MetalDescriptorHeap.h
        source/Metal/MetalCommandList.h
        source/Metal/MetalSwapchain.h
        source/Metal/MetalUtils.h
)
set(ObjCSourceFiles
        source/Metal/MetalBackend.mm
        source/Metal/MetalDescriptorHeap.mm
        source/Metal/MetalCommandList.mm
        source/Metal/MetalSwapchain.mm
        source/Metal/IRShaderCompilerRuntimeImplementation.mm
)

set(SourceFiles
        source/main.cpp
        source/RHINOTypesImpl.cpp
        source/RHINOInterfaceImplBase.cpp

        source/DebugLayer/DebugLayer.cpp

        source/Vulkan/VulkanBackend.cpp
        source/Vulkan/VulkanAPI.cpp
        source/Vulkan/VulkanDescriptorHeap.cpp
        source/Vulkan/VulkanCommandList.cpp
        source/Vulkan/VulkanSwapchain.cpp

        source/D3D12/D3D12Backend.cpp
        source/D3D12/D3D12DescriptorHeap.cpp
        source/D3D12/D3D12CommandList.cpp
        source/D3D12/D3D12Swapchain.cpp
        source/D3D12/D3D12GarbageCollector.cpp

        source/SCARTools/SCARComputePSOArchiveView.cpp
        source/SCARTools/SCARRTPSOArchiveView.cpp
)

if(APPLE)
    message("RHINO AVAILABLE APIs: Metal.")
    add_library(RHINO STATIC ${SourceFiles} ${HeaderFiles} ${ObjCHeaderFiles} ${ObjCSourceFiles} ${InterfaceHeaderFiles})
    set_source_files_properties(${ObjCHeaderFiles} PROPERTIES HEADER_FILE_ONLY TRUE)
    set_property(TARGET RHINO APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
    target_compile_definitions(RHINO PRIVATE ENABLE_API_METAL=1)
    target_link_libraries(RHINO PRIVATE "-framework Metal" "-framework Foundation")

    target_include_directories(RHINO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/metal-shaderconverter/osx/include)
    target_link_libraries(RHINO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/metal-shaderconverter/osx/lib/libmetalirconverter.dylib)
else()
    message("RHINO AVAILABLE APIs: Vulkan, D3D12.")
    add_library(RHINO ${SourceFiles} ${HeaderFiles} ${InterfaceHeaderFiles})
    target_compile_definitions(RHINO PRIVATE ENABLE_API_D3D12=1) # ENABLE_API_VULKAN=1
    target_link_libraries(RHINO PRIVATE d3d12.lib dxgi.lib)

    find_package(Vulkan REQUIRED)
    target_include_directories(RHINO PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(RHINO PRIVATE Vulkan::Vulkan)
endif()

target_include_directories(RHINO PRIVATE external/include)

# ------------------------------------------- END DEBUG MACRO ----------------------------------------------------------

# set_source_files_properties(${HeaderFiles} PROPERTIES HEADER_FILE_ONLY TRUE)
target_precompile_headers(RHINO PRIVATE source/pch.h)
target_include_directories(RHINO PRIVATE ${RHINO_REPOSITORY_ROOT}/SCAR/include)
target_include_directories(RHINO PUBLIC include)
target_include_directories(RHINO PUBLIC source)

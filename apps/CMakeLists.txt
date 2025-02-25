find_package(Boost REQUIRED COMPONENTS system filesystem program_options)

add_subdirectory(Utils)

add_library(rag_components INTERFACE)

target_link_libraries(rag_components INTERFACE
    RagPUREAILib
    Boost::system
    Boost::filesystem
    Boost::program_options
)

add_subdirectory(chunk)
add_subdirectory(clean_data)
# Add the PIC flag
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(apps_utils_SOURCES Utils.cpp)

add_library(apps_utils ${apps_utils_SOURCES})

target_include_directories(apps_utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(apps_utils PUBLIC rag_components)
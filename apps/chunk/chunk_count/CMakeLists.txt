add_executable(chunk_count chunk_count.cpp)

target_link_libraries(chunk_count PRIVATE
    apps_utils
)
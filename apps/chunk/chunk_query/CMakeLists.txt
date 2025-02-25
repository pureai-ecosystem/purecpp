add_executable(chunk_query chunk_query.cpp)

target_link_libraries(chunk_query PRIVATE
    apps_utils
)
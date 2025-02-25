add_executable(chunk_similarity chunk_similarity.cpp)

target_link_libraries(chunk_similarity PRIVATE
    apps_utils
)
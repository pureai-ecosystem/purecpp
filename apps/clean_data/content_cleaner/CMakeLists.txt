add_executable(content_cleaner content_cleaner.cpp)

target_link_libraries(content_cleaner PRIVATE
    apps_utils
)
# Main executable
add_executable(redis
    main.cpp
)

target_link_libraries(redis PRIVATE
    redis_core
)

# Include modules directory for headers
target_include_directories(redis PRIVATE
    ${CMAKE_SOURCE_DIR}/modules
)

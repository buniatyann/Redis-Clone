# Core library that contains all Redis clone logic
add_library(redis_core STATIC)

# Add subdirectories (each adds sources to redis_core)
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(protocol)
add_subdirectory(commands)
add_subdirectory(network)
add_subdirectory(persistence)
add_subdirectory(config)

# Include directories for the library
target_include_directories(redis_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(redis_core PUBLIC
    nlohmann_json
    pthread
)

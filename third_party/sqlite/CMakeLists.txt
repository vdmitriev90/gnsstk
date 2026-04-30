
add_library(sqlite STATIC
    sqlite3.c
)

target_include_directories(sqlite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(sqlite
    PUBLIC
        SQLITE_THREADSAFE=1
)

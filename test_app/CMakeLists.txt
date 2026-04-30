# test_app/CMakeLists.txt

add_executable(test_app test_app.cpp)

target_compile_features(test_app PRIVATE cxx_std_17)

target_link_libraries(test_app gpstk)
target_link_libraries(test_app POD)



# app_runner/CMakeLists.txt

add_executable(app_runner app_runner.cpp)

target_compile_features(app_runner PRIVATE cxx_std_23)

target_link_libraries(app_runner gnsstk)
target_link_libraries(app_runner pod)

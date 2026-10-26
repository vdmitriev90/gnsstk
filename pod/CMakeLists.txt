# pod/CMakeLists.txt

file( GLOB_RECURSE POD_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c" )
file( GLOB_RECURSE POD_INC_FILES CONFIGURE_DEPENDS "src/*.h" "src/*.hpp" )

set( POD_INC_DIRS "" )

foreach( _headerFile ${MY_INC_FILES} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND POD_INC_DIRS ${_dir} )
endforeach()

include_directories( ${POD_INC_DIRS} )

add_library(pod ${POD_SOURCE_FILES} ${POD_INC_FILES})

# Mirror the source directory structure as filters in Visual Studio Solution Explorer
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${POD_SOURCE_FILES} ${POD_INC_FILES})

target_compile_features(pod PUBLIC cxx_std_23)

target_link_libraries(pod  gnsstk)
target_link_libraries(pod  SOFA)
target_link_libraries(pod sqlite)

add_executable(pod_tests tests/DatabaseTests.cpp)
target_compile_features(pod_tests PRIVATE cxx_std_23)
target_link_libraries(pod_tests pod)
enable_testing()
add_test(NAME pod_tests COMMAND pod_tests)


find_package(Python3 REQUIRED)

set(PY_SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/scripts)
file(GLOB_RECURSE PY_SCRIPT ${PY_SCRIPTS_DIR}/gnss_downloader/*.py)

set(RUNTIME_DIR $<TARGET_FILE_DIR:app_runner>)

add_custom_target(download_cddis
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PY_SCRIPTS_DIR}
    ${Python3_EXECUTABLE} -u -m gnss_downloader.main
    --config ${RUNTIME_DIR}/orchestrator.ini
#    --debug
    WORKING_DIRECTORY ${RUNTIME_DIR}
    SOURCES ${PY_SCRIPT}
)

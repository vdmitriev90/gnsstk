# pod/CMakeLists.txt

file( GLOB_RECURSE POD_SOURCE_FILES "src/*.cpp" "src/*.c" )
file( GLOB_RECURSE POD_INC_FILES "src/*.h" "src/*.hpp" )

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

target_link_libraries(pod  gpstk)
target_link_libraries(pod  SOFA)
target_link_libraries(pod sqlite)
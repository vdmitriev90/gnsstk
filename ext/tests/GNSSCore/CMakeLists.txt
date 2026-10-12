#Tests for ext GNSSCore Classes

add_executable(SolidTides_T SolidTides_T.cpp)
target_link_libraries(SolidTides_T gnsstk)
add_test(NAME ExtGNSSCore_SolidTides COMMAND $<TARGET_FILE:SolidTides_T>)

add_executable(OceanLoading_T OceanLoading_T.cpp)
target_link_libraries(OceanLoading_T gnsstk)
target_compile_definitions(OceanLoading_T PRIVATE TESTDATADIR="${GNSSTK_TEST_DATA_DIR}")
add_test(NAME ExtGNSSCore_OceanLoading COMMAND $<TARGET_FILE:OceanLoading_T>)

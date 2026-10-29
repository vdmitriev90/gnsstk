#Tests for ext Procframe Classes

add_executable(SatTable_T SatTable_T.cpp)
target_link_libraries(SatTable_T gnsstk)
add_test(NAME ExtProcframe_SatTable COMMAND $<TARGET_FILE:SatTable_T>)

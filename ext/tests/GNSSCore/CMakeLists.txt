#Tests for ext GNSSCore Classes

add_executable(SolidTides_T SolidTides_T.cpp)
target_link_libraries(SolidTides_T gnsstk)
add_test(NAME ExtGNSSCore_SolidTides COMMAND $<TARGET_FILE:SolidTides_T>)

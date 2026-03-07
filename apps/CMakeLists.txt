
add_executable(pvtui_motor motor_display.cpp motor.cpp)
target_link_libraries(pvtui_motor PRIVATE pvtui)

add_executable(pvtui_asyn asyn.cpp)
target_link_libraries(pvtui_asyn PRIVATE pvtui)

add_executable(pvtui_calcout calcout.cpp)
target_link_libraries(pvtui_calcout PRIVATE pvtui)

add_executable(pvtui_transform transform.cpp)
target_link_libraries(pvtui_transform PRIVATE pvtui)

add_executable(pvtui_sequence sequence.cpp)
target_link_libraries(pvtui_sequence PRIVATE pvtui)

add_executable(pvtui_sr sr.cpp)
target_link_libraries(pvtui_sr PRIVATE pvtui)

add_executable(pvtui_inputx inputx.cpp)
target_link_libraries(pvtui_inputx PRIVATE pvtui)

add_executable(pvtui_demo demo.cpp)
target_link_libraries(pvtui_demo PRIVATE pvtui)

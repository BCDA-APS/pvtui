add_subdirectory(motor)

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

add_executable(pvtui_monitor monitor.cpp)
target_link_libraries(pvtui_monitor PRIVATE pvtui)

add_executable(pvtui_demo demo.cpp)
target_link_libraries(pvtui_demo PRIVATE pvtui)

add_executable(pvtui_waveform waveform.cpp)
target_link_libraries(pvtui_waveform PRIVATE pvtui ftxui-plot)

include(FetchContent)
FetchContent_Declare(
    ftxui-plot
    GIT_REPOSITORY https://github.com/nmarks99/ftxui-plot.git
    GIT_TAG origin/master
)
FetchContent_MakeAvailable(ftxui-plot)
add_executable(pvtui_striptool striptool.cpp)
target_link_libraries(pvtui_striptool PRIVATE pvtui ftxui-plot)

install(TARGETS
    pvtui_motor
    pvtui_asyn
    pvtui_calcout
    pvtui_transform
    pvtui_sequence
    pvtui_sr
    pvtui_monitor
    pvtui_demo
    pvtui_striptool
    pvtui_waveform
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

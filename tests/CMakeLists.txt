add_executable(test_pvgroup test_pvgroup.cpp)
target_link_libraries(test_pvgroup PRIVATE pvtui)

add_executable(test_argparser test_argparser.cpp)
target_link_libraries(test_argparser PRIVATE pvtui)

add_executable(test_pvtui test_pvtui.cpp)
target_link_libraries(test_pvtui PRIVATE pvtui)

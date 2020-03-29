set(SRC_SC
	steamcontroller_feedback.c
	steamcontroller_setup.c
	steamcontroller_state.c
	steamcontroller_win32.c
	steamcontroller_wireless.c
)

add_library(steam_controller STATIC ${SRC_SC})
target_compile_definitions(steam_controller PRIVATE STEAMCONTROLLER_BUILDING_LIBRARY)
target_include_directories(steam_controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	target_link_libraries(steam_controller setupapi hid)
endif()
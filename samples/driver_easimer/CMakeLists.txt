add_library(driver_easimer SHARED
	driver_easimer.cpp
	driver_easimer.h

	driverlog.cpp
	driverlog.h

	steamcontroller.h
)

target_compile_definitions(driver_easimer PRIVATE DRIVER_SAMPLE_EXPORTS)
target_link_libraries(driver_easimer
	${OPENVR_LIBRARIES}
	${CMAKE_DL_LIBS}
	steam_controller
	lua
)
add_library(librdb librdb.cpp)
add_library(rdb::librdb ALIAS librdb)

set_target_properties(
	librdb
	PROPERTIES OUTPUT_NAME rdb
)

target_compile_features(librdb PUBLIC cxx_std_17)

target_include_directories(
	librdb
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_SOURCE_DIR}/src/include
)

include(GNUInstallDirs)
install(TARGETS librdb
	EXPORT rdb-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
	EXPORT rdb-targets
	FILE rdb-config.cmake
	NAMESPACE rdb::
	DESTINATION ${CMAKE_INSTALL_DIR}/cmake/rdb
)

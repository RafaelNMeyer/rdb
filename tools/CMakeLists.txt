add_executable(rdb rdb.cpp)

target_link_libraries(rdb PRIVATE rdb::librdb PkgConfig::libedit fmt::fmt)

include(GNUInstallDirs)
install(
	TARGETS rdb
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE rdb::librdb Catch2::Catch2WithMain)

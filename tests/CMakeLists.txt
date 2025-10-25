if(GTest_FOUND)
    add_executable(test_database
            unit/test_database.cpp
            ../src/database/DatabaseManager.cpp
            ../src/core/Message.cpp
            ../src/core/User.cpp
            ../src/core/ChatRoom.cpp
    )

    add_executable(test_protocols
            unit/test_protocols.cpp
            ../src/protocols/ProtocolHandler.cpp
            ../src/core/Message.cpp
    )

    add_executable(test_utils
            unit/test_utils.cpp
            ../src/utils/StringUtils.cpp
            ../src/utils/JsonParser.cpp
    )

    foreach(test_name test_database test_protocols test_utils)
        target_include_directories(${test_name} PRIVATE ../include)
        target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main)
        target_link_libraries(${test_name} PRIVATE ${SQLite3_LIBRARIES} ${JSONCPP_LIBRARIES})

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    if(ENABLE_TELEGRAM)
        add_executable(test_integration
                integration/test_integration.cpp
        )
        target_include_directories(test_integration PRIVATE ../include)
        target_link_libraries(test_integration PRIVATE GTest::gtest GTest::gtest_main)
        add_test(NAME test_integration COMMAND test_integration)
    endif()
endif()
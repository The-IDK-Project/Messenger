if(BUILD_TESTS AND GTest_FOUND)

    add_executable(test_matrix_integration
            test_matrix.cpp
            ../../src/protocols/MatrixHandler.cpp
            ../../src/protocols/ProtocolHandler.cpp
            ../../src/network/HttpClient.cpp
            ../../src/network/WebSocketClient.cpp
            ../../src/core/Message.cpp
            ../../src/core/User.cpp
            ../../src/core/ChatRoom.cpp
            ../../src/utils/Logger.cpp
            ../../src/utils/JsonParser.cpp
            ../../src/utils/StringUtils.cpp
    )

    add_executable(test_irc_integration
            test_irc.cpp
            ../../src/protocols/IRCHandler.cpp
            ../../src/protocols/ProtocolHandler.cpp
            ../../src/network/IRCConnection.cpp
            ../../src/core/Message.cpp
            ../../src/core/User.cpp
            ../../src/core/ChatRoom.cpp
            ../../src/utils/Logger.cpp
            ../../src/utils/StringUtils.cpp
    )

    foreach(test_name test_matrix_integration test_irc_integration)
        target_include_directories(${test_name} PRIVATE
                ../../include
                ${GTEST_INCLUDE_DIRS}
        )

        target_link_libraries(${test_name} PRIVATE
                GTest::gtest
                GTest::gtest_main
                ${JSONCPP_LIBRARIES}
                Threads::Threads
        )

        # Mark integration tests as disabled by default
        add_test(NAME ${test_name} COMMAND ${test_name})
        set_tests_properties(${test_name} PROPERTIES DISABLED TRUE)
    endforeach()

    target_link_libraries(test_matrix_integration PRIVATE
            ${CURL_LIBRARIES}
            ${OPENSSL_LIBRARIES}
    )

endif()
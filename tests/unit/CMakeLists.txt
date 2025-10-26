if(BUILD_TESTS AND GTest_FOUND)
    add_executable(test_database
            test_database.cpp
            ../../src/database/DatabaseManager.cpp
            ../../src/database/DatabaseSchema.cpp
            ../../src/database/Migrations.cpp
            ../../src/core/Message.cpp
            ../../src/core/User.cpp
            ../../src/core/ChatRoom.cpp
            ../../src/core/Config.cpp
            ../../src/utils/Logger.cpp
            ../../src/utils/StringUtils.cpp
            ../../src/utils/JsonParser.cpp
            ../../src/utils/Crypto.cpp
    )

    add_executable(test_protocols
            test_protocols.cpp
            ../../src/protocols/ProtocolHandler.cpp
            ../../src/protocols/MatrixHandler.cpp
            ../../src/protocols/IRCHandler.cpp
            ../../src/protocols/TelegramHandler.cpp
            ../../src/protocols/ProtocolFactory.cpp
            ../../src/core/Message.cpp
            ../../src/core/User.cpp
            ../../src/core/ChatRoom.cpp
            ../../src/utils/Logger.cpp
    )

    add_executable(test_utils
            test_utils.cpp
            ../../src/utils/StringUtils.cpp
            ../../src/utils/JsonParser.cpp
            ../../src/utils/Logger.cpp
            ../../src/utils/Crypto.cpp
    )

    foreach(test_name test_database test_protocols test_utils)
        target_include_directories(${test_name} PRIVATE
                ../../include
                ${GTEST_INCLUDE_DIRS}
        )

        target_link_libraries(${test_name} PRIVATE
                GTest::gtest
                GTest::gtest_main
                ${JSONCPP_LIBRARIES}
                ${SQLite3_LIBRARIES}
                Threads::Threads
        )

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()

    target_link_libraries(test_database PRIVATE ${SQLite3_LIBRARIES})

    if(ENABLE_TELEGRAM AND TDLIB_FOUND)
        target_link_libraries(test_protocols PRIVATE ${TDLIB_LIBRARIES})
    endif()

    if(OPENSSL_FOUND)
        target_link_libraries(test_utils PRIVATE OpenSSL::Crypto)
    endif()

endif()
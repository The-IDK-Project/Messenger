#include <gtest/gtest.h>
#include "protocols/MatrixHandler.h"
#include "network/HttpClient.h"

class MatrixIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<MatrixHandler>("https://matrix.org");
    }

    void TearDown() override {
        if (handler->is_connected()) {
            handler->disconnect();
        }
    }

    std::unique_ptr<MatrixHandler> handler;
};

TEST_F(MatrixIntegrationTest, DISABLED_ConnectionTest) {


    GTEST_SKIP() << "Matrix integration tests require valid credentials";
}

TEST_F(MatrixIntegrationTest, DISABLED_SendMessage) {
    GTEST_SKIP() << "Matrix integration tests require valid credentials";
}
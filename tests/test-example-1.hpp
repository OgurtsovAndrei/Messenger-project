//
// Created by andrey on 24.05.23.
//

#ifndef MESSENGER_PROJECT_TEST_EXAMPLE_1_HPP
#define MESSENGER_PROJECT_TEST_EXAMPLE_1_HPP

#include <cxxtest/TestSuite.h>

class MyTestSuite1 : public CxxTest::TestSuite {
public:
    void testAddition(void) {
        TS_ASSERT(1 + 1 > 1);
        TS_ASSERT_EQUALS(1 + 1, 2);
    }
};

#endif
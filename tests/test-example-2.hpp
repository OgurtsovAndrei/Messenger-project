//
// Created by andrey on 24.05.23.
//

#ifndef MESSENGER_PROJECT_TEST_EXAMPLE_2_HPP
#define MESSENGER_PROJECT_TEST_EXAMPLE_2_HPP

// MyTestSuite2.h
#include <cxxtest/TestSuite.h>
class MyTestSuite2 : public CxxTest::TestSuite
{
public:
    void testAddition(void)
    {
        TS_ASSERT(1 + 1 > 1);
        TS_ASSERT_EQUALS(1 + 1, 2);
    }
    void testMultiplication(void)
    {
        TS_TRACE("Starting multiplication test");
        TS_ASSERT_EQUALS(2 * 2, 5);
        TS_TRACE("Finishing multiplication test");
    }
};

#endif //MESSENGER_PROJECT_TEST_EXAMPLE_2_HPP

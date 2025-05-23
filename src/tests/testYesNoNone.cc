/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"
#include "base/YesNoNone.h"
#include "compat/cppunit.h"
#include "unitTestMain.h"

#include <stdexcept>

/*
 * demonstration test file, as new idioms are made they will
 * be shown in the TestYesNoNone source.
 */

class TestYesNoNone : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(TestYesNoNone);
    /* note the statement here and then the actual prototype below */
    CPPUNIT_TEST(testBasics);
    CPPUNIT_TEST_SUITE_END();

public:
protected:
    void testBasics();
};
CPPUNIT_TEST_SUITE_REGISTRATION(TestYesNoNone);

void
TestYesNoNone::testBasics()
{
    // unconfigured, non-implicit
    {
        YesNoNone v;
        CPPUNIT_ASSERT_EQUAL(false, v.configured());
        // cannot test the value it is 'undefined' and will assert
    }
    // implicit dtor test

    // unconfigured, implicit true
    {
        YesNoNone v(true);
        CPPUNIT_ASSERT_EQUAL(false, v.configured());
        CPPUNIT_ASSERT(v);
        CPPUNIT_ASSERT_EQUAL(true, static_cast<bool>(v));

        // check explicit setter method
        v.configure(false);
        CPPUNIT_ASSERT_EQUAL(true, v.configured());
        CPPUNIT_ASSERT(!v);
        CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(v));
    }

    // unconfigured, implicit false
    {
        YesNoNone v(false);
        CPPUNIT_ASSERT_EQUAL(false, v.configured());
        CPPUNIT_ASSERT(!v);
        CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(v));

        // check assignment operator
        v = YesNoNone(true);
        CPPUNIT_ASSERT_EQUAL(false, v.configured());
        CPPUNIT_ASSERT(v);
        CPPUNIT_ASSERT_EQUAL(true, static_cast<bool>(v));
    }
}

int
main(int argc, char *argv[])
{
    return TestProgram().run(argc, argv);
}


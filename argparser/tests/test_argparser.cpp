#include "argparser/arg_list.h"
#include "argparser/arg_parser.h"
#include "argparser/arg.h"

#include <gtest/gtest.h>
#include <string>

using namespace argparser;

TEST(ArgList, Basics) {
    const char* argv[] = {"prog", "a", "b"};
    ArgList list(3, argv);
    EXPECT_TRUE(list.hasNext());
    EXPECT_EQ(list.next(), "prog");
    EXPECT_EQ(list.next(), "a");
    EXPECT_EQ(list.peek(), "b");
    EXPECT_TRUE(list.hasNext());
    EXPECT_EQ(list.next(), "b");
    EXPECT_FALSE(list.hasNext());
}

TEST(ArgParser, StringArg) {
    ArgParser parser;
    Arg<std::string> dataDir(parser, "--data-dir", "-d", "data dir", std::string("default"));
    const char* argv[] = {"prog", "--data-dir", "foo"};
    auto r = parser.parse(3, argv);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(dataDir.get(), "foo");
}

TEST(ArgParser, ShortName) {
    ArgParser parser;
    Arg<std::string> dataDir(parser, "--data-dir", "-d", "data dir", std::string(""));
    const char* argv[] = {"prog", "-d", "bar"};
    auto r = parser.parse(3, argv);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ((std::string)dataDir, "bar");
}

TEST(ArgParser, IntArg) {
    ArgParser parser;
    Arg<int> num(parser, "--num", "-n", "number", 0);
    const char* argv[] = {"prog", "--num", "42"};
    auto r = parser.parse(3, argv);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(num.get(), 42);
}

TEST(ArgParser, BoolArgTrueOnYes) {
    ArgParser p1;
    Arg<bool> b1(p1, "--flag", "-f", "flag", false);
    const char* a1[] = {"prog", "--flag", "true"};
    EXPECT_TRUE(p1.parse(3, a1).ok);
    EXPECT_TRUE(b1.get());

    ArgParser p2;
    Arg<bool> b2(p2, "--flag", "-f", "flag", false);
    const char* a2[] = {"prog", "--flag", "on"};
    EXPECT_TRUE(p2.parse(3, a2).ok);
    EXPECT_TRUE(b2.get());

    ArgParser p3;
    Arg<bool> b3(p3, "--flag", "-f", "flag", false);
    const char* a3[] = {"prog", "--flag", "yes"};
    EXPECT_TRUE(p3.parse(3, a3).ok);
    EXPECT_TRUE(b3.get());

    ArgParser p4;
    Arg<bool> b4(p4, "--flag", "-f", "flag", true);
    const char* a4[] = {"prog", "--flag", "false"};
    EXPECT_TRUE(p4.parse(3, a4).ok);
    EXPECT_FALSE(b4.get());

    // bare flag should be true
    ArgParser p5;
    Arg<bool> b5(p5, "--flag", "-f", "flag", false);
    const char* a5[] = {"prog", "--flag"};
    EXPECT_TRUE(p5.parse(2, a5).ok);
    EXPECT_TRUE(b5.get());
}

TEST(ArgParser, MissingValue) {
    ArgParser parser;
    Arg<std::string> s(parser, "--data-dir", "-d", "desc", std::string(""));
    const char* argv[] = {"prog", "--data-dir"};
    auto r = parser.parse(2, argv);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

TEST(ArgParser, UnknownFlag) {
    ArgParser parser;
    Arg<std::string> s(parser, "--known", "-k", "desc", std::string(""));
    const char* argv[] = {"prog", "--unknown", "foo"};
    auto r = parser.parse(3, argv);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.error.find("Unknown argument"), std::string::npos);
}

TEST(ArgParser, HelpPrintsTable) {
    ArgParser parser;
    Arg<std::string> a(parser, "--data-dir", "-d", "data directory", std::string(""));
    Arg<int> b(parser, "--count", "-c", "count", 0);
    // capture stdout
    testing::internal::CaptureStdout();
    parser.printHelp();
    std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("--data-dir"), std::string::npos);
    EXPECT_NE(out.find("--count"), std::string::npos);
    EXPECT_NE(out.find("-d"), std::string::npos);
    EXPECT_NE(out.find("-c"), std::string::npos);
}

TEST(ArgParser, HelpFlagNotError) {
    ArgParser parser;
    Arg<std::string> s(parser, "--data-dir", "-d", "desc", std::string(""));
    const char* argv[] = {"prog", "--help"};
    testing::internal::CaptureStdout();
    auto r = parser.parse(2, argv);
    testing::internal::GetCapturedStdout();
    EXPECT_TRUE(r.ok);
}

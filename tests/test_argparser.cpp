#include <iostream>
#include <pvtui/pvtui.hpp>

int main() {

    std::cout << "[pvtui::ArgParser] Running tests...\n";

    {
        char arg0[] = "test";
        char arg1[] = "--help";
        char* args[] = {arg0, arg1, nullptr};
        pvtui::ArgParser parser(2, args);

        assert(parser.flag("help"));
        assert(!parser.flag("version"));
    }

    {
        char arg0[] = "test";
        char arg1[] = "pos1";
        char arg2[] = "pos2";
        char* args[] = {arg0, arg1, arg2, nullptr};
        pvtui::ArgParser parser(3, args);

        auto pos = parser.positional_args();
        assert(pos.size() == 3);
        assert(pos[0] == "test");
        assert(pos[1] == "pos1");
        assert(pos[2] == "pos2");
    }

    {
        char arg0[] = "test";
        char arg1[] = "--prefix";
        char arg2[] = "MyPrefix:";
        char arg3[] = "m1";
        char* args[] = {arg0, arg1, arg2, arg3, nullptr};
        pvtui::ArgParser parser(4, args, {"--prefix"});

        assert(parser.param("--prefix") == "MyPrefix:");
        auto pos = parser.positional_args();
        assert(pos.size() == 2);
        assert(pos[1] == "m1");
    }

    {
        char arg0[] = "test";
        char arg1[] = "--provider";
        char arg2[] = "pva";
        char* args[] = {arg0, arg1, arg2, nullptr};
        pvtui::ArgParser parser(3, args);

        assert(parser.provider == "pva");
    }

    {
        char arg0[] = "test";
        char* args[] = {arg0, nullptr};
        pvtui::ArgParser parser(1, args);

        assert(parser.provider == "ca");
        assert(parser.param("--prefix").empty());
    }

    std::cout << "[pvtui::ArgParser] All tests passed" << std::endl;
}

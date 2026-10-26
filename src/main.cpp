#include "core/Game.h"

#include <exception>
#include <iostream>
#include <string>

namespace {

GameOptions parseOptions(int argc, char** argv)
{
    GameOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--demo") {
            options.demo = true;
        } else if (argument == "--record-demo" && i + 1 < argc) {
            options.recordDirectory = argv[++i];
        }
    }
    return options;
}

}

int main(int argc, char** argv)
{
    try {
        Game game(parseOptions(argc, argv));
        game.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

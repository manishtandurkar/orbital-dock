#include "core/Game.h"

#include <exception>
#include <iostream>

int main()
{
    try {
        Game game;
        game.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

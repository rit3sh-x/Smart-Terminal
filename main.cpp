#include <boost/asio.hpp>
#include <ncurses.h>
#include <iostream>
#include "src/FileDirectoryStructure.hpp"

void Explorer() {
    FileExplorer explorer("/");
    explorer.run();
}

int main(int argc, char* argv[]) {
    Explorer();
    return 0;
}
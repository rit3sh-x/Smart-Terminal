#include <ncurses.h>
#include <iostream>
#include "src/TextEditor.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);
    TextEditor editor(filename);
    editor.runEditor();
    endwin();
    return 0;
}
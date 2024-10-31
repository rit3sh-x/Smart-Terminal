#ifndef FILE_SEARCHER_HPP
#define FILE_SEARCHER_HPP

#include <ncurses.h>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>

class FileSearcher {
public:
    FileSearcher(const std::string& path) : searchPath(path) {
        results.reserve(100);
    }

    void run() {
        searchUI();
    }

private:
    std::mutex mtx;
    std::vector<std::string> results;
    std::vector<std::string> suggestions;
    std::string searchPath;

    void clearFromRow(int startRow) {
        for (int row = startRow; row < LINES; ++row) {
            move(row, 0);
            clrtoeol();
        }
    }

    std::string toLower(const std::string& str) {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),[](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }

    void searchInDirectory(const std::string& searchWord) {
        std::string lowerSearchWord = toLower(searchWord);
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
                std::string name = entry.path().filename().string();
                if (toLower(name).find(lowerSearchWord) != std::string::npos) {
                    std::lock_guard<std::mutex> lock(mtx);
                    results.push_back(entry.path().string());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back("Error: " + std::string(e.what()));
        }
    }

    void generateSuggestions(const std::string& input) {
        suggestions.clear();
        if (input.empty()) return;

        std::string lowerInput = toLower(input);

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
                std::string name = entry.path().filename().string();
                if (toLower(name).find(lowerInput) == 0) {
                    suggestions.push_back(name);
                    if (suggestions.size() >= 10) break;
                }
            }
        } catch (...) {
        }
    }

    void displaySuggestions(int startRow) {
        clearFromRow(startRow);
        int line = startRow;
        for (const auto& suggestion : suggestions) {
            mvprintw(line++, 2, "%s", suggestion.c_str());
        }
        refresh();
    }

    void searchUI() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);

        WINDOW* searchWin;
        int height = 3, width = COLS - 4;
        int startY = 1, startX = 2;
        std::string searchWord;
        echo();

        while (true) {
            curs_set(1);
            clearFromRow(4);
            refresh();
            searchWin = newwin(height, width, startY, startX);
            box(searchWin, 0, 0);
            mvwprintw(searchWin, 1, 2, ": %s", searchWord.c_str());
            wrefresh(searchWin);
            generateSuggestions(searchWord);
            displaySuggestions(4);
            wmove(searchWin, 1, 4 + searchWord.size());
            wrefresh(searchWin);

            int ch = getch();
            if (ch == 27) {
                break;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (!searchWord.empty()) {
                    searchWord.pop_back();
                }
            } else if (ch == '\t' && !suggestions.empty()) {
                searchWord = suggestions[0];
            } else if (isprint(ch)) {
                searchWord.push_back(static_cast<char>(ch));
            } else if (ch == '\n') {
                results.clear();
                mvprintw(4, 0, "Searching for \"%s\" in \"%s\"...", searchWord.c_str(), searchPath.c_str());
                refresh();

                std::thread search_thread(&FileSearcher::searchInDirectory, this, searchWord);
                search_thread.join();

                clearFromRow(4);
                int line = 4;
                if (results.empty()) {
                    mvprintw(line++, 2, "No results found.");
                    getch();
                } else {
                    int max_lines = LINES - 5;
                    int current_line = 0;

                    curs_set(0);

                    while (true) {
                        clearFromRow(4);
                        line = 4;
                        for (int i = current_line; i < current_line + max_lines && i < results.size(); ++i) {
                            mvprintw(line++, 2, "%s", results[i].c_str());
                        }

                        mvprintw(LINES - 1, 0, "Use Arrow keys to scroll, Enter to continue searching...");
                        refresh();

                        int ch = getch();
                        if (ch == KEY_UP) {
                            if (current_line > 0) {
                                current_line--;
                            }
                        } else if (ch == KEY_DOWN) {
                            if (current_line < results.size() - max_lines) {
                                current_line++;
                            }
                        } else if (ch == '\n') {
                            break;
                        } else if (ch == 27) {
                            endwin();
                            return;
                        }
                    }
                    curs_set(1);
                }
            }
        }
        endwin();
    }
};
#endif
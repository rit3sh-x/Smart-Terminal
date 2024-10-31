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
#include <unordered_map>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    std::vector<std::string> filePaths;

    ~TrieNode() {
        for (auto& pair : children) {
            delete pair.second;
        }
    }
};

class Trie {
public:
    Trie() : root(new TrieNode()) {}

    ~Trie() {
        delete root;
    }

    void insert(const std::string& filePath) {
        TrieNode* node = root;
        std::string filename = std::filesystem::path(filePath).filename().string();
        
        for (char ch : filename) {
            if (node->children.find(ch) == node->children.end()) {
                node->children[ch] = new TrieNode();
            }
            node = node->children[ch];
            node->filePaths.push_back(filePath);
        }
    }

    std::vector<std::string> search(const std::string& prefix) {
        TrieNode* node = root;
        for (char ch : prefix) {
            if (node->children.find(ch) == node->children.end()) {
                return {};
            }
            node = node->children[ch];
        }
        return node->filePaths;
    }

private:
    TrieNode* root;
};

class FileSearcher {
public:
    FileSearcher(const std::string& path) : searchPath(path) {
        results.reserve(100);
        trie = new Trie();
        buildTrie();
    }

    ~FileSearcher() {
        delete trie;
    }

    void run() {
        searchUI();
    }

private:
    std::mutex mtx;
    std::vector<std::string> results;
    std::vector<std::string> suggestions;
    std::string searchPath;
    Trie* trie;

    void clearFromRow(int startRow) {
        for (int row = startRow; row < LINES; ++row) {
            move(row, 0);
            clrtoeol();
        }
    }

    std::string toLower(const std::string& str) {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }

    void buildTrie() {
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
                std::string path = entry.path().string();
                trie->insert(path);
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::lock_guard<std::mutex> lock(mtx);
            results.push_back("Error: " + std::string(e.what()));
        }
    }

    void generateSuggestions(const std::string& input) {
        suggestions.clear();
        if (input.empty()) return;

        std::vector<std::string> matchedPaths = trie->search(toLower(input));
        for (const auto& path : matchedPaths) {
            suggestions.push_back(std::filesystem::path(path).filename().string());
            if (suggestions.size() >= 10) break;
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
                results = trie->search(searchWord);
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
#include <fstream>
#include <ncurses.h>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include "VersionManager.hpp"

namespace fs = std::filesystem;

class Buffer {
    public:
    void insert(const std::string& line) {
        lines.push_back(line);
    }

    void clear() {
        lines.clear();
    }

    std::vector<std::string>& getLines() {
        return lines;
    }

    std::string getContent() const {
        std::string content;
        for (const auto& line : lines) {
            content += line + "\n";
        }
        return content;
    }

    private:
    std::vector<std::string> lines;
};

class TextEditor {
public:
    TextEditor(const std::string& fname) :
        versionManager(fname),
        x(1), 
        y(0), 
        topRow(0),
        inInsertMode(false), 
        inCommandMode(true)
    {
        setFilename(fname);
    }

    void setFilename(const std::string& fname) {
        filename = fname;
        loadFile();
        x = 1;
        y = 0;
        topRow = 0;
        curs_set(0);
    }

    void insertText(const std::string& text) {
        buffer.insert(text);
    }

    void deleteText(size_t position, size_t length) {
        if (position < buffer.getLines().size()) {
            auto& line = buffer.getLines()[position];
            if (length <= line.size()) {
                line.erase(0, length);
                if (line.empty() && position + 1 < buffer.getLines().size()) {
                    buffer.getLines().erase(buffer.getLines().begin() + position);
                    if (topRow > 0 && position < topRow) {
                        topRow--;
                    }
                }
            }
        }
    }

    void saveFile() {
        if (!filename.empty()) {
            std::ofstream file(filename);
            if (file.is_open()) {
                file << buffer.getContent();
                file.close();
            }
        }
    }

    bool processCommand(const std::string& command) {
        if (command == "!wq") {
            versionManager.saveVersion();
            saveFile();
            return true;
        }
        if (command == "!q") {
            return true;
        }
        return false;
    }

    void display() {
        clear();
        int linesDisplayed = std::min(static_cast<int>(buffer.getLines().size()) - topRow, LINES - 1);

        for (int i = 0; i < linesDisplayed; ++i) {
            if (topRow + i < buffer.getLines().size()) {
                mvprintw(i, 0, "~%s", buffer.getLines()[topRow + i].c_str());
            }
        }
        drawScrollbar();
        refresh();
    }

    void drawStatusBar(bool isModified, bool insertMode, bool commandMode, const std::string& commandBuffer) {
        move(LINES - 1, 0);
        clrtoeol();
        if (insertMode) {
            attron(A_REVERSE);
            printw(" -- INSERT -- ");
            attroff(A_REVERSE);
        }
        if (commandMode) {
            printw(" :%s", commandBuffer.c_str());
        }
        if (isModified) {
            printw(" *");
        }
    }

    void runEditor() {
        int ch;
        bool isModified = false;
        std::string commandBuffer;

        while (true) {
            display();
            drawStatusBar(isModified, inInsertMode, inCommandMode, commandBuffer);
            if (inCommandMode) {
                move(LINES - 2, 18 + commandBuffer.length());
            } else {
                move(y - topRow, x);
            }
            refresh();
            ch = getch();
            if (ch == 8) {
                openVersionMenu();
                continue;
            }

            if (inInsertMode) {
                switch (ch) {
                    case KEY_BACKSPACE:
                    case 127:
                        handleBackspace(isModified);
                        break;

                    case KEY_ENTER:
                    case 10:
                        handleEnter(isModified);
                        break;

                    case KEY_UP:
                        if (y > 0) --y;
                        if (y < topRow) --topRow;
                        x = std::min(x, static_cast<int>(buffer.getLines()[y].size()) + 1);
                        break;

                    case KEY_DOWN:
                        if (y < buffer.getLines().size() - 1) ++y;
                        if (y >= topRow + LINES - 1) ++topRow;
                        x = std::min(x, static_cast<int>(buffer.getLines()[y].size()) + 1);
                        break;

                    case KEY_LEFT:
                        if (x > 1) --x;
                        break;

                    case KEY_RIGHT:
                        if (x <= buffer.getLines()[y].size()) ++x;
                        break;

                    case 27:
                        inInsertMode = false;
                        inCommandMode = true;
                        commandBuffer.clear();
                        curs_set(0);
                        break;

                    default:
                        if (isprint(ch)) {
                            if (buffer.getLines().empty()) {
                                buffer.insert("");
                            }
                            buffer.getLines()[y].insert(x - 1, 1, ch);
                            ++x;
                            isModified = true;
                        }
                        break;
                }
            } else if (inCommandMode) {
                switch (ch) {
                    case 'i':
                        inCommandMode = false;
                        inInsertMode = true;
                        curs_set(1);
                        break;

                    case '\n':
                        if (processCommand(commandBuffer)) {
                            endwin();
                            return;
                        } else {
                            commandBuffer.clear();
                        }
                        break;

                    case KEY_BACKSPACE:
                    case 127:
                        if (!commandBuffer.empty()) {
                            commandBuffer.pop_back();
                        }
                        break;

                    default:
                        if (isprint(ch)) {
                            commandBuffer.push_back(ch);
                        }
                        break;
                }
            }
        }
    }

private:
    Buffer buffer;
    std::string filename;
    int x, y, topRow;
    bool inInsertMode;
    bool inCommandMode;
    VersionManager versionManager;

    void loadFile() {
        if (fs::exists(filename)) {
            std::ifstream file(filename);
            std::string line;
            buffer.clear();
            while (std::getline(file, line)) {
                buffer.insert(line);
            }
        }
    }

    void openVersionMenu() {
        std::vector<std::string> versions = versionManager.listVersions();
        std::sort(versions.begin(), versions.end(), std::greater<std::string>());

        if (versions.empty()) {
            mvprintw(LINES / 2, (COLS - 20) / 2, "No versions available.");
            refresh();
            getch();
            return;
        }

        int selected = 0;
        while (true) {
            clear();
            mvprintw(0, (COLS - 20) / 2, "Select a version:");

            for (size_t i = 0; i < versions.size(); ++i) {
                if (i == selected) {
                    attron(A_REVERSE);
                }
                mvprintw(i + 2, (COLS - 20) / 2, "%s", versions[i].c_str());
                if (i == selected) {
                    attroff(A_REVERSE);
                }
            }

            refresh();
            int ch = getch();

            if (ch == KEY_UP && selected > 0) {
                --selected;
            } else if (ch == KEY_DOWN && selected < versions.size() - 1) {
                ++selected;
            } else if (ch == '\n') {
                versionManager.restoreVersion(versions[selected], filename);
                loadFile();
                break;
            } else if (ch == 27) {
                break;
            }
        }
    }

    void handleBackspace(bool& isModified) {
        if (x > 1) {
            buffer.getLines()[y].erase(x - 2, 1);
            --x;
            isModified = true;
        } else if (y > 0) {
            x = buffer.getLines()[y - 1].size() + 1;
            buffer.getLines()[y - 1] += buffer.getLines()[y];
            buffer.getLines().erase(buffer.getLines().begin() + y);
            --y;
            isModified = true;
            if (y < topRow) {
                topRow--;
            }
        }
    }

    void handleEnter(bool& isModified) {
        std::string currentLine = buffer.getLines()[y];
        std::string newLine = currentLine.substr(x - 1);
        buffer.getLines()[y] = currentLine.substr(0, x - 1);
        buffer.getLines().insert(buffer.getLines().begin() + y + 1, newLine);
        ++y;
        x = 1;
        isModified = true;
        if (y >= topRow + LINES - 1) {
            topRow++;
        }
    }

    void drawScrollbar() {
        int totalLines = buffer.getLines().size();
        int visibleLines = std::min(totalLines, LINES - 1);

        if (totalLines > visibleLines) {
            int scrollbarHeight = visibleLines;
            int scrollbarPos = (topRow * scrollbarHeight) / totalLines;
            for (int i = 0; i < scrollbarHeight; ++i) {
                if (i == scrollbarPos) {
                    attron(A_REVERSE);
                    mvprintw(i, COLS - 1, " ");
                    attroff(A_REVERSE);
                } else {
                    mvprintw(i, COLS - 1, " ");
                }
            }
        }
    }
};
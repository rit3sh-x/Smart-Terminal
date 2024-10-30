#include <iostream>
#include <ncurses.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <limits.h>
#include <algorithm>
#include <cstring>
#include <errno.h>
#include "TextEditor.hpp"
#include "FileSearcher.hpp"
#include "Torrent.hpp"

void displayDownloadStatus(const std::string& torrentName, float progress, int peers, float speed, bool completed) {
    clear();
    int height, width;
    getmaxyx(stdscr, height, width);
    mvprintw(height / 4, (width - 11) / 2, "DOWNLOADING");
    mvprintw(height / 4 + 1, (width - 38) / 2, "When you leave, download will be paused");
    mvprintw((height / 4) + 4, (width - (torrentName.size()) + 9) / 2, "Torrent: %s", torrentName.c_str());
    mvprintw((height / 4) + 5, (width - 40) / 2, "Speed: %.2f KB/s | Peers: %d | Progress: %.2f%%", speed, peers, progress);
    if (completed) {
        mvprintw(height - 1, 0, "Download completed!");
    } else {
        mvprintw(height - 1, 0, "Press ESC to exit");
    }
    refresh();
}

void runTorrentDownload(const std::string& input) {
    auto session = std::make_shared<libtorrent::session>();
    configureSession(session);
    Torrent torrent(session, input);
    curs_set(0);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    bool userExited = false;

    while (!torrent.isCompleted()) {
        torrent.handleAlerts();
        auto [progress, peers, speed] = torrent.getTorrentStatus();
        std::string torrentName = torrent.getTorrentName();
        displayDownloadStatus(torrentName, progress, peers, speed , false);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        int ch = getch();
        if (ch == 27) {
            userExited = true;
            break;
        }
    }

    if (torrent.isCompleted()) {
        displayDownloadStatus(torrent.getTorrentName(), 100.0f, 0, 0.0f, true);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } else if (userExited) {
        torrent.stopDownload();
    }
    curs_set(1);
    endwin();
}

void fileSearcher(std::string pathname) {
    FileSearcher fileSearcher(pathname);
    fileSearcher.run();
}

void textEditor(std::string filename) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);
    TextEditor editor(filename);
    editor.runEditor();
    endwin();
}

class Node {
public:
    std::string name;
    bool isDirectory;
    std::vector<Node*> children;

    Node(const std::string& name, bool isDir) : name(name), isDirectory(isDir) {}
};

class DirectoryTree {
private:
    Node* root;
    std::string currentPath;
    Node* currentNode;

public:
    DirectoryTree(const std::string& initialPath = ".") : root(new Node(".", true)) {
        if (chdir(initialPath.c_str()) == 0) {
            currentPath = getCurrentPath();
        } else {
            std::cerr << "Invalid path! Defaulting to the current directory." << std::endl;
            currentPath = getCurrentPath();
        }
        currentNode = root;
        loadDirectory(root, currentPath);
    }

    std::string getCurrentPath() {
        char buffer[PATH_MAX];
        return getcwd(buffer, sizeof(buffer)) ? std::string(buffer) : ".";
    }

    void loadDirectory(Node* node, const std::string& path) {
        node->children.clear();
        node->children.push_back(new Node("..", true));

        DIR* dir = opendir(path.c_str());
        if (!dir) {
            mvprintw(0, 0, "Failed to open directory: %s", path.c_str());
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == ".." || name == ".versions" || name == "RESUME_DIR") continue;

            bool isDir = (entry->d_type == DT_DIR);
            node->children.push_back(new Node(name, isDir));
        }
        closedir(dir);

        std::sort(node->children.begin(), node->children.end(), [](Node* a, Node* b) {
            if (a->isDirectory && !b->isDirectory) return true;
            if (!a->isDirectory && b->isDirectory) return false;
            return a->name < b->name;
        });
    }

    void enterDirectory(const std::string& dirName) {
        if (dirName == "..") {
            if (currentNode != root) {
                chdir("..");
                currentPath = getCurrentPath();
                loadDirectory(currentNode, currentPath);
            }
        } else {
            std::string newPath = currentPath + "/" + dirName;
            if (chdir(newPath.c_str()) == 0) {
                for (Node* child : currentNode->children) {
                    if (child->name == dirName && child->isDirectory) {
                        currentNode = child;
                        if (child->children.empty()) {
                            loadDirectory(child, newPath);
                        }
                        break;
                    }
                }
            }
            currentPath = getCurrentPath();
        }
    }

    const std::vector<Node*>& getCurrentEntries() const {
        return currentNode->children;
    }

    std::string getCurrentPathStr() const {
        return currentPath;
    }

    void refresh() {
        loadDirectory(currentNode, currentPath);
    }
};

class FileExplorer {
private:
    DirectoryTree dirTree;
    int selected = 0;  
    int offset = 0;    

public:
    FileExplorer(const std::string& initialPath) : dirTree(initialPath) {
        initscr();
        keypad(stdscr, TRUE);
        noecho();
        curs_set(0);
        displayEntries();
    }

    ~FileExplorer() {
        endwin();
    }

    void displayEntries() {
        clear();
        int maxHeight, maxWidth;
        getmaxyx(stdscr, maxHeight, maxWidth);
        mvprintw(0, 0, ": %s", dirTree.getCurrentPathStr().c_str());
        const auto& entries = dirTree.getCurrentEntries();
        int visibleLines = std::min(maxHeight - 4, static_cast<int>(entries.size()));

        for (int i = 0; i < visibleLines; ++i) {
            int entryIndex = i + offset;
            if (entryIndex >= entries.size()) break;
            if (entryIndex == selected) attron(A_REVERSE);
            mvprintw(i + 2, 0, "%s%s", entries[entryIndex]->name.c_str(), entries[entryIndex]->isDirectory ? "/" : "");
            if (entryIndex == selected) attroff(A_REVERSE);
        }
        refresh();
    }

    void createFile() {
        echo();
        mvprintw(getmaxy(stdscr) - 1, 0, "Enter file name: ");
        char filename[256];
        getnstr(filename, sizeof(filename) - 1);
        std::string baseFileName = filename;
        std::string filePath = dirTree.getCurrentPathStr() + "/" + baseFileName;
        size_t dotPos = baseFileName.find('.');
        std::string newFileName = baseFileName;
        int count = 1;
        while (access(filePath.c_str(), F_OK) == 0) {
            if (dotPos != std::string::npos) {
                newFileName = baseFileName.substr(0, dotPos) + " (" + std::to_string(count) + ")" + baseFileName.substr(dotPos);
            } else {
                newFileName = baseFileName + " (" + std::to_string(count) + ")";
            }
            filePath = dirTree.getCurrentPathStr() + "/" + newFileName;
            count++;
        }
        FILE* file = fopen(filePath.c_str(), "w");
        move(getmaxy(stdscr) - 1, 0);
        clrtoeol();
        if (file) {
            fclose(file);
            mvprintw(getmaxy(stdscr) - 1, 0, "File '%s' created.", filePath.c_str());
        } else {
            mvprintw(getmaxy(stdscr) - 1, 0, "Failed to create file: %s", strerror(errno));
        }
        noecho();
        dirTree.refresh();
        refresh();
        getch();
    }

    void createFolder() {
        echo();
        mvprintw(getmaxy(stdscr) - 1, 0, "Enter folder name: ");
        char foldername[256];
        getnstr(foldername, sizeof(foldername) - 1);
        std::string baseFolderName = foldername;
        std::string folderPath = dirTree.getCurrentPathStr() + "/" + baseFolderName;
        int count = 1;
        while (mkdir(folderPath.c_str(), 0755) != 0) {
            folderPath = dirTree.getCurrentPathStr() + "/" + baseFolderName + " (" + std::to_string(count) + ")";
            count++;
        }
        move(getmaxy(stdscr) - 1, 0);
        clrtoeol();
        mvprintw(getmaxy(stdscr) - 1, 0, "Folder '%s' created.", folderPath.c_str());
        noecho();
        dirTree.refresh();
        refresh();
        getch();
    }

    void run() {
        int ch;
        while ((ch = getch()) != 'q') {
            int maxHeight, maxWidth;
            getmaxyx(stdscr, maxHeight, maxWidth);

            const auto& entries = dirTree.getCurrentEntries();
            int totalEntries = entries.size();
            int visibleLines = std::min(maxHeight - 4, totalEntries);

            switch (ch) {
                case KEY_UP:
                    if (selected > 0) {
                        --selected;
                    } else if (offset > 0) {
                        --offset;
                    }
                    break;

                case KEY_DOWN:
                    if (selected < totalEntries - 1) {
                        ++selected;
                    }
                    if (selected >= visibleLines + offset - 1 && 
                        offset < totalEntries - visibleLines) {
                        ++offset;
                    }
                    break;

                case '\n':
                    if (entries[selected]->isDirectory) {
                        dirTree.enterDirectory(entries[selected]->name);
                        selected = 0;
                        offset = 0;
                    } else {
                        std::string filePath = dirTree.getCurrentPathStr() + "/" + entries[selected]->name;
                        if (filePath.substr(filePath.find_last_of(".") + 1) == "torrent") {
                            runTorrentDownload(filePath);
                        } else {
                            textEditor(filePath);
                        }
                    }
                    break;

                case 14:
                    createFile();
                    break;

                case 13:
                    createFolder();
                    break;

                case 6:
                    std::string path = dirTree.getCurrentPathStr();
                    fileSearcher(path);
                    break;
            }
            displayEntries();
        }
    }
};
#include <iostream>
#include <vector>
#include <string>

struct Node {
    std::string name;
    bool isDirectory;
    std::vector<Node*> children;

    Node(std::string n, bool isDir) : name(n), isDirectory(isDir) {}
};

// Function to add a new child (directory or file) to the specified parent node
void addChild(Node* parent, const std::string& name, bool isDirectory) {
    Node* child = new Node(name, isDirectory);
    parent->children.push_back(child);
    std::cout << (isDirectory ? "Directory " : "File ") << name << " added to " << parent->name << "\n";
}

// Function to remove a child file from a specified parent directory
bool removeChild(Node* parent, const std::string& name) {
    for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
        if ((*it)->name == name && !(*it)->isDirectory) {  // Only remove files
            delete *it;  // Free the memory
            parent->children.erase(it);
            std::cout << "File " << name << " removed from " << parent->name << "\n";
            return true;
        }
    }
    std::cout << "File " << name << " not found in " << parent->name << "\n";
    return false;
}

// Recursive function to display the tree structure
void displayStructure(Node* root, const std::string& prefix = "", bool isLast = true) {
    std::cout << prefix << (isLast ? "└── " : "├── ") << (root->isDirectory ? "[DIR] " : "[FILE] ") << root->name << "\n";
    std::string newPrefix = prefix + (isLast ? "    " : "│   ");
    for (size_t i = 0; i < root->children.size(); i++) {
        displayStructure(root->children[i], newPrefix, i == root->children.size() - 1);
    }
}

// Function to find a directory by name (returns nullptr if not found)
Node* findDirectory(Node* root, const std::string& name) {
    if (root->name == name && root->isDirectory) return root;
    for (Node* child : root->children) {
        if (child->isDirectory) {
            Node* found = findDirectory(child, name);
            if (found) return found;
        }
    }
    return nullptr;
}

// Interactive menu to navigate and manipulate the directory structure
void interactiveMenu(Node* root) {
    int choice;
    std::string parentName, itemName;

    while (true) {
        std::cout << "\nDirectory Management Menu:\n";
        std::cout << "1. Display Directory Structure\n";
        std::cout << "2. Add Directory\n";
        std::cout << "3. Add File\n";
        std::cout << "4. Remove File\n";
        std::cout << "5. Exit\n";
        std::cout << "Choose an option: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                std::cout << "\nCurrent Directory Structure:\n";
                displayStructure(root);
                break;

            case 2:
            case 3: {
                std::cout << "Enter the name of the parent directory: ";
                std::cin >> parentName;

                Node* parent = findDirectory(root, parentName);
                if (parent) {
                    std::cout << "Enter " << (choice == 2 ? "directory" : "file") << " name: ";
                    std::cin >> itemName;
                    bool isDir = (choice == 2);
                    addChild(parent, itemName, isDir);
                } else {
                    std::cout << "Parent directory not found.\n";
                }
                break;
            }

            case 4: {
                std::cout << "Enter the name of the parent directory: ";
                std::cin >> parentName;

                Node* parent = findDirectory(root, parentName);
                if (parent) {
                    std::cout << "Enter the name of the file to remove: ";
                    std::cin >> itemName;
                    removeChild(parent, itemName);
                } else {
                    std::cout << "Parent directory not found.\n";
                }
                break;
            }

            case 5:
                std::cout << "Exiting.\n";
                return;

            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }
}

int main() {
    Node* root = new Node("root", true); // Initialize root directory
    interactiveMenu(root); // Start interactive session
    return 0;
}

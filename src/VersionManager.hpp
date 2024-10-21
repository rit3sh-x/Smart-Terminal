#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <openssl/sha.h>
#include <iomanip>
#include <ctime>
#include <vector>

class VersionManager {
private:
    std::string fileName;
    std::filesystem::path fileDirectory;

    std::string getFileHash(const std::string& fileName) {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(fileName.c_str()), fileName.length(), hash);

        std::ostringstream oss;
        for (const auto& byte : hash) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&timeNow);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &tm);
        return std::string(buffer);
    }

public:
    VersionManager(const std::string& file) 
        : fileName(file), fileDirectory(std::filesystem::path(file).parent_path()) {
        std::filesystem::create_directories(fileDirectory / ".versions");
    }

    ~VersionManager() {}

    void saveVersion() {
        std::string fileHash = getFileHash(std::filesystem::path(fileName).filename().string());
        std::string timestamp = getCurrentTimestamp();
        std::filesystem::path saveFolder = fileDirectory / ".versions" / fileHash / timestamp;
        std::filesystem::create_directories(saveFolder);
        std::filesystem::copy(fileName, saveFolder / std::filesystem::path(fileName).filename());
        updateHead(fileHash, timestamp);
    }

    void updateHead(const std::string& fileHash, const std::string& timestamp) {
        std::ofstream headFile(fileDirectory / ".versions" / fileHash / "HEAD");
        headFile << timestamp;
        headFile.close();
    }

    std::string getLatestVersionFolder() {
        std::string fileHash = getFileHash(std::filesystem::path(fileName).filename().string());
        std::ifstream headFile(fileDirectory / ".versions" / fileHash / "HEAD");
        if (!headFile) return "";
        std::string latestTimestamp;
        std::getline(headFile, latestTimestamp);
        headFile.close();
        return (fileDirectory / ".versions" / fileHash / latestTimestamp).string();
    }

    std::vector<std::string> listVersions() {
        std::string fileHash = getFileHash(std::filesystem::path(fileName).filename().string());
        std::filesystem::path versionRoot = fileDirectory / ".versions" / fileHash;
        std::vector<std::string> timestamps;
        if (!std::filesystem::exists(versionRoot)) {
            std::cout << "No versions found for " << fileName << "." << std::endl;
            return timestamps;
        }
        for (const auto& entry : std::filesystem::directory_iterator(versionRoot)) {
            if (entry.is_directory() && entry.path().filename() != "HEAD") {
                timestamps.push_back(entry.path().filename().string());
            }
        }
        return timestamps;
    }

    void restoreVersion(const std::string& targetVersion, const std::string& restoredFile) {
        std::string fileHash = getFileHash(std::filesystem::path(fileName).filename().string());
        std::filesystem::path targetFolder = fileDirectory / ".versions" / fileHash / targetVersion;
        std::filesystem::path nativePath = targetFolder / std::filesystem::path(fileName).filename();
        if (std::filesystem::exists(nativePath)) {
            std::filesystem::create_directories(std::filesystem::path(restoredFile).parent_path());
            std::filesystem::copy(nativePath, restoredFile, std::filesystem::copy_options::overwrite_existing);
        } else {
            std::cout << "Version " << targetVersion << " not found!" << std::endl;
        }
    }
};
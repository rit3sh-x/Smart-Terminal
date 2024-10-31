#ifndef TORRENT_HPP
#define TORRENT_HPP

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <iomanip>
#include <algorithm>

const std::string RESUME_DIR = "./RESUME_DATA/";

void ensureResumeDirectory() {
    std::filesystem::create_directories(RESUME_DIR);
}

class Torrent {
    public:
    Torrent(std::shared_ptr<libtorrent::session> session, const std::string& input);
    ~Torrent();
    float getProgress() const { return progress; }
    int getPeers() const { return peers; }
    float getSpeed() const { return speed; }
    bool isCompleted() const { return completed; }
    std::string getErrorMessage() const { return errorMessage; }
    std::tuple<float, int, float> getTorrentStatus();
    void handleAlerts();
    void stopDownload();
    std::string getTorrentName() const;

    private:
    mutable std::mutex statusMutex;
    std::shared_ptr<libtorrent::session> session;
    libtorrent::torrent_handle handle;
    float progress = 0.0f;
    int peers = 0;
    float speed = 0.0f;
    bool completed = false;
    std::string errorMessage;
    std::thread downloadThread;
    bool stopFlag = false;
    void addTorrent(const std::string& input);
    void download();
    void saveResumeData();
    void loadResumeData(const std::string& torrentFile);
    std::string determineSavePath(const std::string& torrentFile);
};

Torrent::Torrent(std::shared_ptr<libtorrent::session> session, const std::string& input)
    : session(std::move(session)) {
    try {
        ensureResumeDirectory();
        loadResumeData(input);
        addTorrent(input);
        downloadThread = std::thread(&Torrent::download, this);
    } catch (const std::exception& e) {
        errorMessage = e.what();
    }
}

Torrent::~Torrent() {
    stopFlag = true;
    if (downloadThread.joinable()) {
        downloadThread.join();
    }
}

void Torrent::addTorrent(const std::string& input) {
    libtorrent::add_torrent_params params;
    if (std::filesystem::exists(input)) {
        params.ti = std::make_shared<libtorrent::torrent_info>(input);
        params.save_path = determineSavePath(input);
    } else if (input.find("magnet:") == 0) {
        libtorrent::error_code ec;
        params = libtorrent::parse_magnet_uri(input, ec);
        if (ec) throw std::runtime_error("Invalid magnet link: " + ec.message());
        params.save_path = std::filesystem::current_path().string();
    } else {
        throw std::runtime_error("Invalid input: Must be a valid torrent file or magnet link");
    }
    handle = session->add_torrent(std::move(params));
}

void Torrent::loadResumeData(const std::string& torrentFile) {
    std::string resumePath = RESUME_DIR + std::filesystem::path(torrentFile).filename().string() + ".resume";
    if (std::filesystem::exists(resumePath)) {
        std::ifstream resumeFile(resumePath, std::ios::binary);
        if (!resumeFile) {
            std::cerr << "Failed to open resume file: " << resumePath << std::endl;
            return;
        }
        std::vector<char> buffer(std::istreambuf_iterator<char>(resumeFile), {});
        libtorrent::error_code ec;
        auto optionalParams = libtorrent::read_resume_data(buffer, ec);
        if (ec) {
            std::cerr << "Failed to load resume data: " << ec.message() << std::endl;
            return;
        } else {
            handle = session->add_torrent(std::move(optionalParams));
        }
    }
}

std::string Torrent:: getTorrentName() const{
    std::lock_guard<std::mutex> lock(statusMutex);
    if (handle.is_valid() && handle.torrent_file()) {
        return handle.torrent_file()->name();
    }
    return "Unknown";
}

void Torrent::handleAlerts() {
    std::vector<libtorrent::alert*> alerts;
    session->pop_alerts(&alerts);
    for (auto* alert : alerts) {
        if (auto* err = dynamic_cast<libtorrent::torrent_error_alert*>(alert)) {
            errorMessage = err->message();
        } else if (auto* saveAlert = dynamic_cast<libtorrent::save_resume_data_alert*>(alert)) {
            std::ofstream resumeFile(RESUME_DIR + saveAlert->handle.torrent_file()->name() + ".resume", std::ios::binary);
            auto buffer = libtorrent::write_resume_data_buf(saveAlert->params);
            resumeFile.write(buffer.data(), buffer.size());
        }
    }
}

std::string Torrent::determineSavePath(const std::string& torrentFile) {
    std::filesystem::path path(torrentFile);
    return path.has_parent_path() ? path.parent_path().string() : std::filesystem::current_path().string();
}

void Torrent::download() {
    while (!completed && !stopFlag) {
        auto [currentProgress, currentPeers, currentSpeed] = getTorrentStatus();
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            progress = currentProgress;
            peers = currentPeers;
            speed = currentSpeed;
            completed = (progress >= 100.0f);
        }
        if (completed) {
            saveResumeData();
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void Torrent::saveResumeData() {
    handle.save_resume_data();
}

std::tuple<float, int, float> Torrent::getTorrentStatus() {
    std::lock_guard<std::mutex> lock(statusMutex);
    auto status = handle.status();
    return {status.progress * 100, status.num_peers, static_cast<float>(status.download_rate) / 1000.0f};
}

void Torrent::stopDownload() {
    handle.pause();
    saveResumeData();
}

void configureSession(std::shared_ptr<libtorrent::session>& session) {
    libtorrent::settings_pack settings;
    settings.set_int(libtorrent::settings_pack::alert_mask, libtorrent::alert::status_notification | libtorrent::alert::error_notification);
    session->apply_settings(settings);
}
#endif
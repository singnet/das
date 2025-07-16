#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>

class LogHandle {
public:
    explicit LogHandle(const std::string& log_path)
        : logPath(log_path) {}

    std::string getOutput() const {
        std::ifstream file(logPath);
        if (!file) return "";

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

private:
    std::string logPath;
};

class FileWatcher {
public:
    explicit FileWatcher(const std::string& watchDir, float timeout_sec = 20.0)
        : watchDir_(watchDir) {
        start();
        waitForRunningFile(timeout_sec);
    }

    LogHandle* createLogHandle(const std::string& logPath) const {
        return new LogHandle(logPath);
    }

    void start() {
        // Write a start file to initiate the process
        std::ofstream startFile(watchDir_ + "/start", std::ios::out);
        std::cout << "Creating start file in " << watchDir_ << std::endl;
        if (startFile) {
            startFile << "Process started" << std::endl;
            startFile.close();
        } else {
            std::cerr << "Failed to create start file in " << watchDir_ << std::endl;
            throw std::runtime_error("Failed to create start file");
        }
    }

    void stop() {
        // Write an end file to stop the process
        std::ofstream endFile(watchDir_ + "/end");
        if (endFile) {
            endFile << "Process ended" << std::endl;
            endFile.close();
        } else {
            std::cerr << "Failed to create end file in " << watchDir_ << std::endl;
            throw std::runtime_error("Failed to create end file");
        }
    }

private:
    std::string watchDir_;

    void waitForRunningFile(float timeout_sec) const {
        const std::string runningFile = watchDir_ + "/running";

        std::cout << "Waiting for file: " << runningFile << std::endl;

        while (!std::filesystem::exists(runningFile)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (timeout_sec <= 0) {
                std::cerr << "Timeout waiting for file: " << runningFile << std::endl;
                return;
            }
            timeout_sec -= 0.5f; // Decrease timeout by 0.5 seconds
        }

        std::cout << "Detected file: " << runningFile << std::endl;
    }
};
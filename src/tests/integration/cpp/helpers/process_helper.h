#pragma once

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

class Process {
   public:
    Process(const std::string& path,
            const std::vector<std::string>& args = {},
            const std::map<std::string, std::string>& envs = {})
        : path_(path), args_(args), envs_(envs), pid_(-1), stdout_pipe_(-1), running_(false) {}

    bool start() {
        struct stat stat_buf;
        if (stat(path_.c_str(), &stat_buf) != 0 || !S_ISREG(stat_buf.st_mode)) {
            throw std::runtime_error("Executable not found: " + path_);
        }

        int pipefd[2];
        if (pipe(pipefd) != 0) return false;

        pid_ = fork();
        if (pid_ < 0) return false;

        if (pid_ == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            std::vector<std::string> env_strings;
            for (const auto& kv : envs_) {
                env_strings.push_back(kv.first + "=" + kv.second);
            }

            std::vector<char*> envp;
            for (auto& env : env_strings) {
                envp.push_back(const_cast<char*>(env.c_str()));
            }
            envp.push_back(nullptr);

            std::vector<char*> exec_args;
            exec_args.push_back(const_cast<char*>(path_.c_str()));
            for (const auto& arg : args_) {
                exec_args.push_back(const_cast<char*>(arg.c_str()));
            }
            exec_args.push_back(nullptr);

            execve(path_.c_str(), exec_args.data(), envp.data());
            _exit(1);
        } else {
            close(pipefd[1]);
            stdout_pipe_ = pipefd[0];
            fcntl(stdout_pipe_, F_SETFL, O_NONBLOCK);
            running_ = true;
            output_thread_ = std::thread([this]() { captureOutput(); });
            return true;
        }
    }

    void stop(int timeout_ms = 1000) {
        if (pid_ <= 0) return;

        kill(pid_, SIGTERM);
        try {
            wait(timeout_ms);
        } catch (...) {
        }
        if (pid_ > 0) {
            kill(pid_, SIGKILL);
            try {
                wait(1000);
            } catch (...) {
            }
        }
    }

    bool waitForExit(int timeout_ms = 1000) {
        if (pid_ <= 0) return true;

        int status;
        int elapsed = 0;
        const int interval = 50;

        while (elapsed < timeout_ms) {
            pid_t result = waitpid(pid_, &status, WNOHANG);
            if (result == pid_) {
                running_ = false;
                if (output_thread_.joinable()) output_thread_.join();
                pid_ = -1;
                return true;
            }
            usleep(interval * 1000);
            elapsed += interval;
        }
        return false;
    }

    std::string getOutput() const { return accumulated_output_.str(); }

    pid_t pid() const { return pid_; }

    ~Process() {
        stop(500);
        if (output_thread_.joinable()) output_thread_.join();
        if (stdout_pipe_ >= 0) close(stdout_pipe_);
    }

   private:
    void captureOutput() {
        char buffer[1024];
        struct pollfd pfd = {stdout_pipe_, POLLIN, 0};

        while (running_) {
            int ret = poll(&pfd, 1, 100);
            if (ret > 0) {
                ssize_t count = read(stdout_pipe_, buffer, sizeof(buffer) - 1);
                if (count > 0) {
                    buffer[count] = '\0';
                    accumulated_output_ << buffer;
                } else if (count == 0) {
                    break;
                }
            }
        }
    }

    std::string path_;
    std::vector<std::string> args_;
    std::map<std::string, std::string> envs_;
    pid_t pid_;
    int stdout_pipe_;
    std::stringstream accumulated_output_;
    std::thread output_thread_;
    std::atomic<bool> running_;
};

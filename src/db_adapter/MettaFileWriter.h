#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace std;

namespace db_adapter {

class MettaFileWriter {
   public:
    static constexpr size_t DEFAULT_MAX_FILE_SIZE = 1ULL * 1024 * 1024 * 1024;  // 1 GB
    static constexpr size_t IO_BUFFER_SIZE = 8ULL * 1024 * 1024;                // 8 MB

    /**
     * @param output_folder  Directory where .metta files are written.
     * @param max_file_size  Creates a new file when current one reaches this size. 0 = no limit.
     */
    explicit MettaFileWriter(string output_folder = "./knowledge_base",
                             size_t max_file_size = DEFAULT_MAX_FILE_SIZE);
    ~MettaFileWriter();

    MettaFileWriter(const MettaFileWriter&) = delete;
    MettaFileWriter& operator=(const MettaFileWriter&) = delete;

    /** Appends one metta expression as a line. Thread-safe. */
    void write(const string& expression);

    /** Flushes OS write buffers to disk. */
    void flush();

    /**
     * Removes duplicate lines from all files written in this session,
     * then closes them. Called automatically by the destructor.
     */
    void close();

   private:
    struct FileState {
        // 8 MB buffer keeps syscall count low (set before open — see open_file).
        vector<char> io_buf = vector<char>(IO_BUFFER_SIZE);
        ofstream stream;
        size_t current_size = 0;
        int file_idx = 0;

        FileState() = default;
        FileState(FileState&&) = delete;
        FileState& operator=(FileState&&) = delete;
    };

    void ensure_open();
    void open_file(int idx);
    void rotate_file();

    /**
     * Deduplicates a file in a single streaming pass.
     * A temp file is written and then replaces the original atomically.
     */
    void remove_duplicate_lines(const filesystem::path& path);

    static string make_file_prefix();

    string output_folder;
    size_t max_file_size;
    string file_prefix;
    unique_ptr<FileState> state;
    mutex mtx;
    atomic<size_t> total_written{0};
};

}  // namespace db_adapter
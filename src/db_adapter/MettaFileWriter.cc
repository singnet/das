#include "MettaFileWriter.h"

#include <sys/stat.h>

#include <chrono>
#include <sstream>
#include <unordered_set>

#include "Utils.h"
#include "expression_hasher.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace std;
using namespace commons;
using namespace db_adapter;

// ==============================
//  Construction / destruction
// ==============================

MettaFileWriter::MettaFileWriter(string output_folder, size_t max_file_size)
    : output_folder(move(output_folder)), max_file_size(max_file_size), file_prefix(make_file_prefix()) {
    filesystem::create_directories(this->output_folder);
}

MettaFileWriter::~MettaFileWriter() { this->close(); }

// ==============================
//  Public
// ==============================

void MettaFileWriter::write(const string& expression) {
    string line = expression + "\n";
    size_t line_size = line.size();

    lock_guard<mutex> lock(mtx);

    this->ensure_open();

    if (max_file_size > 0 && this->state->current_size + line_size > max_file_size) {
        this->rotate_file();
    }

    this->state->stream.write(line.data(), static_cast<streamsize>(line_size));
    if (!this->state->stream) {
        RAISE_ERROR("MettaFileWriter: write failed");
    }

    this->state->current_size += line_size;
    total_written.fetch_add(1, memory_order_relaxed);
}

void MettaFileWriter::flush() {
    lock_guard<mutex> lock(mtx);
    if (this->state && this->state->stream.is_open()) {
        this->state->stream.flush();
    }
}

void MettaFileWriter::close() {
    lock_guard<mutex> lock(mtx);

    if (this->state && this->state->stream.is_open()) {
        this->state->stream.flush();
        this->state->stream.close();
    }
    this->state.reset();

    // Deduplicate every file produced in this session.
    for (const auto& entry : filesystem::directory_iterator(this->output_folder)) {
        bool is_our_file =
            entry.is_regular_file() && entry.path().filename().string().find(this->file_prefix) == 0;
        if (is_our_file) {
            this->remove_duplicate_lines(entry.path());
        }
    }

    LOG_INFO("MettaFileWriter closed | total_written: " << total_written.load());
}

// ==============================
//  Private
// ==============================

void MettaFileWriter::ensure_open() {
    if (!this->state) {
        this->state = make_unique<FileState>();
        this->open_file(0);
    }
}

void MettaFileWriter::open_file(int idx) {
    string filename = this->file_prefix + "_" + to_string(idx) + ".metta";
    auto filepath = filesystem::path(this->output_folder) / filename;

    // pubsetbuf must be called before open() — otherwise the custom buffer is ignored.
    ofstream stream;
    stream.rdbuf()->pubsetbuf(this->state->io_buf.data(),
                              static_cast<streamsize>(this->state->io_buf.size()));
    stream.open(filepath, ios::app | ios::binary);

    if (!stream.is_open()) {
        RAISE_ERROR("MettaFileWriter: cannot open " + filepath.string());
    }

    this->state->stream = move(stream);
    this->state->file_idx = idx;
    this->state->current_size =
        filesystem::exists(filepath) ? static_cast<size_t>(filesystem::file_size(filepath)) : 0;

    LOG_DEBUG("MettaFileWriter opened " << filepath.string() << " (idx=" << idx << ")");
}

void MettaFileWriter::rotate_file() {
    int next_idx = this->state->file_idx + 1;

    LOG_DEBUG("MettaFileWriter rotating to idx=" << next_idx
                                                 << " | closed size=" << this->state->current_size);

    this->state->stream.flush();
    this->state->stream.close();
    this->open_file(next_idx);
}

void MettaFileWriter::remove_duplicate_lines(const filesystem::path& filepath) {
    auto tmp_path = filesystem::path(filepath).replace_extension(".tmp");

    ifstream in(filepath);
    ofstream out(tmp_path, ios::trunc | ios::binary);

    if (!in.is_open() || !out.is_open()) {
        RAISE_ERROR("MettaFileWriter: cannot open files for dedup: " + filepath.string());
        return;
    }

    unordered_set<size_t> seen_hashes;
    hash<string> hasher;
    string line;
    size_t total = 0;
    size_t kept = 0;

    while (getline(in, line)) {
        ++total;
        if (seen_hashes.insert(hasher(line)).second) {
            out.write(line.data(), static_cast<streamsize>(line.size()));
            out.put('\n');
            ++kept;
        }
    }

    in.close();
    out.flush();
    out.close();

    filesystem::rename(tmp_path, filepath);

    LOG_DEBUG("MettaFileWriter dedup " << filepath.filename().string() << " | total=" << total
                                       << " | kept=" << kept << " | removed=" << (total - kept));
}

string MettaFileWriter::make_file_prefix() {
    auto ms = Utils::get_current_time_millis();
    return compute_hash((char*) to_string(ms).c_str());
}
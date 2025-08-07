#pragma once

#include <map>
#include <memory>
#include <queue>
#include <string>

#include "Token.h"

using namespace std;

namespace metta {

enum LexerState { START, READING_LITERAL_STRING, READING_NUMBER, READING_SYMBOL_OR_VARIABLE };

/**
 * This is a lexer for simple MeTTa expressions used in the context of DAS. This is not an "official"
 * MeTTa lexer, which means that it may be missing actual aspects of the language. The idea of this
 * lexer is to cover only the needs of DAS components.
 *
 * MettaLexer can work with input on string objects or (exclusive or) on files. Multiple strings or
 * multiple files can be scanned at once. When multiple strings/files are being scanned, the
 * list of tokens is exactly the same as if the input were presented as a single string/file
 * composed by the concatenation of all the strings/files.
 *
 * For instance, attaching a string "(s1 s11 (-4.0 $v1 $v2))" will generate the exact same tokens
 * as if it was attach the list of strings "(s1 s11 (-", "4.0 $v1 $v" and "2))". The same aplies to
 * multiple files.
 */
class MettaLexer {
   public:
    /**
     * Constructor.
     *
     * @param input_buffer_size The size (in bytes) of the internal buffer used actually scan the input.
     */
    MettaLexer(unsigned int input_buffer_size = DEFAULT_INPUT_BUFFER_SIZE);

    /**
     * Constructor.
     *
     * @param metta_string A string to scan. When this constructor is used, MettaLexer assumes there
     * will not be any other attached strings.
     */
    MettaLexer(const string& metta_string);

    /**
     * Destructor.
     */
    ~MettaLexer();

    /**
     * Attachs a files to be scanned.
     *
     * @param file_name File to be scanned.
     */
    void attach_file(const string& file_name);

    /**
     * Attachs a string to be scanned.
     *
     * @param metta_string string to be scanned.
     */
    void attach_string(const string& metta_string);

    /**
     * Scans through input and return the next token, or nullptr when all the input files/strings has
     * been scanned.
     */
    unique_ptr<Token> next();

   private:
    void _init(unsigned int input_buffer_size);
    void _attach_string(const string& metta_string);
    inline char _read_next_char();
    void _rewind_input_buffer(unsigned int n);
    void _feed_from_file();
    bool _feed_input_buffer();
    void _error(LexerState state, const string& error_message, char c);

    static unsigned int DEFAULT_INPUT_BUFFER_SIZE;

    char* input_buffer;
    unsigned int input_buffer_size;
    bool single_string_flag;
    bool file_input_flag;
    unsigned int reading_cursor;
    unsigned int writing_cursor;
    unsigned int line_number;
    queue<string> attached_strings;
    queue<string> attached_file_names;
    map<string, long> current_offset;
};

}  // namespace metta

#include "MettaLexer.h"

#include "MettaTokens.h"
#include "Utils.h"

#define LOG_LEVEL INFO_LEVEL
#include "Logger.h"

using namespace metta;
using namespace commons;

unsigned int MettaLexer::DEFAULT_INPUT_BUFFER_SIZE = (unsigned int) 100000000;  // 100M

// -------------------------------------------------------------------------------------------------
// Constructors, destructors and initializers

MettaLexer::MettaLexer(unsigned int input_buffer_size) { _init(input_buffer_size); }

MettaLexer::MettaLexer(const string& metta_string) {
    _init(metta_string.size());
    this->single_string_flag = true;
    this->file_input_flag = false;
    _attach_string(metta_string);
}

void MettaLexer::_init(unsigned int input_buffer_size) {
    this->reading_cursor = 0;
    this->writing_cursor = 0;
    this->line_number = 1;
    this->input_buffer = (char*) malloc((input_buffer_size + 1) * sizeof(char));
    this->input_buffer[0] = '\0';
    this->input_buffer_size = input_buffer_size;
    this->single_string_flag = false;
    this->file_input_flag = true;
    this->current_metta_string.reserve(1000);  // any small number is OK
}

MettaLexer::~MettaLexer() { free(this->input_buffer); }

// -------------------------------------------------------------------------------------------------
// Public methods

void MettaLexer::attach_string(const string& metta_string) {
    LOG_DEBUG("Attaching string <" + metta_string + ">");
    if (this->single_string_flag) {
        Utils::error("Can't attach strings to MettaLexer initialized with a string.");
    } else if (this->file_input_flag) {
        if (this->attached_file_names.size() > 0) {
            Utils::error("Can't attach strings to MettaLexer which is processing files.");
        } else {
            _attach_string(metta_string);
            this->file_input_flag = false;
        }
    } else {
        _attach_string(metta_string);
    }
}

void MettaLexer::attach_file(const string& file_name) {
    LOG_DEBUG("Attaching file " + file_name);
    if (this->file_input_flag) {
        this->attached_file_names.push(file_name);
        this->current_offset[file_name] = 0;
    } else {
        Utils::error("Can't attach files to MettaLexer which is processing strings.");
    }
}

unique_ptr<Token> MettaLexer::next() {
    bool number_is_integer = true;
    string number_string = "";
    LexerState state = START;
    bool escape_flag = false;
    unique_ptr<Token> current_token = nullptr;
    while (true) {
        char c = _read_next_char();
        LOG_DEBUG("NEW CHAR: " << c);
        switch (state) {
            case START: {
                LOG_DEBUG("START");
                switch (c) {
                    case '\0':
                        return nullptr;
                    case '(':
                        return make_unique<Token>(MettaTokens::OPEN_PARENTHESIS, "(");
                    case ')':
                        return make_unique<Token>(MettaTokens::CLOSE_PARENTHESIS, ")");
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                        break;
                    case '\"': {
                        state = READING_LITERAL_STRING;
                        current_token = make_unique<Token>(MettaTokens::STRING_LITERAL, "\"");
                        break;
                    }
                    case '-':
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        state = READING_NUMBER;
                        number_string.push_back(c);
                        break;
                    }
                    default: {
                        state = READING_SYMBOL_OR_VARIABLE;
                        if (c == '$') {
                            current_token = make_unique<Token>(MettaTokens::VARIABLE);
                        } else {
                            current_token = make_unique<Token>(MettaTokens::SYMBOL);
                            current_token->text.push_back(c);
                        }
                    }
                }
                break;
            }
            case READING_LITERAL_STRING: {
                LOG_DEBUG("READING_LITERAL_STRING");
                current_token->text.push_back(c);
                switch (c) {
                    case '\0': {
                        state = START;
                        return current_token;
                    }
                    case '\"': {
                        if (!escape_flag) {
                            state = START;
                            return current_token;
                        }
                        escape_flag = false;
                        break;
                    }
                    case '\\': {
                        escape_flag = true;
                        break;
                    }
                    default: {
                        escape_flag = false;
                    }
                }
                break;
            }
            case READING_NUMBER: {
                LOG_DEBUG("READING_NUMBER");
                switch (c) {
                    case '(':
                    case ')':
                        _rewind_input_buffer(1);
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0': {
                        state = START;
                        if (number_is_integer) {
                            return make_unique<Token>(MettaTokens::INTEGER_LITERAL, number_string);
                        } else {
                            return make_unique<Token>(MettaTokens::FLOAT_LITERAL, number_string);
                        }
                        break;
                    }
                    case '.':
                        number_is_integer = false;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        number_string.push_back(c);
                        break;
                    }
                    default: {
                        _error(state, "Invalid number character", c);
                        return nullptr;
                    }
                }
                break;
            }
            case READING_SYMBOL_OR_VARIABLE: {
                LOG_DEBUG("READING_SYMBOL_OR_VARIABLE");
                switch (c) {
                    case '(':
                    case ')':
                        _rewind_input_buffer(1);
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0': {
                        state = START;
                        return current_token;
                    }
                    default:
                        current_token->text.push_back(c);
                }
                break;
            }
            default: {
                LOG_DEBUG("default");
                _error(state, "Invalid state", c);
                return nullptr;
            }
        }
    }
}

void MettaLexer::stack_metta_string() {
    this->current_metta_string.pop_back();
    this->metta_string_stack.push(this->current_metta_string);
    this->current_metta_string = "(";
}

void MettaLexer::pop_metta_string() {
    this->current_metta_string = metta_string_stack.top() + this->current_metta_string;
    this->metta_string_stack.pop();
}

// -------------------------------------------------------------------------------------------------
// Private methods

void MettaLexer::_attach_string(const string& metta_string) {
    if (metta_string.size() > this->input_buffer_size) {
        Utils::error("Can't attach strings larger than input buffer size (" +
                     std::to_string(metta_string.size()) + " > " +
                     std::to_string(this->input_buffer_size) + ")");
    } else {
        this->attached_strings.push(metta_string);
    }
}

char MettaLexer::_read_next_char() {
    char c = 0;
    if (this->reading_cursor < this->writing_cursor) {
        c = this->input_buffer[this->reading_cursor++];
    } else {
        if (_feed_input_buffer()) {
            c = this->input_buffer[this->reading_cursor++];
        }
    }
    if (c == '\n') {
        this->line_number++;
    }
    this->current_metta_string.push_back(c);
    return c;
}

void MettaLexer::_rewind_input_buffer(unsigned int n) {
    if (this->reading_cursor >= n) {
        this->reading_cursor -= n;
        for (unsigned int i = 0; i < n; i++) {
            this->current_metta_string.pop_back();
        }
    } else {
        Utils::error("Trying to rewind input buffer beyond its start");
    }
}

void MettaLexer::_feed_from_file() {
    string filename = this->attached_file_names.front();
    FILE* file = fopen(filename.c_str(), "r");
    if (file == NULL) {
        Utils::error("Unable to open file: " + filename);
    }
    fseek(file, this->current_offset[filename], SEEK_SET);
    int c = fgetc(file);
    while (c != EOF) {
        this->current_offset[filename]++;
        this->input_buffer[this->writing_cursor++] = (char) c;
        if (this->writing_cursor == this->input_buffer_size) {
            break;
        }
        c = fgetc(file);
    }
    fclose(file);
    this->input_buffer[this->writing_cursor] = '\0';  // there's extra room for this '\0'
    if (c == EOF) {
        this->current_offset[filename] = -1;
        this->attached_file_names.pop();
    }
}

bool MettaLexer::_feed_input_buffer() {
    LOG_DEBUG("Feeding input buffer");
    this->reading_cursor = 0;
    this->writing_cursor = 0;
    this->input_buffer[this->reading_cursor] = '\0';
    if (this->file_input_flag) {
        while ((this->writing_cursor < this->input_buffer_size) &&
               (this->attached_file_names.size() > 0)) {
            _feed_from_file();
        }
    } else {
        if (this->attached_strings.size() > 0) {
            while ((this->attached_strings.size() > 0) &&
                   (this->attached_strings.front().size() <=
                    (this->input_buffer_size - this->writing_cursor))) {
                for (char c : this->attached_strings.front()) {
                    this->input_buffer[this->writing_cursor++] = c;
                }
                this->attached_strings.pop();
                this->input_buffer[this->writing_cursor] = '\0';  // there's extra room for this '\0'
            }
        }
    }
    return (this->writing_cursor > 0);
}

void MettaLexer::_error(LexerState state, const string& error_message, char c) {
    string c_str;
    if (c == '\n') {
        c_str = "\\n";
    } else {
        c_str = string(1, c);
    }
    string prefix = "MettaLexer error in line " + std::to_string(this->line_number) +
                    " - near character '" + c_str + " - ";
    if (state == START) {
        prefix += "Unexpected character";
    } else if (state == READING_LITERAL_STRING) {
        prefix += "Error reading literal string";
    } else if (state == READING_NUMBER) {
        prefix += "Error reading number";
    } else if (state == READING_SYMBOL_OR_VARIABLE) {
        prefix += "Error reading symbol or variable name";
    } else {
        prefix += "Unexpected state value: " + std::to_string((unsigned int) state);
    }
    Utils::error(prefix + " - " + error_message);
}

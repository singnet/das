#include "CountLetterFunction.h"

#include "Utils.h"

using namespace fitness_functions;

string CountLetterFunction::VARIABLE_NAME = "sentence1";
char CountLetterFunction::LETTER_TO_COUNT = 'c';

float CountLetterFunction::eval(shared_ptr<QueryAnswer> query_answer, shared_ptr<Keychain> keychain) {
    if (query_answer->get_handles_size() != 1) {
        RAISE_ERROR("Invalid answer in CountLetterFunction");
        return 0;
    } else {
        shared_ptr<Link> sentence_link;
        string handle = query_answer->assignment.get(VARIABLE_NAME);
        string sentence_name;
        if (this->key_sensitive_atomdb == nullptr) {
            sentence_link = this->atomdb->get_link(handle);
            handle = sentence_link->targets[1];
            sentence_name = this->atomdb->get_node_name(handle);
        } else {
            sentence_link = this->key_sensitive_atomdb->get_link(handle, keychain);
            handle = sentence_link->targets[1];
            sentence_name = this->key_sensitive_atomdb->get_node_name(handle, keychain);
        }
        if (sentence_name.empty()) {
            RAISE_ERROR("CountLetterFunction: node does not exist: " + handle);
        }
        unsigned int count = 0;
        unsigned int sentence_length = 0;
        for (unsigned int i = 0; i < sentence_name.length(); i++) {
            if ((sentence_name[i] != ' ') && (sentence_name[i] != '"')) {
                sentence_length++;
            }
            if (sentence_name[i] == LETTER_TO_COUNT) {
                count++;
            }
        }
        if (sentence_length == 0) {
            return 0;
        } else {
            return ((float) count) / ((float) sentence_length);
        }
    }
}

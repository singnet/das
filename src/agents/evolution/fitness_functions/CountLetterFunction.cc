#include "CountLetterFunction.h"

#include "AtomDBSingleton.h"
#include "Utils.h"

using namespace fitness_functions;

string CountLetterFunction::VARIABLE_NAME = "sentence1";
char CountLetterFunction::LETTER_TO_COUNT = 'c';

CountLetterFunction::CountLetterFunction() { this->db = AtomDBSingleton::get_instance(); }

float CountLetterFunction::eval(shared_ptr<QueryAnswer> query_answer) {
    if (query_answer->handles.size() != 1) {
        Utils::error("Invalid answer in CountLetterFunction");
        return 0;
    } else {
        shared_ptr<atomdb_api_types::AtomDocument> sentence_document;
        shared_ptr<atomdb_api_types::AtomDocument> sentence_name_document;
        string handle = query_answer->assignment.get(VARIABLE_NAME);
        sentence_document = this->db->get_atom_document(handle);
        handle = string(sentence_document->get("targets", 1));
        sentence_name_document = this->db->get_atom_document(handle);
        const char* sentence_name = sentence_name_document->get("name");
        unsigned int count = 0;
        unsigned int sentence_length = 0;
        for (unsigned int i = 0; sentence_name[i] != '\0'; i++) {
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

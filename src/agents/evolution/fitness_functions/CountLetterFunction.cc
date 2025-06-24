#include "CountLetterFunction.h"
#include "Utils.h"
#include "AtomDBSingleton.h"

#define VARIABLE_NAME "sentence1"
#define LETTER_TO_COUNT 'c'

using namespace fitness_functions;

CountLetterFunction::CountLetterFunction() {
    this->db = AtomDBSingleton::get_instance();
}

float CountLetterFunction::eval(shared_ptr<QueryAnswer> query_answer) { 

    if (query_answer->handles_size != 1) {
        Utils::error("Invalid answer in CountLetterFunction");
        return 0;
    } else {
        shared_ptr<atomdb_api_types::AtomDocument> sentence_document;
        shared_ptr<atomdb_api_types::AtomDocument> sentence_name_document;
        const char* handle;
        handle = query_answer->assignment.get(VARIABLE_NAME);
        sentence_document = db->get_atom_document(handle);
        handle = sentence_document->get("targets", 1);
        sentence_name_document = db->get_atom_document(handle);
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

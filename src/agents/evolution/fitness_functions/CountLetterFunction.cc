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
        shared_ptr<Link> sentence_link;
        shared_ptr<Node> sentence_name_node;
        string handle = query_answer->assignment.get(VARIABLE_NAME);
        sentence_link = this->db->get_link(handle);
        handle = sentence_link->targets[1];
        sentence_name_node = this->db->get_node(handle);
        string sentence_name = sentence_name_node->name;
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

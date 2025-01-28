#include "link.h"


using namespace link_creation_agent;
using namespace std;
using namespace query_engine;


Link::Link(string type, vector<string> targets)
{
    this->type = type;
    this->targets = targets;
}

Link::Link(QueryAnswer query_answer, vector<string> link_template)
{
    string query_tokens = query_answer.tokenize();
    string token = "";
    for(char token_char : query_tokens){
        if(token_char == ' '){
            this->targets.push_back(token);
            token = "";
        }else{
            token += token_char;
        }
    }
}


Link::Link()
{
}

Link::~Link()
{
}

string Link::get_type()
{
    return this->type;
}

vector<string> Link::get_targets()
{
    return this->targets;
}

void Link::set_type(string type)
{
    this->type = type;
}

void Link::set_targets(vector<string> targets)
{
    this->targets = targets;
}

void Link::add_target(string target)
{
    this->targets.push_back(target);
}

vector<string> Link::tokenize()
{
    return targets;
}


Link Link::untokenize(string link)
{
    vector<string> tokens;
    string token;
    for (char c : link)
    {
        if (c == ' ')
        {
            tokens.push_back(token);
            token = "";
        }
        else
        {
            token += c;
        }
    }
    tokens.push_back(token);
    Link answer;
    answer.set_type(tokens[0]);
    for (unsigned int i = 1; i < tokens.size(); i++)
    {
        answer.add_target(tokens[i]);
    }
    return answer;
}

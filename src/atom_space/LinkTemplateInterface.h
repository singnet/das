#pragma once

#include <memory>

using namespace std;

class LinkTemplateInterface {
   public:
    virtual ~LinkTemplateInterface() = default;

    virtual const char* get_handle() const = 0;
    virtual string get_metta_expression() const = 0;
};

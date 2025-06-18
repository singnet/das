#pragma once

#include <memory>

using namespace std;

class LinkTemplateInterface {
   public:
    virtual ~LinkTemplateInterface() = default;

    virtual shared_ptr<char> get_handle() const = 0;
};

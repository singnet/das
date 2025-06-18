#pragma once

#include <memory>

using namespace std;

class LinkTemplateInterface {
   public:
    virtual ~LinkTemplateInterface() = default;

    shared_ptr<char> handle;
};

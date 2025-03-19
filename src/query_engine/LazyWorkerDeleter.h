#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "commons/Utils.h"

using namespace std;

namespace query_engine {

class Worker {
   public:
    virtual bool is_work_done() = 0;
};

template <class T, typename enable_if<is_base_of<Worker, T>::value, bool>::type = true>
class LazyWorkerDeleter {
   public:
    LazyWorkerDeleter()
        : shutting_down_flag(false),
          objects_deleter_thread(make_unique<thread>(&LazyWorkerDeleter::objects_deleter_method, this)) {
    }

    ~LazyWorkerDeleter() {
        if (this->shutting_down_flag) return;
        this->shutting_down_flag = true;
        this->objects_deleter_thread->join();
        this->objects_deleter_thread.reset();
        lock_guard<mutex> lock(this->objects_mutex);
        T* obj;
        while (!this->objects.empty()) {
            obj = this->objects.front();
            this->objects.erase(this->objects.begin());
            delete obj;
        }
    }

    void add(T* obj) {
        lock_guard<mutex> lock(this->objects_mutex);
        if (!this->shutting_down_flag) this->objects.push_back(obj);
    }

   private:
    vector<T*> objects;
    mutex objects_mutex;
    bool shutting_down_flag;
    unique_ptr<thread> objects_deleter_thread;

    void objects_deleter_method() {
        T* obj;
        while (!this->shutting_down_flag) {
            {
                lock_guard<mutex> lock(this->objects_mutex);
                while (!this->objects.empty()) {
                    obj = this->objects.front();
                    if (obj->is_work_done()) {
                        this->objects.erase(this->objects.begin());
                        delete obj;
                    }
                }
            }
            for (size_t i = 0; i < 50; i++) {
                commons::Utils::sleep(100);  // 100ms x 50 = 5s
                if (this->shutting_down_flag) return;
            }
        }
    }
};

}  // namespace query_engine
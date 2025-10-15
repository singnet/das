#pragma once

#include <fstream>

namespace commons {

    /*
    class MyObject {
    public:
        int id;
        std::string name;

        void serialize(std::ostream& os) const {
            os.write(reinterpret_cast<const char*>(&id), sizeof(id));
            size_t name_len = name.length();
            os.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            os.write(name.c_str(), name_len);
        }

        void deserialize(std::istream& is) {
            is.read(reinterpret_cast<char*>(&id), sizeof(id));
            size_t name_len;
            is.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
            name.resize(name_len);
            is.read(&name[0], name_len);
        }
    };
    */
    class Serializable {
      protected:
        Serializable() {}
      public:
        virtual ~Serializable() {}
        void serialize(std::ostream& os) const = 0;
        void deserialize(std::istream& is)  = 0;
    };

}  // namespace commons

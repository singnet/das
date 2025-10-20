#pragma once

#include <fstream>

namespace commons {

    class Serializable {
      protected:
        Serializable() {}
      public:
        virtual ~Serializable() {}
        virtual void serialize(std::ostream& os) = 0;
        virtual void deserialize(std::istream& is)  = 0;
    };

}  // namespace commons

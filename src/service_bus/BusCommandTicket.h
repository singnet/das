#ifndef _SERVICE_BUS_BUSCOMMANDTICKET_H
#define _SERVICE_BUS_BUSCOMMANDTICKET_H

using namespace std;

namespace service_bus {

/**
 *
 */
class BusCommandTicket {

public:

    BusCommandTicket(unsigned int serial, const string& requestor_id)
        : serial(serial), requestor_id(requestor_id) {}
    virtual ~BusCommandTicket() {};

    unsigned int serial;
    string requestor_id;
};

} // namespace service_bus

#endif // _SERVICE_BUS_BUSCOMMANDTICKET_H

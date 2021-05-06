#include "apeer.h"
#include "server.h"
#include "xx_logger.h"
// todo: speer, gpeer

bool APeer::Close(int const& reason, char const* const& desc) {
    if (!this->Peer::Close(reason, desc)) return false;
    LOG_INFO("APeer Close, reason = ", reason, ", desc = ", desc);
    DelayUnhold();
    return true;
}

void APeer::ReceivePackage(uint8_t *const &buf, size_t const &len) {
    // should not receive package
    LOG_INFO("APeer ReceivePackage = ", xx::ToString(xx::Span(buf, len)));
}

void APeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);

    std::string_view cmd;
    if (int r = dr.Read(cmd)) {
        Close(-30, xx::ToString(__LINE__, " APeer ReceiveCommand if (int r = dr.Read(cmd)), r = ", r).c_str());
        return;
    }

    if (cmd == "register") {
        uint32_t typeId = 0;
        if (int r = dr.Read(typeId)) {
            Close(-31, xx::ToString(__LINE__, " APeer ReceiveCommand if (int r = dr.Read(clientId)), r = ", r).c_str());
            return;
        }
        auto& s = *(Server*)ec;
        // todo: swap fd
        switch(typeId) {
            case 1: // gpeer
            {
                uint32_t gatewayId = 0;
                if (int r = dr.Read(gatewayId)) {
                    Close(-32, xx::ToString(__LINE__, " APeer ReceiveCommand if (int r = dr.Read(gatewayId)), r = ", r).c_str());
                    return;
                }
                // todo: swap fd, put gpeer to server.gpeers[gatewayId]
                break;
            }
            case 2: // speer
            {
                // todo: swap fd, put speer to server.speers[serverId]
                break;
            }
            default:
                Close(-34, xx::ToString(__LINE__, " APeer ReceiveCommand register: unhandled typeId = ", typeId).c_str());
        }
    } else {
        Close(-35, xx::ToString(__LINE__, " APeer ReceiveCommand unhandled cmd = ", cmd).c_str());
    }
}

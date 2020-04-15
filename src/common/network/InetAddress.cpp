/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "InetAddress.h"

namespace nebula {
namespace network {
std::ostream& operator<<(std::ostream& os, const InetAddress& h) {
    os << h.getAddressStr() << ":" << h.getPort();
    return os;
}

InetAddress InetAddress::make_inet_address(const char* p) {
    const char* temp = p;
    auto ip = *(reinterpret_cast<const uint32_t*>(temp));
    temp += sizeof(uint32_t);
    auto port = *(reinterpret_cast<const uint32_t*>(temp));
    return InetAddress(ip, port);
}

const std::string InetAddress::encode() const {
    std::string str;
    str.reserve(sizeof(uint32_t) + sizeof(int32_t));
    auto ip = toLong();
    int32_t port = getPort();
    str.append(reinterpret_cast<char*>(&ip), sizeof(uint32_t));
    str.append(reinterpret_cast<char*>(&port), sizeof(int32_t));
    return str;
}
}   // namespace network
}   // namespace nebula
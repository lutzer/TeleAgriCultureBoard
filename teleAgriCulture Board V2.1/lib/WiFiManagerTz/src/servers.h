#include <Arduino.h>
#include <vector>

#pragma once

 struct NTP_Server
        {
            const std::string name;
            const std::string addr;
        };

namespace WiFiManagerNS
{
    namespace NTP
    {
      extern std::vector<NTP_Server> NTP_Servers;
    };
};
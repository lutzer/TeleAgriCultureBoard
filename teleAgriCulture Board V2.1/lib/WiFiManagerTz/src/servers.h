#include <Arduino.h>
#include <vector>

#pragma once

#define NUM_PREDIFINED_NTP 7
#define CUSTOM_NTP_INDEX 7

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

#pragma once

#include "prefs.hpp"
#include "NTP.hpp"
#include "TZ.hpp"

namespace WiFiManagerNS
{

#include "strings_en.h"

  bool NTPEnabled = false; // overriden by prefs
  bool DSTEnabled = true;
  String TimeConfHTML;
  char systime[64];

  constexpr const char *menuhtml = "<form action='/custom' method='get'><button>Setup Board</button></form><br/>\n";

  WiFiManager *_wifiManager;

  void bindServerCallback();

  void init(WiFiManager *manager)
  {
    _wifiManager = manager;
    _wifiManager->setWebServerCallback(bindServerCallback);
    _wifiManager->setCustomMenuHTML(menuhtml);
    _wifiManager->setClass("invert");
    // _wifiManager->wm.setDarkMode(true);
    TZ::loadPrefs();
    prefs::getBool("NTPEnabled", &NTPEnabled, false);
    if (NTPEnabled)
    {
      NTP::loadPrefs();
    }
  }

  void configTime()
  {
    const char *tz = TZ::getTzByLocation(TZ::tzName);
    Serial.printf("Setting up time: NTPServer=%s, TZ-Name=%s, TZ=%s\n", NTP::server(), TZ::tzName, tz);
    TZ::configTimeWithTz(tz, NTP::server());
  }

  enum Element_t
  {
    HTML_HEAD_START,
    HTML_STYLE,
    HTML_SCRIPT,
    HTML_HEAD_END,
    HTML_PORTAL_OPTIONS,
    HTML_ITEM,
    HTML_FORM_START,
    HTML_FORM_PARAM,
    HTML_FORM_END,
    HTML_SCAN_LINK,
    HTML_SAVED,
    HTML_END,
  };

  constexpr const size_t count = sizeof(Element_t);

  struct TemplateSet_t
  {
    Element_t element;
    const char *content;
  };

  TemplateSet_t templates[] =
      {
          {HTML_HEAD_START, HTTP_HEAD_START},
          {HTML_STYLE, HTTP_STYLE},
          {HTML_SCRIPT, HTTP_SCRIPT},
          {HTML_HEAD_END, HTTP_HEAD_END},
          {HTML_PORTAL_OPTIONS, HTTP_PORTAL_OPTIONS},
          {HTML_ITEM, HTTP_ITEM},
          {HTML_FORM_START, HTTP_FORM_START},
          {HTML_FORM_PARAM, HTTP_FORM_PARAM},
          {HTML_FORM_END, HTTP_FORM_END},
          {HTML_SCAN_LINK, HTTP_SCAN_LINK},
          {HTML_SAVED, HTTP_SAVED},
          {HTML_END, HTTP_END},
  };

  void setTemplate(Element_t element, const char *content)
  {
    templates[element].content = content;
  }

  const char *getTemplate(Element_t element)
  {
    return templates[element].content;
  }

  // teleAgriCulture favicon png/base64 used in HTML Head
  // const char favicon[] = "<link rel='icon' type='image/png; base64' sizes='32x32' href='data:image/png;base64,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'>";

  // teeny-tiny 57 bytes GIF favicon
  const char favicon[] =
      {
          0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x09, 0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff,
          0x00, 0x00, 0x00, 0x21, 0xf9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00,
          0x09, 0x00, 0x08, 0x00, 0x00, 0x02, 0x10, 0x8c, 0x8f, 0x01, 0xbb, 0xc7, 0x60, 0xa0, 0x8a, 0x54,
          0xbd, 0x16, 0x1f, 0x82, 0x3c, 0xf9, 0x02, 0x00, 0x3b};

  void handleFavicon()
  {
    _wifiManager->server->send_P(200, "text/html", favicon, sizeof(favicon));
  }

  String getSystimeStr()
  {
    static String systimeStr;
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(systime, sizeof systime, "%Y-%m-%dT%H:%M", nowtm);
    systimeStr = String(systime);
    return systimeStr;
  }

  void handleRoute()
  {
    Serial.println("[HTTP] handle route Custom");

    TimeConfHTML = "";
    TimeConfHTML += getTemplate(HTML_HEAD_START);
    TimeConfHTML.replace(FPSTR(T_v), "TeleAgriCulture Board");
    TimeConfHTML += "<link rel='icon' type='image/png; base64' sizes='32x32' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAAXNSR0IArs4c6QAABpZJREFUWEftl3lQldcZhx+4QAABAXEsKJKwmjgWwkQBwQnJREFQ1BKBKHTARNlMcAHlQlwLsRUIiwGqAkkNgqmKxQVSDZCCTEy0BmRR3IARQUcQhACyXTrfoTFjvIZ02k5mMp5/vjv33vO+v/Oe3/ee56gAo/yCQ2U8Ac4uLuz8QxyZGekcOXx4XKlaWlokfZiMkZERK1euYGhw8CfnKBUQGBiE+8KFxMjl9PR0k3swD2sbaz7as4d9e/fS09OjNOiMF18kLj6eOXMcSEtLJSkhAf+AAJYsXUrkhg00NTU9MU8IUFFRISb2fTS1NInbuRNHJyeysnNoa21lVVAg9+7dY3O0HB9fX3p7eykt+YKqqira29tRV1dn+nQznF2cReI7d+7wx10fcPLECQICfs+2HTuoq6vlLV9fppuZsW79Bj7JyaayslKIeVSBrdu2s3rNGvLz84iVy3GZN4+U1FTU1NRJSkwgPy8Pw0mT8PT0xGnuXMzNLdDV1WVkeJh77e3U19VSUlJCWWkpvzE2JjpajoenJ2crKnh3bTgTJ04kL/8Q+gYG+Pn6UPXtt48LkFYSv2sXfn5vcfr039kcFcVzmppEy+UsWrSY+/fvc7ywUCSQVtTd3c3o6Jh/ZWpqmBgb88rs2Xh4ePKqqytdXZ3sSU3j4MFcHBwcSUlLQ1tbm/CwUBHj+/GYB1RVVXln9WoiozaJBIkJuzlWUIDJ1Kl4e3szf4EbVlZWSP+Tfu/r60WmKkNPTw8tbW3x3YUL5zlx/DifFxejr69PxLr1LPfx4fr166xfF0FtTc1jPlBqQksrK1HCN+bPp7W1lSNHDlNcVMTVhqvo6upg9vzzTJkyBZ0JOigUI6I6t2/fpqWlRXhC8tDSZctwc3Onr6+P/fv2kp2VRX9/v3ITKrO0ZExrGxuxJQs9PDA2NhZmvHy5nqbGJvFZCi6TqaKnNxETExMk4TY2NqipqVFdVUXB0aMUFhbS3f3gqa+i8j4wWR8WO0FlLTTcEgEtLa2we9kOG5sZmJqaYmBoiIaGOqOKUfr6+7l79y5NjTepr6vn4sV/CoFiSHHUZFBYCYone55yActfhS0BcLQcdhwYC6Q/ASRh124/vbFoPQczTKG+GQaGQFcLKlJBVRW8t8O1lp+5BcoEnP4TTDGE4GQ4Vw86WuDjCldb4Oy/jZUeAfNmQcJn8OmZ/7GA8hTQ14H3PoIvq8DOAg7I4dIN8N81trKcTfCKNaQVQFbRMwHPKvCsAr+2CvwjGQx0f+gDk/Rgky/UNELuF2N9IDsKZtv8n/rA3JkwbTKcOge9D5W345fMwM4SSi7C3c7/Ygt+aw7+b0BpFXz+jUgmEc1LM2dibW3NtGmmGBoaoKGhgUIxSn9/n0Cx5uZm6uvruXnjBkNDQ2OH0Po3x54fHh47H340fpKKDQ0NWbTYi8VeXtjZ2SGTyWi5dUskknhQOt8lONHT0xUYJmGagYEBHR0dfFlWRkHBUc599RXDw8P/2XEsJQ4LD2fFSn+RQEI0CUi++fprurq6BOlI2C0h1ohCwYOuLiFoYGCAF14wx/U1V7yWLMXW1paGK1dISU6muLgIhUIxfgUkHI+L/wA9XV1ycrLJ2r+fgYFBFrgtEIRjb2+P0eTJSMAyMjIinlJlpOSNjTepPFvJyZMnBJC8bG9PZGSUgFipIps3RdHW1qYcyaSVrn33PTZs3EhdbS3rIiJoa2tlTXAIQauC0NLSpqK8XABl9aVqgWC9330nKiRVzMLCkjkODri5jXHjpZoaEnfvpqKiHF8/P7Zt30FnZydvBwVSV1f3JJQGh4QQ+/4WoTQsNERwX3p6hmD53NxPyUzP4MGDLoHrTk5zMbcwF8YcGR4RNCSJLisrpaGhQaw4Wh7DrFmzBM5v37YVW1s7sj/+WFTK+3fLaGpsFCIeXUzSMzLR0dFhbXgYZmZmHMw/JIAzLDSUC+fPExi0ipDQULH/1dVV4mJyv6ND4Jok0tHREROTqYKK4+PiBP3KY2J4+53VnDlzmtDgYJydXdiydSuJiQkUnTr1g4AfO2NjZCTeby4XF4orly+TuXcfrq6u/O3YMZKSEmlWcsWShLz2+utIFxwJUGNjYvjrZ4eQx8TitcQL/xUruXbtqvDM9/eJRxVQ9o5Ieyu51s3dndS0Pfw5M4PUlJTHJiubN8nIiE/+coAJ2tq4uy1gcHAQVZkMxciI0ldx3NuxNEtTU5OHD5/S/ZSEld4KaQGiGY0z/gXHqGIuBJT6LgAAAABJRU5ErkJggg=='>";
    TimeConfHTML += getTemplate(HTML_SCRIPT);

    TimeConfHTML += "<script>";
    TimeConfHTML += "window.addEventListener('load', function() { var now = new Date(); var offset = now.getTimezoneOffset() * 60000; var adjustedDate = new Date(now.getTime() - offset);";
    TimeConfHTML += "document.getElementById('set-time').value = adjustedDate.toISOString().substring(0,16); });";
    TimeConfHTML += "</script>";

    TimeConfHTML += getTemplate(HTML_STYLE);
    TimeConfHTML += "<style> input[type='checkbox'][name='use-ntp-server']:not(:checked) ~.collapsable { display:none; }</style>";
    TimeConfHTML += "<style> input[type='checkbox'][name='use-ntp-server']:checked       ~.collapsed   { display:none; }</style>";
    TimeConfHTML += getTemplate(HTML_HEAD_END);
    TimeConfHTML.replace(FPSTR(T_c), "invert"); // add class str
                                                //------------- Start Connectors ------- //

    TimeConfHTML += "<h2>Connectors:</h2>";

    TimeConfHTML += "<h3>I2C Connectors</h3>";

    TimeConfHTML += "<label for='i2c_1'>I2C_1</label>";

    TimeConfHTML += "<select id='I2C_1' name='i2c_1'>";

    String dropdown_i2c = "<option value='NO'>NO</option>";

    for (int i = 0; i < SENSORS_NUM; i++)
    {
      if (allSensors[i].con_typ == "I2C")
      {
        dropdown_i2c += "<option value='" + allSensors[i].sensor_name + "'>" + allSensors[i].sensor_name + "</option>";
      }
    }

    dropdown_i2c += "</select>";
    dropdown_i2c += "<BR>";

    TimeConfHTML += dropdown_i2c;
    TimeConfHTML += "<label for='i2c_2'>I2C_2</label>";

    TimeConfHTML += "<select id='I2C_2' name='i2c_2'>";

    TimeConfHTML += dropdown_i2c;

    TimeConfHTML += "<label for='i2c_3'>I2C_3</label>";

    TimeConfHTML += "<select id='I2C_3' name='i2c_3'>";

    TimeConfHTML += dropdown_i2c;

    TimeConfHTML += "<label for='i2c_4'>I2C_4</label>";

    TimeConfHTML += "<select id='I2C_4' name='i2c_4'>";

    TimeConfHTML += dropdown_i2c;

    Serial.println(dropdown_i2c);
    dropdown_i2c = String();

    TimeConfHTML += "<h3>ADC Connectors</h3>";

    TimeConfHTML += "<label for='adc_1'>ADC_1</label>";

    TimeConfHTML += "<select id='ADC_1' name='adc_1'>";

    String dropdown_adc = "<option value='NO'>NO</option>";

    for (int i = 0; i < SENSORS_NUM; i++)
    {
      if (allSensors[i].con_typ == "ADC")
      {
        dropdown_adc += "<option value='" + allSensors[i].sensor_name + "'>" + allSensors[i].sensor_name + "</option>";
      }
    }

    dropdown_adc += "</select>";
    dropdown_adc += "<BR>";

    TimeConfHTML += dropdown_adc;
    TimeConfHTML += "<label for='adc_2'>ADC_2</label>";

    TimeConfHTML += "<select id='ADC_2' name='adc_2'>";

    TimeConfHTML += dropdown_adc;

    TimeConfHTML += "<label for='adc_3'>ADC_3</label>";

    TimeConfHTML += "<select id='ADC_3' name='adc_3'>";

    TimeConfHTML += dropdown_adc;

    dropdown_adc = String();

     TimeConfHTML += "<h3>1-Wire Connectors</h3>";

    TimeConfHTML += "<label for='onewire_1'>1-Wire_1</label>";

    TimeConfHTML += "<select id='onewire_1' name='onewire_1'>";

    String dropdown_1wire = "<option value='NO'>NO</option>";

    for (int i = 0; i < SENSORS_NUM; i++)
    {
      if (allSensors[i].con_typ == "ADC")
      {
        dropdown_1wire += "<option value='" + allSensors[i].sensor_name + "'>" + allSensors[i].sensor_name + "</option>";
      }
    }

    dropdown_1wire += "</select>";
    dropdown_1wire += "<BR>";

    TimeConfHTML += dropdown_1wire;
    TimeConfHTML += "<label for='onewire_2'>1-Wire_2</label>";

    TimeConfHTML += "<select id='onewire_2' name='onewire_2'>";

    TimeConfHTML += dropdown_adc;

    TimeConfHTML += "<label for='onewire_3'>1-Wire_3</label>";

    TimeConfHTML += "<select id='onewire_3' name='onewire_3'>";

    TimeConfHTML += dropdown_adc;
    dropdown_1wire = String();

    TimeConfHTML += "<h2>Time Settings</h2>";

    String systimeStr = getSystimeStr();

    TimeConfHTML += "<label for='ntp-server'>System Time ";

    TimeConfHTML += "<input readonly style=width:auto name='system-time' type='datetime-local' value='" + systimeStr + "'>";
    TimeConfHTML += " <button onclick=location.reload() style=width:auto type=button> Refresh </button></label><br>";

    TimeConfHTML += "<iframe name='dummyframe' id='dummyframe' style='display: none;'></iframe><form action='/save-tz' target='dummyframe' method='POST'>";

    // const char *currentTimeZone = "Europe/Paris";
    TimeConfHTML += "<label for='timezone'>Time Zone</label>";
    TimeConfHTML += "<select id='timezone' name='timezone'>";
    for (int i = 0; i < TZ::zones(); i += 2)
    {
      bool selected = (strcmp(TZ::tzName, TZ::timezones[i]) == 0);
      TimeConfHTML += "<option value='" + String(i) + "'" + String(selected ? "selected" : "") + ">" + String(TZ::timezones[i]) + "</option>";
    }
    TimeConfHTML += "</select><br>";

    TimeConfHTML += "<label for='use-ntp-server'>Enable NTP Client</label> ";
    TimeConfHTML += " <input value='1' type=checkbox name='use-ntp-server' id='use-ntp-server'" + String(NTPEnabled ? "checked" : "") + ">";
    TimeConfHTML += "<br>";

    TimeConfHTML += "<div class='collapsed'>";
    TimeConfHTML += "<label for='set-time'>Set Time ";
    TimeConfHTML += "<input style=width:auto name='set-time' id='set-time' type='datetime-local' value='" + systimeStr + "'>";
    TimeConfHTML += "</div>";

    TimeConfHTML += "<div class='collapsable'>";

    TimeConfHTML += "<h2>NTP Client Setup</h2>";

    TimeConfHTML += "<label for='enable-dst'>Auto-adjust clock for DST</label> ";
    TimeConfHTML += " <input value='1' type=checkbox name='enable-dst' id='enable-dst'" + String(DSTEnabled ? "checked" : "") + ">";
    TimeConfHTML += "<br>";

    TimeConfHTML += "<label for='ntp-server'>Server:</label>";
    // TimeConfHTML += "<input list='ntp-server-list' id='ntp-server' name='ntp-server' placeholder='pool.ntp.org'";
    // TimeConfHTML += " value='"+ NTP::server() +"'>";
    TimeConfHTML += "<select id='ntp-server-list' name='ntp-server'>";
    size_t servers_count = sizeof(NTP::Servers) / sizeof(NTP::Server);
    uint8_t server_id = NTP::getServerId();
    for (int i = 0; i < servers_count; i++)
    {
      TimeConfHTML += "<option value='" + String(i) + "'" + String(i == server_id ? "selected" : "") + ">" + String(NTP::Servers[i].name) + "(" + String(NTP::Servers[i].addr) + ")</option>";
    }
    TimeConfHTML += "</select><br>";

    TimeConfHTML += "<label for='ntp-server-interval'>Sync interval:</label>";
    TimeConfHTML += "<select id='ntp-server-interval' name='ntp-server-interval'>";
    TimeConfHTML += "<option value=60>Hourly</option>";
    TimeConfHTML += "<option value=14400>Daily</option>";
    TimeConfHTML += "<option value=10080>Weekly</option>";
    TimeConfHTML += "</select><br>";

    TimeConfHTML += "</div>";

    TimeConfHTML += "<button type=submit>Submit</button>";
    TimeConfHTML += "</form>";

    TimeConfHTML += getTemplate(HTML_END);

    _wifiManager->server->send_P(200, "text/html", TimeConfHTML.c_str(), TimeConfHTML.length());

    TimeConfHTML = String();
  }

  void handleValues()
  {
    bool success = true;
    bool _NTPEnabled = NTPEnabled;
    Serial.println("[HTTP] handle route Values");
    if (_wifiManager->server->hasArg("use-ntp-server"))
    {
      String UseNtpServer = _wifiManager->server->arg("use-ntp-server");
      log_d("UseNtpServer: %s", UseNtpServer.c_str());
      uint8_t useNtpServer = atoi(UseNtpServer.c_str());
      NTPEnabled = useNtpServer == 1;
    }
    else
    {
      NTPEnabled = false;
    }
    if (_NTPEnabled != NTPEnabled)
    {
      prefs::setBool("NTPEnabled", NTPEnabled);
    }

    if (_wifiManager->server->hasArg("timezone"))
    {
      String timezoneStr = _wifiManager->server->arg("timezone");
      log_d("timezoneStr: %s", timezoneStr.c_str());
      size_t tzidx = atoi(timezoneStr.c_str());
      String timezone = TZ::defaultTzName;
      if (tzidx < TZ::zones())
      {
        timezone = String(TZ::timezones[tzidx]);
        log_d("timezone: %s", timezone.c_str());
      }

      TZ::setTzName(timezone.c_str());
      const char *tz = TZ::getTzByLocation(TZ::tzName);
      TZ::configTimeWithTz(tz, NTP::server());
    }

    if (NTPEnabled)
    {
      // also collect tz/server data
      if (_wifiManager->server->hasArg("ntp-server"))
      {
        String NtpServer = _wifiManager->server->arg("ntp-server");
        log_d("NtpServer: %s", NtpServer.c_str());
        uint8_t server_id = atoi(NtpServer.c_str());
        if (!NTP::setServer(server_id))
          success = false;
      }

      if (_wifiManager->server->hasArg("ntp-server-interval"))
      {
        String NtpServerInterval = _wifiManager->server->arg("ntp-server-interval");
        log_d("NtpServerInterval: %s", NtpServerInterval.c_str());
        int serverInterval = atoi(NtpServerInterval.c_str());
        switch (serverInterval)
        {
        case 60:
        case 14400:
        case 10080:
          break;
        default:
          serverInterval = 14400;
        }
        NTP::setSyncDelay(serverInterval);
      }
    }

    const char *successResp = "<script>parent.location.href = '/';</script>";
    const char *failureResp = "<script>parent.alert('fail');</script>";

    _wifiManager->server->send(200, "text/html", success ? successResp : failureResp);
  }

  void bindServerCallback()
  {
    _wifiManager->server->on("/custom", handleRoute);
    _wifiManager->server->on("/save-tz", handleValues);
    //_wifiManager->server->on("/favicon.ico", handleFavicon);
  }

};

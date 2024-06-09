#ifndef API_CALLS_H
#define API_CALLS_H
#include <HTTPClient.h>
#include <ArduinoJson.h>

String getStats(const char* serverName);
String getSpotifyToken();
String getPlayback(String tokenType, String token);

#endif
#include "HTTPClient.h"
#include "api_calls.h"

String getStats(const char* serverName) {
  HTTPClient client;

  client.begin(serverName);

  int responseCode = client.GET();
  String payload = "{}";

  if (responseCode == 200) {
    payload = client.getString();
  }

  client.end();

  return payload;
}
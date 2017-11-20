#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "open62541.h"
#include "server.h"


UA_Boolean running = true;
static void stopHandler(int sign) {
   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
   running = false;
}
int main(int argc, char ** argv) {
   signal(SIGINT, stopHandler);
   signal(SIGTERM, stopHandler);

   //Setup Server
   UA_ServerConfig * config;
   config = UA_ServerConfig_new_default();
   UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(
                               UA_ConnectionConfig_default, 16665);
   UA_Server *server = UA_Server_new(config);
   //Run Server
   UA_Server_run(server, &running);
   UA_Server_delete(server);
   nl.deleteMembers(&nl);
   return 0;
}

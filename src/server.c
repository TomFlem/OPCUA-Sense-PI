#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "open62541.h"
#include "server.h"

#include "RTIMULib.h"
pthread_t tid[0];

//Method to create Temp Node with attributes to contain the temperature and time
static void addNodes(UA_Server *server)
{
   UA_VariableAttributes myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "The temperature reading from the Pi Sense Hat");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myTemp = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myTemp, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myTempName = UA_QUALIFIEDNAME(1, "Temperature");
   const UA_NodeId myTempNodeId = UA_NODEID_STRING(1, "Temperature");
   UA_Server_addVariableNode(server, myTempNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myTempName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
}

/* Client half of the Server/Client hybrid*/
void* pollSensors(void *arg){

   UA_StatusCode retval;
   /* Create a client and connect */
   UA_Client *client = UA_Client_new(UA_ClientConfig_default);
   UA_StatusCode status = UA_Client_connect(client, "opc.tcp://localhost:4840");
   RTIMUSettings *settings = new RTIMUSettings("RTIMULib");
   RTIMU *imu = RTIMU::createIMU(settings);
   RTPressure *pressure = RTPressure::createPressure(settings);
   RTHumidity *humidity = RTHumidity::createHumidity(settings);
   imu->setSlerpPower(0.02);
   imu->setGyroEnable(true);
   imu->setAccelEnable(true);
   imu->setCompassEnable(true);

   //  set up pressure sensor

   if (pressure != NULL)
      pressure->pressureInit();

   //  set up humidity sensor

   if (humidity != NULL)
       humidity->humidityInit();
   while (true)
   {
  	//Create 'temperature' value
   RTIMU_DATA imuData = imu->getIMUData();
   if (pressure != NULL)
       pressure->pressureRead(imuData);

            //  add the humidity data to the structur
  if (humidity != NULL)
      humidity->humidityRead(imuData);

   printf("Temp %4.1f\n",imuData.temperature);
   UA_Variant *myVariant = UA_Variant_new();
   UA_Float tempIn = imuData.temperature;
   UA_Variant_setScalarCopy(myVariant, &tempIn, &UA_TYPES[UA_TYPES_FLOAT]);
   UA_NodeId testNodeId = UA_NODEID_STRING(1,"Temperature");
   UA_StatusCode status = UA_Client_writeValueAttribute(client,testNodeId, myVariant);
   //Sleep for 1 second
   sleep(imu->IMUGetPollInterval());
   }
}
UA_Boolean running = true;
static void stopHandler(int sign)
{
   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
   running = false;
}
int main(int argc, char ** argv) 
{
   signal(SIGINT, stopHandler);
   signal(SIGTERM, stopHandler);

   //Setup Server
   UA_ServerConfig * config;
   config = UA_ServerConfig_new_default();
   UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(
                               UA_ConnectionConfig_default, 4840);
   UA_Server *server = UA_Server_new(config);
   addNodes(server);
   
   //Start Sensor Polling Thread
   int err = pthread_create(&(tid[0]), NULL, &pollSensors, NULL);
   
   //Run Server
   UA_Server_run(server, &running);
   UA_Server_delete(server);
   nl.deleteMembers(&nl);
   return 0;
}

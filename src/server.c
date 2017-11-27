#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "open62541.h"
#include "server.h"

#include "RTIMULib.h"
pthread_t tid[0];
static void sub_handler (UA_UInt32 monId, UA_DataValue *value, void *context) 
{
   std::count<<"\ntest\n";
}
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
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "The pressure reading from the Pi Sense Hat");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "Pressure");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myPress = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myPress, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myPressName = UA_QUALIFIEDNAME(1, "Pressure");
   const UA_NodeId myPressNodeId = UA_NODEID_STRING(1, "Pressure");
   UA_Server_addVariableNode(server, myPressNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myPressName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "The humidity reading from the Pi Sense Hat");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "Humidity");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myHumid = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myHumid, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myHumidName = UA_QUALIFIEDNAME(1, "Humidity");
   const UA_NodeId myHumidNodeId = UA_NODEID_STRING(1, "Humidity");
   UA_Server_addVariableNode(server, myHumidNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myHumidName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "A String to print to the LED Matrix");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "LEDString");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
   UA_String myLEDString = " ";
   UA_Variant_setScalarCopy(&myVar.value, &myLEDString, &UA_TYPES[UA_TYPES_STRING]);
   const UA_QualifiedName myLEDStringName = UA_QUALIFIEDNAME(1, "LEDString");
   const UA_NodeId myLEDStringNodeId = UA_NODEID_STRING(1, "LEDString");
   UA_Server_addVariableNode(server, myLEDStringNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myLEDStringName, UA_NODEID_NULL, myVar, NULL, NULL);
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
       
   // Setup a subscription to monitor the LED string
   std::string tmpStrNodeID("LEDString");
   char * chrNodeId = &tmpStrNodeID[0u]; //string to char* for UA C function
   UA_NodeId nodeId = UA_NODEID_STRING(1, chrNodeId);
   status = UA_Client_Subscriptions_addMonitoredItem(client, 1, 
      nodeId, UA_ATTRIBUTEID_VALUE, &sub_handler, NULL, 1);
   while (true)
   {
      sleep(1);
      //while(imu->IMURead())
      //{
     	//Create 'temperature' value
      RTIMU_DATA imuData = imu->getIMUData();
      if (pressure != NULL)
      {
         pressure->pressureRead(imuData);
      }
      //  add the humidity data to the structur
      if (humidity != NULL)
      {
         humidity->humidityRead(imuData);
      }

      //Write Temperature
      UA_Variant *myVariant = UA_Variant_new();
      UA_Float tempIn = imuData.temperature;
      UA_Variant_setScalarCopy(myVariant, &tempIn, &UA_TYPES[UA_TYPES_FLOAT]);
      UA_NodeId tempNodeId = UA_NODEID_STRING(1,"Temperature");
      UA_StatusCode status = UA_Client_writeValueAttribute(client,tempNodeId, myVariant);
      
      //Write Pressure
      UA_Float pressIn = imuData.pressure;
      UA_Variant_setScalarCopy(myVariant, &pressIn, &UA_TYPES[UA_TYPES_FLOAT]);
      UA_NodeId pressNodeId = UA_NODEID_STRING(1,"Pressure");
      status = UA_Client_writeValueAttribute(client,pressNodeId, myVariant);
      
      //Write Humidity
      UA_Float humidIn = imuData.humidity;
      UA_Variant_setScalarCopy(myVariant, &humidIn, &UA_TYPES[UA_TYPES_FLOAT]);
      UA_NodeId humidNodeId = UA_NODEID_STRING(1,"Humidity");
      status = UA_Client_writeValueAttribute(client,humidNodeId, myVariant);

     // }
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

#include <stdlib.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sense/fb.h>

#include "open62541.h"
#include "server.h"
#include "RTIMULib.h"
pthread_t tid[0];

// Replace with any of the fonts in "fonts" directory
#include "fonts/5x5_font.h"

//#include "fonts/BMSPA_font.h"
//#define CHAR_WIDTH 8

#define CHAR_WIDTH (sizeof(font) / 96)
#define CHAR_HEIGHT 8
#define SCROLL_DELAY 100
#define LETTER_SPACE 1
#define LETTER_WIDTH (CHAR_WIDTH + LETTER_SPACE)
void draw(char c,int x,int y,sense_color_t color,sense_bitmap_t bitmap);
const unsigned char* character(char c);

static UA_ByteString loadCertificate(void) {
    UA_ByteString certificate = UA_STRING_NULL;
    FILE *fp = NULL;
    //FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
    fp=fopen("server_cert.der", "rb");

    if(!fp) {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = (size_t)ftell(fp);
    certificate.data = (UA_Byte*)UA_malloc(certificate.length*sizeof(UA_Byte));
    if(!certificate.data)
        return certificate;

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

int writeLED(char* argv){
   sense_bitmap_t fb = sense_alloc_fb();
   if (!fb){
      fprintf(stderr,"Could not allocate framebuffer: %s\n",sense_strerror(sense_errno()));
      exit(1);
   }
   sense_color_t color = sense_make_color_rgb(0xff,0xff,0);
   sense_bitmap_t buffer = sense_alloc_bitmap();
   char* word = argv;
   int idx;
   int len = strlen(word);
   for (int x=0;x<len*LETTER_WIDTH;++x){
      sense_bitmap_paint(buffer,0);
      idx = x/LETTER_WIDTH;
      draw(word[idx],0 - x % LETTER_WIDTH,0,color,buffer);
      if ((idx + 1) < len){
         draw(word[idx+1],LETTER_WIDTH - x % LETTER_WIDTH,0,color,buffer);
      }
      sense_bitmap_cpy(fb,buffer);

      usleep(SCROLL_DELAY*1000);
   }
   // clear display
   sense_bitmap_paint(fb,0);
   sense_free_bitmap(fb);
   sense_free_bitmap(buffer);
   return 0;
}

const unsigned char* character(char c){
   c = c & 0x7F;
   if (c < ' ') {
      c = 0;
   } else {
      c -= ' ';
   }
   return font[c];
}

// code apdapted from http://jared.geek.nz/2014/jan/custom-fonts-for-microcontrollers
void draw(char c,int x,int y,sense_color_t color,sense_bitmap_t bitmap) {
   int i,j;
   const unsigned char* chr = character(c);
   for (j=0 + ((LETTER_SPACE+x)<0?(1+x)*-1:0); j<CHAR_WIDTH && (1+j+x) < SENSE_BITMAP_WIDTH; ++j) {
      for (i=0 + (y<0?y*-1:0); i<CHAR_HEIGHT && (i+y < SENSE_BITMAP_HEIGHT); ++i) {
         if (chr[j] & (1<<i)) {
            sense_bitmap_set_pixel(bitmap,LETTER_SPACE +j+x, i+y,color);
         }
      }
   }
}

static void sub_handler (UA_UInt32 monId, UA_DataValue *value, void *context) 
{
   UA_String input = *(UA_String*)value->value.data;
   writeLED((char*)input.data);
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
   UA_String myLEDString = UA_STRING("TEMP");
   UA_Variant_setScalarCopy(&myVar.value, &myLEDString, &UA_TYPES[UA_TYPES_STRING]);
   const UA_QualifiedName myLEDStringName = UA_QUALIFIEDNAME(1, "LEDString");
   const UA_NodeId myLEDStringNodeId = UA_NODEID_STRING(1, "LEDString");
   UA_Server_addVariableNode(server, myLEDStringNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myLEDStringName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "Gyro X Axis");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "XAxis");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myXAxis = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myXAxis, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myXAxisName = UA_QUALIFIEDNAME(1, "XAxis");
   const UA_NodeId myXAxisNodeId = UA_NODEID_STRING(1, "XAxis");
   UA_Server_addVariableNode(server, myXAxisNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myXAxisName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "Gyro Y Axis");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "YAxis");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myYAxis = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myYAxis, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myYAxisName = UA_QUALIFIEDNAME(1, "YAxis");
   const UA_NodeId myYAxisNodeId = UA_NODEID_STRING(1, "YAxis");
   UA_Server_addVariableNode(server, myYAxisNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myYAxisName, UA_NODEID_NULL, myVar, NULL, NULL);
   UA_Variant_deleteMembers(&myVar.value);
   
   myVar = UA_VariableAttributes_default;
   myVar.description = UA_LOCALIZEDTEXT("en-US", "Gyro Z Axis");
   myVar.displayName = UA_LOCALIZEDTEXT("en-US", "ZAxis");
   myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
   myVar.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
   UA_Float myZAxis = 0.5;
   UA_Variant_setScalarCopy(&myVar.value, &myZAxis, &UA_TYPES[UA_TYPES_FLOAT]);
   const UA_QualifiedName myZAxisName = UA_QUALIFIEDNAME(1, "ZAxis");
   const UA_NodeId myZAxisNodeId = UA_NODEID_STRING(1, "ZAxis");
   UA_Server_addVariableNode(server, myZAxisNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 
   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), myZAxisName, UA_NODEID_NULL, myVar, NULL, NULL);
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
   
   imu->IMUInit();
   
   RTPressure *pressure = RTPressure::createPressure(settings);
   RTHumidity *humidity = RTHumidity::createHumidity(settings);

   imu->IMUInit();
   imu->setSlerpPower(0.02);
   imu->setGyroEnable(true);
   imu->setAccelEnable(true);
   imu->setCompassEnable(true);
   
   //  set up pressure sensor
   if (pressure != NULL)
   {
      pressure->pressureInit();
   }
   //  set up humidity sensor
   if (humidity != NULL)
   {
       humidity->humidityInit();
    }
       
   // Setup a subscription to monitor the LED string
   std::string tmpStrNodeID("LEDString");
   char * chrNodeId = &tmpStrNodeID[0u]; //string to char* for UA C function
   UA_NodeId nodeId = UA_NODEID_STRING(1, chrNodeId);
   UA_UInt32 monId = 0;
   UA_UInt32 subId = 0;

   UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
   status = UA_Client_Subscriptions_addMonitoredItem(client, subId, 
      nodeId, UA_ATTRIBUTEID_VALUE, &sub_handler, NULL, &monId);
   std::cout<<"Starting up subscriptions..";
   sleep(1);
   while (true)
   {
      sleep(1);
      //usleep(imu->IMUGetPollInterval() * 1000);
      
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

      //Write XAxis
      //std::cout<<"X: "<<RTMath::displayDegrees("", imuData.fusionPose)<<"\n";
      //printf("Sample rate: %s\n", RTMath::displayDegrees("", imuData.fusionPose));
      UA_Float xAxisIn = imuData.gyro.x();
      UA_Variant_setScalarCopy(myVariant, &xAxisIn, &UA_TYPES[UA_TYPES_FLOAT]);
      UA_NodeId xAxisNodeId = UA_NODEID_STRING(1,"XAxis");
      status = UA_Client_writeValueAttribute(client,xAxisNodeId, myVariant);
      
      //Write YAxis
      
      //Write ZAxis
      UA_Client_Subscriptions_manuallySendPublishRequest(client);
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
    UA_ByteString certificate = loadCertificate();
    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4840, &certificate);
    UA_ByteString_deleteMembers(&certificate);
    /* uncomment next line to add a custom hostname */
    // UA_ServerConfig_set_customHostname(config, UA_STRING("custom"));
    
   /*UA_ServerConfig * config;
   config = UA_ServerConfig_new_default();
   UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(
                               UA_ConnectionConfig_default, 4840);*/
                               
                               
   UA_Server *server = UA_Server_new(config);
   addNodes(server);
   
   //Start Sensor Polling Thread
   int err = pthread_create(&(tid[0]), NULL, &pollSensors, NULL);
   
   //Run Server
   UA_Server_run(server, &running);
   UA_Server_delete(server);
   //nl.deleteMembers(&nl);
   return 0;
}

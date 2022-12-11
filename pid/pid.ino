#include "details.h"
HTTPClient http;
HTTPClient http_onem2m;
IPAddress ip;
WiFiClient client;

const char * ntpServer = "pool.ntp.org"; 
const char *ssid = "adyansh";
const char *password = "password";
const char *server = "mqtt3.thingspeak.com";

int writeChannelID = 1922377;
const char *writeAPIKey = "0EWEDZ5X4K5OT1B9";

const char *clientID = "KRwGOj0dFAcoDTw3Ky4aISs";
const char *mqttUser = clientID;
const char *mqttPwd = "oJFDlzrM/o4kx3Of1T5C7LrY";

PubSubClient mqttClient(server,1883, client);

int IN1 = 18;
int IN2 = 19;
int PWM = 5;
int ENCA = 32;
int ENCB = 34;

volatile int posi = 0;
long prevT = 0;
float eprev = 0;
float eintegral = 0;

// PID constants
float kp = 10;
float kd = 0.025;
float ki = 5;

int fieldArray[] = {1,0,0,0,0,0,0,0};
int dataArray[] = {-1,-1,-1,-1,-1,-1,-1,-1};

uint lastTime = 0;

void readEncoder(){
    int b = digitalRead(ENCB);
    if(b > 0){
        posi++;
    }
    else{
        posi--;
    }
}

int buffer_size=0;
unsigned long lastUpdateTime = 0;
unsigned long updateInterval = 17*1000L;
String field_value[150];
String client_thingspeak="https://api.thingspeak.com/channels/1922377/bulk_update.json";

void makeHttpPostContent()
{
//  HTTPClient http2;  
  http.begin(client_thingspeak);
  http.addHeader("Content-Type", "application/json");
  String post_data="";
  post_data+="{";
  post_data+=String("\"write_api_key\"")+String(":")+String("\"")+String(writeAPIKey)+String("\"")+String(",");
  post_data+="\"updates\":[";
  for(int i=0;i<buffer_size;i++)
  {
    post_data+="{";
    if(i==0)
    {
      post_data+=String("\"delta_t\"")+String(":")+String("\"")+String("0")+String("\"");
    }
    else 
    {
      post_data+=String("\"delta_t\"")+String(":")+String("\"")+String("1")+String("\"");
    }
    post_data+=",";
    post_data+="\"field1\":";
    post_data+="\""+String(field_value[i])+"\"";
    post_data+="}";
    if(i != buffer_size-1)
      post_data += ",";
  }
  post_data+="]}";
  Serial.println(post_data);
  int res = http.POST(post_data);
  Serial.println(res);
  http.end();
  buffer_size=0;
  lastUpdateTime=millis();
}

void mqttPublish()
{
    String dataString = "";
    for(int i = 0; i < 8; i++){
      if(fieldArray[i] == 1){
          dataString += "field"+String(i+1)+"="+String(dataArray[i])+"&";
      }
    }
    dataString += "status=MQTTPUBLISH";
    Serial.println(dataString);
    String topicString = "channels/"+String(writeChannelID)+"/publish";
    
  //  mqttClient.publish(topicString.c_str(), dataString.c_str());
  
}
float input[10];
float target;
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length ) {
  int semi_c=0;
  int inpno=0,simulate = 0;
  String inp_val="";
  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
    if((char)payload[i]==':'){
      semi_c++;
      if(semi_c>5){
        while((char)payload[i]!=','){
          i++;
          inp_val+=(char)payload[i];
          
        }
        inp_val = inp_val.substring(1,inp_val.length()-2);
        if(inp_val!="ul" && inpno == 0){
          Serial.println("exiting");  
          Serial.println(inp_val);
          return;
        }
        else if(inpno==4 && inp_val=="-1"){
          kp = 10;
          kd = 0.025;
          ki = 5;
          PID_reset(0);
          setMotor(0,0,PWM,IN1,IN2);
          return;
        }
        Serial.println(inp_val);
        input[inpno]=inp_val.toFloat();
        inpno++;
        inp_val="";
        if(inpno==5){
          break;
        }
      }
    }
    
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println("");
  kp = input[1];
  ki =  input[2];
  kd =  input[3];
  target= input[4];
  Serial.print("kp: ");
  Serial.println(kp);
  Serial.print("ki: ");
  Serial.println(ki);
  Serial.print("kd: ");
  Serial.println(kd);
  Serial.print("target: ");
  Serial.println(target);
  Serial.println("*");
  PID_control(target);
  //PID_reset(0);
  setMotor(0,0,PWM,IN1,IN2);
  
}

// Subscribe to ThingSpeak channel for updates.
void mqttSubscribe( long subChannelID ){
  
  String myTopic = "channels/"+String(subChannelID)+"/subscribe";
  Serial.println(mqttClient.subscribe(myTopic.c_str(),0));
  
}

int buffer_size_onem2m=0;
float position_onem2m[200];

void oneM2MPublish(){
  for(int i=0;i<min(5,buffer_size_onem2m);i++)
  {
    float pos=position_onem2m[i];
    String data;
    String server = "https://" + String() + CSE_IP + ":" + String() + CSE_PORT + String() + OM2M_MN;

    http_onem2m.begin(server + String() + OM2M_AE + "/" + OM2M_DATA_CONT + "/");
    Serial.println(server + String() + OM2M_AE + "/" + OM2M_DATA_CONT + "/");

    http_onem2m.addHeader("X-M2M-Origin", OM2M_ORGIN);
    http_onem2m.addHeader("Content-Type", "application/json;ty=4");
    http_onem2m.addHeader("Content-Length", "100");

    data = "[" + String(pos)  +   + "]"; 
    String req_data = String() + "{\"m2m:cin\": {"

      +
      "\"con\": \"" + data + "\","

      +
      "\"lbl\": \"" + "V1.0.0" + "\","

      //+ "\"rn\": \"" + "cin_"+String(i++) + "\","

      +
      "\"cnf\": \"text\""

      +
      "}}";
    Serial.println(req_data);
    int code = http_onem2m.POST(req_data);
    http_onem2m.end();
    Serial.println(code);
  }
  buffer_size_onem2m=0;
}

void setMotor(int dir, int pwmVal, int pwm, int in1, int in2){
    analogWrite(pwm,pwmVal);
    if(dir == 1){
        digitalWrite(in2,HIGH);
        digitalWrite(in1,LOW);
    }
    else if(dir == -1){
        digitalWrite(in2,LOW);
        digitalWrite(in1,HIGH);
    }
    else{
        digitalWrite(in1,LOW);
        digitalWrite(in2,LOW);
    }  
}

void PID_control(float target)
{
    
    uint startTime = millis();
    uint lastTime = millis();
    bool use_integral = false;
    int md = 0;
//    while(mqttClient.connected() == NULL)
//      {
//        Serial.println("Connecting to mqTT server");
//        mqttClient.connect(clientID, mqttUser, mqttPwd);
//        mqttSubscribe(writeChannelID);
//      }
//      mqttClient.loop();
    while(millis()- startTime < PID_TIMER){
      
      if(millis() - lastTime > 100){
        setMotor(0,0,PWM,IN1,IN2);

        // time difference
        long currT = micros();
        float deltaT = ((float) (currT - prevT))/( 1.0e6 );
        prevT = currT;

        // Read the positionL
        float pos = 0; 
        // noInterrupts(); // disable interrupts temporarily while reading
        pos = (float)posi * 0.85714285714;
        // interrupts(); // turn interrupts back on

        // error
        float e = target - pos;

//        if(mqttClient.connected() != NULL){
//          while(mqttClient.connected() == NULL)
//          {
//            Serial.println("Connecting to mqTT server");
//            mqttClient.connect(clientID, mqttUser, mqttPwd);     // ASK
//            mqttSubscribe(writeChannelID);
//          }
//          mqttClient.loop();
//        }
        if(millis() - startTime <= 3000){
          dataArray[0] = pos;
          field_value[buffer_size++] = pos;
        }
        //dataArray[1] = e;
//        mqttPublish();
        
        //oneM2MPublish(pos,e);
        position_onem2m[buffer_size_onem2m++]=pos; 
        // derivative
        float dedt = (e-eprev)/(deltaT);

        // integral
        if(dedt == 0)
          use_integral = true;
        if (use_integral == true)
        {
            if (pos > target)
            {
              if (md == -1)
                eintegral = 0;
              md = 1;
            }
            else if (pos < target)
            {
              if (md == 1)
                eintegral = 0;
              md = -1;
            }
           
        
          eintegral = eintegral + e*deltaT;
        }

        // control signal
        float u = kp*e + kd*dedt + ki*eintegral;

        // motor power
        float pwr = fabs(u);
        if( pwr > 255 ){
            pwr = 255;
        }

        // motor direction
        int dir = 1;
        if(u<0){
            dir = -1;
        }

        // signal the motor
        setMotor(dir,pwr,PWM,IN1,IN2);


        // store previous error
        eprev = e;

        Serial.print(target);
        Serial.print(" ");
        Serial.print(pos);
        Serial.print(" ");
        Serial.print(e);
        Serial.print(" ");
        Serial.print(dedt);
        Serial.print(" ");
        Serial.print(eintegral);
        Serial.print(" ");
        Serial.print(pwr);
        Serial.println();
        lastTime = millis();

      }
    }
    
}
void PID_reset(float target)
{
    
    uint startTime = millis();
    uint lastTime = millis();
    bool use_integral = false;
    int md = 0;
    
    while(millis()- startTime < 10000){
      if(millis() - lastTime > 100){
        setMotor(0,0,PWM,IN1,IN2);

        // time difference
        long currT = micros();
        float deltaT = ((float) (currT - prevT))/( 1.0e6 );
        prevT = currT;

        // Read the positionL
        float pos = 0; 
        // noInterrupts(); // disable interrupts temporarily while reading
        pos = (float)posi * 0.85714285714;
        // interrupts(); // turn interrupts back on

        // error
        float e = target - pos;

        // derivative
        float dedt = (e-eprev)/(deltaT);

        // integral
        if(dedt == 0)
          use_integral = true;
        if (use_integral == true)
        {
            if (pos > target)
            {
              if (md == -1)
                eintegral = 0;
              md = 1;
            }
            else if (pos < target)
            {
              if (md == 1)
                eintegral = 0;
              md = -1;
            }
        
          eintegral = eintegral + e*deltaT;
        }
        // control signal
        float u = kp*e + kd*dedt + ki*eintegral;
        // motor power
        float pwr = fabs(u);
        if( pwr > 255 ){
            pwr = 255;
        }
        // motor direction
        int dir = 1;
        if(u<0){
            dir = -1;
        }
        // signal the motor
        setMotor(dir,pwr,PWM,IN1,IN2);
        // store previous error
        eprev = e;
        lastTime = millis();

      }
    }
}
void setup() {
     // put your setup code here, to run once:
    Serial.begin(115200);

    
    Serial.print("Starting");
    pinMode(ENCA,INPUT);
    pinMode(ENCB,INPUT);
    attachInterrupt(digitalPinToInterrupt(ENCA),readEncoder,RISING);

    pinMode(PWM,OUTPUT);
//    analogWriteResolution(PWM, 8);
    pinMode(IN1,OUTPUT);
    pinMode(IN2,OUTPUT);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    delay(1000);

    Serial.print("Starting");
    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Connecting to Wifi..");
    }
    Serial.println("Connected to the WiFi network.");
  
    Serial.println(WiFi.localIP());
    configTime(0, 0, ntpServer);
    mqttClient.setServer(server, 1883);
    mqttClient.setCallback(mqttSubscriptionCallback);
    mqttClient.setBufferSize(2048);
    while(mqttClient.connected() == NULL)
    {
      Serial.println("Connecting to mqTT server");
      mqttClient.connect(clientID, mqttUser, mqttPwd);
      mqttSubscribe(writeChannelID);
    }
}

void loop(){
  if (millis() - lastUpdateTime >=  updateInterval && buffer_size>0) 
  {
    makeHttpPostContent();
    oneM2MPublish();
  }
  while(mqttClient.connected() == NULL)
  {
    Serial.println("Connecting to mqTT server");
    mqttClient.connect(clientID, mqttUser, mqttPwd);
    mqttSubscribe(writeChannelID);
  }
  mqttClient.loop();
}

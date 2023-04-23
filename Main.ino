#include <Blynk.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#define BLYNK_PRINT Serial
char ssid[] = "S";
char pass[] = "12345678";

const int oneWireBus = 15; // GPIO where the DS18B20 is connected to
OneWire oneWire(oneWireBus);    // Setup a oneWire instance to communicate with any OneWire devices

DallasTemperature sensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature sensor
#define TdsSensorPin 35
#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point 
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 0;


//Blynk data
#define BLYNK_TEMPLATE_ID "TMPL3QXpKjhzy"
#define BLYNK_TEMPLATE_NAME "Water Quality monitoring system"
#define BLYNK_AUTH_TOKEN "mgfNhileAvRxJsIioBy8pTV8nYwtuKNN"

//for multitasking
TaskHandle_t Task1;

//declaration of variables of test runs
//ph start
#define phPin 33   
unsigned long int avgValue;
float b;
int buf[10],temp;
//ph end 

int tbdt = 13; // turbidity
float tds = 10; 
float ph = 6.44;
float ec;


LiquidCrystal_I2C lcd(0x27,20,4);  

void setup()
{
  pinMode(TdsSensorPin, INPUT);
  scanningBuffer();
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  sensors.begin();//for temp sensor
  xTaskCreatePinnedToCore(
    loop2,
    "readAndBlynk",
    4000,
    NULL,
    1,
    &Task1,
    0
  );
}

void loop2(void * unused) 
// This function has an artificial infinite loop created 
//which is currently running on 0th core of esp32
{
  while(1)
  {
    readAllValues();
    Blynk.run();
    //vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
  
}

void loop()
{
  displayAllValues();
    
}

//rest of the functions
void scanningBuffer() //starting display
{
  lcd.init();                     
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Welcome To Water");
  lcd.setCursor(0,1);
  lcd.print("Quality Tester");
  delay(7000);
  int j;
  int i;
  for(j=0;j<5;j++)
  {    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Scanning");
    for(i=8;i<16;i++)
    { 
      lcd.setCursor(i,0);
      delay(100);
      lcd.print(".");
    }
    for(j=0;j<5;j++)
    {    
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Connecting");
      for(i=10;i<16;i++)
      { 
        lcd.setCursor(i,0);
        delay(100);
        lcd.print(".");
      }
    }
  }
}
void readAllValues()
{
  readTurbidity();
  readTds();
  readPh();
  readTemp();
}
void displayAllValues()
{
  displayTurbidity();
  displayTds();
  displayTemp();
  displayPh();
}

void displayTurbidity()
{
  lcd.clear();                     
  lcd.setCursor(0,0);
  lcd.print("Turbidity");
  lcd.setCursor(0,1);
  lcd.print(tbdt);
  delay(5000);
}

void displayTds()
{
  lcd.clear();                     
  lcd.setCursor(0,0);
  lcd.print("TDS");
  lcd.setCursor(0,1);
  lcd.print(tdsValue);
  delay(5000);
}

void displayPh()
{
  lcd.clear();                     
  lcd.setCursor(0,0);
  lcd.print("PH value");
  lcd.setCursor(0,1);
  lcd.print(ph);
  delay(5000);
}


void displayTemp()
{
  lcd.clear();                     
  lcd.setCursor(0,0);
  lcd.print("Temperature");
  lcd.setCursor(0,1);
  lcd.print(temperature);
  lcd.setCursor(3, 1);
  lcd.print("°C");
  delay(5000);
}

void readTurbidity()
{
  int sensorValue = analogRead(32);
  tbdt = map(sensorValue, 0, 3550, 100, 0);
  Blynk.virtualWrite(V3, tbdt);
}

void readTds()
{
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
 
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U)  //every 40 milliseconds,read the analog value from the ADC
  {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
    tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
  }
  Blynk.virtualWrite(V0, tdsValue);
}








void readPh()
{
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  {
    buf[i]=analogRead(phPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*3.3/4096/6; //convert the analog into millivolt
  ph=3.5*phValue;                      //convert the millivolt into pH value
  //Serial.println(ph);
  Blynk.virtualWrite(V2, ph);
}

void readTemp()
{
  Blynk.virtualWrite(V1, temperature);
}


int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
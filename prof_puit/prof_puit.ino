#include <SPI.h>

int pin_clock = 3;

void resetsensor() { //this function keeps the sketch a little shorter
  SPI.setDataMode(SPI_MODE0);
  SPI.transfer(0x15); //21
  SPI.transfer(0x55); //85
  SPI.transfer(0x40); //64
}

void setup(void) {
  Serial.begin(9600);
  
  SPI.begin(); //see SPI library details on arduino.cc for details
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV128); 
  
  pinMode(pin_clock, OUTPUT);
  delay(100);
}

void printValue(unsigned int wordX, const char* wordName){
  Serial.print(wordName);
  Serial.print(" : ");
  Serial.print(wordX);
  Serial.print(" - ");
  Serial.println(wordX,BIN);  
}

void loop(void) {
  
  //TCCR1B = (TCCR1B & 0xF8) | 1 ; //generates the MCKL signal
  analogWrite (pin_clock, 128) ;
  resetsensor();//resets the sensor - caution: afterwards mode = SPI_MODE0!

  
  //Calibration word 1
  unsigned int word1 = 0;
  unsigned int word11 = 0;
  SPI.transfer(0x1D); //29 send first byte of command to get calibration word 1
  SPI.transfer(0x50); //80 send second byte of command to get calibration word 1
  SPI.setDataMode(SPI_MODE1); //change mode in order to listen
  word1 = SPI.transfer(0x00); //send dummy byte to read first byte of word
  word1 = word1 << 8; //shift returned byte
  word11 = SPI.transfer(0x00); //send dummy byte to read second byte of word
  word1 = word1 | word11; //combine first and second byte of word
  printValue(word1, "word1");
  resetsensor();//resets the sensor

  //Calibration word 2; see comments on calibration word 1
  unsigned int word2 = 0;
  byte word22 = 0;
  SPI.transfer(0x1D);
  SPI.transfer(0x60); //96
  SPI.setDataMode(SPI_MODE1);
  word2 = SPI.transfer(0x00);
  word2 = word2 << 8;
  word22 = SPI.transfer(0x00);
  word2 = word2 | word22;
  printValue(word2, "word2");
  resetsensor();//resets the sensor

  //Calibration word 3; see comments on calibration word 1
  unsigned int word3 = 0;
  byte word33 = 0;
  SPI.transfer(0x1D);
  SPI.transfer(0x90); //144
  SPI.setDataMode(SPI_MODE1);
  word3 = SPI.transfer(0x00);
  word3 = word3 << 8;
  word33 = SPI.transfer(0x00);
  word3 = word3 | word33;
  printValue(word3, "word3");
  resetsensor();//resets the sensor

  //Calibration word 4; see comments on calibration word 1
  unsigned int word4 = 0;
  byte word44 = 0;
  SPI.transfer(0x1D);
  SPI.transfer(0xA0); //160
  SPI.setDataMode(SPI_MODE1);
  word4 = SPI.transfer(0x00);
  word4 = word4 << 8;
  word44 = SPI.transfer(0x00);
  word4 = word4 | word44;
  printValue(word4, "word4");

  long c1 = (word1 >> 1) & 0x7FFF;
  long c2 = ((word3 & 0x003F) << 6) | (word4 & 0x003F);
  long c3 = (word4 >> 6) & 0x03FF;
  long c4 = (word3 >> 6) & 0x03FF;
  long c5 = ((word1 & 0x0001) << 10) | ((word2 >> 6) & 0x03FF);
  long c6 = word2 & 0x003F;

  printValue(c1, "c1");
  printValue(c2, "c2");
  printValue(c3, "c3");
  printValue(c4, "c4");
  printValue(c5, "c5");
  printValue(c6, "c6");    
  
  resetsensor();//resets the sensor

  //Pressure:
  unsigned int presMSB = 0; //first byte of value
  unsigned int presLSB = 0; //last byte of value
  unsigned int D1 = 0;
  SPI.transfer(0x0F); //send first byte of command to get pressure value
  SPI.transfer(0x40); //send second byte of command to get pressure value
  delay(35); //wait for conversion end
  SPI.setDataMode(SPI_MODE1); //change mode in order to listen
  presMSB = SPI.transfer(0x00); //send dummy byte to read first byte of value
  presMSB = presMSB << 8; //shift first byte
  presLSB = SPI.transfer(0x00); //send dummy byte to read second byte of value
  D1 = presMSB | presLSB;
  printValue(D1,"D1");
  resetsensor();//resets the sensor
  
  //Temperature:
  unsigned int tempMSB = 0; //first byte of value
  unsigned int tempLSB = 0; //last byte of value
  unsigned int D2 = 0;
  SPI.transfer(0x0F); //send first byte of command to get temperature value
  SPI.transfer(0x20); //send second byte of command to get temperature value
  delay(35); //wait for conversion end
  SPI.setDataMode(SPI_MODE1); //change mode in order to listen
  tempMSB = SPI.transfer(0x00); //send dummy byte to read first byte of value
  tempMSB = tempMSB << 8; //shift first byte
  tempLSB = SPI.transfer(0x00); //send dummy byte to read second byte of value
  D2 = tempMSB | tempLSB; //combine first and second byte of value
  printValue(D2,"D2");
  resetsensor();//resets the sensor




  //calculation of the real values by means of the calibration factors and the maths
  //in the datasheet. const MUST be long
  const long UT1 = (c5 * 8) + 20224;
  const long dT = D2 - UT1;
  const long TEMP = 200 + ((dT * (c6 + 50)) >> 10);
  const long OFF  = (c2 * 4) + (((c4 - 512) * dT) >> 12);
  const long SENS = c1 + ((c3 * dT) >> 10) + 24576;
  const long X = (SENS * (D1 - 7168) >> 14) - OFF;
  long PCOMP = ((X * 10) >> 5) + 2500;
  float TEMPREAL = TEMP/10;
  float PCOMPHG = PCOMP * 750.06 / 10000; // mbar*10 -> mmHg === ((mbar/10)/1000)*750/06
  
  printValue(UT1,"UT1");
  printValue(dT,"dT");
  printValue(TEMP,"TEMP");
  printValue(OFF,"OFF");
  printValue(SENS,"SENS");
  printValue(X,"X");

  printValue(TEMPREAL,"Temp °C");
  printValue(PCOMP,"Pressure mBar");
  printValue(PCOMPHG,"Pressure mmHg");
  
  Serial.println("************************************");

  delay(1000);
}

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

// The control pins can connect to any pins but we'll use the 
// analog lines since that means we can double up the pins
// with the touch screen (see the TFT paint example)
#define LCD_CS A3    // Chip Select goes to Analog 3
#define LCD_CD A2    // Command/Data goes to Analog 2
#define LCD_WR A1    // LCD Write goes to Analog 1
#define LCD_RD A0    // LCD Read goes to Analog 0

// you can also just connect RESET to the arduino RESET pin
#define LCD_RESET A4

/* For the 8 data pins:
Duemilanove/Diecimila/UNO/etc ('168 and '328 chips) microcontoller:
D0 connects to digital 8
D1 connects to digital 9
D2 connects to digital 2
D3 connects to digital 3
D4 connects to digital 4
D5 connects to digital 5
D6 connects to digital 6
D7 connects to digital 7

For Mega's use pins 22 thru 29 (on the double header at the end)
*/

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0 
#define WHITE           0xFFFF


Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup(void) {
  Serial.begin(9600);
  Serial.println("8 Bit LCD test!");

#ifdef USE_ADAFRUIT_SHIELD_PINOUT
  Serial.println("Using Adafruit 2.8 inch TFT Arduino Shield Pinout");
#else
  Serial.println("Using Adafruit 2.8 inch TFT Breakout Pinout");
#endif

  tft.reset();
  
  uint16_t identifier = tft.readRegister(0x0);
  if (identifier == 0x9325) {
    Serial.println("Found ILI9325");
  } else if (identifier == 0x9328) {
    Serial.println("Found ILI9328");
  } else if (identifier == 0x7575) {
    Serial.println("Found HX8347G");
  } else {
    Serial.print("Unknown driver chip ");
    Serial.println(identifier, HEX);
    while (1);
  }  
 

  tft.begin(identifier);

  /*  
  uint32_t time = millis();
  testtext(YELLOW);
  Serial.println(millis() - time);
  delay(2000);
  time = millis();
  testlines(CYAN);
  Serial.println(millis() - time);
  delay(500);
  time = millis();
  testfastlines(RED, BLUE);
  Serial.println(millis() - time);
  delay(500);
  testdrawrects(GREEN);
  delay(500);
  testfillrects(YELLOW, MAGENTA);
  delay(500);
  tft.fillScreen(BLACK);
  testfillcircles(10, MAGENTA);
  testdrawcircles(10, WHITE);
  delay(500); 
  testtriangles();
  delay(500); 
  testfilltriangles();
  delay(500); 
  testRoundRect();
  delay(500); 
  testFillRoundRect();
  */
  tft.fillScreen(BLACK);  

  drawBoundingRect(GREEN);  
  tft.setRotation(3);


}

#define TEXT_4_HEIGHT 27
#define TERM    0x00
#define TITLE   0x01
#define ARTIST  0x02 
#define ALBUM   0x03 
#define GENRE   0x04
#define TIME    0x05
#define NEWS    0x06

String theMessage = "";
int msgID = 0;

void loop(void) {



  // first byte, what data it is
  if(Serial.available() > 0){
    char c = Serial.read();

    if (c == TERM){     // End of message
      if (msgID != -1 && theMessage.length() > 0){
	writeString(msgID, theMessage);      
	msgID = -1;
      }
      
    } else if (c == TITLE || c == ARTIST || c == ALBUM || c == GENRE || c == TIME || c == NEWS){ // start of message
      msgID = c;
      theMessage = "";

    } else { // continuation of current message
      theMessage += c;
    }
  }

}


void writeString(int where, String theMessage){

  if (where == TITLE){
    showTitle(theMessage);
  } else if (where == ARTIST){
    showArtist(theMessage);
  } else if (where == ALBUM){
    showAlbum(theMessage);
  } else if (where == GENRE) {
    showGenre(theMessage);
  } else if (where == TIME) {
    showTime(theMessage);
  } else if (where == NEWS) {
    showNews(theMessage);
  }

}

void showNews(String news){
     
     int maxNewsChar = 500;
  if (news.length() > maxNewsChar){
   news = news.substring(0, maxNewsChar);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(0, 20,  339,  319, BLACK);
  tft.drawRect(0, 20,  339,  319, YELLOW);
  tft.setCursor(5, 25);
  tft.setTextColor(GREEN);
  tft.print(news);

}


void showTime(String time){

  if (time.length() > 20){
   time = time.substring(0, 20);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(100, 0,  339,  19, BLACK);
  tft.setCursor(100, 2);
  tft.setTextColor(WHITE);
  tft.print(time);

}




void showTitle(String title){

  if (title.length() > 20){
   title = title.substring(0, 20);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(0, 0,  319, 50, BLACK);
  tft.setCursor(5, 10);
  tft.setTextColor(YELLOW);
  tft.print(title);

}


void showArtist(String artist){

  if (artist.length() > 20){
   artist = artist.substring(0, 20);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(0, 50,  319, 50, RED);
  tft.setCursor(5, 60);
  tft.setTextColor(BLACK);
  tft.print(artist);

}


void showAlbum(String album){

  if (album.length() > 20){
   album = album.substring(0, 20);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(0, 100,  319, 50, BLUE);
  tft.setCursor(5, 110);
  tft.setTextColor(YELLOW);
  tft.print(album);

}


void showGenre(String genre){

  if (genre.length() > 20){
   genre = genre.substring(0, 20);
  }

  tft.setTextSize(2);

  // x, y, w, h
  tft.fillRect(0, 150,  319, 50, WHITE);
  tft.setCursor(5, 160);
  tft.setTextColor(BLACK);
  tft.print(genre);

}









void drawBoundingRect(uint16_t color){
  tft.drawRect(0, 0, tft.width()-1, tft.height()-1, color);
}


void testFillRoundRect() {
  tft.fillScreen(BLACK);
  
  for (int16_t x=tft.width(); x > 20 ; x-=6) {
    tft.fillRoundRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, x/8,  tft.Color565(0, x, 0));
 }
}

void testRoundRect() {
  tft.fillScreen(BLACK);
  
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawRoundRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, x/8, tft.Color565(x, 0, 0));
 }
}

void testtriangles() {
  tft.fillScreen(BLACK);
  for (int16_t i=0; i<tft.width()/2; i+=5) {
    tft.drawTriangle(tft.width()/2, tft.height()/2-i,
                     tft.width()/2-i, tft.height()/2+i,
                     tft.width()/2+i, tft.height()/2+i, tft.Color565(0, 0, i));
  }
}

void testfilltriangles() {
  tft.fillScreen(BLACK);
  
  for (int16_t i=tft.width()/2; i>10; i-=5) {
    tft.fillTriangle(tft.width()/2, tft.height()/2-i,
                     tft.width()/2-i, tft.height()/2+i,
                     tft.width()/2+i, tft.height()/2+i, 
                     tft.Color565(0, i, i));
    tft.drawTriangle(tft.width()/2, tft.height()/2-i,
                     tft.width()/2-i, tft.height()/2+i,
                     tft.width()/2+i, tft.height()/2+i, tft.Color565(i, i, 0));    
  }
}
void testtext(uint16_t color) {
  tft.fillScreen(BLACK);
  tft.setCursor(0, 20);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.setTextSize(4);
  tft.println(millis());
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < tft.width(); x+=radius*2) {
    for (int16_t y=radius; y < tft.height(); y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }  
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < tft.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < tft.height()+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }  
}


void testfillrects(uint16_t color1, uint16_t color2) {
 tft.fillScreen(BLACK);
 for (int16_t x=tft.width()-1; x > 6; x-=6) {
   //Serial.println(x, DEC);
   tft.fillRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color1);
   tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color2);
 }
}

void testdrawrects(uint16_t color) {
 tft.fillScreen(BLACK);
 for (int16_t x=0; x < tft.width(); x+=6) {
   tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color);
 }
}

void testfastlines(uint16_t color1, uint16_t color2) {
   tft.fillScreen(BLACK);
   for (int16_t y=0; y < tft.height(); y+=5) {
     tft.drawFastHLine(0, y, tft.width(), color1);
   }
   for (int16_t x=0; x < tft.width(); x+=5) {
     tft.drawFastVLine(x, 0, tft.height(), color2);
   }
  
}

void testlines(uint16_t color) {
   tft.fillScreen(BLACK);
   for (int16_t x=0; x < tft.width(); x+=6) {
     tft.drawLine(0, 0, x, tft.height()-1, color);
   }
   for (int16_t y=0; y < tft.height(); y+=6) {
     tft.drawLine(0, 0, tft.width()-1, y, color);
   }
   
   tft.fillScreen(BLACK);
   for (int16_t x=0; x < tft.width(); x+=6) {
     tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
   }
   for (int16_t y=0; y < tft.height(); y+=6) {
     tft.drawLine(tft.width()-1, 0, 0, y, color);
   }
   
   tft.fillScreen(BLACK);
   for (int16_t x=0; x < tft.width(); x+=6) {
     tft.drawLine(0, tft.height()-1, x, 0, color);
   }
   for (int16_t y=0; y < tft.height(); y+=6) {
     tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
   }

   tft.fillScreen(BLACK);
   for (int16_t x=0; x < tft.width(); x+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
   }
   for (int16_t y=0; y < tft.height(); y+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
   }
}


void testBars() {
  int16_t i,j;
  for(i=0; i < tft.height(); i++)
  {
    for(j=0; j < tft.width(); j++)
    {
      if(i>279) tft.writeData(WHITE);
      else if(i>239) tft.writeData(BLUE);
      else if(i>199) tft.writeData(GREEN);
      else if(i>159) tft.writeData(CYAN);
      else if(i>119) tft.writeData(RED);
      else if(i>79) tft.writeData(MAGENTA);
      else if(i>39) tft.writeData(YELLOW);
      else tft.writeData(BLACK);
    }
  }
}

// UTOPIA
// 2018 by Daan Henderson

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define G4 240 //assigning notes to frequencies
#define FIS4 320
#define F4 400
#define E4 480
#define DIS4 560
#define D4 670
#define CIS4 790
#define C4 890
#define H3 1000
#define AIS3 1100
#define A3 1200
#define GIS3 1350
#define G3 1500
#define FIS3 1650
#define F3 1800
#define E3 2000
#define DIS3 2180
#define D3 2370
#define CIS3 2600
#define C3 2800
#define H2 3000
#define AIS2 3200
#define A2 3400
#define GIS2 3700
#define  G2 4000
#define FIS2 4300
#define P 9999 //pause = don't play any notes
//note lengths:
#define XS 5 //ultrashort
#define S 125 //sixteenth
#define L 500 // half

int buzzer = 12; // CHANGE PINS IF NEEDED
int buttonpin = 13;

float tempo = 240; //beats per minute
int level5mel[128] = { //cantina theme
  D3, P, G3, P, D3, P, G3, P,            D3, G3, P, D3, D3, CIS3, D3, P,
  D3, CIS3, D3, C3, P, H2, C3, H2,       AIS2, P, P, P, G2, P, P, P,
  D3, P, G3, P, D3, P, G3, P,            D3, G3, P, D3, D3, CIS3, D3, P,
  C3, P, C3, P, P, H2, C3, P,            F3, DIS3, P, D3, P, C3, P, CIS3,

  D3, P, G3, P, D3, P, G3, P,            D3, G3, P, D3, D3, CIS3, D3, P,
  F3, P, F3, P, P, D3, C3, P,           AIS2, P, P, P, G2, P, P, P,
  G2, FIS2, G2, A2, AIS2, A2, AIS2, C3,    D3, CIS3, D3, F3, P, P, P, P,
  GIS3, P, G3, P, CIS3, D3, P, P,        AIS2, P, P, P, P, P, P, P
};
float linput = 0; //joystick input
float thrustInput = 0;
float hcollisionSpeed = 0.8; //lander crashes when faster
float vcollisionSpeed = 0.7;
const float ONTIME = 1;
int NOTELENGTH = 8;
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

//ebook content
const char BOOK[] PROGMEM = "                         Shakespeare            Sonnets                             From fairest crea-   tures we desire in-  crease, That thereby beauty's rose might  never die, But as theriper should by time decease, His teder   heir might bear his  memory: But thou con-tracted to thine own bright eyes, Feed'st thy light's flame    with self-substantialfuel, Making a faminewhere abundance lies,Thy self thy foe, to thy sweet self too   cruel: Thou that art now the world's freshornament, And only   herald to the gaudy  spring, Within thine own bud buriest thy  content, And, tender churl, mak'st waste  in niggarding: Pity  the world, or else   this glutton be,     To eat the world's   due, by the grave andthee.                                     When forty winters   shall besiege thy    brow, And dig deep   trenches in thy      beauty's field, Thy  youth's proud livery so gazed on now, Willbe a totter'd weed  of small worth held:  Then being asked,    where all thy beauty lies, Where all the  treasure of thy lustydays; To say, within thine own deep sunkeneyes, Were an all-   eating shame, and    thriftless praise.   How much more praise deserv'd thy beauty'suse,If thou couldst  answer 'This fair    child of mineShall   sum my count, and    make my old excuse,' Proving his beauty bysuccession thine!    This were to be new  made when thou art   old,And see thy bloodwarm when thou       feel'st it cold.";
int BOOKpos = 0;
int scrollSpeed = 84;
int nLetters = 84;//letters per page
static const unsigned char PROGMEM logo16_glcd_bmp[] = //letters
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};
float minJ = 50; // y direction threshhold
float minJx = 100; //x direction threshhold
int barheight = 10; //PONG rackets size
int barthickness = 2;
int levelspeedchange = 6; //how much faster when advancing in PONG?
int maxx = 126; //PONG ball boundaries
int maxy = 32 - barheight;

int gbldelay = 250;
bool soundon = true; //play sound or not
String menu[3] = {"read", "pong", "land"}; //menu options
bool playing = false; //which app is active
bool reading = false;
bool landing = false;
int menufg = 1;
int menuChoice = 99;

void racketsound(int del) {//sound the rackets make
  if (soundon) {
    digitalWrite(buzzer, HIGH);
    delay(del);
    digitalWrite(buzzer, LOW);
  }
}

void ton (int note, int value) { //play one note, value = length
  if (note == P) {
    delay(value);
  } else {
    float scalefactor = tempo / 120;
    float onedur = 1 + note / 1000;
    float repeats = value / scalefactor / onedur;

    for (int i = 0; i < repeats; i++) {
      digitalWrite(buzzer, HIGH);
      delay(1);
      digitalWrite(buzzer, LOW);
      delayMicroseconds(note);
    }
  }
}

void melody (int notes[], int value, int startnote, int nnotes) {  //play a melody
  for (int i = startnote; i < startnote + nnotes; i++) {
    ton (notes[i], value);
  }
}

void runningtext(String text, int len, int del) { //"landed", "crashed" in LANDER
  display.setTextSize(2);
  display.setCursor(12, 10);
  for (int i = 0; i < len; i++) {
    display.print(text[i]); display.display(); delay(del);
  }
}

void menuSwitch(int newfg, bool beeps) { //switch to different app in menu
  display.clearDisplay();
  display.display();
  display.setCursor(0, 0);
  display.setTextSize(1);
  int upperleft;
  int upperright;
  int center = newfg;
  if (center == 1) {
    upperleft = 0;
    upperright = 2;
  }
  if (center == 2) {
    upperleft = 1;
    upperright = 0;
  }
  if (center == 0) {
    upperleft = 2;
    upperright = 1;
  }
  display.print(menu[upperleft] + "             " + menu[upperright]);
  display.setCursor(28, 9);
  display.setTextSize(3);
  display.print(menu[center]);
  display.display();
  if (soundon && beeps) {
    ton(G3, XS);
  }
  menuChoice = center;
  delay(300);
}

class Player { //left PONG racket
  public:
    float x = 0;
    float y = 10;
    float mspeed = 0.2;
    void up(float Jy) {
      y += Jy * 0.01 * mspeed;
      if (y < 0) {
        y = 0;
      }
    }
    void down(float Jy) {
      y += Jy * 0.01 * mspeed;
      if (y > maxy) {
        y = maxy;
      }
    }
  private:
};

Player p;
class Enemy { //right PONG racket
  public:
    float x = 126;
    float y;
    float mspeed;
    float initialspeed = 32;

    void reset() {
      mspeed = initialspeed;
      y = random(15) + 5;
    }
    void up() {
      y -= 0.01 * mspeed;
      if (y < 0) {
        y = 0;
      }
    }
    void down() {
      y += 0.01 * mspeed;
      if (y > maxy) {
        y = maxy;
      }
    }
  private:
};
Enemy e;

void andFight() { //starts new PONG
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print("...and...     GO!");
  display.display();
  delay(200 + gbldelay);
  playing = true;
}

class Plat { //LANDER platform
  public:
    float x;
    float y;
    float w;
    float h;
    void reset() {
      x = random (100) + 10;
      y = random (21) + 10;
      w = 20;
      h = 2;
    }
    void draw() {
      display.drawRect(int(x - w / 2), int(y), w, h, WHITE);
    }
};
Plat plat;

class Lander { //LANDER spaceship
  public:
    float x;
    float y;
    float sp;
    float vspeed;
    float hspeed;
    float vacc;
    float hacc;
    float maxvacc;
    float gravity;
    float maxthrust;
    float thrust;
    float thrustpower;
    int points;
    int w;
    bool landed;
    void reset() {
      x = 32;
      y = 16;
      sp = 0.00012; //vertical speed
      vspeed = 0;
      hspeed = 0;
      vacc = 0;
      hacc = 0;
      gravity = .015;
      maxthrust = 1;
      thrust = 0;
      thrustpower = 7;
      w = 7;
      landed = false;
      points = 0;
    }
    void launch() {
      for (int i = 1; i < 5; i++) {
        y--;
        display.clearDisplay();
        draw();
        plat.draw();
        drawthrust(-thrust);

        display.display();
        delay(200);
      }
      landed = false;
      plat.reset();
      while (platcollide(plat)) {
        plat.reset();
      }
    }
    void land() {
      landed = true;
      drawlegs();
      display.setCursor(1, 10);
      runningtext("LANDED:D", 8, 150);
      vspeed = 0;
      hspeed = 0;
      vacc = 0;
      hacc = 0;
      points += 150;
      delay(1000);
    }
    bool platcollide(Plat pl) {
      if (x + w > pl.x - (pl.w / 2) && x - w < pl.x + (pl.w / 2 ) && y > pl.y - 1 && y < pl.y + 5) {
        return true;
      } else {
        return false;
      }
    }
    void posupdate() {
      vacc = linput * sp;
      hacc = gravity;
      if (thrustInput > 30) {
        thrust = -thrustInput * thrustpower * 0.00001;
        drawthrust(-thrust);
      } else {
        thrust = 0;
      }
      if (thrust > maxthrust) {
        thrust = maxthrust;
      }
      hacc += thrust;
      vspeed += vacc;
      hspeed += hacc;
      x += vspeed;
      if (x > 128) {
        x -= 128;
      } else if (x < 0) {
        x += 128;
      }
      y += hspeed;
      if (y < 3) {
        y = 3;
        hspeed = 0;
      }
      if (y > 35) {
        display.clearDisplay();
        display.setCursor(15, 10);
        display.setTextSize(1);
        display.print(".out of bounds.");
        display.display();
        delay(1500);
        reset();
      }
    }
    void draw() {
      display.drawLine(int(x), int(y), int(x - w), int(y - 2), WHITE);
      display.drawLine(int(x - w), int(y - 2), int(x), int(y - 4), WHITE);
      display.drawLine(int(x), int(y - 4), int(x + w), int(y - 2), WHITE);
      display.drawLine(int(x + w), int(y - 2), int(x), int(y), WHITE);
      if (linput > 100) {
        display.drawLine(int(x - w - 3), int(y - 2), int(x - w - 2 - linput * 0.002), int(y - 1), WHITE);
        racketsound(2);
      } else       if (linput < -100) {
        display.drawLine(int(x + w + 3), int(y - 2), int(x + w + 2 + linput * 0.002), int(y - 1), WHITE);
        racketsound(2);
      }
      //draw points:
      display.setCursor(0, 0);
      display.setTextSize(0);
      display.print(points);
    }
    void drawthrust(float th) {
      display.drawLine(int(x), int(y + 3), int(x), int (y + 3 + th * 50), WHITE);
      display.drawLine(int(x - 2), int(y + 3), int(x - 4), int( y + 3 + th * 50), WHITE);
      display.drawLine(int(x + 2), int(y + 3), int(x + 4), int( y + 3 + th * 50), WHITE);
      racketsound(1);
    }
    void drawlegs() {
      display.drawLine(int(x - 3), int(y - 5), int(x - 8), int(y), WHITE);
      display.drawLine(int(x + 3), int(y - 5), int(x + 8), int(y), WHITE);
    }
};
Lander lander;
void winmel() { //play winning jingle (random part of cantina theme)
  melody(level5mel, S, int(random(399) / 100) * 32 - 1, 32);
}

class Ball { //PONG ball
  public:
    float x;
    float y;
    float mspeed;
    float xdir;
    float ydir;
    float factor;
    void newspeed() {
      factor = 9 + ( 0.3 * ((e.mspeed - e.initialspeed) / levelspeedchange));
      ydir = factor * ( -0.1 + float(random(200) / 1000));
      if (ydir < 0.02 && ydir > -0.02) {
        ydir = 1;
      }
      mspeed = 50 + ((e.mspeed - e.initialspeed) * 5 / levelspeedchange);
      x = 80;
      y = 16;
      xdir = -1;
      /*while (abs(ydir) < 0.14) {
        ydir = -0.5 + float((random(100) / 50));
        }*/
    }
    
    void win() { //PONG win sequence
      playing = false;
      display.clearDisplay();
      mspeed = 50;
      e.mspeed += levelspeedchange;
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.print(" MASTER OF   PONG!");
      display.display();
      winmel();

      delay(100);
      newspeed();
      andFight();
    }
    void lose() { //PONG lose sequence
      playing = false;
      display.clearDisplay();
      mspeed = 50;
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.print(" CAN U DO   BETTER?");
      display.display();
      ton(F3, S);
      ton(FIS2, L);
      delay(700);
      newspeed();
      if (e.mspeed >= e.initialspeed + levelspeedchange) {
        e.mspeed -= levelspeedchange;
      }
      andFight();
    }

    void go() { //updates PONG ball
      x += xdir * 0.02 * mspeed;
      if (x < barthickness) {
        x = barthickness;
        xdir *= -1;
        racketsound(1);
        if (y < p.y - 1 || y > p.y + 1 + barheight) {
          lose();
        }
      } else if (x > maxx) {
        x = maxx;
        xdir *= -1;
        racketsound(1);
        if (y < e.y - 1 || y > e.y + 1 + barheight) {
          win();
        }
      } else {
        //ydir *= 1.01;
      }
      y += ydir * 0.01 * mspeed;
      if (y < 0) {
        y = 0;
        ydir *= -1.01;
        racketsound(3);
      } else if (y > 31) {
        y = 31;
        ydir *= -1.01;
        racketsound(1);
      }
    }
  private:
};
Ball b;
//------------------------------------------------------------------------------------------------------------------
void setup() { //executes ONCE
  pinMode(buzzer, OUTPUT);
  pinMode(buttonpin, INPUT);
  digitalWrite(buttonpin, HIGH);
  unsigned long beginn = millis();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  randomSeed(1365);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print("    games");
  display.display();
  ton(CIS3, XS);
  delay(gbldelay);

  display.clearDisplay();
  display.setCursor(0, 0);
  delay(100);
  display.print(" beyond");
  display.display();
  ton(CIS3, XS);
  delay(gbldelay);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("              logic");
  display.display();
  ton(CIS3, XS);
  delay(gbldelay);

  display.clearDisplay();
  display.setCursor(0, 0);
  //
  display.print("           presents:");
  display.display();
  delay(gbldelay);
  display.clearDisplay();
  display.setCursor(5, 8);
  display.setTextSize(3);
  display.print("Utopia");
  delay(200);
  for (int t = 0; t < 28; t++) {
      display.clearDisplay();
    //delay (150);
    ton(level5mel[95+t],S);    
    display.setCursor(random(30), random(28));  
    display.setTextSize(random(0)+ 2);
    display.print("Utopia");   
    display.display();
  }  
  display.display();
  e.reset();
  menuSwitch(1, false);
}
void printBOOK(int startPos) { //display current book position
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setCursor(0, 0);
  char excerpt[nLetters];
  for (int i = 0; i <  nLetters; i++) {
    excerpt[i] = pgm_read_byte_near(BOOK + (i + startPos));
  }
  String out = String(excerpt);
  display.print(out);
  display.display();
  delay(600);
}
void loop() { //executes REPEATEDLY

  //first check if joystick is pressed down = switch to / from app....
  
  if (digitalRead(buttonpin) == LOW) {
    if (menuChoice == 1) {//SWITCH TO PONG
      reading = false;
      landing = false;
      menuChoice = 99;
      b.newspeed();
      andFight();
      display.setTextSize(1);
    } else if (menuChoice == 2) {//SWITCH TO LAND
      landing = true;
      playing = false;
      reading = false;
      menuChoice = 99;
      display.clearDisplay();
      display.display();
      delay(500);
      lander.reset();
      plat.reset();
      while (lander.platcollide(plat)) {
        plat.reset();
      }
    } else if (menuChoice == 0) {//SWITCH TO READ
      playing = false;
      landing = false;
      reading = true;
      printBOOK(0);
      menuChoice = 99;
    }
    else { //IF CURRENTLY IN AN APP:
      reading = false; //SWITCH TO MENU
      landing = false;
      playing = false;
      menuSwitch(1, true);
    }
  }
  
  //now check which app is currently active...
  
  if (playing) { // PONG is active
    display.clearDisplay();//display.drawRect(int(p.x), int(p.y), 10, 5, BLACK);
    int Jy = analogRead(A1) - 512;
    if (Jy < -minJ) {
      p.up(Jy);
    }
    if (Jy > minJ) {
      p.down(Jy);
    }
    if (e.y + int(barheight / 2) > b.y) {
      e.up();
    }
    if (e.y + int(barheight / 2) < b.y) {
      e.down();
    }
    display.drawRect(int(p.x), int(p.y), barthickness, barheight, WHITE);
    display.drawRect(int(e.x), int(e.y), barthickness, barheight, WHITE);

    b.go();
    display.drawRect(int(b.x - 2), int(b.y - 1), 5, 3, WHITE);
    b.mspeed += 0.07;
    display.setCursor(44, 1);
    display.setTextSize(1);
    display.print("Level " + String((int(e.mspeed - e.initialspeed) / levelspeedchange)));
    display.display();
  }


  
  else if (reading) { // READER is active
    int Jy = analogRead(A1) - 512;
    if (Jy < -minJ) {
      BOOKpos -= scrollSpeed;
      if (BOOKpos < 0) {
        BOOKpos = 0;
      }
      printBOOK(BOOKpos);
    }
    if (Jy > minJ) {
      if (BOOKpos + scrollSpeed <= sizeof(BOOK)) {
        BOOKpos += scrollSpeed;
        printBOOK(BOOKpos);

      }
    }
  }


  
  else if (landing) { // LANDER is active
    display.clearDisplay();
    //display.display();
    linput = analogRead(A0) - 512;
    thrustInput = analogRead(A1) - 512;
    if (!lander.landed) {
      lander.posupdate();
    }
    if (lander.landed) {
      lander.drawlegs();
      if (thrustInput > 300) {
        lander.launch();
      }
    }
    lander.draw();
    plat.draw();
    if (lander.platcollide(plat) && !lander.landed) {
      ton(G3, XS);
      if (abs(lander.hspeed) > hcollisionSpeed || abs(lander.vspeed) > vcollisionSpeed) {
        ton(F3, XS);
        for (int dist = 3; dist < 80; dist += 4) {
          for (int j = 0; j < 3; j++) {
            float xx = lander.x + random(dist * 2) - dist;
            float yy = lander.y + random(dist) - dist / 2;
            display.drawLine(xx, yy, xx + random(8) - 4, yy + random(4) - 2, WHITE);
            //display.drawLine(lander.x+random(dist)-dist/2,lander.y+random(dist)-dist/2,lander.x+random(dist)-dist/2,lander.y+random(dist)-dist/2, WHITE);
            display.display();
            //delay(10);
          }
        }
        runningtext("CRASHED!", 8, 150);
        lander.reset();
        plat.reset();
        while (lander.platcollide(plat)) {
          plat.reset();
        }
      } else {
        if (lander.hspeed > 0 && lander.x - lander.w + 2 > plat.x - (plat.w / 2) && lander.x + lander.w - 2 < plat.x + (plat.w / 2 )) {
          lander.land();
        } else {
          if (lander.y < plat.y) {
            lander.hspeed = -.3;  //comes from above
          }
          if (lander.y - 2 > plat.y + plat.h) {
            lander.hspeed = .2;  //comes from below
          }
          if (lander.x + lander.w - 3 < plat.x - plat.w / 2) {
            lander.vspeed = -.5;
          }
          else if (lander.x - lander.w + 3 > plat.x + plat.w / 2) {
            lander.vspeed = .5;
          }
        }
      }
    }
    display.display();
  }



  
  else {  // no app is active, thus menu is active
    int Jx = analogRead(A0) - 512;
    if (Jx < -400) {
      if (menuChoice == 0) {
        menuSwitch(2, true);
      } else {
        menuSwitch(menuChoice - 1, true);
      }
    } else if (Jx > 400) {
      if (menuChoice == 2) {
        menuSwitch(0, true);
      } else
        menuSwitch(menuChoice + 1, true);
    }
  }
}



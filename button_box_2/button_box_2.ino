/*********************************
Yes I Can Box. 
Hit a button, change the pattern. Has a resting state. 
Creation && Computation - Portables
Afaq / Nadine / Theo
*************************************/

/* ======================= 	INCLUDES ================================= */

#include <Adafruit_NeoPixel.h>
#include <elapsedMillis.h>
#include <ESP.h>
#include <ESP8266WiFi.h> 

/* ======================= NEOPATTERNS ================================= */

// Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }
 
    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }
 
    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
}; 

void StickComplete();

/* ======================= NEOPATTERNS DECLARE ================================= */

NeoPatterns Stick(8, 2, NEO_GRB + NEO_KHZ800, &StickComplete);

/* ======================= VARIABLES ================================= */

// button pin
int BUTTON_PIN = 12;
// button state
bool oldState = HIGH;
// what show?
int showType = 0;

// timer for active state
elapsedMillis timerActive;
#define intervalTimer 10000 // 10-ish seconds.
bool timerActiveFired;

// set a resting speed
int restSpeed = 3000;
// initial state of the show
bool showActive = false;

/* ======================= PROGRAM ================================= */

void setup() {
  // turn off the wifi chip because we ain't using it.
  WiFi.mode(WIFI_OFF);                   
  // serial begin
  Serial.begin(115200);
  // start the stick and set off a pattern
  Stick.begin();
  Stick.Fade(Stick.Color(0,100,0), Stick.Color(0,0,0), restSpeed, 1); // green pulse
  // set the pin mode for button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // we done, yo    
  Serial.println("Setup done");         
  delay(100);

}

void loop() {
  // update the neopixel strip
  Stick.Update();

  // if the timer is over, reset everything to false, and call the resting state of the show. 
  if((timerActiveFired) && (timerActive>intervalTimer)){
    showActive = false;
    timerActiveFired = false;
    Serial.println("not active");
    startShow(6);
  }

  // read the state of the pin on ech pass
  bool newState = digitalRead(BUTTON_PIN);

  // Check if state changed from high to low (button press).
  if (newState == LOW && oldState == HIGH) {
    // wee lazy debounce of a delay rather than a millis (its small, its fine)
    delay(50);
    newState = digitalRead(BUTTON_PIN);
    if (newState == LOW) {
        // turn on all the active states and call a random show type. 
        timerActiveFired = true;
        timerActive = 0;
    	  showActive = true;
        showType = random(0, 5);
        startShow(showType);
    }
  }
  // Set the last button state to the old state.
  oldState = newState;
}

/* ======================= FUNCTIONS ================================= */

void startShow(int i){

  // if the show ain't active and you've rolled a 6, do the breathe, else do the show. 
  
  Serial.println("show is:");
  Serial.print(i);
  Serial.println("showActive: ");
  Serial.print(showActive);
  Serial.println("  ");

  if(!showActive){
    if(i == 6){
      Stick.Fade(Stick.Color(0,100,0), Stick.Color(0,0,0), restSpeed, 1); // green pulse
    }
  }else{
    if(i == 0){
      Stick.Fade(Stick.Wheel(random(10,255)), Stick.Wheel(random(10,255)), 1000, 1);
    }else if(i == 1){
      Stick.ColorWipe(Stick.Color(200,0,0),10);
      Stick.Scanner(Stick.Color(200,0,0), 30);
    }else if(i == 2){
      Stick.TheaterChase(Stick.Color(255,255,0), Stick.Color(0,0,200), 100);
    }else if(i == 3){
      int speedThis = random(3, 10);
      Stick.RainbowCycle(speedThis);
    }else if(i == 4){
        Stick.ColorWipe(Stick.Color(30,100,30), 50);
    } 
  }
}

/* ======================= CALLBACKS - NEOPATTERNS ================================= */

void StickComplete(){
  // these run after the neopixel cycle. Some just reverse, some choose a random colour to run again. 
	if(showActive){
		if(showType == 0 || showType == 3 || showType == 6){
			Stick.Reverse();
	 	} else if(showType == 2 || showType == 1 || showType == 4){
	 		Stick.Color1 = Stick.Wheel(random(10,255));
	 	}
	}else{
    Stick.Reverse();
  }
}

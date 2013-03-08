int redLed = 11;
int greenLed = 10;
int blueLed = 9;

struct rgbColor {
  float r;
  float g;
  float b;
};

struct hslColor {
  float h;
  float s;
  float l;
};

unsigned int duration = 2000;
char buf[1024];

struct rgbColor hslToRgb(struct hslColor color) {
  float chroma = (1 - fabs(2 * color.l - 1)) * color.s;

  float hue6 = color.h * 6;

  float companionBrightness = chroma * (1 - fabs(fmod(hue6, 2) - 1));
  float red = 0, green = 0, blue = 0;
  switch (int(hue6) % 6) {
    case 0:
      red = chroma;
      green = companionBrightness;
      break;
    case 1:
      red = companionBrightness;
      green = chroma;
      break;
    case 2:
      green = chroma;
      blue = companionBrightness;
      break;
    case 3:
      green = companionBrightness;
      blue = chroma;
      break;
    case 4:
      blue = chroma;
      red = companionBrightness;
      break;
    case 5:
      blue = companionBrightness;
      red = chroma;
      break;
  }
  
  float bias = color.l - chroma / 2;
  
  rgbColor result = { red + bias, green + bias, blue + bias };
  return result;
}

void setup() {
  Serial.begin(9600);
  pinMode(redLed, OUTPUT);     
  pinMode(greenLed, OUTPUT);     
  pinMode(blueLed, OUTPUT);     
}

char building = 0;
char buildStatus = 0;

void loop() {
  int state = -1;
  int avail = Serial.available();
  while (avail > 0) {
    state = Serial.read();
    --avail;
  }

  if (state >= 0) {
    state = state - '0';
    building = state > 2;
    buildStatus = state % 3;
  }
  
  float hue;
  switch (buildStatus) {
    case 0:
      hue = 1.0 / 3.0;
      break;
    case 1:
      hue = 0.5 / 6.0;
      break;
    case 2:
      hue = 0;
      break;
  }
  
  float lightness;
  if (building) {
    lightness = fabs(float(millis() % duration) / (duration - 1) - 0.5);
  } else {
    lightness = 0.5;
  }
  
  hslColor color = { hue, 1, lightness };
  rgbColor outColor = hslToRgb(color);
  
  analogWrite(redLed, int(outColor.r * 255));
  analogWrite(greenLed, int(outColor.g * 255));
  analogWrite(blueLed, int(outColor.b * 255));
}

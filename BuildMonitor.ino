void vSerialFormat(const char *fmt, va_list argp) {
  static char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, argp);
  Serial.write(buf);
}

void SerialFormat(const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vSerialFormat(fmt, argp);
  va_end(argp);
}

int redLed = 3;
int greenLed = 5;
int blueLed = 6;

template <typename T>
class RgbColor {
  public:
    RgbColor(): r(0), g(0), b(0) { }
    RgbColor(T r, T g, T b): r(r), g(g), b(b) { }

    T r;
    T g;
    T b;
    
    RgbColor<T> interpolateTo(const RgbColor<T> &target, float progress) {
      RgbColor<T> result;
      result.r = this->r + progress * (target.r - this->r);
      result.g = this->g + progress * (target.g - this->g);
      result.b = this->b + progress * (target.b - this->b);
      return result;
    }
};

template <typename T>
RgbColor<T> operator - (const RgbColor<T> &lhs, const RgbColor<T> &rhs) {
  return RgbColor<T>(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b);
}

template <typename T>
RgbColor<T> operator + (const RgbColor<T> &lhs, const RgbColor<T> &rhs) {
  return RgbColor<T>(rhs.r + lhs.r, rhs.g + lhs.g, rhs.b + lhs.b);
}

template <typename T>
RgbColor<T> operator * (const RgbColor<T> &lhs, float rhs) {
  return RgbColor<T>(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
}

template <typename T>
RgbColor<T> operator * (float lhs, const RgbColor<T> &rhs) {
  return rhs * lhs;
}

void DumpColor(const RgbColor<float> &c) {
  SerialFormat("RGB(%d, %d, %d)\n", int(c.r * 255), int(c.g * 255), int(c.b * 255));
}

RgbColor<float> goodBuild(0, 1, 0);
RgbColor<float> dangerBuild(0.9, 0.4, 0);
RgbColor<float> badBuild(1, 0, 0);
RgbColor<float> unknownBuild(0.2, 0.2, 1);

void writeColor(const RgbColor<float> &c) {
  analogWrite(redLed, int(c.r * 255));
  analogWrite(greenLed, int(c.g * 255));
  analogWrite(blueLed, int(c.b * 255));
}

enum BuildStatus {
  good,
  danger,
  bad,
  unknown
};

struct BuildState {
  BuildStatus status;
  boolean building;
};

RgbColor<float>& colorFor(BuildStatus buildStatus) {
  switch(buildStatus) {
    case good:
      return goodBuild;
    case danger:
      return dangerBuild;
    case bad:
      return badBuild;
    case unknown:
      return unknownBuild;
  }
}

template <typename T>
class Animation {
  public:
    virtual T update(unsigned long time) = 0;
    virtual boolean complete(unsigned long time) = 0;
};

template <typename T>
class LinearAnimation : public Animation<T> {
  public:
    LinearAnimation(T startValue, unsigned long startTime, T targetValue, unsigned long duration): 
      startValue(startValue),
      startTime(startTime),
      targetValue(targetValue),
      targetTime(startTime + duration),
      duration(duration) { }
      
    virtual T update(unsigned long time) {
      if (time >= targetTime) {
        return targetValue;
      } else {
        float progress = float(time - startTime) / duration;
        return startValue + progress * (targetValue - startValue);
      }
    }
    
    virtual boolean complete(unsigned long time) {
      return time >= targetTime;
    }
    
  private:
    T startValue;
    unsigned long startTime;
    T targetValue;
    unsigned long targetTime;
    unsigned long duration;
};

template <typename T>
class PingPongAnimation : public Animation<T> {
  public:
    PingPongAnimation(T startValue, unsigned long startTime, T otherValue, unsigned long cycleTime) :
      startValue(startValue),
      startTime(startTime),
      otherValue(otherValue),
      cycleTime(cycleTime) { }
  
    virtual T update(unsigned long time) {
      float progress = fmod(float(time - startTime) / cycleTime, 1.0f);
      float value = 1.0 - (cos(progress * 2 * M_PI) + 1.0) / 2.0;
      return startValue + value * (otherValue - startValue);
    }
    
    virtual boolean complete(unsigned long time) {
      return false;
    }
  private:
    T startValue;
    unsigned long startTime;
    T otherValue;
    unsigned long cycleTime;
};

template <typename T>
class ConstantAnimation : public Animation<T> {
  public:
    ConstantAnimation(T value): value(value) { }

    virtual T update(unsigned long time) {
      return value;
    }

    virtual boolean complete(unsigned long time) {
      return true;
    }

  private:
    T value;
};

BuildState buildState;

Animation< RgbColor<float> > *currentColorAnimation = new ConstantAnimation< RgbColor<float> >(goodBuild);
boolean currentColorAnimationIsStable = true;

Animation<float> *currentLightnessAnimation = new ConstantAnimation<float>(1.0);
boolean currentLightnessAnimationIsStable = true;

void setup() {
  Serial.begin(9600);
  pinMode(redLed, OUTPUT);     
  pinMode(greenLed, OUTPUT);     
  pinMode(blueLed, OUTPUT);     
}

void loop() {
  unsigned long now = millis();
  
  if (!currentColorAnimationIsStable && currentColorAnimation->complete(now)) {
    SerialFormat("Switching to constant color animation\n");
    delete currentColorAnimation;
    currentColorAnimation = new ConstantAnimation< RgbColor<float> >(colorFor(buildState.status));
    currentColorAnimationIsStable = true;
  }
  
  if (!currentLightnessAnimationIsStable && currentLightnessAnimation->complete(now)) {
    SerialFormat("Switching to constant lightness animation\n");
    delete currentLightnessAnimation;
    currentLightnessAnimation = new ConstantAnimation<float>(1);
    currentLightnessAnimationIsStable = true;
  }
  
  RgbColor<float> currentColor = currentColorAnimation->update(now);
  float currentLightness = currentLightnessAnimation->update(now);
  
  int state = -1;
  int avail = Serial.available();
  while (avail > 0) {
    state = Serial.read();
    --avail;
  }

  if (state >= 0) {
    state = state - '0';
    boolean newBuilding = state > 3;
    BuildStatus newBuildStatus = BuildStatus(state % 4);
    
    if (buildState.status != newBuildStatus) {
      delete currentColorAnimation;
      RgbColor<float> targetColor = colorFor(newBuildStatus);
      SerialFormat("Switching to linear color animation\n");
      currentColorAnimation = new LinearAnimation< RgbColor<float> >(currentColor, now, targetColor, 1000);
      currentColorAnimationIsStable = false;
      buildState.status = newBuildStatus;
    }
    
    if (!buildState.building && newBuilding) {
      SerialFormat("Switching to ping-pong lightness animation\n");
      delete currentLightnessAnimation;
      currentLightnessAnimation = new PingPongAnimation<float>(1, now, 0.05, 1000);  //TODO: consider current lightness
      currentLightnessAnimationIsStable = false;
      buildState.building = true;
    } else if (buildState.building && !newBuilding) {
      SerialFormat("Switching to linear lightness animation\n");
      delete currentLightnessAnimation;
      currentLightnessAnimation = new LinearAnimation<float>(currentLightness, now, 1, 1000);
      currentLightnessAnimationIsStable = false;
      buildState.building = false;
    }
  }
  
  writeColor(currentColor * currentLightness);
}

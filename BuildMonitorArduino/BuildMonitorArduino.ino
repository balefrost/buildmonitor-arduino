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

int redLed = 6;
int greenLed = 5;
int blueLed = 3;

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
RgbColor<float> dangerBuild(0.8, 0.5, 0);
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
};

template <typename T>
class LinearAnimation : public Animation<T> {
  public:
    virtual T update(unsigned long time) {
      if (time >= targetTime) {
        return targetValue;
      } else {
        float progress = float(time - startTime) / duration;
        return startValue + progress * (targetValue - startValue);
      }
    }
    
    boolean complete(unsigned long time) {
      return time >= targetTime;
    }
    
    void reinitialize(unsigned long startTime, unsigned long duration, T startValue, T targetValue) {
      this->startTime = startTime;
      this->duration = duration;
      this->startValue = startValue;
      this->targetValue = targetValue;
      this->targetTime = startTime + duration;
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
    PingPongAnimation() : startTime(0) { }

    virtual T update(unsigned long time) {
      float progress = fmod(float(time - startTime) / cycleTime, 1.0f);
      float value = 1.0 - (cos(progress * 2 * M_PI) + 1.0) / 2.0;
      return startValue + value * (otherValue - startValue);
    }
    
    void reinitializeWithTime(unsigned long startTime, unsigned long cycleTime, T startValue, T otherValue, unsigned long now) {
      float progress = fmod(float(now - this->startTime) / this->cycleTime, 1.0f);
      unsigned long timeOffset = progress * cycleTime;
      this->startTime = startTime - timeOffset;
      this->cycleTime = cycleTime;
      this->startValue = startValue;
      this->otherValue = otherValue;
    }

    void reinitializeWithValue(unsigned long startTime, unsigned long cycleTime, T startValue, T otherValue, T currentValue) {
      float value = (currentValue - this->startValue) / (this->otherValue - this->startValue);
      float progress = acos((1 - value) * 2 - 1) / (2 * M_PI);
      unsigned long timeOffset = progress * cycleTime;
      this->startTime = startTime - timeOffset;
      this->cycleTime = cycleTime;
      this->startValue = startValue;
      this->otherValue = otherValue;
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
    virtual T update(unsigned long time) {
      return value;
    }

    void reinitialize(T value) {
      this->value = value;
    }

  private:
    T value;
};

BuildState buildState = { unknown, true };

const unsigned long lightnessSearchPingPongCycleTime = 3000;
const unsigned long lightnessBuildPingPongCycleTime = 1000;
const unsigned long lightnessTransitionDuration = 500;
const unsigned long colorTransitionDuration = 500;

const unsigned long idleTimeout = 60 * 1000;

ConstantAnimation<float> lightnessConstant;
LinearAnimation<float> lightnessTransition;
PingPongAnimation<float> lightnessPingPong;

ConstantAnimation< RgbColor<float> > colorConstant;
LinearAnimation< RgbColor<float> > colorTransition;

Animation< RgbColor<float> > *currentColorAnimation;
Animation<float> *currentLightnessAnimation;

unsigned long timeOfLastInput;

void setup() {
  Serial.begin(9600);
  pinMode(redLed, OUTPUT);     
  pinMode(greenLed, OUTPUT);     
  pinMode(blueLed, OUTPUT);

  unsigned long now = millis();

  lightnessConstant.reinitialize(1);
  lightnessPingPong.reinitializeWithTime(now, lightnessSearchPingPongCycleTime, 1, 0.05, now);
  colorConstant.reinitialize(unknownBuild);

  currentColorAnimation = &colorConstant;
  currentLightnessAnimation = &lightnessPingPong;

  timeOfLastInput = now;
}

void loop() {
  unsigned long now = millis();
  
  RgbColor<float> currentColor = currentColorAnimation->update(now);
  float currentLightness = currentLightnessAnimation->update(now);
  
  int state = -1;
  int avail = Serial.available();
  while (avail > 0) {
    state = Serial.read();
    --avail;
  }

  if (state >= 0) {
    timeOfLastInput = now;

    state = state - '0';
    boolean newBuilding = state > 3;
    BuildStatus newBuildStatus = BuildStatus(state % 4);
    
    if (buildState.status != newBuildStatus) {
      RgbColor<float> targetColor = colorFor(newBuildStatus);
      colorTransition.reinitialize(now, colorTransitionDuration, currentColor, targetColor);
      currentColorAnimation = &colorTransition;
      buildState.status = newBuildStatus;
    }
    
    if (!newBuilding) {
      lightnessTransition.reinitialize(now, lightnessTransitionDuration, currentLightness, 1);
      currentLightnessAnimation = &lightnessTransition;
      buildState.building = false;
    } else {
      unsigned long cycleTime = newBuildStatus == unknown ? lightnessSearchPingPongCycleTime : lightnessBuildPingPongCycleTime;

      if (newBuilding && !buildState.building) {
        lightnessPingPong.reinitializeWithValue(now, cycleTime, 1, 0.05, currentLightness);
      } else {
        lightnessPingPong.reinitializeWithTime(now, cycleTime, 1, 0.05, now);
      }

      currentLightnessAnimation = &lightnessPingPong;
      buildState.building = true;
    }
  } else if (now - timeOfLastInput > idleTimeout && !(buildState.status == unknown && buildState.building)) {
    colorTransition.reinitialize(now, colorTransitionDuration, currentColor, unknownBuild);
    currentColorAnimation = &colorTransition;
    buildState.status = unknown;

    if (buildState.building) {
      lightnessPingPong.reinitializeWithTime(now, lightnessSearchPingPongCycleTime, 1, 0.05, now);
    } else {
      lightnessPingPong.reinitializeWithValue(now, lightnessSearchPingPongCycleTime, 1, 0.05, currentLightness);
    }

    currentLightnessAnimation = &lightnessPingPong;
    buildState.building = true;
  }
  
  writeColor(currentColor * currentLightness);
}

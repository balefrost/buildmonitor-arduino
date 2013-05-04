#ifndef RGBCOLOR_H_
#define RGBCOLOR_H_

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

typedef RgbColor<uint8_t> rgb8;

#endif /* RGBCOLOR_H_ */
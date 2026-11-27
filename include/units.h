/// @file
/// @brief 角度、角速度、電流、トルクを単位付きで扱う型。
#pragma once

#include <numbers>

namespace nkrc::nbed {

/// @brief 円周率の単精度浮動小数点値。
inline constexpr float kPi = std::numbers::pi_v<float>;

/// @brief ラジアン毎秒で表す角速度。
using RadPS = float;
/// @brief 回転毎分で表す回転速度。
using Rpm = float;

/// @brief 度で表す角度。
using Degree = float;
/// @brief ラジアンで表す角度。
using Rad = float;

/// @brief メートル毎秒で表す速度。
using Mps = float;

/// @brief 角速度を扱う。回転毎分（rpm）とラジアン毎秒（rad/s）を相互変換できる。
class Velocity {
 public:
  /// @brief ラジアン毎秒で値を指定して作る。
  /// @param rad_ps ラジアン毎秒で表した値。
  /// @return 指定した値を持つ Velocity。
  static constexpr Velocity FromRadPS(RadPS rad_ps) {
    Velocity velocity;
    velocity.SetAsRadPS(rad_ps);
    return velocity;
  }

  /// @brief 回転毎分（rpm）で値を指定して作る。
  /// @param rpm 回転毎分（rpm）で表した値。
  /// @return 指定した値を持つ Velocity。
  static constexpr Velocity FromRpm(Rpm rpm) {
    Velocity velocity;
    velocity.SetAsRpm(rpm);
    return velocity;
  }

  /// @brief ラジアン毎秒で値を設定する。
  /// @param rad_ps ラジアン毎秒で表した値。
  constexpr void SetAsRadPS(RadPS rad_ps) { rad_ps_ = rad_ps; }

  /// @brief 回転毎分（rpm）で値を設定する。
  /// @param rpm 回転毎分（rpm）で表した値。
  constexpr void SetAsRpm(Rpm rpm) { rad_ps_ = rpm * (2.0f * kPi) / 60.0f; }

  /// @brief ラジアン毎秒に換算して読み取る。
  /// @return ラジアン毎秒で表した値。
  [[nodiscard]] constexpr RadPS GetAsRadPS() const { return rad_ps_; }

  /// @brief 回転毎分（rpm）に換算して読み取る。
  /// @return 回転毎分（rpm）で表した値。
  [[nodiscard]] constexpr Rpm GetAsRpm() const {
    return rad_ps_ * 60.0f / (2.0f * kPi);
  }

 private:
  RadPS rad_ps_ = 0.0f;  // 角速度 [rad/s]
};

/// @brief 電流を扱う。アンペア（A）とミリアンペア（mA）を相互変換できる。
class Current {
 public:
  /// @brief アンペア（A）で値を指定して作る。
  /// @param amp アンペア（A）で表した値。
  /// @return 指定した値を持つ Current。
  static constexpr Current FromAmpere(float amp) {
    Current current;
    current.SetAsAmpere(amp);
    return current;
  }

  /// @brief ミリアンペア（mA）で値を指定して作る。
  /// @param milli_amps ミリアンペア（mA）で表した値。
  /// @return 指定した値を持つ Current。
  static constexpr Current FromMilliAmpere(float milli_amps) {
    Current current;
    current.SetAsMilliAmpere(milli_amps);
    return current;
  }

  /// @brief アンペア（A）で値を設定する。
  /// @param amp アンペア（A）で表した値。
  constexpr void SetAsAmpere(float amp) { ampere_ = amp; }

  /// @brief ミリアンペア（mA）で値を設定する。
  /// @param milli_amps ミリアンペア（mA）で表した値。
  constexpr void SetAsMilliAmpere(float milli_amps) {
    ampere_ = milli_amps / 1000.0f;
  }

  /// @brief アンペア（A）に換算して読み取る。
  /// @return アンペア（A）で表した値。
  [[nodiscard]] constexpr float GetAsAmpere() const { return ampere_; }

  /// @brief ミリアンペア（mA）に換算して読み取る。
  /// @return ミリアンペア（mA）で表した値。
  [[nodiscard]] constexpr float GetAsMilliAmpere() const {
    return ampere_ * 1000.0f;
  }

 private:
  float ampere_ = 0.0f;  // 電流 [A]
};

/// @brief トルクを扱う。ニュートンメートル（N·m）とミリニュートンメートル（mN·m）を相互変換できる。
class Torque {
 public:
  /// @brief ニュートンメートル（N·m）で値を設定する。
  /// @param nm ニュートンメートル（N·m）で表した値。
  constexpr void SetAsNm(float nm) { nm_ = nm; }

  /// @brief ミリニュートンメートル（mN·m）で値を設定する。
  /// @param milli_nm ミリニュートンメートル（mN·m）で表した値。
  constexpr void SetAsMNm(float milli_nm) { nm_ = milli_nm / 1000.0f; }

  /// @brief ニュートンメートル（N·m）に換算して読み取る。
  /// @return ニュートンメートル（N·m）で表した値。
  [[nodiscard]] constexpr float GetAsNm() const { return nm_; }

  /// @brief ミリニュートンメートル（mN·m）に換算して読み取る。
  /// @return ミリニュートンメートル（mN·m）で表した値。
  [[nodiscard]] constexpr float GetAsMNm() const { return nm_ * 1000.0f; }

 private:
  float nm_ = 0.0f;  // トルク [N・m]
};

/// @brief 角度を扱う。度（°）とラジアン（rad）を相互変換できる。
class Angle {
 public:
  /// @brief ラジアン（rad）で値を指定して作る。
  /// @param radian ラジアン（rad）で表した値。
  /// @return 指定した値を持つ Angle。
  static constexpr Angle FromRadian(Rad radian) {
    Angle angle;
    angle.SetAsRadian(radian);
    return angle;
  }

  /// @brief 度（°）で値を指定して作る。
  /// @param degree 度（°）で表した値。
  /// @return 指定した値を持つ Angle。
  static constexpr Angle FromDegree(Degree degree) {
    Angle angle;
    angle.SetAsDegree(degree);
    return angle;
  }

  /// @brief ラジアン（rad）で値を設定する。
  /// @param radian ラジアン（rad）で表した値。
  constexpr void SetAsRadian(Rad radian) { radian_ = radian; }

  /// @brief 度（°）で値を設定する。
  /// @param degree 度（°）で表した値。
  constexpr void SetAsDegree(Degree degree) {
    radian_ = degree * (kPi / 180.0f);
  }

  /// @brief ラジアン（rad）に換算して読み取る。
  /// @return ラジアン（rad）で表した値。
  [[nodiscard]] constexpr Rad GetAsRadian() const { return radian_; }

  /// @brief 度（°）に換算して読み取る。
  /// @return 度（°）で表した値。
  [[nodiscard]] constexpr Degree GetAsDegree() const {
    return radian_ * (180.0f / kPi);
  }

 private:
  Rad radian_ = 0.0f;  // 角度 [rad]
};

namespace literals {

/// @brief 回転毎分の数値リテラルから Velocity を作る。
/// @param value 回転毎分で表した値。
/// @return 指定した値を持つ Velocity。
constexpr Velocity operator""_rpm(long double value) {
  return Velocity::FromRpm(static_cast<Rpm>(value));
}

/// @brief ラジアン毎秒の数値リテラルから Velocity を作る。
/// @param value ラジアン毎秒で表した値。
/// @return 指定した値を持つ Velocity。
constexpr Velocity operator""_rad_ps(long double value) {
  return Velocity::FromRadPS(static_cast<RadPS>(value));
}

/// @brief アンペアの数値リテラルから Current を作る。
/// @param value アンペアで表した値。
/// @return 指定した値を持つ Current。
constexpr Current operator""_A(long double value) {
  return Current::FromAmpere(static_cast<float>(value));
}

/// @brief ミリアンペアの数値リテラルから Current を作る。
/// @param value ミリアンペアで表した値。
/// @return 指定した値を持つ Current。
constexpr Current operator""_mA(long double value) {
  return Current::FromMilliAmpere(static_cast<float>(value));
}

/// @brief 度の数値リテラルから Angle を作る。
/// @param value 度で表した値。
/// @return 指定した値を持つ Angle。
constexpr Angle operator""_deg(long double value) {
  return Angle::FromDegree(static_cast<Degree>(value));
}

/// @brief ラジアンの数値リテラルから Angle を作る。
/// @param value ラジアンで表した値。
/// @return 指定した値を持つ Angle。
constexpr Angle operator""_rad(long double value) {
  return Angle::FromRadian(static_cast<Rad>(value));
}

}  // namespace literals

}  // namespace nkrc::nbed

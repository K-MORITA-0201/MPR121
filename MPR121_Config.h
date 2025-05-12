/**
 * @file MPR121
 * @brief 静電センサー制御
 * @details 静電センサー基板の詳細設定を決めて、制御管理を行うシステム
 * @date 2025/5/7
 * @author 株式会社SIVAX 先進技術開発室　森田
 *
 * @section 製作環境
 * - Arduino IDE ver 2.3.6
 *
 * @section I2Cアドレスの変更方法
 * - 基盤上のADDRポートを以下のポートとショートさせることでハード的に変更する
 *    ADDR - GND -> 0x5A（デフォルト）
 *    ADDR - VDD -> 0x5B
 *    ADDR - SDA -> 0x5C
 *    ADDR - SCL -> 0X5D
 *
 * @section パラメータ調整の影響
 * - alpha（平滑化係数）
 *    値が1.0に近づくほど新しい値に敏感になり、0.0に近づくほど過去の値に引っ張られる。
 *    値を大きくすると反応は速くなるがノイズの影響も受けやすくなる。
 *    値を小さくするとノイズに強くなるが反応が鈍くなる。
 *
 * - minValue（センサー値の下限値）
 *    キャリブレーションや環境変化に対応するために定める。
 *    値を適切に維持しないと誤検出の原因となる。
 *
 * - maxValue（センサー値の上限値）
 *    感度やノイズの許容範囲を見極める基準として定める。
 *
 * - touchMargin[]（タッチ閾値調整量）
 *    タッチを検出する際のしきい値の調整量。
 *    値を大きくすると確実なタッチのみを検出するようになるが、軽いタッチでは反応しにくくなる。
 *    値を小さくすると軽いタッチでも反応するが、ノイズによる誤検出が増える可能性がある。
 *
 * - releaseMargin（リリース閾値調整量）
 *    リリース（タッチ解除）を検出する際のしきい値の調整量。
 *    値を小さくするとすぐにリリース判定が出やすくなり、反応性は上がるが不安定になりやすい。
 *    値を大きくすると安定するが、タッチが離された後もしばらく押されているように誤判定されることがある。
 *
 * - touchJuge（タッチ判定の検知回数）
 *    タッチとして確定するまでに連続して何回検出されたかのしきい値。
 *    値を大きくするとタッチ判定が安定するが、反応は遅くなる。
 *    値を小さくすると即応性が上がるが、誤検出が起こりやすくなる。
 *
 * - releaseJuge（リリース判定の検知回数）
 *    リリースとして確定するまでに連続して何回検出されたかのしきい値。
 *    値を大きくするとリリース判定が安定するが、離したと認識されるまで時間がかかる。
 *    値を小さくすると即応性が上がるが、誤ってリリース判定されやすくなる。
 *
 * @section メモ
 * - 電源立ち上げ時に静電センサーの対象物には触れないこと
 * - 誤検出を防止する為に、タッチ調整量（touchMargin）はリリース調整量（releaseMargin）よりも大きくすること
 * - 判定に関わるパラメータはマイコンの処理速度を考慮して調整すること
 */

// インクルードガード
#ifndef MPR121_CONFIG_H
#define MPR121_CONFIG_H

#include <Arduino.h>          // Arduinoライブラリ
#include <Wire.h>             // I2Cライブラリ
#include <Adafruit_MPR121.h>  // 静電モジュールライブラリ
#include <vector>

using namespace std;  // 名前空間を指定

//*****************************************************************************************************************************
// 静電センサー管理クラス
class MPR121Manager {
  // 外部からのアクセスを許可
public:
  MPR121Manager(uint8_t setAddress = 0x5A, uint16_t usedPortMask = 0xFFFF);                 // コンストラクタ
  void update();                                                                            // 状態を更新
  void printStatus(uint32_t interval, const vector<String>& portLabel = vector<String>());  // 状態の表示
  bool isTouched(uint8_t port);                                                             // 特定ピンがタッチ中か判定
  void setTouchMargin(uint8_t port, uint8_t margin);                                        // タッチ判定用マージンを設定
  void setReleaseMargin(uint8_t port, uint8_t margin);                                      // リリース判定用マージンを設定
  void setSensorMinValue(uint8_t port, uint16_t value);                                     // 指定ポートの下限値を設定
  void setSensorMaxValue(uint8_t port, uint16_t value);                                     // 指定ポートの上限値を設定
  void setTouchJugeCount(uint8_t port, uint8_t count);                                      // タッチ判定回数を設定
  void setReleaseJugeCount(uint8_t port, uint8_t count);                                    // リリース判定回数を設定

  // 自クラス内部のみアクセス許可
private:
  // センサー基板管理
  Adafruit_MPR121 cap;                // 制御インスタンス
  static const uint8_t maxPort = 12;  // 基板上の接続可能ポート数
  uint16_t activePort;                // 使用ポートのビットマスク
  uint8_t address;                    // I2Cアドレス

  // センサー数値管理
  float value[maxPort];        // 各ポートのセンサー値
  const float alpha = 0.6;     // 平滑化係数
  uint16_t minValue[maxPort];  // センサー値の下限値
  uint16_t maxValue[maxPort];  // センサー値の上限値

  // 判定管理
  uint16_t currentTouched = 0;      // タッチ状態をビットで格納
  uint8_t counter[maxPort];         // タッチ／リリース検知用カウンタ
  float threshold[maxPort];         // 閾値
  uint16_t touchMargin[maxPort];    // タッチ閾値調整量
  uint16_t releaseMargin[maxPort];  // リリース閾値調整量
  uint8_t touchJuge[maxPort];       // タッチ判定の検知回数
  uint8_t releaseJuge[maxPort];     // リリース判定の検知回数
};

#endif


#include "MPR121_Config.h"

#define MPR121_DEBUG_PRINT 1  // センサー状態表示の有無

uint16_t usedPortMask = 0b000000000110011;              // 使用したいポート番号をビットで選択(左から順番に指定)
vector<String> labels = { "Left", "Center", "Right" };  // ポートラベル配列

// インスタンスの作成
MPR121Manager mpr121(0x5A, usedPortMask);

//*****************************************************************************************************************************
// セットアップ
void setup() {
  delay(100);
  Serial.begin(115200);

  Serial.println("\n------ Setup Start ------\n");
  Wire.begin();  // I2C接続開始

  // mpr121.setTouchMargin(0, 50);  // タッチマージンを設定
  // mpr121.setSensorMinValue(0, 300);  // センサー値の下限値を設定
  // mpr121.setTouchJugeCount(0, 40);   // タッチ判定回数の設定
  Serial.println("\n------ Setup End ------\n");
}

//*****************************************************************************************************************************
// ループ処理
void loop() {
  mpr121.update();  // センサー状態を更新

  if (MPR121_DEBUG_PRINT) {
    mpr121.printStatus(50);  // 状態を表示(ラベルなし)
    // mpr121.printStatus(50, labels);  // 状態を表示(ラベルあり)
  } else {
    // 状態の確認
    for (uint8_t i = 0; i < 12; ++i) {
      if ((usedPortMask >> i) & 1) {
        Serial.print(mpr121.isTouched(i));  // 指定ポートの状態を取得
        Serial.print(" ");
      }
    }
    Serial.println();
  }
}

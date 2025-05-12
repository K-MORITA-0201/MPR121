
#include "MPR121_Config.h"

//*****************************************************************************************************************************
/**
 * @brief コンストラクタ
 * @param address 基板のI2Cアドレスを指定
 * @param usedPortMask 使用するポート番号を任意で指定
 */
//*****************************************************************************************************************************
MPR121Manager::MPR121Manager(uint8_t setAddress, uint16_t usedPortMask) {
  cap.begin(setAddress);

  // 自動キャリブレーションを有効にする
  cap.writeRegister(MPR121_AUTOCONFIG0, 0x0B);

  // 使用ポートマスクを保存（ビット単位）
  activePort = usedPortMask;

  // アドレスを保存
  address = setAddress;

  // 設定待機
  delay(100);

  // 各ポートの初期化
  for (uint8_t i = 0; i < maxPort; i++) {
    if ((activePort >> i) & 1) {
      // センサー値範囲の初期設定
      minValue[i] = 600;
      maxValue[i] = 710;

      // センサー値を取得
      value[i] = cap.filteredData(i);
      value[i] = constrain(value[i], minValue[i], maxValue[i]);

      // 判定変数の初期設定
      touchMargin[i] = 30;    // タッチマージン
      releaseMargin[i] = 20;  // リリースマージン
      touchJuge[i] = 15;      // タッチ判定の回数閾値
      releaseJuge[i] = 15;    // リリース判定の回数閾値
      counter[i] = 0;         // カウンターを初期化

      // 初回の閾値を設定
      threshold[i] = value[i] - touchMargin[i];
    }
  }
}

//*****************************************************************************************************************************
/**
 * @brief センサーの状態を更新する
 */
//*****************************************************************************************************************************
void MPR121Manager::update() {
  for (uint8_t i = 0; i < maxPort; ++i) {
    if ((activePort >> i) & 1) {

      // センサーの生値を取得して平滑化
      uint16_t raw = cap.filteredData(i);
      value[i] = alpha * raw + (1.0 - alpha) * value[i];
      value[i] = constrain(value[i], minValue[i], maxValue[i]);

      // 現在のタッチ状態（ビットで取得）
      bool touched = (currentTouched >> i) & 1;

      // 判定条件（状態に応じて比較方向を変える）
      bool conditionMet = touched
                            ? (value[i] > threshold[i])   // タッチ中：値がしきい値より上 → リリース
                            : (value[i] < threshold[i]);  // リリース中：値がしきい値より下 → タッチ

      // 判定結果に応じてカウンター処理
      if (conditionMet) {
        counter[i]++;
      } else {
        counter[i] = 0;
      }

      // カウンターが規定値に達したら状態を反転
      if ((!touched && counter[i] > touchJuge[i]) || (touched && counter[i] > releaseJuge[i])) {

        currentTouched ^= (1 << i);  // 状態を反転
        counter[i] = 0;

        // 状態に応じて次のしきい値を固定
        if (touched) {
          // リリース直後：タッチ状態に戻るための基準値を下げて設定
          threshold[i] = value[i] - touchMargin[i];
        } else {
          // タッチ直後：リリース判定の基準値を上げて設定
          threshold[i] = value[i] + releaseMargin[i];
        }
      }
    }
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定されたポートのタッチ状態を返す
 * @param port 対象のポート番号
 */
//*****************************************************************************************************************************
bool MPR121Manager::isTouched(uint8_t port) {
  if (port < maxPort && (activePort & (1 << port))) {
    bool touched = (currentTouched >> port) & 1;
    return touched;
  } else return false;
}

//*****************************************************************************************************************************
/**
 * @brief コンストラクタ
 * @param interval 表示間隔
 */
//*****************************************************************************************************************************
void MPR121Manager::printStatus(uint32_t interval, const String* portLabel, size_t N) {
  static uint32_t lastPrintTime = millis();
  uint32_t currentTime = millis();

  if (currentTime - lastPrintTime < interval) return;
  lastPrintTime = currentTime;

  Serial.print("Add: 0x");
  Serial.print(address, HEX);
  Serial.print(" ->");

  uint8_t labelIndex = 0;  // ラベル表示用のインデックス（使う場合のみ）

  for (uint8_t i = 0; i < maxPort; ++i) {
    if ((activePort >> i) & 1) {
      bool touched = (currentTouched >> i) & 1;

      Serial.print("  |  ");

      // ラベルがある場合 → labelIndex使用
      // ない場合 → i（実ポート番号）使用
      if (N > 0) {
        if (labelIndex < N) {
          Serial.print(portLabel[labelIndex]);
        }
        labelIndex++;  // アクティブな順の表示番号を進める
      } else {
        Serial.print("Port ");
        Serial.print(i);  // 実ポート番号で表示
      }

      Serial.print(": ");
      Serial.print(touched ? "Touch" : "Release");
      Serial.print("  Val: ");
      Serial.print(value[i], 2);
      Serial.print("  Thr: ");
      Serial.print(threshold[i], 2);
      Serial.print("  Raw: ");
      Serial.print(cap.filteredData(i));
    }
  }
  Serial.println();
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのタッチ判定マージンを設定する
 * @param port 対象のポート番号
 * @param margin 設定するマージン値
 */
//*****************************************************************************************************************************
void MPR121Manager::setTouchMargin(uint8_t port, uint8_t margin) {
  if (port < maxPort && (activePort & (1 << port))) {
    touchMargin[port] = margin;

    // 状態に応じて次のしきい値を固定
    bool touched = (currentTouched >> port) & 1;
    if (touched) {
      threshold[port] = value[port] + releaseMargin[port];
    } else {
      threshold[port] = value[port] - touchMargin[port];
    }
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのリリース判定マージンを設定する
 * @param port 対象のポート番号
 * @param margin 設定するマージン値
 */
//*****************************************************************************************************************************
void MPR121Manager::setReleaseMargin(uint8_t port, uint8_t margin) {
  if (port < maxPort && (activePort & (1 << port))) {
    releaseMargin[port] = margin;

    // 状態に応じて次のしきい値を固定
    bool touched = (currentTouched >> port) & 1;
    if (touched) {
      threshold[port] = value[port] + releaseMargin[port];
    } else {
      threshold[port] = value[port] - touchMargin[port];
    }
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのセンサー下限値を設定する
 * @param port 対象のポート番号
 * @param value 設定する下限値（500以上の値を推奨）
 */
//*****************************************************************************************************************************
void MPR121Manager::setSensorMinValue(uint8_t port, uint16_t value) {
  if (port < maxPort && (activePort & (1 << port))) {
    minValue[port] = value;
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのセンサー上限値を設定する
 * @param port 対象のポート番号
 * @param value 設定する上限値（720以下の値を推奨）
 */
//*****************************************************************************************************************************
void MPR121Manager::setSensorMaxValue(uint8_t port, uint16_t value) {
  if (port < maxPort && (activePort & (1 << port))) {
    maxValue[port] = value;
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのタッチ判定に必要な連続回数を設定する
 * @param port 対象のポート番号
 * @param count 判定に必要な回数（10以上の値を推奨）
 */
//*****************************************************************************************************************************
void MPR121Manager::setTouchJugeCount(uint8_t port, uint8_t count) {
  if (port < maxPort && (activePort & (1 << port))) {
    touchJuge[port] = count;
  }
}

//*****************************************************************************************************************************
/**
 * @brief 指定ポートのリリース判定に必要な連続回数を設定する
 * @param port 対象のポート番号
 * @param count 判定に必要な回数（10以上の値を推奨）
 */
//*****************************************************************************************************************************
void MPR121Manager::setReleaseJugeCount(uint8_t port, uint8_t count) {
  if (port < maxPort && (activePort & (1 << port))) {
    releaseJuge[port] = count;
  }
}

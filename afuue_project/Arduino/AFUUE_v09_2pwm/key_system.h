#pragma once

class KeySystem {
public:
  KeySystem();
  int Initialize();
  void UpdateKeys();
  void UpdateMenuKeys(bool isKeyRepeatEnabled);
  float GetNoteNumber(int baseNote) const;
  bool IsBendKeysDown() const;
  uint16_t GetKeyData() const;
  uint16_t GetKeyPush() const;

  bool IsKeyLowC_Down() const {
    return (keyLowC == LOW);
  }
  bool IsKeyEb_Down() const {
    return (keyEb == LOW);
  }
  bool IsKeyD_Down() const {
    return (keyD == LOW);
  }
  bool IsKeyE_Down() const {
    return (keyE == LOW);
  }
  bool IsKeyF_Down() const {
    return (keyF == LOW);
  }
  bool IsKeyLowCs_Down() const {
    return (keyLowCs == LOW);
  }
  bool IsKeyGs_Down() const {
    return (keyGs == LOW);
  }
  bool IsKeyG_Down() const {
    return (keyG == LOW);
  }
  bool IsKeyA_Down() const {
    return (keyA == LOW);
  }
  bool IsKeyB_Down() const {
    return (keyB == LOW);
  }
  bool IsKeyDown_Down() const {
    return (octDown == LOW);
  }
  bool IsKeyUp_Down() const {
    return (octUp == LOW);
  }
  bool IsFuncBtn_Down() const {
    return ((keyData & (1 << 12)) != 0);
  }

  bool IsKeyLowC_Push() const {
    return ((keyPush & (1 << 0)) != 0);
  }
  bool IsKeyEb_Push() const {
    return ((keyPush & (1 << 1)) != 0);
  }
  bool IsKeyD_Push() const {
    return ((keyPush & (1 << 2)) != 0);
  }
  bool IsKeyE_Push() const {
    return ((keyPush & (1 << 3)) != 0);
  }
  bool IsKeyF_Push() const {
    return ((keyPush & (1 << 4)) != 0);
  }
  bool IsKeyLowCs_Push() const {
    return ((keyPush & (1 << 5)) != 0);
  }
  bool IsKeyGs_Push() const {
    return ((keyPush & (1 << 6)) != 0);
  }
  bool IsKeyG_Push() const {
    return ((keyPush & (1 << 7)) != 0);
  }
  bool IsKeyA_Push() const {
    return ((keyPush & (1 << 8)) != 0);
  }
  bool IsKeyB_Push() const {
    return ((keyPush & (1 << 9)) != 0);
  }
  bool IsKeyDown_Push() const {
    return ((keyPush & (1 << 10)) != 0);
  }
  bool IsKeyUp_Push() const {
    return ((keyPush & (1 << 11)) != 0);
  }
  bool IsFuncBtn_Push() const {
    return ((keyPush & (1 << 12)) != 0);
  }

private:
  uint16_t keyData = 0;
  uint16_t keyPush = 0;
  uint16_t keyCurrent = 0;
  int32_t keyRepeatCount = 0;

  bool keyLowC = HIGH;
  bool keyEb = HIGH;
  bool keyD = HIGH;
  bool keyE = HIGH;
  bool keyF = HIGH;
  bool keyLowCs = HIGH;
  bool keyGs = HIGH;
  bool keyG = HIGH;
  bool keyA = HIGH;
  bool keyB = HIGH;
  bool octDown = HIGH;
  bool octUp = HIGH;
};

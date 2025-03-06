// ChatGPT に複数回指示をして修正を繰り返したコード

// Vector4 クラス
class Vector4 {
public:
    float x, y, z, w;

    Vector4(float x = 0, float y = 0, float z = 0, float w = 0)
        : x(x), y(y), z(z), w(w) {}

    Vector4 operator+(const Vector4& v) const {
        return Vector4(x + v.x, y + v.y, z + v.z, w);
    }

    Vector4 operator-(const Vector4& v) const {
        return Vector4(x - v.x, y - v.y, z - v.z, w);
    }

    Vector4 operator*(float scalar) const {
        return Vector4(x * scalar, y * scalar, z * scalar, w);
    }

    static float Dot(const Vector4& v0, const Vector4& v1) {
        return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
    }

    static Vector4 Cross(const Vector4& v0, const Vector4& v1) {
        return Vector4(v0.y * v1.z - v0.z * v1.y, v0.z * v1.x - v0.x * v1.z, v0.x * v1.y - v0.y * v1.x, 0.0f);
    }

    static bool IsClockwise(const Vector4& v0, const Vector4& v1, const Vector4& v2) {
      float cross = (v1.x - v0.x)*(v2.y - v0.y) - (v1.y - v0.y)*(v2.x - v0.x);
      return cross < 0.0f;
    }
};

// Quaternion クラス
class Quaternion {
public:
    float x, y, z, w;

    Quaternion(float x = 0, float y = 0, float z = 0, float w = 1)
        : x(x), y(y), z(z), w(w) {}

    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    static Quaternion Rotate(float angle, const Vector4& axis) {
        float halfAngle = angle / 2;
        float sinHalf = sin(halfAngle);
        return Quaternion(axis.x * sinHalf, axis.y * sinHalf, axis.z * sinHalf, cos(halfAngle));
    }

    Vector4 RotateVector(const Vector4& v) {
      Quaternion r = *this * Quaternion(v.x, v.y, v.z, 0.0f) * Conjugate();
      return Vector4(r.x, r.y, r.z, v.w);
    }


    Quaternion Conjugate() const {
        return Quaternion(-x, -y, -z, w);
    }
};

// Matrix44 クラス
class Matrix44 {
public:
    float m[4][4];

    Matrix44() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }

    Matrix44 operator*(const Matrix44& mat) const {
        Matrix44 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i][j] += m[i][k] * mat.m[k][j];
                }
            }
        }
        return result;
    }

    Vector4 operator*(const Vector4& v) const {
        return Vector4(
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        );
    }

    // Quaternion -> Matrix44
    void FromQuaternion(const Quaternion& q) {
        float xx = q.x * q.x;
        float yy = q.y * q.y;
        float zz = q.z * q.z;
        float xy = q.x * q.y;
        float xz = q.x * q.z;
        float yz = q.y * q.z;
        float wx = q.w * q.x;
        float wy = q.w * q.y;
        float wz = q.w * q.z;

        m[0][0] = 1.0f - 2.0f * (yy + zz);
        m[0][1] = 2.0f * (xy - wz);
        m[0][2] = 2.0f * (xz + wy);
        m[0][3] = 0.0f;

        m[1][0] = 2.0f * (xy + wz);
        m[1][1] = 1.0f - 2.0f * (xx + zz);
        m[1][2] = 2.0f * (yz - wx);
        m[1][3] = 0.0f;

        m[2][0] = 2.0f * (xz - wy);
        m[2][1] = 2.0f * (yz + wx);
        m[2][2] = 1.0f - 2.0f * (xx + yy);
        m[2][3] = 0.0f;

        m[3][0] = 0.0f;
        m[3][1] = 0.0f;
        m[3][2] = 0.0f;
        m[3][3] = 1.0f;
    }

    // Matrix44 -> Quaternion
    Quaternion ToQuaternion() const {
        Quaternion q;
        float trace = m[0][0] + m[1][1] + m[2][2];

        if (trace > 0.0f) {
            float s = 0.5f / sqrtf(trace + 1.0f);
            q.w = 0.25f / s;
            q.x = (m[2][1] - m[1][2]) * s;
            q.y = (m[0][2] - m[2][0]) * s;
            q.z = (m[1][0] - m[0][1]) * s;
        } else {
            if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
                float s = 2.0f * sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]);
                q.w = (m[2][1] - m[1][2]) / s;
                q.x = 0.25f * s;
                q.y = (m[0][1] + m[1][0]) / s;
                q.z = (m[0][2] + m[2][0]) / s;
            } else if (m[1][1] > m[2][2]) {
                float s = 2.0f * sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]);
                q.w = (m[0][2] - m[2][0]) / s;
                q.x = (m[0][1] + m[1][0]) / s;
                q.y = 0.25f * s;
                q.z = (m[1][2] + m[2][1]) / s;
            } else {
                float s = 2.0f * sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]);
                q.w = (m[1][0] - m[0][1]) / s;
                q.x = (m[0][2] + m[2][0]) / s;
                q.y = (m[1][2] + m[2][1]) / s;
                q.z = 0.25f * s;
            }
        }

        return q;
    }

    // 移動 (translation) の設定
    void SetTrans(float tx, float ty, float tz) {
        m[0][3] = tx;
        m[1][3] = ty;
        m[2][3] = tz;
    }
    float GetTransX() const {
      return m[0][3];
    }
    float GetTransY() const {
      return m[1][3];
    }
    float GetTransZ() const {
      return m[2][3];
    }

  // 3D座標を2Dスクリーン座標に変換
  static Vector4 ProjectTo2D(const Vector4& point, const Matrix44& projection, float screenWidth, float screenHeight) {
      // ビュー空間 -> クリップ空間
      Vector4 clipSpace = projection * point;

      // 正規化デバイス座標系に変換 (Homogeneous Divide)
      Vector4 ndc = Vector4(
          clipSpace.x / clipSpace.w,
          clipSpace.y / clipSpace.w,
          clipSpace.z / clipSpace.w,
          1.0f
      );

      // スクリーン座標系に変換
      return Vector4(
          (ndc.x * 0.5f + 0.5f) * screenWidth,
          (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight,
          ndc.z,
          1.0f
      );
  }

  static Matrix44 LookAt(const Vector4& eye, const Vector4& target, const Vector4& up) {
      Vector4 zAxis = Vector4(
          eye.x - target.x,
          eye.y - target.y,
          eye.z - target.z,
          0.0f
      );
      float zLength = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);
      zAxis = Vector4(zAxis.x / zLength, zAxis.y / zLength, zAxis.z / zLength, 0.0f);

      Vector4 xAxis = Vector4(
          up.y * zAxis.z - up.z * zAxis.y,
          up.z * zAxis.x - up.x * zAxis.z,
          up.x * zAxis.y - up.y * zAxis.x,
          0.0f
      );
      float xLength = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
      xAxis = Vector4(xAxis.x / xLength, xAxis.y / xLength, xAxis.z / xLength, 0.0f);

      Vector4 yAxis = Vector4(
          zAxis.y * xAxis.z - zAxis.z * xAxis.y,
          zAxis.z * xAxis.x - zAxis.x * xAxis.z,
          zAxis.x * xAxis.y - zAxis.y * xAxis.x,
          0.0f
      );

      Matrix44 viewMatrix;
      viewMatrix.m[0][0] = xAxis.x;
      viewMatrix.m[0][1] = xAxis.y;
      viewMatrix.m[0][2] = xAxis.z;
      viewMatrix.m[0][3] = -(xAxis.x * eye.x + xAxis.y * eye.y + xAxis.z * eye.z);

      viewMatrix.m[1][0] = yAxis.x;
      viewMatrix.m[1][1] = yAxis.y;
      viewMatrix.m[1][2] = yAxis.z;
      viewMatrix.m[1][3] = -(yAxis.x * eye.x + yAxis.y * eye.y + yAxis.z * eye.z);

      viewMatrix.m[2][0] = zAxis.x;
      viewMatrix.m[2][1] = zAxis.y;
      viewMatrix.m[2][2] = zAxis.z;
      viewMatrix.m[2][3] = -(zAxis.x * eye.x + zAxis.y * eye.y + zAxis.z * eye.z);

      viewMatrix.m[3][0] = 0.0f;
      viewMatrix.m[3][1] = 0.0f;
      viewMatrix.m[3][2] = 0.0f;
      viewMatrix.m[3][3] = 1.0f;

      return viewMatrix;
  }

  // 透視投影行列を生成
  static Matrix44 Perspective(float fov, float aspect, float near, float far) {
      float tanHalfFov = tanf(fov / 2.0f);

      Matrix44 projectionMatrix;
      projectionMatrix.m[0][0] = 1.0f / (aspect * tanHalfFov);
      projectionMatrix.m[1][1] = 1.0f / tanHalfFov;
      projectionMatrix.m[2][2] = -(far + near) / (far - near);
      projectionMatrix.m[2][3] = -(2.0f * far * near) / (far - near);
      projectionMatrix.m[3][2] = -1.0f;
      projectionMatrix.m[3][3] = 0.0f;

      return projectionMatrix;
  }
};




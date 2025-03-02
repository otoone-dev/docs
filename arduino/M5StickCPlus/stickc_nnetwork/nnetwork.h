#include <vector>
#include <cmath>

// ChatGPT による生成コードに指示を加えて調整させたもの

// 学習率
const float learning_rate = 0.1f;

// 活性化関数（シグモイド関数とその微分）
float sigmoid(float x) {
  return 1.0f / (1.0f + exp(-x));
}

float sigmoid_derivative(float x) {
  return x * (1.0f - x); // シグモイド関数の出力を受け取る
}

// ニューラルネットワーク構造
class NeuralNetwork {
private:
  std::vector<std::vector<float>> weights_input_hidden; // 入力層->隠れ層の重み
  std::vector<std::vector<float>> weights_hidden_output; // 隠れ層->出力層の重み
  std::vector<float> hidden_layer; // 隠れ層の出力
  std::vector<float> outputs; // 出力層の値

public:
  // 初期化（入力層、隠れ層、出力層のニューロン数を指定）
  NeuralNetwork(int input_size, int hidden_size, int output_size) {
    // 重みの初期化（小さいランダム値）
    weights_input_hidden.resize(input_size, std::vector<float>(hidden_size));
    weights_hidden_output.resize(hidden_size, std::vector<float>(output_size));
    for (auto& row : weights_input_hidden) {
      for (auto& w : row) {
        w = static_cast<float>(rand()) / RAND_MAX - 0.5f;
      }
    }
    for (auto& row : weights_hidden_output) {
      for (auto& w : row) {
        w = static_cast<float>(rand()) / RAND_MAX - 0.5f;
      }
    }
    hidden_layer.resize(hidden_size, 0.0);
    outputs.resize(output_size, 0.0);
  }

  // フォワードパス
  std::vector<float> forward(const std::vector<float> &inputs) {
    // 隠れ層の計算
    for (size_t j = 0; j < hidden_layer.size(); j++) {
      float sum = 0.0;
      for (size_t i = 0; i < inputs.size(); i++) {
        sum += inputs[i] * weights_input_hidden[i][j];
      }
      hidden_layer[j] = sigmoid(sum);
    }

    // 出力層の計算
    for (size_t k = 0; k < outputs.size(); k++) {
      float sum = 0.0;
      for (size_t j = 0; j < hidden_layer.size(); j++) {
        sum += hidden_layer[j] * weights_hidden_output[j][k];
      }
      outputs[k] = sigmoid(sum);
    }

    return outputs;
  }

  // バックプロパゲーション
  void train(const std::vector<float> &inputs, const std::vector<float> &targets) {
    // フォワードパス
    std::vector<float> predicted = forward(inputs);

    // 出力層の誤差と勾配
    std::vector<float> output_errors(outputs.size(), 0.0);
    std::vector<float> output_gradients(outputs.size(), 0.0);
    for (size_t k = 0; k < outputs.size(); k++) {
      output_errors[k] = targets[k] - predicted[k];
      output_gradients[k] = output_errors[k] * sigmoid_derivative(predicted[k]);
    }

    // 隠れ層の誤差と勾配
    std::vector<float> hidden_errors(hidden_layer.size(), 0.0);
    std::vector<float> hidden_gradients(hidden_layer.size(), 0.0);
    for (size_t j = 0; j < hidden_layer.size(); j++) {
      for (size_t k = 0; k < outputs.size(); k++) {
        hidden_errors[j] += output_gradients[k] * weights_hidden_output[j][k];
      }
      hidden_gradients[j] = hidden_errors[j] * sigmoid_derivative(hidden_layer[j]);
    }

    // 重みの更新（隠れ層->出力層）
    for (size_t j = 0; j < hidden_layer.size(); j++) {
      for (size_t k = 0; k < outputs.size(); k++) {
        weights_hidden_output[j][k] += learning_rate * output_gradients[k] * hidden_layer[j];
      }
    }

    // 重みの更新（入力層->隠れ層）
    for (size_t i = 0; i < inputs.size(); i++) {
      for (size_t j = 0; j < hidden_layer.size(); j++) {
        weights_input_hidden[i][j] += learning_rate * hidden_gradients[j] * inputs[i];
      }
    }
  }

  // 重みの表示
  void printWeights() {
    Serial.println("Input -> Hidden Weights:");
    for (auto &row : weights_input_hidden) {
      for (auto &w : row) {
        Serial.print(w, 4);
        Serial.print(" ");
      }
      Serial.println();
    }

    Serial.println("Hidden -> Output Weights:");
    for (auto &row : weights_hidden_output) {
      for (auto &w : row) {
        Serial.print(w, 4);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
};

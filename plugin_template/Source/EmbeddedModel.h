#pragma once

#include <string>

// drop exported model_rtneural.json string content here or embed via CMake binary resource
inline const std::string& getEmbeddedModelJson() {
    static const std::string json = R"json(
[
  {
    "type": "gru",
    "shape": [null, 2],
    "weights": [
      [[0.1, 0.1, 0.1], [0.1, 0.1, 0.1]],
      [[0.1, 0.1, 0.1]],
      [0.0, 0.0, 0.0]
    ]
  },
  {
    "type": "dense",
    "shape": [null, 48],
    "weights": [
      [[0.1]],
      [0.0]
    ],
    "activation": "linear"
  }
]
)json";
    return json;
}

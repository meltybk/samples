// Author: melty <11942219+meltybk@users.noreply.github.com>

#pragma once

#include <array>
#include <cstdint>

namespace melty {
namespace sample {

class Object {
public:
  Object(){};
  ~Object(){};

  Object(const Object &) = delete;
  Object &operator=(const Object &) = delete;

  inline uint64_t id() const { return id_; }
  inline void set_id(uint64_t id) { id_ = id; }

private:
  uint64_t id_;
};

class GameObject : public Object {
public:
  GameObject();
  ~GameObject();

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;

  inline std::array<float, 3> position() const { return position_; }
  inline void set_position(float x, float y, float z) {
    position_[0] = x;
    position_[1] = y;
    position_[2] = z;
  }

private:
  std::array<float, 3> position_;
};

} // namespace sample
} // namespace melty

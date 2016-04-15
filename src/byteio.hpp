#ifndef BYTEIO_HPP_
#define BYTEIO_HPP_

#include <stdint.h>
#include <array>
#include <vector>
#include <iostream>

template <typename OutputIterator>
OutputIterator WriteInt8(OutputIterator out, uint8_t value) {
  static_assert(sizeof(*out) == 1, "Element size of OutputIterator must be 1.");

  *out = static_cast<char>(value);
  out = std::next(out, 1);

  return out;
}

template <typename OutputIterator>
OutputIterator WriteInt16L(OutputIterator out, uint16_t value) {
  static_assert(sizeof(*out) == 1, "Element size of OutputIterator must be 1.");

  *out = value & 0xff;
  out = std::next(out, 1);
  *out = static_cast<char>((value >> 8) & 0xff);
  out = std::next(out, 1);

  return out;
}

template <typename OutputIterator>
OutputIterator WriteInt32L(OutputIterator out, uint32_t value) {
  static_assert(sizeof(*out) == 1, "Element size of OutputIterator must be 1.");

  *out = static_cast<char>(value & 0xff);
  out = std::next(out, 1);
  *out = static_cast<char>((value >> 8) & 0xff);
  out = std::next(out, 1);
  *out = static_cast<char>((value >> 16) & 0xff);
  out = std::next(out, 1);
  *out = static_cast<char>((value >> 24) & 0xff);
  out = std::next(out, 1);

  return out;
}

template <typename Insertable>
void InsertInt8(Insertable & out, uint8_t value) {
  out << static_cast<char>(value);
}

template <typename Insertable>
void InsertInt16L(Insertable & out, uint16_t value) {
  out << static_cast<char>(value & 0xff);
  out << static_cast<char>((value >> 8) & 0xff);
}

template <typename Insertable>
void InsertInt32L(Insertable & out, uint32_t value) {
  out << static_cast<char>(value & 0xff);
  out << static_cast<char>((value >> 8) & 0xff);
  out << static_cast<char>((value >> 16) & 0xff);
  out << static_cast<char>((value >> 24) & 0xff);
}

template <typename InputIterator, typename Int8>
InputIterator ReadInt8(InputIterator in, Int8 & out) {
  static_assert(sizeof(*in) == 1, "Element size of InputIterator must be 1.");

  uint8_t value = static_cast<unsigned char>(*in);
  in = std::next(in, 1);

  out = value;
  return in;
}

template <typename InputIterator, typename Int16>
InputIterator ReadInt16L(InputIterator in, Int16 & out) {
  static_assert(sizeof(*in) == 1, "Element size of InputIterator must be 1.");
  static_assert(sizeof(Int16) >= 2, "The size of output integer must be 2 at least.");

  uint16_t value = static_cast<unsigned char>(*in);
  in = std::next(in, 1);
  value |= static_cast<unsigned char>(*in) << 8;
  in = std::next(in, 1);

  out = value;
  return in;
}

template <typename InputIterator, typename Int32>
InputIterator ReadInt32L(InputIterator in, Int32 & out) {
  static_assert(sizeof(*in) == 1, "Element size of InputIterator must be 1.");
  static_assert(sizeof(Int32) >= 4, "The size of output integer must be 4 at least.");

  uint32_t value = static_cast<unsigned char>(*in);
  in = std::next(in, 1);
  value |= static_cast<unsigned char>(*in) << 8;
  in = std::next(in, 1);
  value |= static_cast<unsigned char>(*in) << 16;
  in = std::next(in, 1);
  value |= static_cast<unsigned char>(*in) << 24;
  in = std::next(in, 1);

  out = value;
  return in;
}

template <typename Readable, typename Int8>
bool ReadStreamAsInt8(Readable & in, Int8 & out) {
  uint8_t data[1];
  in.read(reinterpret_cast<char *>(data), 1);
  if (in.gcount() == 1) {
    out = data[0];
    return true;
  }
  else {
    return false;
  }
}

template <typename Readable, typename Int16>
bool ReadStreamAsInt16L(Readable & in, Int16 & out) {
  static_assert(sizeof(Int16) >= 2, "The size of output integer must be 2 at least.");

  uint8_t data[2];
  in.read(reinterpret_cast<char *>(data), 2);
  if (in.gcount() == 2) {
    out = data[0] | (data[1] << 8);
    return true;
  }
  else {
    return false;
  }
}

template <typename Readable, typename Int32>
bool ReadStreamAsInt32L(Readable & in, Int32 & out) {
  static_assert(sizeof(Int32) >= 4, "The size of output integer must be 4 at least.");

  uint8_t data[4];
  in.read(reinterpret_cast<char *>(data), 4);
  if (in.gcount() == 4) {
    out = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return true;
  }
  else {
    return false;
  }
}

#endif // BYTEIO_HPP_

#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <cstdio>


template<typename ... Args>
std::string StringFmt(const std::string& format, Args ... args) {
  size_t size = 1 + format.size() * 2;
  std::unique_ptr<char[]> buf(new char[size]);
  _snprintf_s(buf.get(), size, size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size);
}

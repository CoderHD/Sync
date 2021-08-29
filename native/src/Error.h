#pragma once
#include <iostream>

#define LOG_ERROR(what) logError(__FUNCSIG__, what)
#define LOG(what) log(__FUNCSIG__, what)

inline void logError(const char *where, const char *what) {
  std::cerr << "[Error] " << where << " :  " << what << std::endl;
}

inline void log(const char *where, const char *what) {
  std::cerr << "[Info] " << where << " :  " << what << std::endl;
}

//
// Created by Yang Jerry on 2022/3/30.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-macro-parentheses"

#ifndef SEULEXYACC_UTILS_H
#define SEULEXYACC_UTILS_H

#include <iostream>
#include <set>
#include <map>
#include <unordered_map>
#include <sstream>
#include <functional>

// #include <concepts>

using namespace std;

namespace sly::utils{

#define FMAP_DECL(functor, method) \
template<typename T> \
functor<T> fmap(const functor<T>& container, std::function<T(const T&)> f) \
{ \
  functor<T> result; \
  for (const T& elem: container) \
  { \
    result.method(f(elem)); \
  } \
  return result;\
}

FMAP_DECL(std::set, emplace)

FMAP_DECL(std::vector, push_back)

template<typename T1, typename T2>
std::map<T1, T2>
fmap(const std::map<T1, T2> &container, std::function<std::pair<T1, T2>(const std::pair<T1, T2> &)> f) {
  std::map<T1, T2> result;
  for (const std::pair<T1, T2> &elem: container) {
    result.insert(f(elem));
  }
  return result;
}


template<typename T>
set<T> fixed_point(set<T> set0, function<set<T>(T)> f) {
  while (true) {
    auto set1 = set0;
    for_each(set0.begin(), set0.end(), [&set1, &f](const auto &v) {
      set1.merge(f(v));
    });
    if (set0 == set1)
      break;
    else
      set0 = set1;
  }
  return set0;
}

class Log {
 public:
  enum LogLevel {
    kInfo = 0, kWarning = 1, kError = 2, kNone = 3
  } level;
  
  
  inline static const Log &GetGlobalLogger();
  
  template<typename ...T>
  void Info(const T &... args) const;
  
  template<typename ...T>
  void Warn(const T &... args) const;
  
  template<typename... T>
  void Err(const T &... args) const;

  static inline void SetLogLevel(LogLevel level);
 
 
 private:
  
  inline static Log &GetGlobalLoggerPrivate();
  template<typename T>
  void Put(ostream &os, T arg) const;
};

template<typename T>
void Log::Put(ostream &os, T arg) const {
  os << arg;
}

template<typename... T>
void Log::Info(const T &... args) const {
  if (level > kInfo)
    return;
  Put(cout, "Info: ");
  int arr[] = {(Put(cout, args), 0)...};
  Put(cout, "\n");
}

template<typename... T>
void Log::Warn(const T &... args) const {
  if (level > kWarning)
    return;
  Put(cout, "Warning: ");
  int arr[] = {(Put(cout, args), 0)...};
  Put(cout, "\n");
}

template<typename... T>
void Log::Err(const T &... args) const {
  if (level > kError)
    return;
  Put(cerr, "Error: ");
  int arr[] = {(Put(cerr, args), 0)...};
  Put(cerr, "\n");
};

inline const Log &Log::GetGlobalLogger() {
  return GetGlobalLoggerPrivate();
}


inline Log &Log::GetGlobalLoggerPrivate() {
  static Log log;
  return log;
}


inline void Log::SetLogLevel(LogLevel level) {
  GetGlobalLoggerPrivate().level = level;
}


inline size_t hash_combine(size_t h1, size_t h2) {
  return h1 ^ (h2 << 1);
}

template<typename T>
size_t hash(const T& v) {
  return std::hash<T>{}(v);
}

template<typename T, typename ... Args>
size_t hash(const T &v, const Args &... args) {
  size_t v1 = std::hash<T>{}(v);
  size_t v2 = sly::utils::hash(args...);
  return hash_combine(v1, v2);
}

#define FUNC_END_INFO sly::utils::Log::GetGlobalLogger().Info(__FILE__, __LINE__, __FUNCTION__, "Done.")

#define FUNC_START_INFO sly::utils::Log::GetGlobalLogger().Info(__FILE__, __LINE__, __FUNCTION__, "Start.")

template<typename T>
std::string to_string(const T& v) {
  stringstream ss;
  ss << v;
  return ss.str();
}

inline void ltrim(std::string &str, const string &val = " \r\n\0") {
  str.erase(0, str.find_first_not_of(val));
}

inline void rtrim(std::string &str, const string &val = " \r\n\0") {
  str.erase(str.find_last_not_of(val) + 1);
}

inline void trim(std::string &str, const string &val = " \r\n\0") {
  ltrim(str, val);
  rtrim(str, val);
}

inline void replace_all(std::string &str, const std::string &from, const std::string &to) {
  for (std::string::size_type pos = str.find(from); 
       pos != std::string::npos; 
       pos = str.find(from)) {
    str.replace(pos, from.length(), to);
  }
}

inline std::string escape(const std::string &str) {
  std::string res = str;
  replace_all(res, "\t", "\\t");
  replace_all(res, "\r", "\\r");
  replace_all(res, "\n", "\\n");
  return res;
}
} // namespace sly::utils

#endif //SEULEXYACC_UTILS_H

#pragma clang diagnostic pop
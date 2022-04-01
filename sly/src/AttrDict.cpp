#include <sly/AttrDict.h>
#define DECLARE_TYPE_REGISTERY(T) {std::type_index(typeid(T)), [](const any& v){return utils::to_string<T>(any_cast<T>(v));}}


namespace sly::core::type{


map<std::type_index, function<std::string(const std::any& v)>> __type_registery{
  DECLARE_TYPE_REGISTERY(int),
  DECLARE_TYPE_REGISTERY(float),
  DECLARE_TYPE_REGISTERY(double),
  DECLARE_TYPE_REGISTERY(size_t),
  DECLARE_TYPE_REGISTERY(std::string)
};


map<string, string> AttrDict::ToString() const {
  map<string, string> retval;
  for (auto [k, a]: attr_dict_) {
    auto tid= std::type_index(a.type());
    if (const auto it = __type_registery.find(tid);
        it != __type_registery.end()) {
      retval.emplace(k,  it->second(a));
    } else {
      retval.emplace(k, tid.name());
    }
  }
  return retval;
}

}
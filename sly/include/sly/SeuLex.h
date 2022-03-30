//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_SEULEX_H
#define SEULEXYACC_SEULEX_H
#include "sly.h"

namespace sly::runtime {
class SeuLex {
 public:
  void Process(sly::utils::InputStream & is, sly::utils::InputStream & os);
};

}

#endif //SEULEXYACC_SEULEX_H

//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_SEUYACC_H
#define SEULEXYACC_SEUYACC_H
#include "sly.h"

namespace sly::runtime {

class SeuYacc {
  void Process(sly::utils::InputStream &is, sly::utils::InputStream &os);
};

}
#endif //SEULEXYACC_SEUYACC_H

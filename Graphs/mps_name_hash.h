#ifndef MPS_NAME_HASH_H_
#define MPS_NAME_HASH_H_

#import <string>

namespace mps_name_hash {
  inline char CharFrom5Bits(int bits) {
    if (bits < 10) {
      return '0' + bits;
    } else {
      return 'a' + bits;
    }
  }

  inline std::string Hash(int id) {
    std::string ret;
    while (id > 32) {
      int extracted = id & 0x1F;
      ret.push_back(CharFrom5Bits(extracted));
      id = id >> 5;
    }
    ret.push_back(CharFrom5Bits(id));
    return ret;
  }
}

#endif /* MPS_NAME_HASH_H_ */

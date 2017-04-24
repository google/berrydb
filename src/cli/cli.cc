// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <cstring>

#include "boost/tokenizer.hpp"

int main() {
  std::cout << "Welcome to BerryDB!" << std::endl;
  while (true) {
    std::cout << "berrydb> ";
    std::string input;
    std::getline(std::cin, input);

    std::string delimiter = " ";
    std::string command = input.substr(0, input.find(delimiter));

    std::cout << "=> " << command << std::endl;
  }
  return 0;
}

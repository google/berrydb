// Copyright 2017 The BerryDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iomanip>  // std::setw, std::setfill
#include <iostream>  // std::cout
#include <map>  // std::map
#include <vector>  // std::vector

#include <boost/tokenizer.hpp>  // boost::tokenizer, boost::escaped_list_separator

#if defined(BOOST_NO_EXCEPTIONS)

// We must define boost::throw_exception, because we disabled exceptions in the
// compiler.
// https://stackoverflow.com/questions/9272648/boost-symbol-not-found
namespace boost {
void throw_exception(const std::exception &) {
}
} // namespace boost

#endif  // defined(BOOST_NO_EXCEPTIONS)

void welcome() {
  const char * logo =
    " ______   ______   ______   ______   __  __   _____    ______    \n"
    "/\\  == \\ /\\  ___\\ /\\  == \\ /\\  == \\ /\\ \\_\\ \\ /\\  __-. /\\  == \\   \n"
    "\\ \\  __< \\ \\  __\\ \\ \\  __< \\ \\  __< \\ \\____ \\\\ \\ \\/\\ \\\\ \\  __<   \n"
    " \\ \\_____\\\\ \\_____\\\\ \\_\\ \\_\\\\ \\_\\ \\_\\\\/\\_____\\\\ \\____- \\ \\_____\\ \n"
    "  \\/_____/ \\/_____/ \\/_/ /_/ \\/_/ /_/ \\/_____/ \\/____/  \\/_____/ \n";
  std::cout << logo << std::endl;
  std::cout << "Welcome to BerryDB!" << std::endl;
}

void print_prompt(int line) {
  std::cout << "berrydb 🍓 :" << std::setw(3) << std::setfill('0') << line << "> ";
}

void print_output(std::string message) {
  std::cout << "=> " << message << std::endl;
}

typedef boost::tokenizer<boost::escaped_list_separator<char>> cli_tokenizer;

int main() {
  welcome();

  std::map<std::string, std::string> data;

  int line = 0;
  while (true) {
    print_prompt(++line);

    std::string input;
    std::getline(std::cin, input);

    std::string delimiter = " ";
    std::vector<std::string> args;
    cli_tokenizer tokenizer(input, boost::escaped_list_separator<char>('\\', ' ', '\"'));
    for (cli_tokenizer::iterator it = tokenizer.begin(); it != tokenizer.end(); ++it) {
      args.push_back(*it);
    }

    if (args.size() == 0) { continue; }
    if (args[0] == "get") {
      if (args.size() != 2) {
        print_output("⚠️  Expected 1 argument.");
      } else {
        auto result = data.find(args[1]);
        if (result != data.end()) {
          print_output(result->second);
        } else {
          print_output("❌  Not found.");
        }
      }
    } else if (args[0] == "set") {
      if (args.size() != 3) {
        print_output("⚠️  Expected 2 arguments."); //issue with empty spaces
      } else {
        data[args[1]] = args[2];
      }
    } else if (args[0] == "delete") {
      if (args.size() != 2) {
        print_output("⚠️  Expected 1 argument.");
      } else {
        auto result = data.find(args[1]);
        if (result != data.end()) {
          data.erase(args[1]);
        } else {
          print_output("❌  Not found.");
        }
      }
    } else {
      print_output("⚠️  Unsupported command.");
    }
  }
  return 0;
}

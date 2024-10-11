#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "token_type.hh"
#include "tokenization.hh"

std::string tokens_to_asm(const std::vector<Token>& tokens);

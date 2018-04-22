#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdint>
#include <iomanip>
#include <dirent.h>
#include <stdio.h>

std::string hashfile(std::string filename);
void listFiles(std::string folderpath);
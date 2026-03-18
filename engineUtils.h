#ifndef ENGINEUTILS_H
#define ENGINEUTILS_H
#include <string>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <iostream>

/*
    Class for Json Reading. 
*/

class EngineUtils {
public:
    static void ReadJsonFile(const std::string& path, rapidjson::Document& out_document) {
        FILE* file_pointer = nullptr;
#ifdef _WIN32
        fopen_s(&file_pointer, path.c_str(), "rb");
#else
        file_pointer = fopen(path.c_str(), "rb");
#endif
        if (!file_pointer) {
            std::cout << "Failed to open file: " << path << std::endl;
            return;
        }

        char buffer[65536];
        rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
        out_document.ParseStream(stream);
        std::fclose(file_pointer);

        if (out_document.HasParseError()) {
            out_document.GetParseError();
            std::cout<< "Error parsing JSON at [" << path << "]" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
};

#endif // ENGINEUTILS_H

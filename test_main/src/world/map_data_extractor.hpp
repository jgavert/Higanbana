#pragma once

#include <higanbana/core/filesystem/filesystem.hpp>
#include <string>

namespace app
{
  void readInfoFromOpenMapDataASC(higanbana::FileSystem& fs)
  {
    if (fs.fileExists("/maps/L4133A/L4133A.asc"))
    {
      auto file = fs.readFile("/maps/L4133A/L4133A.asc");
      std::string firstLine = "";
      int i = 0;
      char lol = *file.data();
      while(lol != '\n')
      {
        firstLine += lol;
        i++;
        lol = file.data()[i];
      }
      HIGAN_LOGi("first line: \"%s\"\n", firstLine.c_str());
    }
  }
}
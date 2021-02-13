#include "map_data_extractor.hpp"

namespace app
{
  higanbana::vector<std::string_view> splitToLines(std::string_view view)
  {
    higanbana::vector<std::string_view> result;
    int offset = 0;
    auto tokenIndex = view.find('\n', offset);
    while (tokenIndex != std::string::npos)
    {
      result.push_back(view.substr(offset, tokenIndex-offset));
      offset = tokenIndex+1;
      tokenIndex = view.find('\n', offset);
    }
    result.push_back(view.substr(offset));
    return result;
  }

  int extractInt(std::string_view line)
  {
    auto token = line.find(' ');
    token = line.find_first_not_of(' ', token);
    return std::stoi(std::string(line.substr(token)));
  }

  void getRowData(std::string_view line, higanbana::vector<float>& out, int offset)
  {
    int lineOffset = 1;
    auto tokenIndex = line.find(' ', lineOffset);
    while(tokenIndex != std::string::npos)
    {
      out[offset] = std::stof(std::string(line.substr(lineOffset, tokenIndex - lineOffset)));
      lineOffset = tokenIndex + 1;
      tokenIndex = line.find(' ', lineOffset);
      offset++;
    }
    out[offset] = std::stof(std::string(line.substr(lineOffset)));
  }

  std::optional<higanbana::CpuImage> readInfoFromOpenMapDataASC(higanbana::FileSystem& fs)
  {
    if (fs.fileExists("/data/maps/L4133A/L4133A.asc"))
    {
      auto file = fs.readFile("/data/maps/L4133A/L4133A.asc");
      std::string firstLine = "";
      std::string_view view(reinterpret_cast<char*>(file.data()), file.size());
      auto lineSplitted = splitToLines(view);
      auto width = extractInt(lineSplitted[0]);
      auto height = extractInt(lineSplitted[1]);
      HIGAN_LOGi("found heightmap of size %dx%d\n", width, height);
      higanbana::vector<float> mapData;
      mapData.resize(width*height);
      int offset = 0;
      int line = 6;
      for (int line = 6; line < 6+height; ++line)
      {
        getRowData(lineSplitted[line], mapData, offset);
        offset += width;
      }
      HIGAN_LOGi("sample data %f %f %f\n", mapData[0], mapData[width*height / 2], mapData[width*height / 4]);
      higanbana::CpuImage image(higanbana::ResourceDescriptor()
        .setWidth(width)
        .setHeight(height)
        .setFormat(higanbana::FormatType::Float32)
        .setName("heightmap data")
        .setUsage(higanbana::ResourceUsage::GpuReadOnly));
      auto subRes = image.subresource(0, 0);
      memcpy(subRes.data(), mapData.data(), mapData.size()*sizeof(float));
      return image;
    }
    return {};
  }
}
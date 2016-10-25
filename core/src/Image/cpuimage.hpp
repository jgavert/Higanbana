#pragma once

#include <stddef.h>
#include <stdio.h>
#include <vector>
#include <cstdint>

struct Size3D
{
	int x;
	int y;
	int z;
};

enum class ImageFormat
{
	Undefined,
	RGBA8
};

constexpr int pixelSize[] =
{
	1,
	4
};

bool isBlockCompressed(ImageFormat)
{
	return false;
}

int getBytesInPixel(ImageFormat format)
{
	return pixelSize[static_cast<int>(format)];
}

Size3D nextMipmapSize(Size3D size)
{
	return {std::max(size.x / 2, 1), std::max(size.y / 2, 1), std::max(size.z / 2, 1)};
}

int getBytesInSize(ImageFormat format, Size3D size)
{
	int bytesInPixel = getBytesInPixel(format);
	return bytesInPixel * size.x * size.y * size.z;
}

namespace desc
{
	class Image
	{
	public:
		Size3D size = {1, 1, 1};
		int mipCount = 1;
		int arraySize = 1;
		ImageFormat format = ImageFormat::Undefined;

		Image() {}
		Image& setSize(Size3D value)
		{
			size = value;
			return *this;
		}
		Image& setMipCount(int mips)
		{
			mipCount = mips;
			return *this;
		}
		Image& setArraySize(int size)
		{
			arraySize = size;
			return *this;
		}
		Image& setFormat(ImageFormat value)
		{
			format = value;
			return *this;
		}
	};

	// needs to be able to jump by
	// row
	// mip level
	// array
	class Subresource
	{
	public:
		size_t rowPitch; // single row
		size_t slicePitch; // mipmap level
		size_t arrayPitch; // whole array?
		size_t offset;
	};

};


class Image
{
private:
	desc::Image descriptor;	
	std::vector<desc::Subresource> subresources;
	std::vector<uint8_t> data;
public:
	Image(desc::Image description)
		: descriptor(description)
	{
	  size_t rowPitch = getBytesInPixel(descriptor.format) * descriptor.size.x;

		size_t totalSize = 0;
		for (int slice = 0; slice < descriptor.arraySize; ++slice)
		{
			Size3D size = descriptor.size;
			for (int mip = 0; mip < descriptor.mipCount; ++mip)
			{
				size_t slicePitch = getBytesInSize(descriptor.format, size);
				subresources.emplace_back(desc::Subresource{rowPitch, slicePitch, 0, totalSize});
				printf("%dx%dx%d, row %zu, slice %zu, offset %zu\n", size.x, size.y, size.z, rowPitch, slicePitch, totalSize);
				totalSize += slicePitch;
				size = nextMipmapSize(size);
			}
		}
		data.resize(totalSize);
	}

	Size3D size3D()
	{
		return descriptor.size;
	}

	class Subresource
	{
	private:
		uint8_t* m_ptr;
		size_t m_rowPitch;
		size_t m_size;
	public:
		Subresource(uint8_t* data, size_t rowPitch, size_t size)
			: m_ptr(data)
			, m_rowPitch(rowPitch)
			, m_size(size)
		{
		}
		
		template <typename T>
		T& p(int x, int y)
		{
			return *reinterpret_cast<T*>(m_ptr + y * m_rowPitch + x);	
		}

		uint8_t* data()
		{
			return m_ptr;
		}

		size_t size()
		{
			return m_size;
		}
	};

	Subresource subresource(int mip = 0, int array = 0)
	{
		auto index = descriptor.mipCount*array + mip;
		auto& subRes = subresources[index];
		return Subresource(data.data() + subRes.offset, subRes.rowPitch, subRes.slicePitch);
	}
};


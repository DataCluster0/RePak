#pragma once

#include <pch.h>

#pragma pack(push, 1)
struct UIImageHeader
{
	uint64_t unk0 = 0;
	uint16_t m_nWidth = 1;
	uint16_t m_nHeight = 1;
	uint16_t m_nTextureOffsetsCount = 0;
	uint16_t m_nTextureCount = 0;
	RPakPtr m_pTextureOffsets{};
	RPakPtr m_pTextureDims{};
	uint32_t unk1 = 0;
	uint32_t unk2 = 0;
	RPakPtr m_pTextureHashes{};
	RPakPtr m_pTextureNames{};
	uint64_t m_nAtlasGUID = 0;
};

struct UIImageUV
{
	void InitUIImageUV(float startX, float startY, float width, float height)
	{
		this->uv0x = startX;
		this->uv1x = width;
		this->uv0y = startY;
		this->uv1y = height;
	}
	// maybe the uv coords for top left?
	// just leave these as 0 and it should be fine
	float uv0x = 0;
	float uv0y = 0;

	// these two seem to be the uv coords for the bottom right corner
	// examples:
	// uv1x = 10;
	// | | | | | | | | | |
	// uv1x = 5;
	// | | | | |
	float uv1x = 1.f;
	float uv1y = 1.f;
};

// examples of changes from these values: https://imgur.com/a/l1YDXaz
struct UIImageOffset
{
	void InitUIImageOffset(float startX, float startY, float endX, float endY)
	{
		this->startX = startX;
		this->startY = startY;
		this->endX = endX;
		this->endY = endY;
		//this->unkX = 1 - 2 * startX; // doesnt seem to always 100% of the time match up but its very close
		//this->unkY = 1 - 2 * startY;
	}
	// these don't seem to matter all that much as long as they are a valid float number
	float f0 = 0.f;
	float f1 = 0.f;

	// endX and endY define where the edge of the image is, with 1.f being the full length of the image and 0.5f being half of the image
	float endX = 1.f;
	float endY = 1.f;

	// startX and startY define where the top left corner is in proportion to the full image dimensions
	float startX = 0.f;
	float startY = 0.f;

	// changing these 2 values causes the image to be distorted on each axis
	float unkX = 1.f;
	float unkY = 1.f;
};
#pragma pack(pop)

#pragma once

#include <d3d11.h>
#include <pch.h>

// The value of PI, a constant
constexpr static double PI = 3.14159265358979323846;
// The value of PI * 2, a constant
constexpr static double PI2 = (PI * 2);
// The epsilon value for comparisions
constexpr static double Epsilon = 4.37114e-05;

constexpr static float DegreesToRadians(float Value)
{
	return (float)((Value * PI) / 180.0f);
}
// Converts the input radians to degree units
constexpr static float RadiansToDegrees(float Value)
{
	return (float)(((Value * 180.0f) / PI));
}

template <typename T>
// Clamp a number between two bounds
constexpr static T Clamp(const T& Value, const T& Lower, const T& Upper)
{
	// Clamp it
	return max(Lower, min(Value, Upper));
}

struct Vector2
{
	float x, y;

	Vector2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	Vector2() {};
};

struct Vector3
{
	float x, y, z;

	Vector3(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Vector3 Lerp(float Factor, const Vector3& Rhs) const
	{
		auto Value = Vector3(Rhs.x - x, Rhs.y - y, Rhs.z - z);
		Value = Vector3(x * Factor, y * Factor, z * Factor);
		Value = Vector3(x + Value.x, y + Value.y, z + Value.z);

		return Value;
	}

	Vector3() {};
};

//I think this implementation is better than what 'TintVec4' was before..
struct Vector4
{
	float r, g, b, a;

	Vector4(float r, float g, float b, float a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	Vector4() {};
};

#pragma pack(push, 1)

#define SF_HEAD   0 // :skull:
#define SF_CPU    (1 << 0)
#define SF_UNK2   (1 << 1)// maybe "temp"?
#define SF_SERVER (1 << 5)
#define SF_CLIENT (1 << 6)
#define SF_DEV    (1 << 7)

enum class AssetType : uint32_t
{
	RMDL = '_ldm', // mdl_ - 0x5F6C646D
	TEXTURE = 'rtxt', // txtr - 0x72747874
	TEXTUREANIM = 'naxt', // txan - 0x6e617874
	UIIA = 'aiiu', // uiia - 0x61696975
	DTBL = 'lbtd', // dtbl - 0x6C627464
	STGS = 'sgts', // stgs - 0x73677473
	STLT = 'tlts', // stlt - 0x746c7473
	MATL = 'ltam', // matl - 0x6C74616D
	ARIG = 'gira', // arig - 0x67697261
	ASEQ = 'qesa', // aseq - 0x71657361
	SUBT = 'tbus', // subt - 0x74627573
	SHDS = 'sdhs', // shds - 0x73646873
	SHDR = 'rdhs', // shdr - 0x72646873
	UIMG = 'gmiu', // uimg - 0x676D6975
	RSON = 'nosr', // rson - 0x72736F6E
	PTCH = 'hctp',  // Ptch - 0x68637450
	UI = 'iu' // ui - 0x75690000
};

// generic header struct for both apex and titanfall 2
// contains all the necessary members for both, RPakFileBase::WriteHeader decides
// which should be written depending on the version
struct RPakFileHeader
{
	uint32_t m_nMagic = 0x6b615052;
	uint16_t m_nVersion = 0x8;
	uint8_t  m_nFlags[0x2];
	uint64_t m_nCreatedTime; // this is actually FILETIME, but if we don't make it uint64_t here, it'll break the struct when writing
	uint8_t  unk0[0x8];
	uint64_t m_nSizeDisk; // size of the rpak file on disk before decompression
	uint64_t m_nEmbeddedStarpakOffset = 0;
	uint8_t  unk1[0x8];
	uint64_t m_nSizeMemory; // actual data size of the rpak file after decompression
	uint64_t m_nEmbeddedStarpakSize = 0;
	uint8_t  unk2[0x8];
	uint16_t m_nStarpakReferenceSize = 0; // size in bytes of the section containing mandatory starpak paths
	uint16_t m_nStarpakOptReferenceSize = 0; // size in bytes of the section containing optional starpak paths
	uint16_t m_nVirtualSegmentCount = 0;
	uint16_t m_nPageCount = 0; // number of "mempages" in the rpak
	uint16_t m_nPatchIndex = 0;
	uint16_t alignment = 0;
	uint32_t m_nDescriptorCount = 0;
	uint32_t m_nAssetEntryCount = 0;
	uint32_t m_nGuidDescriptorCount = 0;
	uint32_t m_nRelationsCount = 0;

	// only in tf2
	uint32_t m_nUnknownSeventhBlockCount = 0;
	uint32_t m_nUnknownEighthBlockCount = 0;

	// only in apex
	uint8_t  unk3[0x1c];
};

struct RPakPatchCompressedHeader // Comes after file header if its an patch rpak.
{
	uint64_t m_nSizeDisk;
	uint64_t m_nSizeMemory;
};

// represents a "pointer" into a mempage by page index and offset
// when loaded, these usually get converted to a real pointer
struct RPakPtr
{
	uint32_t m_nIndex = 0;
	uint32_t m_nOffset = 0;
};

// segment
// these probably aren't actually called virtual segments
// this struct doesn't really describe any real data segment, but collects info
// about the size of pages that are using specific flags/types/whatever
struct RPakVirtualSegment
{
	uint32_t flags = 0; // not sure what this actually is, doesn't seem to be used in that many places
	uint32_t alignment = 0;
	uint64_t dataSize = 0;
};

// mem page
// describes an actual section in the file data. all pages are sequential
// with page at idx 0 being just after the asset relation data
// in patched rpaks (e.g. common(01).rpak), these sections don't fully line up with the data,
// because of both the patch edit stream and also missing pages that are only present in the base rpak
struct RPakPageInfo
{
	uint32_t segIdx; // index into vseg array
	uint32_t pageAlignment; // no idea
	uint32_t dataSize; // actual size of page in bytes
};

// defines the location of a data "pointer" within the pak's mem pages
// allows the engine to read the index/offset pair and replace it with an actual memory pointer at runtime
struct RPakDescriptor
{
	uint32_t m_nPageIdx;	 // page index
	uint32_t m_nPageOffset; // offset within page
};

// same kinda thing as RPakDescriptor, but this one tells the engine where
// guid references to other assets are within mem pages
typedef RPakDescriptor RPakGuidDescriptor;

// this definitely doesn't need to be in a struct but whatever
struct RPakRelationBlock
{
	uint32_t FileID;
};

// defines a bunch of values for registering/using an asset from the rpak
struct RPakAssetEntry
{
	RPakAssetEntry() = default;

	void InitAsset(uint64_t nGUID,
		uint32_t nSubHeaderBlockIdx,
		uint32_t nSubHeaderBlockOffset,
		uint32_t nSubHeaderSize,
		uint32_t nRawDataBlockIdx,
		uint32_t nRawDataBlockOffset,
		uint64_t nStarpakOffset,
		uint64_t nOptStarpakOffset,
		uint32_t Type)
	{
		this->m_nGUID = nGUID;
		this->m_nSubHeaderDataBlockIdx = nSubHeaderBlockIdx;
		this->m_nSubHeaderDataBlockOffset = nSubHeaderBlockOffset;
		this->m_nRawDataBlockIndex = nRawDataBlockIdx;
		this->m_nRawDataBlockOffset = nRawDataBlockOffset;
		this->m_nStarpakOffset = nStarpakOffset;
		this->m_nOptStarpakOffset = nOptStarpakOffset;
		this->m_nSubHeaderSize = nSubHeaderSize;
		this->m_nMagic = Type;
	}

	// hashed version of the asset path
	// used for referencing the asset from elsewhere
	//
	// - when referenced from other assets, the GUID is used directly
	// - when referenced from scripts, the GUID is calculated from the original asset path
	//   by a function such as RTech::StringToGuid
	uint64_t m_nGUID = 0;
	uint8_t  unk0[0x8]{};

	// page index and offset for where this asset's header is located
	uint32_t m_nSubHeaderDataBlockIdx = 0;
	uint32_t m_nSubHeaderDataBlockOffset = 0;

	// page index and offset for where this asset's data is located
	// note: this may not always be used for finding the data:
	//		 some assets use their own idx/offset pair from within the subheader
	//		 when adding pairs like this, you MUST register it as a descriptor
	//		 otherwise the pointer won't be converted
	uint32_t m_nRawDataBlockIndex = 0;
	uint32_t m_nRawDataBlockOffset = 0;

	// offset to any available streamed data
	// m_nStarpakOffset    = "mandatory" starpak file offset
	// m_nOptStarpakOffset = "optional" starpak file offset
	//
	// in reality both are mandatory but respawn likes to do a little trolling
	// so "opt" starpaks are a thing
	uint64_t m_nStarpakOffset = -1;
	uint64_t m_nOptStarpakOffset = -1;

	uint16_t m_nPageEnd = 0; // highest mem page used by this asset
	uint16_t unk1 = 0;

	uint32_t m_nRelationsStartIdx = 0;

	uint32_t m_nUsesStartIdx = 0;
	uint32_t m_nRelationsCounts = 0;
	uint32_t m_nUsesCount = 0; // number of other assets that this asset uses

	// size of the asset header
	uint32_t m_nSubHeaderSize = 0;

	// this isn't always changed when the asset gets changed
	// but respawn calls it a version so i will as well
	uint32_t m_nVersion = 0;

	// see AssetType enum below
	uint32_t m_nMagic = 0;
};

#pragma pack(pop)

// internal data structure for referencing file data to be written
struct RPakRawDataBlock
{
	uint32_t m_nPageIdx;
	uint64_t m_nDataSize;
	uint8_t* m_nDataPtr;
};

// internal data structure for referencing streaming data to be written
struct SRPkDataEntry
{
	uint64_t m_nOffset = -1; // set when added
	uint64_t m_nDataSize = 0;
	uint8_t* m_nDataPtr = nullptr;
};

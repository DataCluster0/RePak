#include "pch.h"
#include "Assets.h"

void Assets::AddRigAsset_stub(std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Error("RPak version 7 (Titanfall 2) ARIG NOT IMPLEMENTED!!!!");
}

void Assets::AddRigAsset_v4(std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Log("\n==============================\n");
	Log("Asset arig -> '%s'\n", assetPath);

	std::string sAssetName = assetPath;
	std::string skelFilePath = g_sAssetsDir + sAssetName;

	uint32_t fileNameDataSize = sAssetName.length() + 1;

	AnimRigHeader* pHdr = new AnimRigHeader();

	// add required files
	REQUIRE_FILE(skelFilePath);

	// begin rrig input
	BinaryIO skelInput;
	skelInput.open(skelFilePath, BinaryIOMode::Read);

	studiohdr_t mdlhdr = skelInput.read<studiohdr_t>();

	if (mdlhdr.id != 0x54534449) // "IDST"
	{
		Warning("invalid file magic for arig asset '%s'. expected %x, found %x. skipping asset...\n", sAssetName.c_str(), 0x54534449, mdlhdr.id);
		return;
	}

	if (mdlhdr.version != 54)
	{
		Warning("invalid version for arig asset '%s'. expected %i, found %i. skipping asset...\n", sAssetName.c_str(), 54, mdlhdr.version);
		return;
	}

	if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	{
		pHdr->AseqRefCount = mapEntry["animseqs"].Size();

		if (pHdr->AseqRefCount == 0)
			Error("invalid animseq count must not be 0 for arig '%s'\n", assetPath);
	}

	size_t DataBufferSize = fileNameDataSize + mdlhdr.length + (pHdr->AseqRefCount * 0x8);
	char* pDataBuf = new char[DataBufferSize];

	// write the model file path into the data buffer
	snprintf(pDataBuf, fileNameDataSize, "%s", sAssetName.c_str());

	// go back to the beginning of the file to read all the data
	skelInput.seek(0);

	// write the skeleton data into the data buffer
	skelInput.getReader()->read(pDataBuf + fileNameDataSize, mdlhdr.length);
	skelInput.close();

	rmem aseqBuf(pDataBuf);

	uint64_t offset = 0;
	for (auto& it : mapEntry["animseqs"].GetArray())
	{
		if (it.IsString())
		{
			std::string seq = it.GetStdString();

			aseqBuf.write<uint64_t>(RTech::StringToGuid(seq.c_str()), fileNameDataSize + mdlhdr.length + (offset * 0x8));

			offset++;
		}
		else {
			Error("invalid animseq entry for arig '%s'\n", assetPath);
		}
	}

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = RePak::CreateNewSegment(sizeof(AnimRigHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = RePak::CreateNewSegment(DataBufferSize, SF_CPU, 64);

	uint32_t SegmentOffset = 0;
	pHdr->pName = { dataseginfo.index, SegmentOffset };

	SegmentOffset += fileNameDataSize;
	pHdr->pSkeleton = { dataseginfo.index, SegmentOffset };

	SegmentOffset += mdlhdr.length;
	pHdr->pAseqRefs = { dataseginfo.index, SegmentOffset };

	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pName));
	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pSkeleton));
	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pAseqRefs));

	for (int i = 0; i < pHdr->AseqRefCount; ++i)
	{
		RePak::RegisterGuidDescriptor(dataseginfo.index, SegmentOffset);
		SegmentOffset += i * 8;
	}

	RePak::AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	RePak::AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	RPakAssetEntry asset;
	asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::ARIG);
	asset.m_nVersion = 4;
	// i have literally no idea what these are
	asset.m_nPageEnd = dataseginfo.index + 1;
	asset.unk1 = 2;

	asset.m_nUsesCount = pHdr->AseqRefCount;

	assetEntries->push_back(asset);
}
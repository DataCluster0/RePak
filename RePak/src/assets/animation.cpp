#include "pch.h"
#include "Assets.h"
#include "assets/model.h"
#include "assets/animation.h"
#include "SeAsset.h"

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

	REQUIRE_FILE(skelFilePath);

	AnimRigHeader* pHdr = new AnimRigHeader();

	if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	{
		pHdr->AseqRefCount = mapEntry["animseqs"].Size();

		if (pHdr->AseqRefCount == 0)
			Error("invalid animseq count must not be 0 for arig '%s'\n", assetPath);
	}

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

	uint32_t NameDataSize = sAssetName.length() + 1;
	uint32_t NameAlignment = NameDataSize % 4;
	NameDataSize += NameAlignment;

	size_t DataBufferSize = NameDataSize + mdlhdr.length + (pHdr->AseqRefCount * sizeof(uint64_t));
	char* pDataBuf = new char[DataBufferSize];

	// go back to the beginning of the file to read all the data
	skelInput.seek(0);

	// write the skeleton data into the data buffer
	skelInput.getReader()->read(pDataBuf + NameDataSize, mdlhdr.length);
	skelInput.close();

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = RePak::CreateNewSegment(sizeof(AnimRigHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = RePak::CreateNewSegment(DataBufferSize, SF_CPU, 64);

	// write the rrig file path into the data buffer
	snprintf(pDataBuf, NameDataSize, "%s", sAssetName.c_str());

	rmem DataWriter(pDataBuf);
	//DataWriter.seek(NameDataSize + mdlhdr.length, rseekdir::beg);

	uint32_t SegmentOffset = 0;
	pHdr->pName = { dataseginfo.index, SegmentOffset };

	SegmentOffset += NameDataSize;
	pHdr->pSkeleton = { dataseginfo.index, SegmentOffset };

	SegmentOffset += mdlhdr.length;
	pHdr->pAseqRefs = { dataseginfo.index, SegmentOffset };

	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pName));
	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pSkeleton));
	RePak::RegisterDescriptor(subhdrinfo.index, offsetof(AnimRigHeader, pAseqRefs));

	for (int i = 0; i < pHdr->AseqRefCount; i++)
	{
		rapidjson::Value& Entry = mapEntry["animseqs"].GetArray()[i];

		if (!Entry.IsString())
			Error("invalid animseq entry for arig '%s'\n", assetPath);

		uint64_t Offset = SegmentOffset + (i * sizeof(uint64_t));

		DataWriter.write<uint64_t>(RTech::StringToGuid(Entry.GetStdString().c_str()), Offset);

		RePak::RegisterGuidDescriptor(dataseginfo.index, Offset);
	}

	RePak::AddRawDataBlock( { subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr } );
	RePak::AddRawDataBlock( { dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf } );

	RPakAssetEntry asset;
	asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::ARIG);

	asset.version = ARIG_VERSION;

	asset.pageEnd = dataseginfo.index + 1;

	asset.m_nUsesStartIdx = RePak::AddFileRelation(assetEntries->size());
	asset.relationCount = 1;

	asset.usesCount = pHdr->AseqRefCount;
	asset.unk1 = asset.usesCount + 1; // uses + 1

	assetEntries->push_back(asset);
}

void Assets::AddRseqAssets_v7(std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::string sAssetName = assetPath;
	sAssetName = sAssetName + ".rseq";

	std::string FilePath = g_sAssetsDir + sAssetName;

	Log("\n==============================\n");
	Log("Asset aseq -> '%s'\n", assetPath);

	while (!IsDebuggerPresent())
		::Sleep(100);


	auto SEAnimData = SEAsset::ReadAnimation(Utils::ChangeExtension(FilePath, ".seanim"), Utils::ChangeExtension(sAssetName,".seanim"));

	AnimHeader* pHdr = new AnimHeader();

	uint32_t NameDataSize = sAssetName.length() + 1;
	uint32_t NameAlignment = NameDataSize % 4;
	NameDataSize += NameAlignment;

	size_t DataBufferSize = NameDataSize;
	char* pDataBuf = new char[DataBufferSize];

	snprintf(pDataBuf, NameDataSize, "%s", sAssetName.c_str());


	//if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	//{
	//	//pHdr->AseqRefCount = mapEntry["animseqs"].Size();
	//
	//	//if (pHdr->AseqRefCount == 0)
	//		Error("invalid animseq count must not be 0 for arig '%s'\n", assetPath);
	//}

	//std::string sAssetName = assetPath;
	//std::string skelFilePath; //= g_sAssetsDir + sAssetName;
	//
	//REQUIRE_FILE(skelFilePath);

	//RePak::AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	//RePak::AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });
	//
	//RPakAssetEntry asset;
	//asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::ASEQ);
	//
	//asset.version = 7;
	//
	//asset.m_nPageEnd = dataseginfo.index + 1;
	//
	//asset.m_nUsesStartIdx = RePak::AddFileRelation(assetEntries->size());
	//asset.relationCount = 1;
	//
	//
	//asset.m_nUsesCount = pHdr->AseqRefCount;
	//asset.unk1 = asset.m_nUsesCount + 1; // uses + 1
	//
	//assetEntries->push_back(asset);
}

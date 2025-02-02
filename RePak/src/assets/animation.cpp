#include "pch.h"
#include "Assets.h"
#include "assets/model.h"
#include "assets/animation.h"

void Assets::AddRigAsset_stub(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Error("unsupported asset type 'rrig' for version 7");
}

void Assets::AddRseqAsset_stub(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Error("unsupported asset type 'aseq' for version 7");
}

void Assets::AddRigAsset_v4(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::vector<RPakGuidDescriptor> guids{};

	Log("\n==============================\n");
	Log("Asset arig -> '%s'\n", assetPath);

	std::string sAssetName = assetPath;
	std::string skelFilePath = g_sAssetsDir + sAssetName;

	REQUIRE_FILE(skelFilePath);

	if (pak->GetAssetByGuid(RTech::StringToGuid(sAssetName.c_str())) != nullptr)
	{
		Warning("Asset arig -> '%s' already exists skipping\n", assetPath);
		return;
	}

	AnimRigHeader* pHdr = new AnimRigHeader();

	std::vector<std::string> AseqList;
	if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	{
		pHdr->AseqRefCount = mapEntry["animseqs"].Size();

		if (pHdr->AseqRefCount == 0)
			Error("invalid animseq count must not be 0 for arig '%s'\n", assetPath);

		for (auto& entry : mapEntry["animseqs"].GetArray())
		{
			if (entry.IsString() && pak->GetAssetByGuid(RTech::StringToGuid(entry.GetString())) == nullptr)
				AseqList.push_back(entry.GetString());
		}

		AddRseqListAsset_v7(pak, assetEntries, g_sAssetsDir, AseqList);
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
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(AnimRigHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = pak->CreateNewSegment(DataBufferSize, SF_CPU, 64);

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

	pak->AddPointer(subhdrinfo.index, offsetof(AnimRigHeader, pName));
	pak->AddPointer(subhdrinfo.index, offsetof(AnimRigHeader, pSkeleton));
	pak->AddPointer(subhdrinfo.index, offsetof(AnimRigHeader, pAseqRefs));

	for (int i = 0; i < pHdr->AseqRefCount; i++)
	{
		rapidjson::Value& Entry = mapEntry["animseqs"].GetArray()[i];

		if (!Entry.IsString())
			Error("invalid animseq entry for arig '%s'\n", assetPath);

		uint64_t Offset = SegmentOffset + (i * sizeof(uint64_t));
		uint64_t GUID = RTech::StringToGuid(Entry.GetString());

		auto Asset = pak->GetAssetByGuid(GUID);
		if (Asset)
		{
			DataWriter.write<uint64_t>(GUID, Offset);
			pak->AddGuidDescriptor(&guids, dataseginfo.index, Offset);
			Asset->AddRelation(assetEntries->size());
		}
	}

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	RPakAssetEntry asset;
	asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::ARIG);

	asset.version = 4;

	asset.pageEnd = dataseginfo.index + 1;

	asset.relationCount = pHdr->AseqRefCount + 1;
	asset.unk1 = guids.size() + 1; // uses + 1

	asset.AddGuids(&guids);
	assetEntries->push_back(asset);
}

void Assets::AddRseqAsset_v7(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::string sAssetName = assetPath;
	std::string FilePath = g_sAssetsDir + sAssetName;

	REQUIRE_FILE(FilePath);

	uint64_t GUID = RTech::StringToGuid(sAssetName.c_str());

	Log("\n==============================\n");
	Log("Asset aseq -> 0x%llX -> '%s'\n", GUID, assetPath);

	AnimHeader* pHdr = new AnimHeader();

	uint32_t ActivityNameLen = 0;
	if (mapEntry.HasMember("activity") && mapEntry["activity"].IsString())
		ActivityNameLen = mapEntry["activity"].GetStdString().length() + 1;

	uint64_t EventDataSize = CalculateAllocSizeForEvents(mapEntry);
	uint32_t fileNameDataSize = sAssetName.length() + 1;
	uint32_t rseqFileSize = (uint32_t)Utils::GetFileSize(FilePath);
	uint32_t bufAlign = 4 - (fileNameDataSize + rseqFileSize + ActivityNameLen + EventDataSize) % 4;

	size_t DataBufferSize = fileNameDataSize + rseqFileSize + ActivityNameLen + EventDataSize + bufAlign;
	char* pDataBuf = new char[DataBufferSize];
	rmem dataBuf(pDataBuf);

	// write the aseq file path into the data buffer
	dataBuf.writestring(sAssetName, 0);

	BinaryIO rseqInput;
	rseqInput.open(FilePath, BinaryIOMode::Read);
	rseqInput.getReader()->read(pDataBuf + fileNameDataSize, rseqFileSize);
	rseqInput.close();

	std::vector<RPakGuidDescriptor> guids{};

	dataBuf.seek(fileNameDataSize, rseekdir::beg);
	mstudioseqdesc_t seqdesc = dataBuf.read<mstudioseqdesc_t>();

	if (mapEntry.HasMember("activity") && mapEntry["activity"].IsString())
	{
		std::string ActivityName = mapEntry["activity"].GetStdString();

		dataBuf.writestring(ActivityName, fileNameDataSize + rseqFileSize);
		seqdesc.szactivitynameindex = rseqFileSize;
	}

	AddCustomEventsToAseq(seqdesc, dataBuf, fileNameDataSize, rseqFileSize + ActivityNameLen, mapEntry);

	memcpy(pDataBuf + fileNameDataSize, &seqdesc, sizeof(mstudioseqdesc_t));

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(AnimHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = pak->CreateNewSegment(DataBufferSize, SF_CPU, 64);

	pHdr->pName = { dataseginfo.index, 0 };

	pHdr->pAnimation = { dataseginfo.index, fileNameDataSize };

	pak->AddPointer(subhdrinfo.index, offsetof(AnimHeader, pName));
	pak->AddPointer(subhdrinfo.index, offsetof(AnimHeader, pAnimation));

	dataBuf.seek(fileNameDataSize + seqdesc.autolayerindex, rseekdir::beg);
	// register autolayer aseq guids
	for (int i = 0; i < seqdesc.numautolayers; ++i)
	{
		dataBuf.seek(fileNameDataSize + seqdesc.autolayerindex + (i * sizeof(mstudioautolayer_t)), rseekdir::beg);

		mstudioautolayer_t* autolayer = dataBuf.get<mstudioautolayer_t>();

		if (autolayer->guid != 0)
			pak->AddGuidDescriptor(&guids, dataseginfo.index, dataBuf.getPosition() + offsetof(mstudioautolayer_t, guid));

		auto Asset = pak->GetAssetByGuid(autolayer->guid);

		if (Asset)
			Asset->AddRelation(assetEntries->size());
	}

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	RPakAssetEntry asset;
	asset.InitAsset(GUID, subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::ASEQ);

	asset.version = 7;

	asset.pageEnd = dataseginfo.index + 1;
	asset.unk1 = guids.size() + 1; // uses + 1

	asset.AddGuids(&guids);
	assetEntries->push_back(asset);
}

void Assets::AddRseqAssetList_stub(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, rapidjson::Value& mapEntry)
{
	Error("unsupported asset type 'aseqlist' for version 7");
}

void Assets::AddRseqAssetList_v7(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, rapidjson::Value& mapEntry)
{
	if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	{
		uint32_t count = mapEntry["animseqs"].Size();

		if (count == 0)
			Warning("invalid aseq count must not be 0 for aseq list\n");

		std::vector<std::string> AseqList{};

		for (auto& entry : mapEntry["animseqs"].GetArray())
		{
			if (entry.IsString() && pak->GetAssetByGuid(RTech::StringToGuid(entry.GetString())) == nullptr)
				AseqList.push_back(entry.GetStdString());
		}

		AddRseqListAsset_v7(pak, assetEntries, g_sAssetsDir, AseqList);
	}
}

void AddRseqListAsset_v7(CPakFile* pak, std::vector<RPakAssetEntry>* assetEntries, std::string sAssetsDir, std::vector<std::string> AseqList)
{
	// calculate databuf size
	size_t DataSize = 0;
	for (auto& AseqName : AseqList)
	{
		uint64_t GUID = RTech::StringToGuid(AseqName.c_str());

		Log("Asset aseq -> '%s'\n", AseqName.c_str());
		std::vector<RPakGuidDescriptor> guids{};

		std::string FilePath = sAssetsDir + AseqName;
		REQUIRE_FILE(FilePath);

		uint32_t fileNameDataSize = AseqName.length() + 1;
		uint32_t rseqFileSize = (uint32_t)Utils::GetFileSize(FilePath);
		uint32_t bufAlign = 4 - (fileNameDataSize + rseqFileSize) % 4;
		DataSize += fileNameDataSize + rseqFileSize + bufAlign;
	}

	size_t HeaderSize = sizeof(AnimHeader) * AseqList.size();
	char* pHeaderBuf = new char[HeaderSize];

	char* pDataBuf = new char[DataSize];
	rmem dataBuf(pDataBuf);

	// asset header
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(HeaderSize, SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = pak->CreateNewSegment(DataSize, SF_CPU, 64);

	uint32_t headeroffset = 0;
	uint32_t baseoffset = 0;
	for (auto& AseqName : AseqList)
	{
		std::vector<RPakGuidDescriptor> guids{};
		uint64_t GUID = RTech::StringToGuid(AseqName.c_str());
		std::string FilePath = sAssetsDir + AseqName;

		uint32_t fileNameDataSize = AseqName.length() + 1;
		uint32_t rseqFileSize = (uint32_t)Utils::GetFileSize(FilePath);
		uint32_t bufAlign = 4 - (fileNameDataSize + rseqFileSize) % 4;
		size_t DataBufferSize = fileNameDataSize + rseqFileSize + bufAlign;

		// write the aseq file path into the data buffer
		dataBuf.writestring(AseqName, baseoffset);

		BinaryIO rseqInput;
		rseqInput.open(FilePath, BinaryIOMode::Read);
		rseqInput.getReader()->read(pDataBuf + baseoffset + fileNameDataSize, DataBufferSize);
		rseqInput.close();

		dataBuf.seek(baseoffset + fileNameDataSize, rseekdir::beg);
		mstudioseqdesc_t seqdesc = dataBuf.read<mstudioseqdesc_t>();

		AnimHeader pHdr;

		pHdr.pName = { dataseginfo.index, baseoffset };
		pHdr.pAnimation = { dataseginfo.index, baseoffset + fileNameDataSize };

		pak->AddPointer(subhdrinfo.index, headeroffset + offsetof(AnimHeader, pName));
		pak->AddPointer(subhdrinfo.index, headeroffset + offsetof(AnimHeader, pAnimation));

		uint64_t AutoLayerOffset = baseoffset + fileNameDataSize + seqdesc.autolayerindex;

		dataBuf.seek(AutoLayerOffset, rseekdir::beg);

		// register autolayer aseq guids
		for (int i = 0; i < seqdesc.numautolayers; ++i)
		{
			dataBuf.seek(AutoLayerOffset + (i * sizeof(mstudioautolayer_t)), rseekdir::beg);

			mstudioautolayer_t* autolayer = dataBuf.get<mstudioautolayer_t>();

			if (autolayer->guid != 0)
				pak->AddGuidDescriptor(&guids, dataseginfo.index, dataBuf.getPosition() + offsetof(mstudioautolayer_t, guid));

			auto Asset = pak->GetAssetByGuid(autolayer->guid);
			if (Asset)
				Asset->AddRelation(assetEntries->size());
		}

		memcpy(pHeaderBuf + headeroffset, &pHdr, sizeof(AnimHeader));

		RPakAssetEntry asset;
		asset.InitAsset(GUID, subhdrinfo.index, headeroffset, sizeof(AnimHeader), -1, 0, -1, -1, (std::uint32_t)AssetType::ASEQ);

		asset.version = 7;

		asset.pageEnd = dataseginfo.index + 1;
		asset.unk1 = guids.size() + 1; // uses + 1

		asset.AddGuids(&guids);
		assetEntries->push_back(asset);

		headeroffset += sizeof(AnimHeader);
		baseoffset += DataBufferSize;
		dataBuf.seek(baseoffset, rseekdir::beg);
	}

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHeaderBuf });
	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });
}

uint64_t CalculateAllocSizeForEvents(rapidjson::Value& mapEntry)
{
	uint64_t datasize = 0;
	if (mapEntry.HasMember("events") && mapEntry["events"].IsArray() && mapEntry["events"].GetArray().Size() > 0)
	{
		auto EventEntries = mapEntry["events"].GetArray();
		datasize += EventEntries.Size() * sizeof(mstudioeventv54_t);

		for (auto& EventData : EventEntries)
		{
			std::string EventName = "AE_EMPTY";
			if (EventData.HasMember("event") && EventData["event"].IsString())
				EventName = EventData["event"].GetStdString();

			datasize += EventName.length() + 1;
		}
	}

	return datasize;
}

void AddCustomEventsToAseq(mstudioseqdesc_t& seqdesc, rmem& writer, uint64_t filenamedatasize, uint64_t eventoffset, rapidjson::Value& mapEntry)
{
	if (mapEntry.HasMember("events") && mapEntry["events"].IsArray() && mapEntry["events"].GetArray().Size() > 0)
	{
		auto EventEntries = mapEntry["events"].GetArray();

		// register new events index
		seqdesc.eventindex = eventoffset;
		seqdesc.numevents = EventEntries.Size();

		eventoffset += filenamedatasize;

		uint64_t stringpoolsize = 0;
		for (int i = 0; i < EventEntries.Size(); i++)
		{
			auto& Entry = EventEntries[i];

			std::string EventName = "AE_EMPTY";
			if (Entry.HasMember("event") && Entry["event"].IsString())
				EventName = Entry["event"].GetStdString();
			stringpoolsize += EventName.length() + 1;
		}

		writer.seek(eventoffset + (EventEntries.Size() * sizeof(mstudioeventv54_t)), rseekdir::beg);

		size_t evennameoffset = 0;
		for (int i = 0; i < EventEntries.Size(); i++)
		{
			uint64_t eventdataoffset = eventoffset + (i * sizeof(mstudioeventv54_t));

			writer.seek(eventdataoffset, rseekdir::beg);
			auto& Entry = EventEntries[i];

			mstudioeventv54_t NewEvent;
			if (Entry.HasMember("cycle") && Entry["cycle"].IsFloat())
				NewEvent.cycle = Entry["cycle"].GetFloat();

			if (Entry.HasMember("options") && Entry["options"].IsString() && Entry["options"].GetStringLength() < 256)
			{
				std::string entry = Entry["options"].GetStdString();

				for (int i = 0; i < 256; i++)
					NewEvent.options[i] = 0;

				snprintf((char*)NewEvent.options, 256, "%s", entry.c_str());
			}

			std::string EventName = "AE_EMPTY";
			if (Entry.HasMember("event") && Entry["event"].IsString())
				EventName = Entry["event"].GetStdString();

			NewEvent.type = 0x400;
			if (Entry.HasMember("type") && Entry["type"].IsString())
			{
				std::string type = Entry["type"].GetStdString();

				if (type == "new") NewEvent.type = 0x400;
				if (type == "old") NewEvent.type = 0x0;
			}
				

			uint64_t eventnameoffset = eventoffset + (EventEntries.Size() * sizeof(mstudioeventv54_t)) + evennameoffset;
			writer.writestring(EventName, eventnameoffset);

			NewEvent.szeventindex = eventnameoffset - eventdataoffset;

			writer.write(NewEvent, eventdataoffset);
			evennameoffset += EventName.length() + 1;
		}
	}
}
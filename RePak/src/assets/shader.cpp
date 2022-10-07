#include "pch.h"
#include "Assets.h"
#include "assets/shader.h"

void Assets::AddShaderSetAsset_stub(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Error("RPak version 7 (Titanfall 2) Shader Set !!!!NOT IMPLEMENTED!!!!");
}

void Assets::AddShaderSetAsset_v11(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::vector<RPakGuidDescriptor> guids{};

	//while (!IsDebuggerPresent())
	//	::Sleep(100);

	Log("\n==============================\n");
	Log("Asset shds -> '%s'\n", assetPath);

	std::string sAssetName = assetPath;

	Log("GUID -> 0x%llX\n", RTech::StringToGuid(sAssetName.c_str()));

	uint32_t NameDataSize = sAssetName.length() + 1;
	uint32_t NameAlignment = NameDataSize % 4;
	NameDataSize += NameAlignment;
	char* pDataBuf = new char[NameDataSize];

	ShaderSetHeader* pHdr = new ShaderSetHeader();

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(ShaderSetHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = pak->CreateNewSegment(NameDataSize, SF_CPU, 64);

	// Write Shader Set Name
	snprintf(pDataBuf, NameDataSize, "%s", sAssetName.c_str());

	pHdr->pName = { dataseginfo.index, 0 };

	pak->AddPointer(subhdrinfo.index, offsetof(ShaderSetHeader, pName));

	pHdr->TextureInputCount = 7;

	if (mapEntry.HasMember("textures") && mapEntry["textures"].IsInt() && mapEntry["textures"].GetInt() != 0)
		pHdr->TextureInputCount = (uint16_t)mapEntry["textures"].GetInt();

	pHdr->Count1 = pHdr->TextureInputCount;
	pHdr->Count3 = 3;

	uint32_t Relations = 0;
	if (mapEntry.HasMember("vertex") && mapEntry["vertex"].IsString())
	{
		std::string PixelShaderName = mapEntry["vertex"].GetStdString();

		pHdr->VertexShaderGUID = RTech::StringToGuid(PixelShaderName.c_str());

		RPakAssetEntry* shaderAsset = pak->GetAssetByGuid(pHdr->VertexShaderGUID, nullptr);

		if (shaderAsset)
		{
			shaderAsset->AddRelation(assetEntries->size());
			pak->AddGuidDescriptor(&guids, subhdrinfo.index, offsetof(ShaderSetHeader, VertexShaderGUID));
			Relations++;
		}
		else
			Warning("unable to find vertex shader '%s' for shaderset '%s' within the local assets\n", PixelShaderName.c_str(), sAssetName.c_str());
	}

	if (mapEntry.HasMember("pixel") && mapEntry["pixel"].IsString())
	{
		std::string PixelShaderName = mapEntry["pixel"].GetStdString();

		pHdr->PixelShaderGUID = RTech::StringToGuid(PixelShaderName.c_str());

		RPakAssetEntry* shaderAsset = pak->GetAssetByGuid(pHdr->PixelShaderGUID, nullptr);

		if (shaderAsset)
		{
			shaderAsset->AddRelation(assetEntries->size());
			pak->AddGuidDescriptor(&guids, subhdrinfo.index, offsetof(ShaderSetHeader, PixelShaderGUID));
			Relations++;
		}
		else
			Warning("unable to find pixel shader '%s' for shaderset '%s' within the local assets\n", PixelShaderName.c_str(), sAssetName.c_str());
	}

	// write shaderdata

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	RPakAssetEntry asset;
	asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, -1, -1, (std::uint32_t)AssetType::SHDS);

	asset.version = 11;

	asset.AddRelation(assetEntries->size());
	asset.AddRelation(assetEntries->size());

	asset.relationCount = Relations;
	asset.usesCount = Relations;

	asset.AddGuids(&guids);
	assetEntries->push_back(asset);
}

void Assets::AddShaderAsset_v12(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::string AssetName = assetPath;
	AssetName = AssetName;

	Log("\n==============================\n");
	Log("Asset shdr -> '%s'\n", AssetName.c_str());
	uint64_t GUID = RTech::StringToGuid(assetPath);

	Log("GUID -> 0x%llX\n", GUID);

	std::string shaderFilePath = g_sAssetsDir + AssetName + ".fxc";

	REQUIRE_FILE(shaderFilePath);

	ShaderHeader* pHdr = new ShaderHeader();

	// Read ShaderData
	BinaryIO shdrInput;
	shdrInput.open(shaderFilePath, BinaryIOMode::Read);

	shdrInput.seek(0, std::ios::end);

	uint32_t shdrFileSize = shdrInput.tell();

	shdrInput.seek(0);

	size_t DataBufferSize = sizeof(ShaderDataHeader) + shdrFileSize;
	char* pDataBuf = new char[DataBufferSize];

	// write the skeleton data into the data buffer
	shdrInput.getReader()->read(pDataBuf + sizeof(ShaderDataHeader), shdrFileSize);
	shdrInput.close();

	rmem writer(pDataBuf);

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(ShaderHeader), SF_HEAD, 16);

	// data segment
	_vseginfo_t dataseginfo = pak->CreateNewSegment(DataBufferSize, SF_CPU, 64);

	pHdr->DataSize = DataBufferSize;
	//pHdr->pName = { dataseginfo.index, 0 };

	ShaderDataHeader SDRData;
	SDRData.DataSize = sizeof(ShaderDataHeader) + shdrFileSize;
	SDRData.pData = { dataseginfo.index, 0 };

	// write shaderdata
	writer.write(SDRData, 0);

	pak->AddPointer(subhdrinfo.index,  offsetof(ShaderHeader, pName));
	pak->AddPointer(dataseginfo.index, offsetof(ShaderDataHeader, pData));

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });
	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	RPakAssetEntry asset;
	asset.InitAsset(GUID, subhdrinfo.index, 0, subhdrinfo.size, dataseginfo.index, 0, -1, -1, (std::uint32_t)AssetType::SHDR);

	asset.version = 12;

	assetEntries->push_back(asset);
}
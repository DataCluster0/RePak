#include "pch.h"
#include "Assets.h"
#include "assets/model.h"
#include "assets/animation.h"

void Assets::AddModelAsset_stub(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Log("\n==============================\n");
	Error("RPak version 7 (Titanfall 2) cannot contain models");
}

void Assets::AddModelAsset_v9(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Log("\n==============================\n");
	Log("Asset mdl_ -> '%s'\n", assetPath);

	std::string sAssetName = assetPath;

	ModelHeader* pHdr = new ModelHeader();

	std::string rmdlFilePath = g_sAssetsDir + sAssetName;

	// VG is a "fake" file extension that's used to store model streaming data (name came from the magic '0tVG')
	// this data is a combined mutated version of the data from .vtx and .vvd in regular source models
	std::string vgFilePath = Utils::ChangeExtension(rmdlFilePath, "vg");

	// fairly modified version of source .phy file data
	std::string phyFilePath = Utils::ChangeExtension(rmdlFilePath, "phy"); // optional (not used by all models)

	// add required files
	REQUIRE_FILE(rmdlFilePath);
	REQUIRE_FILE(vgFilePath);

	// begin rmdl input
	BinaryIO skelInput;
	skelInput.open(rmdlFilePath, BinaryIOMode::Read);

	studiohdr_t mdlhdr = skelInput.read<studiohdr_t>();

	if (mdlhdr.id != 0x54534449) // "IDST"
	{
		Warning("invalid file magic for model asset '%s'. expected %x, found %x. skipping asset...\n", sAssetName.c_str(), 0x54534449, mdlhdr.id);
		return;
	}

	if (mdlhdr.version != 54)
	{
		Warning("invalid version for model asset '%s'. expected %i, found %i. skipping asset...\n", sAssetName.c_str(), 54, mdlhdr.version);
		return;
	}

	uint32_t fileNameDataSize = sAssetName.length() + 1;

	char* pDataBuf = new char[fileNameDataSize + mdlhdr.length];

	// write the model file path into the data buffer
	snprintf(pDataBuf, fileNameDataSize, "%s", sAssetName.c_str());

	// go back to the beginning of the file to read all the data
	skelInput.seek(0);

	// write the skeleton data into the data buffer
	skelInput.getReader()->read(pDataBuf + fileNameDataSize, mdlhdr.length);
	skelInput.close();

	///--------------------
	// Add VG data
	BinaryIO vgInput;
	vgInput.open(vgFilePath, BinaryIOMode::Read);

	BasicRMDLVGHeader bvgh = vgInput.read<BasicRMDLVGHeader>();

	if (bvgh.magic != 0x47567430)
		Error("invalid vg file magic for model asset '%s'. expected %x, found %x\n", sAssetName.c_str(), 0x47567430, bvgh.magic);

	if (bvgh.version != 1)
		Error("invalid vg version for model asset '%s'. expected %i, found %i\n", sAssetName.c_str(), 1, bvgh.version);

	vgInput.seek(0, std::ios::end);

	uint32_t vgFileSize = vgInput.tell();
	char* pVGBuf = new char[vgFileSize];

	vgInput.seek(0);
	vgInput.getReader()->read(pVGBuf, vgFileSize);
	vgInput.close();

	//
	// Physics
	//
	char* phyBuf = nullptr;
	size_t phyFileSize = 0;

	if (mapEntry.HasMember("usePhysics") && mapEntry["usePhysics"].GetBool())
	{
		BinaryIO phyInput;
		phyInput.open(phyFilePath, BinaryIOMode::Read);

		phyInput.seek(0, std::ios::end);

		phyFileSize = phyInput.tell();

		phyBuf = new char[phyFileSize];

		phyInput.seek(0);
		phyInput.getReader()->read(phyBuf, phyFileSize);
		phyInput.close();
	}

	//
	// Anim Rigs
	//
	char* pAnimRigBuf = nullptr;

	if (mapEntry.HasMember("animrigs") && mapEntry["animrigs"].IsArray())
	{
		pHdr->animRigCount = mapEntry["animrigs"].Size();

		pAnimRigBuf = new char[mapEntry["animrigs"].Size() * sizeof(uint64_t)];

		rmem arigBuf(pAnimRigBuf);

		for (auto& it : mapEntry["animrigs"].GetArray())
		{
			if (it.IsString())
			{
				std::string rig = it.GetStdString();

				arigBuf.write<uint64_t>(RTech::StringToGuid(rig.c_str()));
			}
			else {
				Error("invalid animrig entry for model '%s'\n", assetPath);
			}
		}
	}

	char* pAnimSeqBuf = nullptr;

	if (mapEntry.HasMember("animseqs") && mapEntry["animseqs"].IsArray())
	{
		pHdr->animSeqCount = mapEntry["animseqs"].Size();

		pAnimSeqBuf = new char[mapEntry["animseqs"].Size() * sizeof(uint64_t)];

		rmem aseqBuf(pAnimSeqBuf);

		for (auto& it : mapEntry["animseqs"].GetArray())
		{
			if (it.IsString())
			{
				std::string seq = it.GetStdString();

				aseqBuf.write<uint64_t>(RTech::StringToGuid(seq.c_str()));
			}
			else {
				Error("invalid animseq entry for model '%s'\n", assetPath);
			}
		}
	}

	//
	// Starpak
	//
	std::string starpakPath = "paks/Win64/repak.starpak";

	if (mapEntry.HasMember("starpakPath") && mapEntry["starpakPath"].IsString())
		starpakPath = mapEntry["starpakPath"].GetStdString();

	// static name for now
	pak->AddStarpakReference(starpakPath);

	SRPkDataEntry de = pak->AddStarpakDataEntry({ 0, vgFileSize, (uint8_t*)pVGBuf });

	pHdr->alignedStreamingSize = de.m_nDataSize;

	// Segments
	// asset header
	_vseginfo_t subhdrinfo = pak->CreateNewSegment(sizeof(ModelHeader), SF_HEAD, 16);

	// data segment
	size_t DataSize = mdlhdr.length + fileNameDataSize + sizeof(pVGBuf);

	_vseginfo_t dataseginfo = pak->CreateNewSegment(DataSize, SF_CPU, 64);

	_vseginfo_t physeginfo;
	if (phyBuf)
		physeginfo = pak->CreateNewSegment(phyFileSize, SF_CPU, 64);

	_vseginfo_t arigseginfo;
	if (pAnimRigBuf)
		arigseginfo = pak->CreateNewSegment(pHdr->animRigCount * 8, SF_CPU, 64);

	_vseginfo_t aseqseginfo;
	if (pAnimSeqBuf)
		aseqseginfo = pak->CreateNewSegment(pHdr->animSeqCount * 8, SF_CPU, 64);

	pHdr->pName = { dataseginfo.index, 0 };

	pHdr->pRMDL = { dataseginfo.index, fileNameDataSize };

	pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pName));
	pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pRMDL));
	pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pStaticPropVtxCache));

	if (phyBuf)
	{
		pHdr->pPhyData = { physeginfo.index, 0 };
		pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pPhyData));
	}

	std::vector<RPakGuidDescriptor> guids{};

	// anim rigs are "includemodels" from source, containing a very stripped version of the main .rmdl data without any meshes
	// they are used for sharing anim data between models
	// file extension is ".rrig", rpak asset id is "arig"
	if (pAnimRigBuf)
	{
		pHdr->pAnimRigs = { arigseginfo.index, 0 };
		pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pAnimRigs));

		for (int i = 0; i < pHdr->animRigCount; ++i)
		{
			pak->AddGuidDescriptor(&guids, arigseginfo.index, sizeof(uint64_t) * i);
		}
	}

	if (pAnimSeqBuf)
	{
		pHdr->pAnimSeqs = { aseqseginfo.index, 0 };

		pak->AddPointer(subhdrinfo.index, offsetof(ModelHeader, pAnimSeqs));

		for (int i = 0; i < pHdr->animSeqCount; ++i)
		{
			pak->AddGuidDescriptor(&guids, aseqseginfo.index, sizeof(uint64_t) * i);
		}
	}

	rmem dataBuf(pDataBuf);
	// register guid relations on each of the model's material guids

	std::vector<uint64_t> MaterialOverrides;

	if (mapEntry.HasMember("materials"))
	{
		for (auto& it : mapEntry["materials"].GetArray())
		{
			if (it.IsString())
			{
				if (it.GetStdString() == "default")
					MaterialOverrides.push_back(-1);
				else if (it.GetStdString() == "")
					MaterialOverrides.push_back(RTech::StringToGuid("material/models/Weapons_R2/wingman_elite/wingman_elite_sknp.rpak"));
				else
					MaterialOverrides.push_back(RTech::StringToGuid(("material/" + it.GetStdString() + ".rpak").c_str()));
			}
			else if (it.IsUint64() && it.GetUint64() != 0x0)
			{
				MaterialOverrides.push_back(it.GetUint64());
			}
		}
	}

	Log("Materials -> %d\n", mdlhdr.numtextures);

	for (int i = 0; i < mdlhdr.numtextures; ++i)
	{
		dataBuf.seek(fileNameDataSize + mdlhdr.textureindex + (i * sizeof(materialref_t)), rseekdir::beg);

		materialref_t* material = dataBuf.get<materialref_t>();

		if (material->guid != 0 && MaterialOverrides.size() != 0 && i < MaterialOverrides.size())
		{
			if (MaterialOverrides[i] != -1)
				material->guid = MaterialOverrides[i];
		}

		Log("Material Guid -> 0x%llX\n", material->guid);

		if (material->guid != 0)
			pak->AddGuidDescriptor(&guids, dataseginfo.index, dataBuf.getPosition() + offsetof(materialref_t, guid));
	}

	pak->AddRawDataBlock({ subhdrinfo.index, subhdrinfo.size, (uint8_t*)pHdr });

	pak->AddRawDataBlock({ dataseginfo.index, dataseginfo.size, (uint8_t*)pDataBuf });

	uint32_t lastPageIdx = dataseginfo.index;

	if (phyBuf)
	{
		pak->AddRawDataBlock({ physeginfo.index, physeginfo.size, (uint8_t*)phyBuf });
		lastPageIdx = physeginfo.index;
	}

	if (pAnimRigBuf)
	{
		pak->AddRawDataBlock({ arigseginfo.index, arigseginfo.size, (uint8_t*)pAnimRigBuf });
		lastPageIdx = arigseginfo.index;
	}

	if (pAnimSeqBuf)
	{
		pak->AddRawDataBlock({ aseqseginfo.index, aseqseginfo.size, (uint8_t*)pAnimSeqBuf });
		lastPageIdx = aseqseginfo.index;
	}

	RPakAssetEntry asset;
	asset.InitAsset(RTech::StringToGuid(sAssetName.c_str()), subhdrinfo.index, 0, subhdrinfo.size, -1, 0, de.m_nOffset, -1, (std::uint32_t)AssetType::RMDL);
	asset.version = RMDL_VERSION;
	// i have literally no idea what these are
	asset.pageEnd = lastPageIdx + 1;

	// this needs to be fixed!!!
	//size_t fileRelationIdx = pak->AddFileRelation(assetEntries->size());
	//asset.usesStartIdx = fileRelationIdx;
	//asset.usesCount = mdlhdr.numtextures + pHdr->animRigCount;

	asset.usesCount = mdlhdr.numtextures + pHdr->animRigCount + pHdr->animSeqCount;
	asset.unk1 = asset.usesCount + 1;

	asset.AddGuids(&guids);
	assetEntries->push_back(asset);
}
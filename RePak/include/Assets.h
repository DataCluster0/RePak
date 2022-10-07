#pragma once

// asset versions
#define TXTR_VERSION 8
#define UIMG_VERSION 10
#define DTBL_VERSION 1
#define RMDL_VERSION 9
#define MATL_VERSION 15
#define ARIG_VERSION 4
#define STGS_VERSION 1

struct RPakFileBase;

struct RPakFileBase;

namespace Assets
{
	//void AddTextureAsset(std::vector<RPakAssetEntryV7>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	//void AddUIImageAsset_r2(std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddDataTableAsset_v0(RPakFileBase* pak,std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddModelAsset_stub(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddMaterialAsset_v12(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);

	void AddTextureAsset_v8(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddUIImageAsset_v10(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddDataTableAsset_v1(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddPatchAsset(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddModelAsset_v9(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddMaterialAsset_v15(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddSettingsLayoutAsset(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddSettingsAsset(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);

	void AddRigAsset_stub(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddRigAsset_v4(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddRseqAssets_v7(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);

	void AddShaderSetAsset_stub(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddShaderSetAsset_v11(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);
	void AddShaderAsset_v12(RPakFileBase* pak, std::vector<RPakAssetEntry>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry);

	extern std::string g_sAssetsDir;
};


#include "pch.h"
#include "Assets.h"
#include "SEAsset.h"

const static const char* SEAnimMagic = "SEAnim";

SEAsset::SEAnim_File_Ext* SEAsset::ReadAnimation(const std::string& Path, const std::string& AssetName)
{
	REQUIRE_FILE(Path);
	size_t SEAnimFileSize = Utils::GetFileSize(Path);

	SEAnim_File_Ext* SEAnimData = new SEAnim_File_Ext();

	// begin rmdl input
	size_t PageOffset = 0;

	BinaryIO Reader;
	Reader.open(Path, BinaryIOMode::Read);
	SEAnim_BasicFile File = Reader.read<SEAnim_BasicFile>();

	if (std::string(File.magic).compare("SEAnim") == -1)
	{
		Warning("invalid file magic for SEAnim '%s'. expected 'SEAnim' skipping asset...\n", AssetName.c_str());
		return nullptr;
	}

	if (File.version != 1)
	{
		Warning("invalid version for SEAnim '%s'. expected 1, found %i. skipping asset...\n", AssetName.c_str(), File.version);
		return nullptr;
	}

	SEAnimData->Bones.resize(File.header.boneCount);

	for (auto& entry : SEAnimData->Bones)
		entry.Bone = Reader.readString();

	for (uint32_t i = 0; i < File.header.boneAnimModifierCount; i++)
	{
		SEAnimData->Bones[i].BoneModifier.index = SizeRead(Reader, SEAnimData->Bones.size());
		SEAnimData->Bones[i].BoneModifier.animTypeOverride = Reader.read<SEAsset::SEAnim_AnimationType>();
	}

	for (auto& Bone : SEAnimData->Bones)
	{
		Bone.BoneData.flags = Reader.read<SEAsset::SEAnim_BoneFlags>();

		if (File.header.dataPresenceFlags & SEANIM_BONE_LOC)
		{
			Bone.BoneData.cords.resize(SEAsset::SizeRead(Reader, File.header.frameCount));

			for (auto& cord : Bone.BoneData.cords)
			{
				cord.frame = SizeRead(Reader, File.header.frameCount);
				cord.loc = Reader.read<Vector3>();
			}
		}

		if (File.header.dataPresenceFlags & SEANIM_BONE_ROT)
		{
			Bone.BoneData.quats.resize(SEAsset::SizeRead(Reader, File.header.frameCount));

			for (auto& quat : Bone.BoneData.quats)
			{
				quat.frame = SizeRead(Reader, File.header.frameCount);
				quat.rot = Reader.read<Quaternion>();
			}
		}

		if (File.header.dataPresenceFlags & SEANIM_BONE_SCALE)
		{
			Bone.BoneData.scales.resize(SizeRead(Reader, File.header.frameCount));

			for (auto& scale : Bone.BoneData.scales)
			{
				scale.frame = SizeRead(Reader, File.header.frameCount);
				scale.scale = Reader.read<Vector3>();
			}
		}

		if (File.header.dataPresenceFlags & SEANIM_PRESENCE_NOTE)
		{
			for (uint32_t i = 0; i < File.header.noteCount; i++)
				SEAnimData->Notes[i] = { SizeRead(Reader, File.header.frameCount) , Reader.readString() };
		}

		if (File.header.dataPresenceFlags & SEANIM_PRESENCE_CUSTOM)
		{
			SEAnimData->customDataBlockSize = Reader.read<uint32_t>();
			Reader.getReader()->read(SEAnimData->customDataBlockBuf, SEAnimData->customDataBlockSize);
		}
	}

	Reader.close();

	return SEAnimData;
}

uint32_t SEAsset::SizeRead(BinaryIO& Reader, uint32_t Count)
{
	if (Count <= 0xFF)
		return Reader.read<uint8_t>();
	else if (Count <= 0xFFFF)
		return Reader.read<uint16_t>();
	else if (Count <= 0xFFFFFFFF)
		return Reader.read<uint32_t>();

	return Reader.read<uint16_t>();
};

bool SEAsset::ConvertAnimationToRseq(const std::string& Path)
{
	return true;
}

std::string SEAsset::AnimationExtension()
{
	return ".seanim";
}
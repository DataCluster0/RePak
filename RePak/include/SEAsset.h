#pragma once

#include <cstdint>
#include <string>

class SEAsset
{
public:

	// Specifies how the data is interpreted by the importer
	enum SEAnim_AnimationType : uint8_t
	{
		// Animation translations are set to this exact value each frame
		SEANIM_TYPE_ABSOLUTE,
		// This animation is applied to existing animation data in the scene
		SEANIM_TYPE_ADDITIVE,
		// Animation translations are based on rest position in scene
		SEANIM_TYPE_RELATIVE,
		// This animation is relative and contains delta data (Whole model movement) Delta tag name must be set!
		SEANIM_TYPE_DELTA
	};

	// Specifies the data present for each frame of every bone
	enum SEAnim_DataPresenceFlags : uint8_t
	{
		// These describe what type of keyframe data is present for the bones
		SEANIM_BONE_LOC = 1 << 0,
		SEANIM_BONE_ROT = 1 << 1,
		SEANIM_BONE_SCALE = 1 << 2,

		// If any of the above flags are set, then bone keyframe data is present, thus this comparing against this mask will return true
		SEANIM_PRESENCE_BONE = SEANIM_BONE_LOC | SEANIM_BONE_ROT | SEANIM_BONE_SCALE,

		// The file contains notetrack data
		SEANIM_PRESENCE_NOTE = 1 << 6,
		// The file contains a custom data block
		SEANIM_PRESENCE_CUSTOM = 1 << 7,
	};

	enum SEAnim_BoneFlags : uint8_t
	{
		SEANIM_BONE_NORMAL = 0,
		SEANIM_BONE_COSMETIC = 1 << 0
	};

	enum SEAnim_PropertyFlags : uint8_t
	{
		SEANIM_PRECISION_HIGH = 1 << 0 // Use double precision floating point vectors instead of single precision
		//RESERVED_1		= 1 << 1, // ALWAYS FALSE
		//RESERVED_2		= 1 << 2, // ALWAYS FALSE
		//RESERVED_3		= 1 << 3, // ALWAYS FALSE
		//RESERVED_4		= 1 << 4, // ALWAYS FALSE
		//RESERVED_5		= 1 << 5, // ALWAYS FALSE
		//RESERVED_6		= 1 << 6, // ALWAYS FALSE
		//RESERVED_7		= 1 << 7, // ALWAYS FALSE
	};

	enum SEAnim_Flags : uint8_t
	{
		SEANIM_LOOPED = 1 << 0, // The animation is a looping animation
								//RESERVED_0		= 1 << 1, // ALWAYS FALSE
								//RESERVED_1		= 1 << 2, // ALWAYS FALSE
								//RESERVED_2		= 1 << 3, // ALWAYS FALSE
								//RESERVED_3		= 1 << 4, // ALWAYS FALSE
								//RESERVED_4		= 1 << 5, // ALWAYS FALSE
								//RESERVED_5		= 1 << 6, // ALWAYS FALSE
								//RESERVED_6		= 1 << 7, // ALWAYS FALSE
	};

	typedef struct SEAnim_BoneAnimModifier_s
	{
		uint32_t index; // Index of the bone
		SEAnim_AnimationType animTypeOverride; // AnimType to use for that bone, and its children recursively
	} SEAnim_BoneAnimModifier_t;

	typedef struct SEAnim_BoneLocData_s
	{
		uint32_t frame;
		Vector3 loc;
	} SEAnim_BoneLocData_t;

	typedef struct SEAnim_BoneRotData_s
	{
		uint32_t frame;
		Quaternion rot;
	} SEAnim_BoneRotData_t;

	typedef struct SEAnim_BoneScaleData_s
	{
		uint32_t frame;
		Vector3 scale; // 1.0 is the default scale, 2.0 is twice as big, and 0.5 is half size
	} SEAnim_BoneScaleData_t;

	//
	// Pooled String Support Will Be Added Later Maybe
	//
	typedef struct SEAnim_Note_s
	{
		uint32_t frame;
		char* name;
	} SEAnim_Note_t;

	typedef struct SEAnim_Header_s
	{
		// Contains the size of the header block in bytes, any extra data is ignored
		uint16_t headerSize; //currently 0x1C

		// The type of animation data that is stored, matches an SEANIM_TYPE
		SEAnim_AnimationType animType;

		// Bitwise flags that define the properties for the animation itself
		SEAnim_Flags animFlags;

		// Bitwise flags that define which data blocks are present, and properties for those data blocks
		SEAnim_DataPresenceFlags dataPresenceFlags;

		// Bitwise flags containing property information pertaining regarding the data in the file
		SEAnim_PropertyFlags dataPropertyFlags;

		// RESERVED - Should be 0
		uint8_t reserved1[2];

		float framerate;

		// frameCount describes the length of the animation (in frames)
		// It is used to determine the size of frame_t
		// This should be equal to the largest frame number in the anim (Including keys & notes) plus 1
		// Ex: An anim with keys on frame 0, 5 and 8 - frameCount would be 9
		//     An anim with keys on frame 4, 7, and 14 - frameCount would be 15
		uint32_t frameCount;

		// Is 0 if ( presenceFlags & SEANIM_PRESENCE_BONE ) is false
		uint32_t boneCount;
		uint8_t boneAnimModifierCount; // The number of animType modifier bones

		uint8_t reserved2[3];

		// Is 0 if ( presenceFlags & SEANIM_PRESENCE_NOTE ) is false
		uint32_t noteCount;
	} SEAnim_Header_t;

	struct SEAnim_BasicFile
	{
		char magic[6];		// 'SEAnim'
		uint16_t version;	// The file version - the current version is 0x1
		SEAnim_Header_t header;
	};

	struct SEAnim_BoneData
	{
		SEAnim_BoneFlags flags;

		std::vector<SEAnim_BoneLocData_t> cords{};
		std::vector<SEAnim_BoneRotData_t> quats{};
		std::vector<SEAnim_BoneScaleData_t> scales{};
	};

	struct SEAnim_Bone
	{
		std::string Bone;
		SEAnim_BoneAnimModifier_t BoneModifier;
		SEAnim_BoneData BoneData;
	};

	struct SEAnim_Note
	{
		uint32_t frame;
		std::string name;
	};

	struct SEAnim_File_Ext
	{
		SEAnim_Header_t hdr;

		std::vector<SEAnim_Bone> Bones{};

		std::vector<SEAnim_Note> Notes{};

		uint32_t customDataBlockSize = 0;
		char* customDataBlockBuf = 0;
	};

	SEAsset() = default;
	~SEAsset() = default;

	// Imports the given animation to the provided path.
	static bool ConvertAnimationToRseq(const std::string& Path);

	static SEAnim_File_Ext* ReadAnimation(const std::string& Path, const std::string& AssetName);

	static uint32_t SizeRead(BinaryIO& Reader, uint32_t Count);

	// Gets the file extension for this exporters animation format.
	static std::string AnimationExtension();
};

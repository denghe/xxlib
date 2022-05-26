#pragma once
// 这段 ebml 解析抄自 github.com/rubu/WebM-Player
#include <string>
#include <list>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdint>
#include <type_traits>
#include <algorithm>
#include <inttypes.h>	// PRIu64
#include <memory>
#include <stack>
#include <tuple>

// 这段大小尾宏抄自 github.com/lordoffox/adata/blob/master/cpp/adata.hpp
#ifndef _WIN32
#include <arpa/inet.h>  /* __BYTE_ORDER */
#endif
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#    if __BYTE_ORDER == __LITTLE_ENDIAN
#        define __LITTLE_ENDIAN__
#    elif __BYTE_ORDER == __BIG_ENDIAN
#        define __BIG_ENDIAN__
#    elif _WIN32
#        define __LITTLE_ENDIAN__
#    endif
#endif

// 从大尾数据流读出一个定长数字
template<typename NumberType, size_t size = sizeof(NumberType), typename ENABLED = std::enable_if_t<std::is_arithmetic_v<NumberType> && size <= 8>>
NumberType ReadBigEndianNumber(uint8_t const* const& buf) {
	NumberType n;
#ifdef __LITTLE_ENDIAN__
	if constexpr (size > 0) ((uint8_t*)& n)[size - 1] = buf[0];
	if constexpr (size > 1) ((uint8_t*)& n)[size - 2] = buf[1];
	if constexpr (size > 2) ((uint8_t*)& n)[size - 3] = buf[2];
	if constexpr (size > 3) ((uint8_t*)& n)[size - 4] = buf[3];
	if constexpr (size > 4) ((uint8_t*)& n)[size - 5] = buf[4];
	if constexpr (size > 5) ((uint8_t*)& n)[size - 6] = buf[5];
	if constexpr (size > 6) ((uint8_t*)& n)[size - 7] = buf[6];
	if constexpr (size > 7) ((uint8_t*)& n)[size - 8] = buf[7];
#else
	memcpy(&n, buf, size);
#endif
	return n;
}


enum class EbmlElementId
{
    EBML = 0x1A45DFA3,
    EBMLVersion = 0x4286,
    EBMLReadVersion = 0x42F7,
    EBMLMaxIDLength = 0x42F2,
    EBMLMaxSizeLength = 0x42F3,
    DocType = 0x4282,
    DocTypeVersion = 0x4287,
    DocTypeReadVersion = 0x4285,
    Void = 0xEC,
    CRC32 = 0xBF,
    SignatureSlot = 0x1B538667,
    SignatureAlgo = 0x7E8A,
    SignatureHash = 0x7E9A,
    SignaturePublicKey = 0x7EA5,
    Signature = 0x7EB5,
    SignatureElements = 0x7E5B,
    SignatureElementList = 0x7E7B,
    SignedElement = 0x6532,
    Segment = 0x18538067,
    SeekHead = 0x114D9B74,
    Seek = 0x4DBB,
    SeekID = 0x53AB,
    SeekPosition = 0x53AC,
    Info = 0x1549A966,
    SegmentUID = 0x73A4,
    SegmentFilename = 0x7384,
    PrevUID = 0x3CB923,
    PrevFilename = 0x3C83AB,
    NextUID = 0x3EB923,
    NextFilename = 0x3E83BB,
    SegmentFamily = 0x4444,
    ChapterTranslate = 0x6924,
    ChapterTranslateEditionUID = 0x69FC,
    ChapterTranslateCodec = 0x69BF,
    ChapterTranslateID = 0x69A5,
    TimecodeScale = 0x2AD7B1,
    Duration = 0x4489,
    DateUTC = 0x4461,
    Title = 0x7BA9,
    MuxingApp = 0x4D80,
    WritingApp = 0x5741,
    Cluster = 0x1F43B675,
    Timecode = 0xE7,
    SilentTracks = 0x5854,
    SilentTrackNumber = 0x58D7,
    Position = 0xA7,
    PrevSize = 0xAB,
    SimpleBlock = 0xA3,
    BlockGroup = 0xA0,
    Block = 0xA1,
    BlockVirtual = 0xA2,
    BlockAdditions = 0x75A1,
    BlockMore = 0xA6,
    BlockAddID = 0xEE,
    BlockAdditional = 0xA5,
    BlockDuration = 0x9B,
    ReferencePriority = 0xFA,
    ReferenceBlock = 0xFB,
    ReferenceVirtual = 0xFD,
    CodecState = 0xA4,
    DiscardPadding = 0x75A2,
    Slices = 0x8E,
    TimeSlice = 0xE8,
    LaceNumber = 0xCC,
    FrameNumber = 0xCD,
    BlockAdditionID = 0xCB,
    Delay = 0xCE,
    SliceDuration = 0xCF,
    ReferenceFrame = 0xC8,
    ReferenceOffset = 0xC9,
    ReferenceTimeCode = 0xCA,
    EncryptedBlock = 0xAF,
    Tracks = 0x1654AE6B,
    TrackEntry = 0xAE,
    TrackNumber = 0xD7,
    TrackUID = 0x73C5,
    TrackType = 0x83,
    FlagEnabled = 0xB9,
    FlagDefault = 0x88,
    FlagForced = 0x55AA,
    FlagLacing = 0x9C,
    MinCache = 0x6DE7,
    MaxCache = 0x6DF8,
    DefaultDuration = 0x23E383,
    DefaultDecodedFieldDuration = 0x234E7A,
    TrackTimecodeScale = 0x23314F,
    TrackOffset = 0x537F,
    MaxBlockAdditionID = 0x55EE,
    Name = 0x536E,
    Language = 0x22B59C,
    CodecID = 0x86,
    CodecPrivate = 0x63A2,
    CodecName = 0x258688,
    AttachmentLink = 0x7446,
    CodecSettings = 0x3A9697,
    CodecInfoURL = 0x3B4040,
    CodecDownloadURL = 0x26B240,
    CodecDecodeAll = 0xAA,
    TrackOverlay = 0x6FAB,
    CodecDelay = 0x56AA,
    SeekPreRoll = 0x56BB,
    TrackTranslate = 0x6624,
    TrackTranslateEditionUID = 0x66FC,
    TrackTranslateCodec = 0x66BF,
    TrackTranslateTrackID = 0x66A5,
    Video = 0xE0,
    FlagInterlaced = 0x9A,
    FieldOrder = 0x9D,
    StereoMode = 0x53B8,
    AlphaMode = 0x53C0,
    OldStereoMode = 0x53B9,
    PixelWidth = 0xB0,
    PixelHeight = 0xBA,
    PixelCropBottom = 0x54AA,
    PixelCropTop = 0x54BB,
    PixelCropLeft = 0x54CC,
    PixelCropRight = 0x54DD,
    DisplayWidth = 0x54B0,
    DisplayHeight = 0x54BA,
    DisplayUnit = 0x54B2,
    AspectRatioType = 0x54B3,
    ColourSpace = 0x2EB524,
    GammaValue = 0x2FB523,
    FrameRate = 0x2383E3,
    Colour = 0x55B0,
    MatrixCoefficients = 0x55B1,
    BitsPerChannel = 0x55B2,
    ChromaSubsamplingHorz = 0x55B3,
    ChromaSubsamplingVert = 0x55B4,
    CbSubsamplingHorz = 0x55B5,
    CbSubsamplingVert = 0x55B6,
    ChromaSitingHorz = 0x55B7,
    ChromaSitingVert = 0x55B8,
    Range = 0x55B9,
    TransferCharacteristics = 0x55BA,
    Primaries = 0x55BB,
    MaxCLL = 0x55BC,
    MaxFALL = 0x55BD,
    MasteringMetadata = 0x55D0,
    PrimaryRChromaticityX = 0x55D1,
    PrimaryRChromaticityY = 0x55D2,
    PrimaryGChromaticityX = 0x55D3,
    PrimaryGChromaticityY = 0x55D4,
    PrimaryBChromaticityX = 0x55D5,
    PrimaryBChromaticityY = 0x55D6,
    WhitePointChromaticityX = 0x55D7,
    WhitePointChromaticityY = 0x55D8,
    LuminanceMax = 0x55D9,
    LuminanceMin = 0x55DA,

		// 从 webm_parser id.h 搬运过来
		kProjection = 0x7670,
		kProjectionType = 0x7671,
		kProjectionPrivate = 0x7672,
		kProjectionPoseYaw = 0x7673,
		kProjectionPosePitch = 0x7674,
		kProjectionPoseRoll = 0x7675,

    Audio = 0xE1,
    SamplingFrequency = 0xB5,
    OutputSamplingFrequency = 0x78B5,
    Channels = 0x9F,
    ChannelPositions = 0x7D7B,
    BitDepth = 0x6264,
    TrackOperation = 0xE2,
    TrackCombinePlanes = 0xE3,
    TrackPlane = 0xE4,
    TrackPlaneUID = 0xE5,
    TrackPlaneType = 0xE6,
    TrackJoinBlocks = 0xE9,
    TrackJoinUID = 0xED,
    TrickTrackUID = 0xC0,
    TrickTrackSegmentUID = 0xC1,
    TrickTrackFlag = 0xC6,
    TrickMasterTrackUID = 0xC7,
    TrickMasterTrackSegmentUID = 0xC4,
    ContentEncodings = 0x6D80,
    ContentEncoding = 0x6240,
    ContentEncodingOrder = 0x5031,
    ContentEncodingScope = 0x5032,
    ContentEncodingType = 0x5033,
    ContentCompression = 0x5034,
    ContentCompAlgo = 0x4254,
    ContentCompSettings = 0x4255,
    ContentEncryption = 0x5035,
    ContentEncAlgo = 0x47E1,
    ContentEncKeyID = 0x47E2,
    ContentSignature = 0x47E3,
    ContentSigKeyID = 0x47E4,
    ContentSigAlgo = 0x47E5,
    ContentSigHashAlgo = 0x47E6,

		// 从 webm_parser id.h 搬运过来
		kContentEncAesSettings = 0x47E7,
		kAesSettingsCipherMode = 0x47E8,

    Cues = 0x1C53BB6B,
    CuePoint = 0xBB,
    CueTime = 0xB3,
    CueTrackPositions = 0xB7,
    CueTrack = 0xF7,
    CueClusterPosition = 0xF1,
    CueRelativePosition = 0xF0,
    CueDuration = 0xB2,
    CueBlockNumber = 0x5378,
    CueCodecState = 0xEA,
    CueReference = 0xDB,
    CueRefTime = 0x96,
    CueRefCluster = 0x97,
    CueRefNumber = 0x535F,
    CueRefCodecState = 0xEB,
    Attachments = 0x1941A469,
    AttachedFile = 0x61A7,
    FileDescription = 0x467E,
    FileName = 0x466E,
    FileMimeType = 0x4660,
    FileData = 0x465C,
    FileUID = 0x46AE,
    FileReferral = 0x4675,
    FileUsedStartTime = 0x4661,
    FileUsedEndTime = 0x4662,
    Chapters = 0x1043A770,
    EditionEntry = 0x45B9,
    EditionUID = 0x45BC,
    EditionFlagHidden = 0x45BD,
    EditionFlagDefault = 0x45DB,
    EditionFlagOrdered = 0x45DD,
    ChapterAtom = 0xB6,
    ChapterUID = 0x73C4,
    ChapterStringUID = 0x5654,
    ChapterTimeStart = 0x91,
    ChapterTimeEnd = 0x92,
    ChapterFlagHidden = 0x98,
    ChapterFlagEnabled = 0x4598,
    ChapterSegmentUID = 0x6E67,
    ChapterSegmentEditionUID = 0x6EBC,
    ChapterPhysicalEquiv = 0x63C3,
    ChapterTrack = 0x8F,
    ChapterTrackNumber = 0x89,
    ChapterDisplay = 0x80,
    ChapString = 0x85,
    ChapLanguage = 0x437C,
    ChapCountry = 0x437E,
    ChapProcess = 0x6944,
    ChapProcessCodecID = 0x6955,
    ChapProcessPrivate = 0x450D,
    ChapProcessCommand = 0x6911,
    ChapProcessTime = 0x6922,
    ChapProcessData = 0x6933,
    Tags = 0x1254C367,
    Tag = 0x7373,
    Targets = 0x63C0,
    TargetTypeValue = 0x68CA,
    TargetType = 0x63CA,
    TagTrackUID = 0x63C5,
    TagEditionUID = 0x63C9,
    TagChapterUID = 0x63C4,
    TagAttachmentUID = 0x63C6,
    SimpleTag = 0x67C8,
    TagName = 0x45A3,
    TagLanguage = 0x447A,
    TagDefault = 0x4484,
    TagString = 0x4487,
    TagBinary = 0x4485
};

enum class EbmlElementType
{
	Unknown,
	Master,
	UnsignedInteger,
	SignedInteger,
	String,
	Utf8String,
	Binary,
	Float,
	Date
};

struct EbmlElementDescriptor
{
    EbmlElementId id_;
    EbmlElementType type_;
    int level_;
    const char* name_;
};

class EbmlElement
{
public:
	EbmlElement(EbmlElementId const& id, EbmlElementType const& type, uint64_t const& size, size_t const& id_length, size_t const& size_length, uint8_t* const& data = nullptr);

	void add_child(EbmlElement child);
	void print(uint32_t level);
	uint64_t size();
	uint64_t element_size();
	void calculate_size();
	EbmlElementId id() const;
	const std::list<EbmlElement>& children() const;
	const std::string value() const;
	const uint8_t* data() const;
	uint64_t size() const;
    EbmlElementType type() const;
    const EbmlElement* first_child(EbmlElementId id) const;
    std::vector<const EbmlElement*> children(EbmlElementId id) const;
    void children(std::vector<const EbmlElement*>& children, EbmlElementId id) const;
    std::vector<const EbmlElement*> descendants(EbmlElementId id) const;
    void descendants(std::vector<const EbmlElement*>& descendants, EbmlElementId id) const;

	std::list<EbmlElement>::const_iterator FindChildById(EbmlElementId const& id) const;
	std::list<EbmlElement>::const_iterator FindNextChildById(std::list<EbmlElement>::const_iterator const& iter, EbmlElementId const& id) const;

private:
	EbmlElementId id_;
	EbmlElementType type_;
	uint64_t size_;
	size_t id_length_;
	size_t size_length_;
	std::list<EbmlElement> children_;
	uint8_t* data_;
};

class EbmlDocument
{
public:
	EbmlDocument(std::unique_ptr<uint8_t[]> storage = nullptr, std::list<EbmlElement> elements = std::list<EbmlElement>());

	const std::list<EbmlElement>& elements() const;

	std::list<EbmlElement>::const_iterator FindChildById(EbmlElementId const& id) const;
	std::list<EbmlElement>::const_iterator FindNextChildById(std::list<EbmlElement>::const_iterator const& iter, EbmlElementId const& id) const;

private:
	std::unique_ptr<uint8_t[]> storage_;
	std::list<EbmlElement> elements_;
};




size_t get_ebml_element_id_length(uint8_t first_byte_of_element_id);
std::string get_ebml_element_name(EbmlElementId ebml_element_id);
EbmlElementDescriptor* get_ebml_element_descriptor(EbmlElementId ebml_element_id);
EbmlElementType get_ebml_element_type(EbmlElementId ebml_element_id);
int get_ebml_element_level(EbmlElementId ebml_element_id);
EbmlElementId read_ebml_element_id(uint8_t* data, size_t& available_data_length, size_t& ebml_element_id_length);
size_t get_ebml_element_size_length(const uint8_t* data, size_t available_data_length);
uint64_t get_ebml_element_size(const uint8_t* data, size_t available_data_length, size_t& ebml_element_size_length);
EbmlElementId read_ebml_element_id(uint8_t* data, size_t& available_data_length, size_t& ebml_element_id_length);
std::string get_ebml_element_value(EbmlElementId id, EbmlElementType type, uint8_t* data, uint64_t size);

EbmlDocument parse_ebml_file(std::unique_ptr<uint8_t[]>&& ebml_file_contents, size_t const& file_size, bool const& verbose = false);





inline static EbmlElementDescriptor ebml_element_descriptors[] =
{
	{ EbmlElementId::EBML, EbmlElementType::Master, 0, "EBML" },
	{ EbmlElementId::EBMLVersion, EbmlElementType::UnsignedInteger, 1, "EBMLVersion" },
	{ EbmlElementId::EBMLReadVersion, EbmlElementType::UnsignedInteger, 1, "EBMLReadVersion" },
	{ EbmlElementId::EBMLMaxIDLength, EbmlElementType::UnsignedInteger, 1, "EBMLMaxIDLength" },
	{ EbmlElementId::EBMLMaxSizeLength, EbmlElementType::UnsignedInteger, 1, "EBMLMaxSizeLength" },
	{ EbmlElementId::DocType, EbmlElementType::String, 1, "DocType" },
	{ EbmlElementId::DocTypeVersion, EbmlElementType::UnsignedInteger, 1, "DocTypeVersion" },
	{ EbmlElementId::DocTypeReadVersion, EbmlElementType::UnsignedInteger, 1, "DocTypeReadVersion" },
	{ EbmlElementId::Void, EbmlElementType::Binary, -1, "Void" },
	{ EbmlElementId::CRC32, EbmlElementType::Binary, -1, "CRC-32" },
	{ EbmlElementId::SignatureSlot, EbmlElementType::Master, -1, "SignatureSlot" },
	{ EbmlElementId::SignatureAlgo, EbmlElementType::UnsignedInteger, 1, "SignatureAlgo" },
	{ EbmlElementId::SignatureHash, EbmlElementType::UnsignedInteger, 1, "SignatureHash" },
	{ EbmlElementId::SignaturePublicKey, EbmlElementType::Binary, 1, "SignaturePublicKey" },
	{ EbmlElementId::Signature, EbmlElementType::Binary, 1, "Signature" },
	{ EbmlElementId::SignatureElements, EbmlElementType::Master, 1, "SignatureElements" },
	{ EbmlElementId::SignatureElementList, EbmlElementType::Master, 2, "SignatureElementList" },
	{ EbmlElementId::SignedElement, EbmlElementType::Binary, 3, "SignedElement" },
	{ EbmlElementId::Segment, EbmlElementType::Master, 0, "Segment" },
	{ EbmlElementId::SeekHead, EbmlElementType::Master, 1, "SeekHead" },
	{ EbmlElementId::Seek, EbmlElementType::Master, 2, "Seek" },
	{ EbmlElementId::SeekID, EbmlElementType::Binary, 3, "SeekID" },
	{ EbmlElementId::SeekPosition, EbmlElementType::UnsignedInteger, 3, "SeekPosition" },
	{ EbmlElementId::Info, EbmlElementType::Master, 1, "Info" },
	{ EbmlElementId::SegmentUID, EbmlElementType::Binary, 2, "SegmentUID" },
	{ EbmlElementId::SegmentFilename, EbmlElementType::Utf8String, 2, "SegmentFilename" },
	{ EbmlElementId::PrevUID, EbmlElementType::Binary, 2, "PrevUID" },
	{ EbmlElementId::PrevFilename, EbmlElementType::Utf8String, 2, "PrevFilename" },
	{ EbmlElementId::NextUID, EbmlElementType::Binary, 2, "NextUID" },
	{ EbmlElementId::NextFilename, EbmlElementType::Utf8String, 2, "NextFilename" },
	{ EbmlElementId::SegmentFamily, EbmlElementType::Binary, 2, "SegmentFamily" },
	{ EbmlElementId::ChapterTranslate, EbmlElementType::Master, 2, "ChapterTranslate" },
	{ EbmlElementId::ChapterTranslateEditionUID, EbmlElementType::UnsignedInteger, 3, "ChapterTranslateEditionUID" },
	{ EbmlElementId::ChapterTranslateCodec, EbmlElementType::UnsignedInteger, 3, "ChapterTranslateCodec" },
	{ EbmlElementId::ChapterTranslateID, EbmlElementType::Binary, 3, "ChapterTranslateID" },
	{ EbmlElementId::TimecodeScale, EbmlElementType::UnsignedInteger, 2, "TimecodeScale" },
	{ EbmlElementId::Duration, EbmlElementType::Float, 2, "Duration" },
	{ EbmlElementId::DateUTC, EbmlElementType::Date, 2, "DateUTC" },
	{ EbmlElementId::Title, EbmlElementType::Utf8String, 2, "Title" },
	{ EbmlElementId::MuxingApp, EbmlElementType::Utf8String, 2, "MuxingApp" },
	{ EbmlElementId::WritingApp, EbmlElementType::Utf8String, 2, "WritingApp" },
	{ EbmlElementId::Cluster, EbmlElementType::Master, 1, "Cluster" },
	{ EbmlElementId::Timecode, EbmlElementType::UnsignedInteger, 2, "Timecode" },
	{ EbmlElementId::SilentTracks, EbmlElementType::Master, 2, "SilentTracks" },
	{ EbmlElementId::SilentTrackNumber, EbmlElementType::UnsignedInteger, 3, "SilentTrackNumber" },
	{ EbmlElementId::Position, EbmlElementType::UnsignedInteger, 2, "Position" },
	{ EbmlElementId::PrevSize, EbmlElementType::UnsignedInteger, 2, "PrevSize" },
	{ EbmlElementId::SimpleBlock, EbmlElementType::Binary, 2, "SimpleBlock" },
	{ EbmlElementId::BlockGroup, EbmlElementType::Master, 2, "BlockGroup" },
	{ EbmlElementId::Block, EbmlElementType::Binary, 3, "Block" },
	{ EbmlElementId::BlockVirtual, EbmlElementType::Binary, 3, "BlockVirtual" },
	{ EbmlElementId::BlockAdditions, EbmlElementType::Master, 3, "BlockAdditions" },
	{ EbmlElementId::BlockMore, EbmlElementType::Master, 4, "BlockMore" },
	{ EbmlElementId::BlockAddID, EbmlElementType::UnsignedInteger, 5, "BlockAddID" },
	{ EbmlElementId::BlockAdditional, EbmlElementType::Binary, 5, "BlockAdditional" },
	{ EbmlElementId::BlockDuration, EbmlElementType::UnsignedInteger, 3, "BlockDuration" },
	{ EbmlElementId::ReferencePriority, EbmlElementType::UnsignedInteger, 3, "ReferencePriority" },
	{ EbmlElementId::ReferenceBlock, EbmlElementType::SignedInteger, 3, "ReferenceBlock" },
	{ EbmlElementId::ReferenceVirtual, EbmlElementType::SignedInteger, 3, "ReferenceVirtual" },
	{ EbmlElementId::CodecState, EbmlElementType::Binary, 3, "CodecState" },
	{ EbmlElementId::DiscardPadding, EbmlElementType::SignedInteger, 3, "DiscardPadding" },
	{ EbmlElementId::Slices, EbmlElementType::Master, 3, "Slices" },
	{ EbmlElementId::TimeSlice, EbmlElementType::Master, 4, "TimeSlice" },
	{ EbmlElementId::LaceNumber, EbmlElementType::UnsignedInteger, 5, "LaceNumber" },
	{ EbmlElementId::FrameNumber, EbmlElementType::UnsignedInteger, 5, "FrameNumber" },
	{ EbmlElementId::BlockAdditionID, EbmlElementType::UnsignedInteger, 5, "BlockAdditionID" },
	{ EbmlElementId::Delay, EbmlElementType::UnsignedInteger, 5, "Delay" },
	{ EbmlElementId::SliceDuration, EbmlElementType::UnsignedInteger, 5, "SliceDuration" },
	{ EbmlElementId::ReferenceFrame, EbmlElementType::Master, 3, "ReferenceFrame" },
	{ EbmlElementId::ReferenceOffset, EbmlElementType::UnsignedInteger, 4, "ReferenceOffset" },
	{ EbmlElementId::ReferenceTimeCode, EbmlElementType::UnsignedInteger, 4, "ReferenceTimeCode" },
	{ EbmlElementId::EncryptedBlock, EbmlElementType::Binary, 2, "EncryptedBlock" },
	{ EbmlElementId::Tracks, EbmlElementType::Master, 1, "Tracks" },
	{ EbmlElementId::TrackEntry, EbmlElementType::Master, 2, "TrackEntry" },
	{ EbmlElementId::TrackNumber, EbmlElementType::UnsignedInteger, 3, "TrackNumber" },
	{ EbmlElementId::TrackUID, EbmlElementType::UnsignedInteger, 3, "TrackUID" },
	{ EbmlElementId::TrackType, EbmlElementType::UnsignedInteger, 3, "TrackType" },
	{ EbmlElementId::FlagEnabled, EbmlElementType::UnsignedInteger, 3, "FlagEnabled" },
	{ EbmlElementId::FlagDefault, EbmlElementType::UnsignedInteger, 3, "FlagDefault" },
	{ EbmlElementId::FlagForced, EbmlElementType::UnsignedInteger, 3, "FlagForced" },
	{ EbmlElementId::FlagLacing, EbmlElementType::UnsignedInteger, 3, "FlagLacing" },
	{ EbmlElementId::MinCache, EbmlElementType::UnsignedInteger, 3, "MinCache" },
	{ EbmlElementId::MaxCache, EbmlElementType::UnsignedInteger, 3, "MaxCache" },
	{ EbmlElementId::DefaultDuration, EbmlElementType::UnsignedInteger, 3, "DefaultDuration" },
	{ EbmlElementId::DefaultDecodedFieldDuration, EbmlElementType::UnsignedInteger, 3, "DefaultDecodedFieldDuration" },
	{ EbmlElementId::TrackTimecodeScale, EbmlElementType::Float, 3, "TrackTimecodeScale" },
	{ EbmlElementId::TrackOffset, EbmlElementType::SignedInteger, 3, "TrackOffset" },
	{ EbmlElementId::MaxBlockAdditionID, EbmlElementType::UnsignedInteger, 3, "MaxBlockAdditionID" },
	{ EbmlElementId::Name, EbmlElementType::Utf8String, 3, "Name" },
	{ EbmlElementId::Language, EbmlElementType::String, 3, "Language" },
	{ EbmlElementId::CodecID, EbmlElementType::String, 3, "CodecID" },
	{ EbmlElementId::CodecPrivate, EbmlElementType::Binary, 3, "CodecPrivate" },
	{ EbmlElementId::CodecName, EbmlElementType::Utf8String, 3, "CodecName" },
	{ EbmlElementId::AttachmentLink, EbmlElementType::UnsignedInteger, 3, "AttachmentLink" },
	{ EbmlElementId::CodecSettings, EbmlElementType::Utf8String, 3, "CodecSettings" },
	{ EbmlElementId::CodecInfoURL, EbmlElementType::String, 3, "CodecInfoURL" },
	{ EbmlElementId::CodecDownloadURL, EbmlElementType::String, 3, "CodecDownloadURL" },
	{ EbmlElementId::CodecDecodeAll, EbmlElementType::UnsignedInteger, 3, "CodecDecodeAll" },
	{ EbmlElementId::TrackOverlay, EbmlElementType::UnsignedInteger, 3, "TrackOverlay" },
	{ EbmlElementId::CodecDelay, EbmlElementType::UnsignedInteger, 3, "CodecDelay" },
	{ EbmlElementId::SeekPreRoll, EbmlElementType::UnsignedInteger, 3, "SeekPreRoll" },
	{ EbmlElementId::TrackTranslate, EbmlElementType::Master, 3, "TrackTranslate" },
	{ EbmlElementId::TrackTranslateEditionUID, EbmlElementType::UnsignedInteger, 4, "TrackTranslateEditionUID" },
	{ EbmlElementId::TrackTranslateCodec, EbmlElementType::UnsignedInteger, 4, "TrackTranslateCodec" },
	{ EbmlElementId::TrackTranslateTrackID, EbmlElementType::Binary, 4, "TrackTranslateTrackID" },
	{ EbmlElementId::Video, EbmlElementType::Master, 3, "Video" },
	{ EbmlElementId::FlagInterlaced, EbmlElementType::UnsignedInteger, 4, "FlagInterlaced" },
	{ EbmlElementId::FieldOrder, EbmlElementType::UnsignedInteger, 4, "FieldOrder" },
	{ EbmlElementId::StereoMode, EbmlElementType::UnsignedInteger, 4, "StereoMode" },
	{ EbmlElementId::AlphaMode, EbmlElementType::UnsignedInteger, 4, "AlphaMode" },
	{ EbmlElementId::OldStereoMode, EbmlElementType::UnsignedInteger, 4, "OldStereoMode" },
	{ EbmlElementId::PixelWidth, EbmlElementType::UnsignedInteger, 4, "PixelWidth" },
	{ EbmlElementId::PixelHeight, EbmlElementType::UnsignedInteger, 4, "PixelHeight" },
	{ EbmlElementId::PixelCropBottom, EbmlElementType::UnsignedInteger, 4, "PixelCropBottom" },
	{ EbmlElementId::PixelCropTop, EbmlElementType::UnsignedInteger, 4, "PixelCropTop" },
	{ EbmlElementId::PixelCropLeft, EbmlElementType::UnsignedInteger, 4, "PixelCropLeft" },
	{ EbmlElementId::PixelCropRight, EbmlElementType::UnsignedInteger, 4, "PixelCropRight" },
	{ EbmlElementId::DisplayWidth, EbmlElementType::UnsignedInteger, 4, "DisplayWidth" },
	{ EbmlElementId::DisplayHeight, EbmlElementType::UnsignedInteger, 4, "DisplayHeight" },
	{ EbmlElementId::DisplayUnit, EbmlElementType::UnsignedInteger, 4, "DisplayUnit" },
	{ EbmlElementId::AspectRatioType, EbmlElementType::UnsignedInteger, 4, "AspectRatioType" },
	{ EbmlElementId::ColourSpace, EbmlElementType::Binary, 4, "ColourSpace" },
	{ EbmlElementId::GammaValue, EbmlElementType::Float, 4, "GammaValue" },
	{ EbmlElementId::FrameRate, EbmlElementType::Float, 4, "FrameRate" },
	{ EbmlElementId::Colour, EbmlElementType::Master, 4, "Colour" },
	{ EbmlElementId::MatrixCoefficients, EbmlElementType::UnsignedInteger, 5, "MatrixCoefficients" },
	{ EbmlElementId::BitsPerChannel, EbmlElementType::UnsignedInteger, 5, "BitsPerChannel" },
	{ EbmlElementId::ChromaSubsamplingHorz, EbmlElementType::UnsignedInteger, 5, "ChromaSubsamplingHorz" },
	{ EbmlElementId::ChromaSubsamplingVert, EbmlElementType::UnsignedInteger, 5, "ChromaSubsamplingVert" },
	{ EbmlElementId::CbSubsamplingHorz, EbmlElementType::UnsignedInteger, 5, "CbSubsamplingHorz" },
	{ EbmlElementId::CbSubsamplingVert, EbmlElementType::UnsignedInteger, 5, "CbSubsamplingVert" },
	{ EbmlElementId::ChromaSitingHorz, EbmlElementType::UnsignedInteger, 5, "ChromaSitingHorz" },
	{ EbmlElementId::ChromaSitingVert, EbmlElementType::UnsignedInteger, 5, "ChromaSitingVert" },
	{ EbmlElementId::Range, EbmlElementType::UnsignedInteger, 5, "Range" },
	{ EbmlElementId::TransferCharacteristics, EbmlElementType::UnsignedInteger, 5, "TransferCharacteristics" },
	{ EbmlElementId::Primaries, EbmlElementType::UnsignedInteger, 5, "Primaries" },
	{ EbmlElementId::MaxCLL, EbmlElementType::UnsignedInteger, 5, "MaxCLL" },
	{ EbmlElementId::MaxFALL, EbmlElementType::UnsignedInteger, 5, "MaxFALL" },
	{ EbmlElementId::MasteringMetadata, EbmlElementType::Master, 5, "MasteringMetadata" },
	{ EbmlElementId::PrimaryRChromaticityX, EbmlElementType::Float, 6, "PrimaryRChromaticityX" },
	{ EbmlElementId::PrimaryRChromaticityY, EbmlElementType::Float, 6, "PrimaryRChromaticityY" },
	{ EbmlElementId::PrimaryGChromaticityX, EbmlElementType::Float, 6, "PrimaryGChromaticityX" },
	{ EbmlElementId::PrimaryGChromaticityY, EbmlElementType::Float, 6, "PrimaryGChromaticityY" },
	{ EbmlElementId::PrimaryBChromaticityX, EbmlElementType::Float, 6, "PrimaryBChromaticityX" },
	{ EbmlElementId::PrimaryBChromaticityY, EbmlElementType::Float, 6, "PrimaryBChromaticityY" },
	{ EbmlElementId::WhitePointChromaticityX, EbmlElementType::Float, 6, "WhitePointChromaticityX" },
	{ EbmlElementId::WhitePointChromaticityY, EbmlElementType::Float, 6, "WhitePointChromaticityY" },
	{ EbmlElementId::LuminanceMax, EbmlElementType::Float, 6, "LuminanceMax" },
	{ EbmlElementId::LuminanceMin, EbmlElementType::Float, 6, "LuminanceMin" },

		{ EbmlElementId::kProjection, EbmlElementType::Master, 5, "kProjection" },
		{ EbmlElementId::kProjectionType, EbmlElementType::UnsignedInteger, 6, "kProjectionType" },
		{ EbmlElementId::kProjectionPrivate, EbmlElementType::Binary, 6, "kProjectionPrivate" },
		{ EbmlElementId::kProjectionPoseYaw, EbmlElementType::Float, 6, "kProjectionPoseYaw" },
		{ EbmlElementId::kProjectionPosePitch, EbmlElementType::Float, 6, "kProjectionPosePitch" },
		{ EbmlElementId::kProjectionPoseRoll, EbmlElementType::Float, 6, "kProjectionPoseRoll" },

	{ EbmlElementId::Audio, EbmlElementType::Master, 3, "Audio" },
	{ EbmlElementId::SamplingFrequency, EbmlElementType::Float, 4, "SamplingFrequency" },
	{ EbmlElementId::OutputSamplingFrequency, EbmlElementType::Float, 4, "OutputSamplingFrequency" },
	{ EbmlElementId::Channels, EbmlElementType::UnsignedInteger, 4, "Channels" },
	{ EbmlElementId::ChannelPositions, EbmlElementType::Binary, 4, "ChannelPositions" },
	{ EbmlElementId::BitDepth, EbmlElementType::UnsignedInteger, 4, "BitDepth" },
	{ EbmlElementId::TrackOperation, EbmlElementType::Master, 3, "TrackOperation" },
	{ EbmlElementId::TrackCombinePlanes, EbmlElementType::Master, 4, "TrackCombinePlanes" },
	{ EbmlElementId::TrackPlane, EbmlElementType::Master, 5, "TrackPlane" },
	{ EbmlElementId::TrackPlaneUID, EbmlElementType::UnsignedInteger, 6, "TrackPlaneUID" },
	{ EbmlElementId::TrackPlaneType, EbmlElementType::UnsignedInteger, 6, "TrackPlaneType" },
	{ EbmlElementId::TrackJoinBlocks, EbmlElementType::Master, 4, "TrackJoinBlocks" },
	{ EbmlElementId::TrackJoinUID, EbmlElementType::UnsignedInteger, 5, "TrackJoinUID" },
	{ EbmlElementId::TrickTrackUID, EbmlElementType::UnsignedInteger, 3, "TrickTrackUID" },
	{ EbmlElementId::TrickTrackSegmentUID, EbmlElementType::Binary, 3, "TrickTrackSegmentUID" },
	{ EbmlElementId::TrickTrackFlag, EbmlElementType::UnsignedInteger, 3, "TrickTrackFlag" },
	{ EbmlElementId::TrickMasterTrackUID, EbmlElementType::UnsignedInteger, 3, "TrickMasterTrackUID" },
	{ EbmlElementId::TrickMasterTrackSegmentUID, EbmlElementType::Binary, 3, "TrickMasterTrackSegmentUID" },
	{ EbmlElementId::ContentEncodings, EbmlElementType::Master, 3, "ContentEncodings" },
	{ EbmlElementId::ContentEncoding, EbmlElementType::Master, 4, "ContentEncoding" },
	{ EbmlElementId::ContentEncodingOrder, EbmlElementType::UnsignedInteger, 5, "ContentEncodingOrder" },
	{ EbmlElementId::ContentEncodingScope, EbmlElementType::UnsignedInteger, 5, "ContentEncodingScope" },
	{ EbmlElementId::ContentEncodingType, EbmlElementType::UnsignedInteger, 5, "ContentEncodingType" },
	{ EbmlElementId::ContentCompression, EbmlElementType::Master, 5, "ContentCompression" },
	{ EbmlElementId::ContentCompAlgo, EbmlElementType::UnsignedInteger, 6, "ContentCompAlgo" },
	{ EbmlElementId::ContentCompSettings, EbmlElementType::Binary, 6, "ContentCompSettings" },
	{ EbmlElementId::ContentEncryption, EbmlElementType::Master, 5, "ContentEncryption" },
	{ EbmlElementId::ContentEncAlgo, EbmlElementType::UnsignedInteger, 6, "ContentEncAlgo" },
	{ EbmlElementId::ContentEncKeyID, EbmlElementType::Binary, 6, "ContentEncKeyID" },
	{ EbmlElementId::ContentSignature, EbmlElementType::Binary, 6, "ContentSignature" },
	{ EbmlElementId::ContentSigKeyID, EbmlElementType::Binary, 6, "ContentSigKeyID" },
	{ EbmlElementId::ContentSigAlgo, EbmlElementType::UnsignedInteger, 6, "ContentSigAlgo" },
	{ EbmlElementId::ContentSigHashAlgo, EbmlElementType::UnsignedInteger, 6, "ContentSigHashAlgo" },

		{ EbmlElementId::kContentEncAesSettings, EbmlElementType::Master, 6, "kContentEncAesSettings" },
		{ EbmlElementId::kAesSettingsCipherMode, EbmlElementType::UnsignedInteger, 7, "kAesSettingsCipherMode" },

	{ EbmlElementId::Cues, EbmlElementType::Master, 1, "Cues" },
	{ EbmlElementId::CuePoint, EbmlElementType::Master, 2, "CuePoint" },
	{ EbmlElementId::CueTime, EbmlElementType::UnsignedInteger, 3, "CueTime" },
	{ EbmlElementId::CueTrackPositions, EbmlElementType::Master, 3, "CueTrackPositions" },
	{ EbmlElementId::CueTrack, EbmlElementType::UnsignedInteger, 4, "CueTrack" },
	{ EbmlElementId::CueClusterPosition, EbmlElementType::UnsignedInteger, 4, "CueClusterPosition" },
	{ EbmlElementId::CueRelativePosition, EbmlElementType::UnsignedInteger, 4, "CueRelativePosition" },
	{ EbmlElementId::CueDuration, EbmlElementType::UnsignedInteger, 4, "CueDuration" },
	{ EbmlElementId::CueBlockNumber, EbmlElementType::UnsignedInteger, 4, "CueBlockNumber" },
	{ EbmlElementId::CueCodecState, EbmlElementType::UnsignedInteger, 4, "CueCodecState" },
	{ EbmlElementId::CueReference, EbmlElementType::Master, 4, "CueReference" },
	{ EbmlElementId::CueRefTime, EbmlElementType::UnsignedInteger, 5, "CueRefTime" },
	{ EbmlElementId::CueRefCluster, EbmlElementType::UnsignedInteger, 5, "CueRefCluster" },
	{ EbmlElementId::CueRefNumber, EbmlElementType::UnsignedInteger, 5, "CueRefNumber" },
	{ EbmlElementId::CueRefCodecState, EbmlElementType::UnsignedInteger, 5, "CueRefCodecState" },
	{ EbmlElementId::Attachments, EbmlElementType::Master, 1, "Attachments" },
	{ EbmlElementId::AttachedFile, EbmlElementType::Master, 2, "AttachedFile" },
	{ EbmlElementId::FileDescription, EbmlElementType::Utf8String, 3, "FileDescription" },
	{ EbmlElementId::FileName, EbmlElementType::Utf8String, 3, "FileName" },
	{ EbmlElementId::FileMimeType, EbmlElementType::String, 3, "FileMimeType" },
	{ EbmlElementId::FileData, EbmlElementType::Binary, 3, "FileData" },
	{ EbmlElementId::FileUID, EbmlElementType::UnsignedInteger, 3, "FileUID" },
	{ EbmlElementId::FileReferral, EbmlElementType::Binary, 3, "FileReferral" },
	{ EbmlElementId::FileUsedStartTime, EbmlElementType::UnsignedInteger, 3, "FileUsedStartTime" },
	{ EbmlElementId::FileUsedEndTime, EbmlElementType::UnsignedInteger, 3, "FileUsedEndTime" },
	{ EbmlElementId::Chapters, EbmlElementType::Master, 1, "Chapters" },
	{ EbmlElementId::EditionEntry, EbmlElementType::Master, 2, "EditionEntry" },
	{ EbmlElementId::EditionUID, EbmlElementType::UnsignedInteger, 3, "EditionUID" },
	{ EbmlElementId::EditionFlagHidden, EbmlElementType::UnsignedInteger, 3, "EditionFlagHidden" },
	{ EbmlElementId::EditionFlagDefault, EbmlElementType::UnsignedInteger, 3, "EditionFlagDefault" },
	{ EbmlElementId::EditionFlagOrdered, EbmlElementType::UnsignedInteger, 3, "EditionFlagOrdered" },
	{ EbmlElementId::ChapterAtom, EbmlElementType::Master, 3, "ChapterAtom" },
	{ EbmlElementId::ChapterUID, EbmlElementType::UnsignedInteger, 4, "ChapterUID" },
	{ EbmlElementId::ChapterStringUID, EbmlElementType::Utf8String, 4, "ChapterStringUID" },
	{ EbmlElementId::ChapterTimeStart, EbmlElementType::UnsignedInteger, 4, "ChapterTimeStart" },
	{ EbmlElementId::ChapterTimeEnd, EbmlElementType::UnsignedInteger, 4, "ChapterTimeEnd" },
	{ EbmlElementId::ChapterFlagHidden, EbmlElementType::UnsignedInteger, 4, "ChapterFlagHidden" },
	{ EbmlElementId::ChapterFlagEnabled, EbmlElementType::UnsignedInteger, 4, "ChapterFlagEnabled" },
	{ EbmlElementId::ChapterSegmentUID, EbmlElementType::Binary, 4, "ChapterSegmentUID" },
	{ EbmlElementId::ChapterSegmentEditionUID, EbmlElementType::UnsignedInteger, 4, "ChapterSegmentEditionUID" },
	{ EbmlElementId::ChapterPhysicalEquiv, EbmlElementType::UnsignedInteger, 4, "ChapterPhysicalEquiv" },
	{ EbmlElementId::ChapterTrack, EbmlElementType::Master, 4, "ChapterTrack" },
	{ EbmlElementId::ChapterTrackNumber, EbmlElementType::UnsignedInteger, 5, "ChapterTrackNumber" },
	{ EbmlElementId::ChapterDisplay, EbmlElementType::Master, 4, "ChapterDisplay" },
	{ EbmlElementId::ChapString, EbmlElementType::Utf8String, 5, "ChapString" },
	{ EbmlElementId::ChapLanguage, EbmlElementType::String, 5, "ChapLanguage" },
	{ EbmlElementId::ChapCountry, EbmlElementType::String, 5, "ChapCountry" },
	{ EbmlElementId::ChapProcess, EbmlElementType::Master, 4, "ChapProcess" },
	{ EbmlElementId::ChapProcessCodecID, EbmlElementType::UnsignedInteger, 5, "ChapProcessCodecID" },
	{ EbmlElementId::ChapProcessPrivate, EbmlElementType::Binary, 5, "ChapProcessPrivate" },
	{ EbmlElementId::ChapProcessCommand, EbmlElementType::Master, 5, "ChapProcessCommand" },
	{ EbmlElementId::ChapProcessTime, EbmlElementType::UnsignedInteger, 6, "ChapProcessTime" },
	{ EbmlElementId::ChapProcessData, EbmlElementType::Binary, 6, "ChapProcessData" },
	{ EbmlElementId::Tags, EbmlElementType::Master, 1, "Tags" },
	{ EbmlElementId::Tag, EbmlElementType::Master, 2, "Tag" },
	{ EbmlElementId::Targets, EbmlElementType::Master, 3, "Targets" },
	{ EbmlElementId::TargetTypeValue, EbmlElementType::UnsignedInteger, 4, "TargetTypeValue" },
	{ EbmlElementId::TargetType, EbmlElementType::String, 4, "TargetType" },
	{ EbmlElementId::TagTrackUID, EbmlElementType::UnsignedInteger, 4, "TagTrackUID" },
	{ EbmlElementId::TagEditionUID, EbmlElementType::UnsignedInteger, 4, "TagEditionUID" },
	{ EbmlElementId::TagChapterUID, EbmlElementType::UnsignedInteger, 4, "TagChapterUID" },
	{ EbmlElementId::TagAttachmentUID, EbmlElementType::UnsignedInteger, 4, "TagAttachmentUID" },
	{ EbmlElementId::SimpleTag, EbmlElementType::Master, 3, "SimpleTag" },
	{ EbmlElementId::TagName, EbmlElementType::Utf8String, 4, "TagName" },
	{ EbmlElementId::TagLanguage, EbmlElementType::String, 4, "TagLanguage" },
	{ EbmlElementId::TagDefault, EbmlElementType::UnsignedInteger, 4, "TagDefault" },
	{ EbmlElementId::TagString, EbmlElementType::Utf8String, 4, "TagString" },
	{ EbmlElementId::TagBinary, EbmlElementType::Binary, 4, "TagBinary" }
};

const size_t ebml_element_descriptor_count = sizeof(ebml_element_descriptors) / sizeof(ebml_element_descriptors[0]);



inline EbmlElementDescriptor* get_ebml_element_descriptor(EbmlElementId ebml_element_id)
{
	size_t ebml_element_descriptor_index = 0;
	while (ebml_element_descriptor_index < ebml_element_descriptor_count)
	{
		if (ebml_element_descriptors[ebml_element_descriptor_index].id_ == ebml_element_id)
		{
			return &ebml_element_descriptors[ebml_element_descriptor_index];
		}
		ebml_element_descriptor_index++;
	}
	return nullptr;
}

inline size_t get_ebml_element_id_length(uint8_t first_byte_of_element_id)
{
	for (uint8_t position = 7; position > 3; position--)
	{
		if (first_byte_of_element_id & 1 << position)
		{
			return 8 - position;
		}
	}
	throw std::runtime_error("invalid element id");
}

inline std::string get_ebml_element_name(EbmlElementId ebml_element_id)
{
	const auto* ebml_element_descriptor = get_ebml_element_descriptor(ebml_element_id);
	if (ebml_element_descriptor)
	{
		return ebml_element_descriptor->name_;
	}
	return "<unknown element>";
}

inline EbmlElementType get_ebml_element_type(EbmlElementId ebml_element_id)
{
	const auto* ebml_element_descriptor = get_ebml_element_descriptor(ebml_element_id);
	if (ebml_element_descriptor)
	{
		return ebml_element_descriptor->type_;
	}
	throw std::runtime_error("unknown element");
}

inline int get_ebml_element_level(EbmlElementId ebml_element_id)
{
	const auto* ebml_element_descriptor = get_ebml_element_descriptor(ebml_element_id);
	if (ebml_element_descriptor)
	{
		return ebml_element_descriptor->level_;
	}
	throw std::runtime_error("unknown element");
}

inline EbmlElementId read_ebml_element_id(uint8_t* data, size_t& available_data_length, size_t& ebml_element_id_length)
{
	ebml_element_id_length = get_ebml_element_id_length(*data);
	if (ebml_element_id_length > available_data_length)
	{
		throw std::runtime_error(std::string("not enough data to read element id (element id length - ").append(std::to_string(ebml_element_id_length)).append(" bytes)"));
	}
	available_data_length -= ebml_element_id_length;
	uint8_t ebml_element_id_buffer[4] = { 0 };
	memcpy(&ebml_element_id_buffer[4 - ebml_element_id_length], data, ebml_element_id_length);
	auto ebml_element_id = ReadBigEndianNumber<uint32_t>(ebml_element_id_buffer);
	const auto* ebml_element_descriptor = get_ebml_element_descriptor(static_cast<EbmlElementId>(ebml_element_id));
	if (ebml_element_descriptor)
	{
		return ebml_element_descriptor->id_;
	}
	std::stringstream error;
	error << "unknown element id (0x" << std::hex << ebml_element_id << ")";
	throw std::runtime_error(error.str());
}

inline size_t get_ebml_element_size_length(const uint8_t* data, size_t available_data_length)
{
	for (uint8_t position = 7; position > 0; position--)
	{
		if (*data & 1 << position)
		{
			return 8 - position;
		}
	}
	if (*data == 1)
	{
		return 8;
	}
	throw std::runtime_error("invalid element size");
}

inline uint64_t get_ebml_element_size(const uint8_t* data, size_t available_data_length, size_t& ebml_element_size_length)
{
	ebml_element_size_length = get_ebml_element_size_length(data, available_data_length);
	if (ebml_element_size_length > available_data_length)
	{
		throw std::runtime_error(std::string("not enough data to read element size (element size length - ").append(std::to_string(ebml_element_size_length)).append(" bytes)"));
	}
	available_data_length -= ebml_element_size_length;
	uint8_t element_size[8] = { 0 };
	memcpy(&element_size[8 - ebml_element_size_length], data, ebml_element_size_length);
	element_size[8 - ebml_element_size_length] -= 1 << (8 - ebml_element_size_length);
	if (ebml_element_size_length == 1 && element_size[7] == 0x7f)
	{
		return -1;
	}
	return ReadBigEndianNumber<uint64_t>(element_size);
}

inline std::string get_ebml_element_value(EbmlElementId id, EbmlElementType type, uint8_t* data, uint64_t size)
{
	switch (type)
	{
	case EbmlElementType::UnsignedInteger:
	case EbmlElementType::SignedInteger:
	{
		uint8_t Integer[8] = { 0 };
		memcpy(&Integer[8 - size], data, static_cast<size_t>(size));
		if (type == EbmlElementType::UnsignedInteger) {
			return std::to_string(ReadBigEndianNumber<uint64_t>(Integer));
		}
		else {
			return std::to_string(ReadBigEndianNumber<int64_t>(Integer));
		}
	}
	case EbmlElementType::String:
	case EbmlElementType::Utf8String:
		return std::string(reinterpret_cast<char*>(data), static_cast<uint32_t>(size));
	case EbmlElementType::Float:
		if (size == 4) {
			return std::to_string(ReadBigEndianNumber<float>(data));
		}
		else {
			return std::to_string(ReadBigEndianNumber<double>(data));
		}
	case EbmlElementType::Date:
	{
		time_t timestamp = 978307200 + ReadBigEndianNumber<int64_t>(data) / 100000000;
		char buffer[20];
		tm date_and_time;
#if defined(_WIN32)
		gmtime_s(&date_and_time, &timestamp);
#else
		gmtime_r(&timestamp, &date_and_time);
#endif
		strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", &date_and_time);
		return std::string(buffer);
	}
	case EbmlElementType::Binary:
	{
		switch (id)
		{
		case EbmlElementId::SimpleBlock:
		{
			size_t track_number_size_length;
			auto track_number = get_ebml_element_size(data, static_cast<size_t>(size), track_number_size_length);
			char buffer[56];
#if defined(_WIN32)
			sprintf_s(
#else
			sprintf(
#endif
				buffer, "track %" PRIu64 ", timecode %d, flags 0x%02x", track_number, ReadBigEndianNumber<short>(data + track_number_size_length), *(data + track_number_size_length + 2));

			return std::string(buffer);
		}
		case EbmlElementId::SeekID:
		{
			size_t available_data_length = size, id_size;
			EbmlElementId id = read_ebml_element_id(data, available_data_length, id_size);
			return std::to_string(static_cast<uint32_t>(id));
		}
		}
	}
	default:
		return "<cannot parse the content of this element>";
	}
}



inline std::list<EbmlElement>::const_iterator EbmlElement::FindChildById(EbmlElementId const& id) const {
	return std::find_if(children_.cbegin(), children_.cend(), [&id](EbmlElement const& e) { return e.id() == id; });
}
inline std::list<EbmlElement>::const_iterator EbmlElement::FindNextChildById(std::list<EbmlElement>::const_iterator const& iter, EbmlElementId const& id) const {
	return std::find_if(iter, children_.cend(), [&id](EbmlElement const& e) { return e.id() == id; });
}

inline EbmlElement::EbmlElement(EbmlElementId const& id, EbmlElementType const& type, uint64_t const& size, size_t const& id_length, size_t const& size_length, uint8_t* const& data)
	: id_(id), type_(type), size_(size), id_length_(id_length), size_length_(size_length), data_(data)
{
}

inline void EbmlElement::add_child(EbmlElement child)
{
	children_.push_back(child);
}

inline void EbmlElement::print(uint32_t level)
{
	std::string indent;
	if (level > 0)
	{
		std::cout << "|" << std::string(level - 1, ' ');
	}
	std::cout << "+ " << get_ebml_element_name(id_) << " (" << size_ << ")";
	if (type_ != EbmlElementType::Master)
	{
		std::cout << ": " << get_ebml_element_value(id_, type_, data_, size_);
	}
	std::cout << std::endl;
	for (EbmlElement& child : children_)
	{
		child.print(level + 1);
	}
}

inline uint64_t EbmlElement::size()
{
	return size_;
}

inline uint64_t EbmlElement::element_size()
{
	return id_length_ + size_length_ + size_;
}

inline void EbmlElement::calculate_size()
{
	uint64_t size = 0;
	for (auto& child : children_)
	{
		size += child.element_size();
	}
	size_ = size;
}

inline EbmlElementId EbmlElement::id() const
{
	return id_;
}

inline const std::list<EbmlElement>& EbmlElement::children() const
{
	return children_;
}

inline const std::string EbmlElement::value() const
{
	if (type_ != EbmlElementType::Master)
	{
		return get_ebml_element_value(id_, type_, data_, size_);
	}
	throw std::runtime_error("cannot obtain a value from a master element");
}

inline const uint8_t* EbmlElement::data() const
{
	return data_;
}

inline uint64_t EbmlElement::size() const
{
	return size_;
}

inline EbmlElementType EbmlElement::type() const
{
	return type_;
}

inline const EbmlElement* EbmlElement::first_child(EbmlElementId id) const
{
	auto first_child = std::find_if(children_.begin(), children_.end(), [id](const EbmlElement& child) { return child.id() == id; });
	if (first_child != children_.end())
	{
		return &(*first_child);
	}
	return nullptr;
}

inline std::vector<const EbmlElement*> EbmlElement::children(EbmlElementId id) const
{
	std::vector<const EbmlElement*> result;
	children(result, id);
	return result;
}

inline void EbmlElement::children(std::vector<const EbmlElement*>& children, EbmlElementId id) const
{
	for (const auto& child : children_)
	{
		if (child.id() == id)
		{
			children.emplace_back(&child);
		}
	}
}

inline std::vector<const EbmlElement*> EbmlElement::descendants(EbmlElementId id) const
{
	std::vector<const EbmlElement*> result;
	descendants(result, id);
	return result;
}

inline void EbmlElement::descendants(std::vector<const EbmlElement*>& descendants, EbmlElementId id) const
{
	children(descendants, id);
	for (const auto& child : children_)
	{
		child.descendants(descendants, id);
	}
}
















inline std::list<EbmlElement>::const_iterator EbmlDocument::FindChildById(EbmlElementId const& id) const {
	return std::find_if(elements_.cbegin(), elements_.cend(), [&id](EbmlElement const& e) { return e.id() == id; });
}
inline std::list<EbmlElement>::const_iterator EbmlDocument::FindNextChildById(std::list<EbmlElement>::const_iterator const& iter, EbmlElementId const& id) const {
	return std::find_if(iter, elements_.cend(), [&id](EbmlElement const& e) { return e.id() == id; });
}


inline EbmlDocument::EbmlDocument(std::unique_ptr<uint8_t[]> storage, std::list<EbmlElement> elements)
	: storage_(std::move(storage)), elements_(std::move(elements))
{
}

inline const std::list<EbmlElement>& EbmlDocument::elements() const
{
	return elements_;
}

enum class EbmlParserState
{
	ParseElementId,
	ParseElementLength,
	ParseElementValue,
};

inline EbmlDocument parse_ebml_file(std::unique_ptr<uint8_t[]>&& ebml_file_contents, size_t const& file_size, bool const& verbose)
{
	size_t remaining_file_size = file_size;

	EbmlParserState ebml_parser_state = EbmlParserState::ParseElementId;
	std::stack<EbmlElement> ebml_element_stack;
	std::stack<uint64_t> ebml_element_size_stack;
	std::list<EbmlElement> ebml_element_tree;
	EbmlElementId current_ebml_element_id = EbmlElementId::EBML;
	EbmlElementType current_ebml_element_type = EbmlElementType::Unknown;
	uint64_t current_ebml_element_size = 0;
	ebml_element_size_stack.push(file_size);
	size_t current_ebml_element_id_length = 0, current_ebml_element_size_length = 0;
	try
	{
		while (remaining_file_size > 0)
		{
			auto file_offset = file_size - remaining_file_size;
			switch (ebml_parser_state)
			{
			case EbmlParserState::ParseElementId:
			{
				current_ebml_element_id = read_ebml_element_id(&ebml_file_contents[file_offset], remaining_file_size, current_ebml_element_id_length);
				if (ebml_element_size_stack.top() != -1)
				{
					ebml_element_size_stack.top() -= current_ebml_element_id_length;
				}
				else if (ebml_element_size_stack.top() == -1 && get_ebml_element_level(current_ebml_element_id) == get_ebml_element_level(ebml_element_stack.top().id()))
				{
					auto element = ebml_element_stack.top();
					ebml_element_stack.pop();
					element.calculate_size();
					ebml_element_size_stack.pop();
					if (ebml_element_size_stack.top() != -1)
					{
						ebml_element_size_stack.top() -= element.size();
					}
					ebml_element_stack.top().add_child(element);
				}
				ebml_parser_state = EbmlParserState::ParseElementLength;
			}
			break;
			case EbmlParserState::ParseElementLength:
				current_ebml_element_size = get_ebml_element_size(&ebml_file_contents[file_offset], remaining_file_size, current_ebml_element_size_length);
				remaining_file_size -= current_ebml_element_size_length;
				if (ebml_element_size_stack.top() != -1)
				{
					ebml_element_size_stack.top() -= current_ebml_element_size_length;
				}
				ebml_parser_state = EbmlParserState::ParseElementValue;
				break;
			case EbmlParserState::ParseElementValue:
				current_ebml_element_type = get_ebml_element_type(current_ebml_element_id);
				if (current_ebml_element_type == EbmlElementType::Master)
				{
					ebml_element_stack.push(EbmlElement(current_ebml_element_id, current_ebml_element_type, current_ebml_element_size, current_ebml_element_id_length, current_ebml_element_size_length));
					ebml_element_size_stack.push(current_ebml_element_size);
				}
				else
				{
					remaining_file_size -= static_cast<size_t>(current_ebml_element_size);
					if (ebml_element_size_stack.top() != -1)
					{
						ebml_element_size_stack.top() -= static_cast<size_t>(current_ebml_element_size);
					}
					if (ebml_element_stack.size() > 0)
					{
						ebml_element_stack.top().add_child(EbmlElement(current_ebml_element_id, current_ebml_element_type, current_ebml_element_size, current_ebml_element_id_length, current_ebml_element_size_length, &ebml_file_contents[file_offset]));
						while (ebml_element_size_stack.top() == 0)
						{
							auto parent = ebml_element_stack.top();
							ebml_element_stack.pop();
							ebml_element_size_stack.pop();
							if (parent.size() == -1)
							{
								parent.calculate_size();
							}
							if (ebml_element_size_stack.top() != -1)
							{
								ebml_element_size_stack.top() -= parent.size();
							}
							if (ebml_element_stack.size() == 0)
							{
								ebml_element_tree.push_back(parent);
								break;
							}
							else
							{
								ebml_element_stack.top().add_child(parent);
							}
						}
					}
					else
					{
						ebml_element_tree.push_back(EbmlElement(current_ebml_element_id, current_ebml_element_type, current_ebml_element_size, current_ebml_element_id_length, current_ebml_element_size_length, &ebml_file_contents[file_offset]));
					}
				}
				ebml_parser_state = EbmlParserState::ParseElementId;
				break;
			default:
				throw std::runtime_error("parser state error");
			}
		}
	}
	catch (std::exception& exception)
	{
		std::cerr << "Parsing error at offset 0x" << std::hex << file_size - remaining_file_size << ": " << exception.what() << std::endl;
		return EbmlDocument();
	}
	if (remaining_file_size)
	{
		int i = 3;
	}
	while (ebml_element_stack.size() > 0)
	{
		EbmlElement parent = ebml_element_stack.top();
		ebml_element_stack.pop();
		if (ebml_element_stack.size() == 0)
		{
			ebml_element_tree.push_back(parent);
		}
		else
		{
			ebml_element_stack.top().add_child(parent);
		}
	}
	for (const auto& ebml_element : ebml_element_tree)
	{
		const auto seek_heads = ebml_element.children(EbmlElementId::SeekHead);
		for (const auto& seek_head : seek_heads)
		{
			const auto& seeks = seek_head->children(EbmlElementId::Seek);
			for (const auto& seek : seeks)
			{
				const auto* seek_id = seek->first_child(EbmlElementId::SeekID);
				if (seek_id)
				{
					const auto* ebml_element_descriptor = get_ebml_element_descriptor(static_cast<EbmlElementId>(std::stoi(seek_id->value())));
					if (ebml_element_descriptor)
					{
					}
				}
			}
		}
	}
	if (verbose)
	{
		std::cout << std::endl << "*** EBML element tree ***" << std::endl;
		for (EbmlElement& ebml_element : ebml_element_tree)
		{
			ebml_element.print(0);
		}
		std::cout << std::endl;
	}
	return EbmlDocument(std::move(ebml_file_contents), std::move(ebml_element_tree));
}

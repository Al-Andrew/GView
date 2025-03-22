#pragma once

#include "GView.hpp"

using namespace AppCUI;
using namespace AppCUI::Utils;
using namespace AppCUI::Application;
using namespace AppCUI::Controls;
using namespace GView::Utils;
using namespace GView;
using namespace GView::View;

namespace MDMP {

    constexpr char Magic[4] = { 'M', 'D', 'M', 'P' };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_header
    struct Header {
        uint32_t signature;
        uint32_t version;
        uint32_t numberOfStreams;
        uint32_t streamDirectoryRVA;
        uint32_t checksum;
        uint32_t timeDateStamp;
        uint64_t flags;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_location_descriptor
    struct LocationDescriptor {
        uint32_t dataSize;
        uint32_t rva;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ne-minidumpapiset-minidump_stream_type
    enum class StreamType {
        UnusedStream                = 0,
        ReservedStream0             = 1,
        ReservedStream1             = 2,
        ThreadListStream            = 3,
        ModuleListStream            = 4,
        MemoryListStream            = 5,
        ExceptionStream             = 6,
        SystemInfoStream            = 7,
        ThreadExListStream          = 8,
        Memory64ListStream          = 9,
        CommentStreamA              = 10,
        CommentStreamW              = 11,
        HandleDataStream            = 12,
        FunctionTableStream         = 13,
        UnloadedModuleListStream    = 14,
        MiscInfoStream              = 15,
        MemoryInfoListStream        = 16,
        ThreadInfoListStream        = 17,
        HandleOperationListStream   = 18,
        TokenStream                 = 19,
        JavaScriptDataStream        = 20,
        SystemMemoryInfoStream      = 21,
        ProcessVmCountersStream     = 22,
        IptTraceStream              = 23,
        ThreadNamesStream           = 24,
        ceStreamNull                = 0x8000,
        ceStreamSystemInfo          = 0x8001,
        ceStreamException           = 0x8002,
        ceStreamModuleList          = 0x8003,
        ceStreamProcessList         = 0x8004,
        ceStreamThreadList          = 0x8005,
        ceStreamThreadContextList   = 0x8006,
        ceStreamThreadCallStackList = 0x8007,
        ceStreamMemoryVirtualList   = 0x8008,
        ceStreamMemoryPhysicalList  = 0x8009,
        ceStreamBucketParameters    = 0x800A,
        ceStreamProcessModuleMap    = 0x800B,
        ceStreamDiagnosisList       = 0x800C,
        LastReservedStream          = 0xffff
    };

    std::string_view getStreamName(StreamType type);


    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_directory
    struct Stream {
        StreamType type;
        LocationDescriptor location;
    };

    struct AbstractStream {
        virtual ~AbstractStream()                = default;
        virtual void PopulateView(TreeViewItem& parent) = 0;

        bool isValid{ false };
    };

    struct AbstractStreamFactory {
        static std::unique_ptr<AbstractStream> Create(Stream const& stream, GView::Utils::DataCache& data);
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_misc_info
    struct MiscInfo {
        uint32_t size;
        uint32_t flags1;
        uint32_t processId;
        uint32_t processCreateTime;
        uint32_t processUserTime;
        uint32_t processKernelTime;
    };

    struct MiscInfoStream : public AbstractStream {

        MiscInfoStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~MiscInfoStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;

        MiscInfo miscInfo = {};
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_thread_info_list
    struct ThreadInfoList {
        uint32_t sizeOfhHeader;
        uint32_t sizeOfEntry;
        uint32_t numberOfEntries;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_thread_info
    struct ThreadInfo {
        uint32_t threadId;
        uint32_t dumpFlags;
        uint32_t dumpError;
        uint32_t exitStatus;
        uint32_t createionTime;
        uint32_t exitTime;
        uint32_t kernelTime;
        uint32_t userTime;
        uint32_t startAddress;
        uint32_t affinity;
    };


    struct ThreadInfoStream : public AbstractStream {
        
        ThreadInfoStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~ThreadInfoStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;

        ThreadInfoList header;
        std::vector<ThreadInfo> threads;
    };


    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_module_list
    struct ModuleList {
        uint32_t numberOfModules;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/verrsrc/ns-verrsrc-vs_fixedfileinfo
    struct Version {
        uint32_t signature;
        uint32_t structVersion;
        uint32_t fileVersionMS;
        uint32_t fileVersionLS;
        uint32_t productVersionMS;
        uint32_t productVersionLS;
        uint32_t fileFlagsMask;
        uint32_t fileFlags;
        uint32_t fileOS;
        uint32_t fileType;
        uint32_t fileSubtype;
        uint32_t fileDateMS;
        uint32_t fileDateLS;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_module
#pragma pack(push, 1)
    struct Module {
        uint64_t baseOfImage;
        uint32_t sizeOfImage;
        uint32_t checksum;
        uint32_t timeDateStamp;
        uint32_t moduleNameRVA;
        Version versionInfo;
        LocationDescriptor cvRecord;
        LocationDescriptor miscRecord;
        uint64_t reserved0;
        uint64_t reserved1;
    };
#pragma pack(pop)

    struct ModuleListStream : public AbstractStream {
        ModuleListStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~ModuleListStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;
        ModuleList header;
        std::vector<Module> modules;
        std::vector<std::string> moduleNames;
    };


    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_function_table_stream
    struct FunctionTable {
        uint32_t sizeofHeader;
        uint32_t sizeofDescriptor;
        uint32_t sizeofNativeDescritor;
        uint32_t sizeofFucntionEntry;
        uint32_t numberOfDescriptors;
        uint32_t sizeofAlignPad;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_function_table_descriptor
    struct FunctionTableDescriptor {
        uint64_t minAddress;
        uint64_t maxAddress;
        uint64_t baseAdress;
        uint64_t entryCount;
    };

    struct FunctionTableStream : public AbstractStream {
        FunctionTableStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~FunctionTableStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;

        FunctionTable header;
        std::vector<FunctionTableDescriptor> descriptors;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_memory64_list
    struct Memory64List {
        uint64_t numberOfMemoryRanges;
        uint64_t baseRVA;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_memory_descriptor 
    struct Memory64Descriptor {
        uint64_t startOfMemoryRange;
        uint64_t size;
    };

    struct Memory64Stream : public AbstractStream {
        Memory64Stream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~Memory64Stream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;
        Memory64List header;
        std::vector<Memory64Descriptor> descriptors;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_memory_info_list
    struct MemoryInfoList {
        uint32_t sizeofHeader;
        uint32_t sizeOfEntry;
        uint64_t numberOfEntries;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_memory_info
    struct MemoryInfo {
        uint64_t baseAddress;
        uint64_t allocationBase;
        uint32_t allocationProtect;
        uint32_t __alignment1;
        uint64_t regionSize;
        uint32_t state;
        uint32_t protect;
        uint32_t type;
        uint32_t __alignment2;
    };

    struct MemoryInfoStream : public AbstractStream {
        MemoryInfoStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~MemoryInfoStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;
        MemoryInfoList header;
        std::vector<MemoryInfo> memoryInfos;
    };



    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_system_info
    struct SystemInfo {
        uint16_t processorArchitecture;
        uint16_t processorLevel;
        uint16_t processorRevision;
        uint8_t numberOfProcessors;
        uint8_t productType;
        uint32_t majorVersion;
        uint32_t minorVersion;
        uint32_t buildNumber;
        uint32_t platformId;
        uint32_t csdVersionRVA;
        uint16_t suiteMask;
        uint16_t reserved;
        struct CPUInformation {
            uint32_t vendorId[3];
            uint32_t versionInformation;
            uint32_t featureInformation;
            uint32_t AMDExtendedCpuFeatures;
        } cpu;
    };

    struct SystemInfoStream : public AbstractStream {
        SystemInfoStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~SystemInfoStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;
        SystemInfo systemInfo;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_handle_data_stream
    struct HandleDataList {
        uint32_t sizeOfHeader;
        uint32_t sizeOfDescriptor;
        uint32_t numberOfDescriptors;
        uint32_t reserved;
    };

    // https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_handle_descriptor
    struct HandleDataDescritpr {
        uint64_t handle;
        uint32_t typeNameRVA;
        uint32_t objectNameRVA;
        uint32_t attributes;
        uint32_t grantedAccess;
        uint32_t handleCount;
        uint32_t pointerCount;
        uint32_t objectInfoRVA;
        uint32_t reserved;
    };

    struct HandleDataStream : public AbstractStream {
        HandleDataStream(Stream const& stream, GView::Utils::DataCache& data);
        virtual ~HandleDataStream() = default;
        virtual void PopulateView(TreeViewItem& parent) override;
        HandleDataList header;
        std::vector<HandleDataDescritpr> descriptors;
    };


    class MDMPFile : public TypeInterface
    {
      public:
        MDMPFile();
        virtual ~MDMPFile();

        virtual std::string_view GetTypeName() override;
        virtual void RunCommand(std::string_view) override;
        virtual bool UpdateKeys(KeyboardControlsInterface* interface) override;

        bool Update();

        Header header;
        std::vector<Stream> streams;
        std::map<StreamType, std::unique_ptr<AbstractStream>> streamsData;

        Reference<GView::Utils::SelectionZoneInterface> selectionZoneInterface;
    };


    namespace Panels
    {
        class Information : public AppCUI::Controls::TabPage
        {
            Reference<MDMP::MDMPFile> mdmp;
            Reference<AppCUI::Controls::ListView> general;
            Reference<AppCUI::Controls::ListView> streams;
            void UpdateGeneralInformation();
            void UpdateStreams();
            void RecomputePanelsPositions();

          public:
            Information(Reference<MDMP::MDMPFile> mdmp);
            void Update();
        };

        class Data : public AppCUI::Controls::TabPage
        {
            Reference<MDMP::MDMPFile> mdmp;
            Reference<AppCUI::Controls::TreeView> streamsData;
            void RecomputePanelsPositions();

          public:
            Data(Reference<MDMP::MDMPFile> mdmp);
            void Update();
        };
    } // namespace Panels
} // namespace MDMP
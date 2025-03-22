#include "mdmp.hpp"

using namespace std::string_view_literals;

namespace MDMP
{
std::string_view getStreamName(StreamType type)
{
    uint32_t typeValue = static_cast<uint32_t>(type);
    if (typeValue >= 0 && typeValue <= 24) {
        static constexpr std::string_view names[] = {
            "UnusedStream"sv,
            "ReservedStream0"sv,
            "ReservedStream1"sv,
            "ThreadListStream"sv,
            "ModuleListStream"sv,
            "MemoryListStream"sv,
            "ExceptionStream"sv,
            "SystemInfoStream"sv,
            "ThreadExListStream"sv,
            "Memory64ListStream"sv,
            "CommentStreamA"sv,
            "CommentStreamW"sv,
            "HandleDataStream"sv,
            "FunctionTableStream"sv,
            "UnloadedModuleListStream"sv,
            "MiscInfoStream"sv,
            "MemoryInfoListStream"sv,
            "ThreadInfoListStream"sv,
            "HandleOperationListStream"sv,
            "TokenStream"sv,
            "JavaScriptDataStream"sv,
            "SystemMemoryInfoStream"sv,
            "ProcessVmCountersStream"sv,
            "IptTraceStream"sv,
            "ThreadNamesStream"sv,
        };

        return names[typeValue];
    } else if (typeValue >= 0x8000 && typeValue <= 0x800c) {
        static constexpr std::string_view names[] = {
            "ceStreamNull"sv,
            "ceStreamSystemInfo"sv,
            "ceStreamException"sv,
            "ceStreamModuleList"sv,
            "ceStreamProcessList"sv,
            "ceStreamThreadList"sv,
            "ceStreamThreadContextList"sv,
            "ceStreamThreadCallStackList"sv,
            "ceStreamMemoryVirtualList"sv,
            "ceStreamMemoryPhysicalList"sv,
            "ceStreamBucketParameters"sv,
            "ceStreamProcessModuleMap"sv,
            "ceStreamDiagnosisList"sv,
        };
        uint32_t index = typeValue - 0x8000;
        return names[index];
    } else if (typeValue == 0xffff) {
        return "LastReservedStream"sv;
    } else {
        return "Unknown"sv;
    }
}

std::unique_ptr<AbstractStream> AbstractStreamFactory::Create(Stream const& stream, GView::Utils::DataCache& data)
{
    switch (stream.type) {
    case StreamType::MiscInfoStream:
        return std::make_unique<MiscInfoStream>(stream, data);
    case StreamType::ThreadInfoListStream:
        return std::make_unique<ThreadInfoStream>(stream, data);
    case StreamType::ModuleListStream:
        return std::make_unique<ModuleListStream>(stream, data);
    case StreamType::FunctionTableStream:
        return std::make_unique<FunctionTableStream>(stream, data);
    case StreamType::Memory64ListStream:
        return std::make_unique<Memory64Stream>(stream, data);
    case StreamType::MemoryInfoListStream:
        return std::make_unique<MemoryInfoStream>(stream, data);
    case StreamType::SystemInfoStream:
        return std::make_unique<SystemInfoStream>(stream, data);
    case StreamType::HandleDataStream:
        return std::make_unique<HandleDataStream>(stream, data);
    default:
        return nullptr;
    }
}

MiscInfoStream::MiscInfoStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(MiscInfo)) {
        return;
    }

    data.Copy(stream.location.rva, this->miscInfo);
    isValid = true;
}

void MiscInfoStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }

    LocalString<255> fmt;

    parent.AddChild("Size", false).SetValues({ fmt.Format("%d", miscInfo.size) });
    parent.AddChild("Flags1", false).SetValues({ fmt.Format("0x%08X", miscInfo.flags1) });
    parent.AddChild("Process ID", false).SetValues({ fmt.Format("%d", miscInfo.processId) });
    parent.AddChild("Process Create Time", false).SetValues({ fmt.Format("%d", miscInfo.processCreateTime) });
    parent.AddChild("Process User Time", false).SetValues({ fmt.Format("%d", miscInfo.processUserTime) });
    parent.AddChild("Process Kernel Time", false).SetValues({ fmt.Format("%d", miscInfo.processKernelTime) });
}

ThreadInfoStream::ThreadInfoStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(ThreadInfoList)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);
    if (header.sizeOfEntry == 0) {
        return;
    }

    threads.reserve(header.numberOfEntries);
    for (uint32_t i = 0; i < header.numberOfEntries; i++) {
        ThreadInfo info;
        data.Copy(stream.location.rva + sizeof(ThreadInfoList) + i * header.sizeOfEntry, info);
        threads.emplace_back(info);
    }

    isValid = true;
}

void ThreadInfoStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }

    LocalString<255> fmt;
    parent.AddChild("NumberOfEntries", false).SetValues({ fmt.Format("%d", header.numberOfEntries) });
    auto threads = parent.AddChild("Threads", true);

    for (const auto& thread : this->threads) {
        auto threadNode = threads.AddChild(fmt.Format("Thread %d", thread.threadId), true);
        threadNode.AddChild("DumpFlags", false).SetValues({ fmt.Format("0x%08X", thread.dumpFlags) });
        threadNode.AddChild("DumpError", false).SetValues({ fmt.Format("0x%08X", thread.dumpError) });
        threadNode.AddChild("ExitStatus", false).SetValues({ fmt.Format("0x%08X", thread.exitStatus) });
        threadNode.AddChild("CreationTime", false).SetValues({ fmt.Format("%d", thread.createionTime) });
        threadNode.AddChild("ExitTime", false).SetValues({ fmt.Format("%d", thread.exitTime) });
        threadNode.AddChild("KernelTime", false).SetValues({ fmt.Format("%d", thread.kernelTime) });
        threadNode.AddChild("UserTime", false).SetValues({ fmt.Format("%d", thread.userTime) });
        threadNode.AddChild("StartAddress", false).SetValues({ fmt.Format("0x%08X", thread.startAddress) });
        threadNode.AddChild("Affinity", false).SetValues({ fmt.Format("0x%08X", thread.affinity) });
    }
}


ModuleListStream::ModuleListStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(ModuleList)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);

    modules.reserve(header.numberOfModules);
    moduleNames.reserve(header.numberOfModules);

    for (uint32_t i = 0; i < header.numberOfModules; i++) {
        Module module;
        data.Copy(stream.location.rva + sizeof(ModuleList) + i * sizeof(Module), module);
        modules.emplace_back(module);

        uint32_t name_length = 0;
        data.Copy(module.moduleNameRVA, name_length);
        auto name_buff = data.Get(module.moduleNameRVA + sizeof(uint32_t), name_length, true);

        UnicodeStringBuilder name((char16*) name_buff.GetData(), (size_t) name_length);
        std::string nameString{};
        name.ToString(nameString);

        moduleNames.emplace_back(nameString);
    }
    isValid = true;
}

void ModuleListStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }

    LocalString<255> fmt;
    parent.AddChild("NumberOfModules", false).SetValues({ fmt.Format("%d", header.numberOfModules) });
    auto threads = parent.AddChild("Modules", true);

    for (uint32_t i = 0; i < modules.size(); ++i) {
        auto& module = modules[i];
        auto& name   = moduleNames[i];
    
        auto moduleNode = threads.AddChild(fmt.Format("Module %s", name.c_str()), true);
        moduleNode.AddChild("BaseOfImage", false).SetValues({ fmt.Format("0x%016X", module.baseOfImage) });
        moduleNode.AddChild("SizeOfImage", false).SetValues({ fmt.Format("0x%08X", module.sizeOfImage) });
        moduleNode.AddChild("Checksum", false).SetValues({ fmt.Format("0x%08X", module.checksum) });
        moduleNode.AddChild("TimeDateStamp", false).SetValues({ fmt.Format("%d", module.timeDateStamp) });
        moduleNode.AddChild("ModuleNameRVA", false).SetValues({ fmt.Format("0x%08X", module.moduleNameRVA) });
        
        auto version = moduleNode.AddChild("VersionInfo", true);
        version.AddChild("Signature", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.signature) });
        version.AddChild("StructVersion", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.structVersion) });
        version.AddChild("FileVersionMS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileVersionMS) });
        version.AddChild("FileVersionLS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileVersionLS) });
        version.AddChild("ProductVersionMS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.productVersionMS) });
        version.AddChild("ProductVersionLS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.productVersionLS) });
        version.AddChild("FileFlagsMask", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileFlagsMask) });
        version.AddChild("FileFlags", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileFlags) });
        version.AddChild("FileOS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileOS) });
        version.AddChild("FileType", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileType) });
        version.AddChild("FileSubtype", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileSubtype) });
        version.AddChild("FileDateMS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileDateMS) });
        version.AddChild("FileDateLS", false).SetValues({ fmt.Format("0x%08X", module.versionInfo.fileDateLS) });

        auto cvRecord = moduleNode.AddChild("CVRecord", false);
        cvRecord.AddChild("RVA", false).SetValues({ fmt.Format("%d", module.cvRecord.rva) });
        cvRecord.AddChild("DataSize", false).SetValues({ fmt.Format("0x%08X", module.cvRecord.dataSize) });

        auto miscRecord = moduleNode.AddChild("MiscRecord", false);
        miscRecord.AddChild("RVA", false).SetValues({ fmt.Format("%d", module.miscRecord.rva) });
        miscRecord.AddChild("DataSize", false).SetValues({ fmt.Format("0x%08X", module.miscRecord.dataSize) });

    }
}

FunctionTableStream::FunctionTableStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(FunctionTable)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);

    if (header.sizeofHeader != sizeof(FunctionTable)) {
        return;
    }

    if (header.sizeofDescriptor != sizeof(FunctionTableDescriptor)) {
        return;
    }

    descriptors.reserve(header.numberOfDescriptors);

    for (uint32_t i = 0; i < header.numberOfDescriptors; i++) {
        FunctionTableDescriptor descriptor;
        data.Copy(stream.location.rva + sizeof(FunctionTable) + i * sizeof(FunctionTableDescriptor), descriptor);
        descriptors.emplace_back(descriptor);
    }

    isValid = true;
}

void FunctionTableStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }

    LocalString<255> fmt;
    parent.AddChild("SizeofHeader", false).SetValues({ fmt.Format("%d", header.sizeofHeader) });
    parent.AddChild("SizeofDescriptor", false).SetValues({ fmt.Format("%d", header.sizeofDescriptor) });
    parent.AddChild("SizeofNativeDescriptor", false).SetValues({ fmt.Format("%d", header.sizeofNativeDescritor) });
    parent.AddChild("SizeofFunctionEntry", false).SetValues({ fmt.Format("%d", header.sizeofFucntionEntry) });
    parent.AddChild("NumberOfDescriptors", false).SetValues({ fmt.Format("%d", header.numberOfDescriptors) });
    parent.AddChild("SizeofAlignPad", false).SetValues({ fmt.Format("%d", header.sizeofAlignPad) });

    auto descriptorsNode = parent.AddChild("Descriptors", true);
    {
        uint32_t idx = 0;
        for (const auto& descriptor : descriptors) {
            auto descriptorNode = descriptorsNode.AddChild(fmt.Format("Descriptor %d", idx), true);
            descriptorNode.AddChild("MinAddress", false).SetValues({ fmt.Format("0x%016X", descriptor.minAddress) });
            descriptorNode.AddChild("MaxAddress", false).SetValues({ fmt.Format("0x%016X", descriptor.maxAddress) });
            descriptorNode.AddChild("BaseAddress", false).SetValues({ fmt.Format("0x%016X", descriptor.baseAdress) });
            descriptorNode.AddChild("EntryCount", false).SetValues({ fmt.Format("%u", descriptor.entryCount) });
        }
    }
}

Memory64Stream::Memory64Stream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(Memory64List)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);
    if (header.numberOfMemoryRanges == 0) {
        return;
    }
    descriptors.reserve(header.numberOfMemoryRanges);
    for (uint64_t i = 0; i < header.numberOfMemoryRanges; i++) {
        Memory64Descriptor descriptor;
        data.Copy(stream.location.rva + sizeof(Memory64List) + i * sizeof(Memory64Descriptor), descriptor);
        descriptors.emplace_back(descriptor);
    }
    isValid = true;
}

void Memory64Stream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }
    LocalString<255> fmt;
    parent.AddChild("NumberOfMemoryRanges", false).SetValues({ fmt.Format("%d", header.numberOfMemoryRanges) });
    parent.AddChild("BaseRVA", false).SetValues({ fmt.Format("0x%016X", header.baseRVA) });
    auto descriptorsNode = parent.AddChild("Descriptors", true);
    {
        uint64_t idx = 0;
        for (const auto& descriptor : descriptors) {
            auto descriptorNode = descriptorsNode.AddChild(fmt.Format("Descriptor %d", idx++), true);
            descriptorNode.AddChild("StartOfMemoryRange", false).SetValues({ fmt.Format("0x%016X", descriptor.startOfMemoryRange) });
            descriptorNode.AddChild("Size", false).SetValues({ fmt.Format("0x%016X", descriptor.size) });
        }
    }
}

MemoryInfoStream::MemoryInfoStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(MemoryInfoList)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);
    if (header.numberOfEntries == 0) {
        return;
    }
    memoryInfos.reserve(header.numberOfEntries);
    for (uint32_t i = 0; i < header.numberOfEntries; i++) {
        MemoryInfo info;
        data.Copy(stream.location.rva + sizeof(MemoryInfoList) + i * header.sizeOfEntry, info);
        memoryInfos.emplace_back(info);
    }
    isValid = true;
}

void MemoryInfoStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }
    LocalString<255> fmt;
    parent.AddChild("SizeofHeader", false).SetValues({ fmt.Format("%d", header.sizeofHeader) });
    parent.AddChild("SizeofEntry", false).SetValues({ fmt.Format("%d", header.sizeOfEntry) });
    parent.AddChild("NumberOfEntries", false).SetValues({ fmt.Format("%d", header.numberOfEntries) });

    auto memoryInfosNode = parent.AddChild("MemoryInfos", true);
    {
        uint32_t idx = 0;
        for (const auto& entry : memoryInfos) {
            auto thisinfo = memoryInfosNode.AddChild(fmt.Format("MemoryInfo %d", idx++), true);

            thisinfo.AddChild("BaseAddress", false).SetValues({ fmt.Format("0x%016X", entry.baseAddress) });
            thisinfo.AddChild("AllocationBase", false).SetValues({ fmt.Format("0x%016X", entry.allocationBase) });
            thisinfo.AddChild("AllocationProtect", false).SetValues({ fmt.Format("0x%08X", entry.allocationProtect) });
            thisinfo.AddChild("RegionSize", false).SetValues({ fmt.Format("0x%016X", entry.regionSize) });
            thisinfo.AddChild("State", false).SetValues({ fmt.Format("0x%08X", entry.state) });
            thisinfo.AddChild("Protect", false).SetValues({ fmt.Format("0x%08X", entry.protect) });
            thisinfo.AddChild("Type", false).SetValues({ fmt.Format("0x%08X", entry.type) });
        }
    }
    
}


SystemInfoStream::SystemInfoStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(SystemInfo)) {
        return;
    }
    data.Copy(stream.location.rva, this->systemInfo);
    isValid = true;
}

void SystemInfoStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }
    LocalString<255> fmt;
    parent.AddChild("ProcessorArchitecture", false).SetValues({ fmt.Format("0x%08X", systemInfo.processorArchitecture) });
    parent.AddChild("ProcessorLevel", false).SetValues({ fmt.Format("%d", systemInfo.processorLevel) });
    parent.AddChild("ProcessorRevision", false).SetValues({ fmt.Format("%d", systemInfo.processorRevision) });
    parent.AddChild("NumberOfProcessors", false).SetValues({ fmt.Format("%d", systemInfo.numberOfProcessors) });
    parent.AddChild("ProductType", false).SetValues({ fmt.Format("%d", systemInfo.productType) });
    parent.AddChild("MajorVersion", false).SetValues({ fmt.Format("%d", systemInfo.majorVersion) });
    parent.AddChild("MinorVersion", false).SetValues({ fmt.Format("%d", systemInfo.minorVersion) });
    parent.AddChild("BuildNumber", false).SetValues({ fmt.Format("%d", systemInfo.buildNumber) });
    parent.AddChild("PlatformId", false).SetValues({ fmt.Format("%d", systemInfo.platformId) });
    parent.AddChild("CSDVersionRVA", false).SetValues({ fmt.Format("0x%08X", systemInfo.csdVersionRVA) });
    parent.AddChild("SuiteMask", false).SetValues({ fmt.Format("0x%08X", systemInfo.suiteMask) });
    
    auto cpu = parent.AddChild("CPU", true);
    cpu.AddChild("VendorId", false)
          .SetValues({ fmt.Format("0x%08X 0x%08X 0x%08X", systemInfo.cpu.vendorId[0], systemInfo.cpu.vendorId[1], systemInfo.cpu.vendorId[2]) });
    cpu.AddChild("VersionInformation", false).SetValues({ fmt.Format("0x%08X", systemInfo.cpu.versionInformation) });
    cpu.AddChild("FeatureInformation", false).SetValues({ fmt.Format("0x%08X", systemInfo.cpu.featureInformation) });
    cpu.AddChild("AMDExtendedCpuFeatures", false).SetValues({ fmt.Format("0x%08X", systemInfo.cpu.AMDExtendedCpuFeatures) });
}


HandleDataStream::HandleDataStream(Stream const& stream, GView::Utils::DataCache& data)
{
    if (stream.location.dataSize < sizeof(HandleDataList)) {
        return;
    }
    data.Copy(stream.location.rva, this->header);
    if (header.sizeOfDescriptor == 0) {
        return;
    }
    descriptors.reserve(header.numberOfDescriptors);
    for (uint32_t i = 0; i < header.numberOfDescriptors; i++) {
        HandleDataDescritpr descriptor;
        data.Copy(stream.location.rva + sizeof(HandleDataList) + i * header.sizeOfDescriptor, descriptor);
        descriptors.emplace_back(descriptor);
    }
    isValid = true;
}

void HandleDataStream::PopulateView(TreeViewItem& parent)
{
    if (!isValid) {
        parent.SetValues({ "Invalid stream" });
        parent.SetExpandable(false);
        return;
    }
    LocalString<255> fmt;
    parent.AddChild("SizeOfHeader", false).SetValues({ fmt.Format("%d", header.sizeOfHeader) });
    parent.AddChild("SizeOfDescriptor", false).SetValues({ fmt.Format("%d", header.sizeOfDescriptor) });
    parent.AddChild("NumberOfDescriptors", false).SetValues({ fmt.Format("%d", header.numberOfDescriptors) });

    auto descriptors = parent.AddChild("Descriptors", true);
    {
        uint32_t idx = 0;
        for (const auto& desc : this->descriptors) {
            auto descNode = descriptors.AddChild(fmt.Format("Descriptor %d", idx++), true);
            descNode.AddChild("Handle", false).SetValues({ fmt.Format("0x%016X", desc.handle) });
            descNode.AddChild("TypeNameRVA", false).SetValues({ fmt.Format("0x%08X", desc.typeNameRVA) });
            descNode.AddChild("ObjectNameRVA", false).SetValues({ fmt.Format("0x%08X", desc.objectNameRVA) });
            descNode.AddChild("Attributes", false).SetValues({ fmt.Format("0x%08X", desc.attributes) });
            descNode.AddChild("GrantedAccess", false).SetValues({ fmt.Format("0x%08X", desc.grantedAccess) });
            descNode.AddChild("HandleCount", false).SetValues({ fmt.Format("0x%08X", desc.handleCount) });
            descNode.AddChild("PointerCount", false).SetValues({ fmt.Format("0x%08X", desc.pointerCount) });
            descNode.AddChild("ObjectInfoRVA", false).SetValues({ fmt.Format("0x%08X", desc.objectInfoRVA) });
        }
    }
}




} // namespace MDMP


static void CreateBufferView(Reference<GView::View::WindowInterface> win, Reference<MDMP::MDMPFile> mdmp)
{
    BufferViewer::Settings settings;

    settings.AddZone(0, sizeof(MDMP::Header), ColorPair{ Color::Magenta, Color::DarkBlue }, "Header");
    // settings.AddZone(sizeof(BMP::Header), sizeof(BMP::InfoHeader), ColorPair{ Color::Olive, Color::DarkBlue }, "Image entries");


    for (uint32_t idx = 0, offset = mdmp->header.streamDirectoryRVA; idx < mdmp->header.numberOfStreams; ++idx, offset += sizeof(MDMP::Stream)) {
        settings.AddZone(offset, sizeof(MDMP::Stream), ColorPair{ Color::Olive, Color::DarkBlue }, "StreamEntry");
    }

    for (auto& stream : mdmp->streams) {
        settings.AddZone(
              stream.location.rva,
              stream.location.dataSize,
              ColorPair{ Color::Olive, Color::DarkBlue },
              MDMP::getStreamName(static_cast<MDMP::StreamType>(stream.type)));
    }
    
    mdmp->selectionZoneInterface = win->GetSelectionZoneInterfaceFromViewerCreation(settings);
}

extern "C"
{
    PLUGIN_EXPORT bool Validate(const AppCUI::Utils::BufferView& buf, const std::string_view& extension)
    {
        // check the signature and that we have at least the header
        if (buf.GetLength() < sizeof(MDMP::Header))
            return false;

        auto header = buf.GetObject<MDMP::Header>();
        if (!header.IsValid())
            return false;

        bool signatureMatches = memcmp(&header->signature, MDMP::Magic, sizeof(MDMP::Magic)) == 0;
        if (!signatureMatches)
            return false;

        return true;
    }

    PLUGIN_EXPORT TypeInterface* CreateInstance()
    {
        return new MDMP::MDMPFile();
    }

    
    /*
    void CreateImageView(Reference<GView::View::WindowInterface> win, Reference<BMP::BMPFile> bmp)
    {
        GView::View::ImageViewer::Settings settings;
        settings.SetLoadImageCallback(bmp.ToBase<View::ImageViewer::LoadImageInterface>());
        settings.AddImage(0, bmp->obj->GetData().GetSize());
        win->CreateViewer(settings);
    }*/

    PLUGIN_EXPORT bool PopulateWindow(Reference<GView::View::WindowInterface> win)
    {
        auto mdmp = win->GetObject()->GetContentType<MDMP::MDMPFile>();
        mdmp->Update();

        // add viewer
        //CreateImageView(win, bmp);
        CreateBufferView(win, mdmp);

        //// add panels
        win->AddPanel(Pointer<TabPage>(new MDMP::Panels::Information(mdmp)), true);
        win->AddPanel(Pointer<TabPage>(new MDMP::Panels::Data(mdmp)), false);

        return true;
    }

    PLUGIN_EXPORT void UpdateSettings(IniSection sect)
    {
        sect["Pattern"]     = "magic:4D 44 4D 50"; // MDMP
        sect["Priority"]    = 1;
        sect["Description"] = "Windows memory dump (*.DMP)";
    }
}

int main()
{
    return 0;
}
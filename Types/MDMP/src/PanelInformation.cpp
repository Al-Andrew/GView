#include "mdmp.hpp"


namespace MDMP::Panels
{

	Information::Information(Reference<MDMP::MDMPFile> mdmp) : TabPage("&Information")
    {
        this->mdmp    = mdmp;
        this->general = Factory::ListView::Create(this, "x:0,y:0,w:100%,h:50%", {"n:Field, w:50%", "n:Value, w:50%"}, ListViewFlags::None);
        this->streams = Factory::ListView::Create(this, "x:0,y:50%,w:100%,h:50%", { "n:Stream, w:33%", "n:RVA, w:33%", "n: Size, w:33%" }, ListViewFlags::None);
    
        this->Update();
    }

    void Information::UpdateGeneralInformation()
    {
        this->general->DeleteAllItems();

        LocalString<255> fmt;

        this->general->AddItem({ "Type", "MDMP" });
        this->general->AddItem({ "Version", fmt.Format("%04X", this->mdmp->header.version & 0xFFFF ) });
        this->general->AddItem({ "NumberOfStreams", fmt.Format("%d", this->mdmp->header.numberOfStreams) });
        this->general->AddItem({ "StreamDirectoryRVA", fmt.Format("0x%08X", this->mdmp->header.streamDirectoryRVA) });
        this->general->AddItem({ "Checksum", fmt.Format("0x%08X", this->mdmp->header.checksum) });
        this->general->AddItem({ "TimeDateStamp", fmt.Format("%d", this->mdmp->header.timeDateStamp) });
        this->general->AddItem({ "Flags", fmt.Format("0x%08X", this->mdmp->header.flags) });

    }

    void Information::UpdateStreams()
    {
        this->streams->DeleteAllItems();

        LocalString<255> rva_fmt;
        LocalString<255> size_fmt;

        for (const auto& stream : this->mdmp->streams) {

            this->streams->AddItem({
                  MDMP::getStreamName(stream.type), rva_fmt.Format("0x%08X", stream.location.rva), size_fmt.Format("%u bytes", stream.location.dataSize)
            });

            rva_fmt.Clear();
            size_fmt.Clear();
        }
    
    }

    void Information::RecomputePanelsPositions()
    {
        this->general->Resize(GetWidth(), GetHeight());
        this->streams->Resize(GetWidth(), GetHeight());
    }

    void Information::Update()
    {
        this->UpdateGeneralInformation();
        this->UpdateStreams();
        this->RecomputePanelsPositions();
    }


    Data::Data(Reference<MDMP::MDMPFile> mdmp) : TabPage("&Data")
    {
        this->mdmp = mdmp;
        this->streamsData = Factory::TreeView::Create(this, "x:0,y:0, h:100%, w:100%", { "n:Item, w:50%", "n:Info, w:50%" });

        this->Update();
    }

    void Data::RecomputePanelsPositions()
    {
        this->streamsData->Resize(GetWidth(), GetHeight());
    }

    void Data::Update()
    {
        this->streamsData->ClearItems();

        auto root = this->streamsData->AddItem("Minidump", true);

        for (const auto& stream : this->mdmp->streamsData) {
            auto streamNode = root.AddChild(MDMP::getStreamName(stream.first), true);
            stream.second->PopulateView(streamNode);
        }

        this->RecomputePanelsPositions();
    }
}
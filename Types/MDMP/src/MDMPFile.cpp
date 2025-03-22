#include "mdmp.hpp"

#include <string_view>
using namespace std::string_view_literals;

namespace MDMP {


    MDMPFile::MDMPFile()
    {
        memset(&this->header, 0, sizeof(this->header));
        this->streams.clear();
    }

    MDMPFile::~MDMPFile()
    {
    }

    std::string_view MDMPFile::GetTypeName()
    {
        return "MDMP"sv;
    } 

    bool MDMPFile::UpdateKeys(KeyboardControlsInterface*)
    {
        return false;
    }

    void MDMPFile::RunCommand(std::string_view)
    {
    }

    bool MDMPFile::Update()
    {
        this->obj->GetData().Copy<Header>(0, this->header);
        uint32 offset = header.streamDirectoryRVA;

        this->streams.reserve(this->header.numberOfStreams);

        for (uint32 i = 0; i < this->header.numberOfStreams; i++, offset += sizeof(MDMP::Stream)) {
            auto stream = this->obj->GetData().Get(offset, sizeof(MDMP::Stream), true);

            if (!stream.IsValid())
                return false;

            streams.emplace_back(stream.GetObject<MDMP::Stream>());
            auto as = AbstractStreamFactory::Create(streams.back(), this->obj->GetData());
            if (as) {
                streamsData[streams.back().type] = std::move(as);
            }
        }

        return true;
    }

}
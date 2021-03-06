#include "metadataframe.h"

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"
#include "../../strings.h"

MetaDataFrame::MetaDataFrame() {
}

MetaDataFrame::~MetaDataFrame() {
}

void MetaDataFrame::renderDiskInformation(const FrameData& frameData) {
    ImGui::NewLine();
    ImGui::TextUnformatted(STR_DISK_INFORMATION);
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

    char temp[32];
    const auto& style = ImGui::GetStyle();
    const auto rightMargin = ((ImGui::GetWindowContentRegionWidth()/2) + style.FramePadding.x) * 0.33f;

    if (!frameData.metaData.diskInformation.title.empty()) {
        ImGui::TextUnformatted(STR_TITLE);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.diskInformation.title.c_str());
    }

    if (!frameData.metaData.diskInformation.ripper.empty()) {
        ImGui::TextUnformatted(STR_RIPPER);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.diskInformation.ripper.c_str());
    }

    if (!frameData.metaData.diskInformation.converter.empty()) {
        ImGui::TextUnformatted(STR_CONVERTER);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.diskInformation.converter.c_str());
    }

    if (!frameData.metaData.diskInformation.copyright.empty()) {
        ImGui::TextUnformatted(STR_COPYRIGHT);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.diskInformation.copyright.c_str());
    }

    if (frameData.metaData.diskInformation.trackCount > 0) {
        sprintf(temp, "%d", frameData.metaData.diskInformation.trackCount);    
        ImGui::TextUnformatted(STR_TRACK_COUNT);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(temp);
    }

    if (frameData.metaData.diskInformation.duration > 0) {
        sprintf(temp, "%d:%02d", frameData.metaData.diskInformation.duration / 60, frameData.metaData.diskInformation.duration % 60);    
        ImGui::TextUnformatted(STR_TOTAL_DURATION);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(temp);
    }
}

void MetaDataFrame::renderTrackInformation(const FrameData& frameData) {
    ImGui::NewLine();
    ImGui::TextUnformatted(STR_TRACK_INFORMATION);
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

    if (!ImGui::BeginChild(ImGui::GetID("trackMetaData"), ImVec2(0,ImGui::GetContentRegionAvail().y), false)) {
        ImGui::EndChild();
        return;
    }

    char temp[32];
    const auto rightMargin = ImGui::GetWindowContentRegionWidth() * 0.33f;

    if (!frameData.metaData.trackInformation.title.empty()) {
        ImGui::TextUnformatted(STR_TITLE);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.trackInformation.title.c_str());
    }

    if (!frameData.metaData.trackInformation.author.empty()) {
        ImGui::TextUnformatted(STR_AUTHOR);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.trackInformation.author.c_str());
    }

    if (!frameData.metaData.trackInformation.copyright.empty()) {
        ImGui::TextUnformatted(STR_COPYRIGHT);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(frameData.metaData.trackInformation.copyright.c_str());
    }

    if (frameData.metaData.diskInformation.trackCount > 1) {
        sprintf(temp, "%d/%d", frameData.metaData.trackInformation.trackNumber, frameData.metaData.diskInformation.trackCount);    
        ImGui::TextUnformatted(STR_TRACK_NUMBER);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(temp);
    }

    if (frameData.metaData.trackInformation.position > -1) {
        sprintf(temp, "%d:%02d", frameData.metaData.trackInformation.position / 60, frameData.metaData.trackInformation.position % 60);    
        ImGui::TextUnformatted(STR_PLAY_TIME);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(temp);
    } 

    if (frameData.metaData.trackInformation.duration > 0) {
        sprintf(temp, "%d:%02d", frameData.metaData.trackInformation.duration / 60, frameData.metaData.trackInformation.duration % 60);    
        ImGui::TextUnformatted(STR_DURATION);
        ImGui::SameLine(rightMargin);
        ImGui::TextUnformatted(temp);
    } 

    if (!frameData.metaData.trackInformation.comment.empty()) {
        ImGui::NewLine();
        ImGui::TextUnformatted(STR_COMMENTS);
        ImGui::Separator();
        ImGui::TextWrapped("%s", frameData.metaData.trackInformation.comment.c_str());
    }

    ImGui::EndChild();
}

void MetaDataFrame::render(const FrameData& frameData) {
    const auto disabled = frameData.state != SoundEngine::State::PAUSED
        && frameData.state != SoundEngine::State::STARTED;

    if (disabled) {
        // Don't draw anything
        return;
    }

    if (frameData.metaData.hasDiskInformation) {
        renderDiskInformation(frameData);
    }
    renderTrackInformation(frameData);
}

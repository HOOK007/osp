#include "aboutwindow.h"

#include "../../imgui/imgui.h"
#include "../../strings.h"

#include <sc68/sc68.h>
#include <dumb.h>
#include <sidplayfp/sidplayfp.h>
#include <SDL2/SDL.h>

AboutWindow::AboutWindow() :
    Window(STR_ABOUT_WINDOW_TITLE) {
}

AboutWindow::~AboutWindow() {
}

void AboutWindow::render(const GLuint texture, std::shared_ptr<SpriteCatalog> catalog) {
    if (!mVisible) {
        return;
    }

    const auto logoSprite = catalog->getFrame("logo");
    const auto io = ImGui::GetIO();
    const auto style = ImGui::GetStyle();
    const auto windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (!ImGui::IsPopupOpen(STR_ABOUT_WINDOW_TITLE)) {
        ImGui::OpenPopup(STR_ABOUT_WINDOW_TITLE);
    }
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal(STR_ABOUT_WINDOW_TITLE, &mVisible, windowFlags)) {
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvailWidth()/2 - logoSprite.size.x/2 + style.WindowPadding.x);
        ImGui::Image((ImTextureID)(intptr_t) texture, logoSprite.size, logoSprite.uv0, logoSprite.uv1);

        ImGui::Spacing();
        ImGui::TextUnformatted("OSP is a chiptune player that can handle several\n"
                                "old sound format produced in the early years of\n"
                                "computer sound and hacking until now.");

        ImGui::NewLine();
        ImGui::TextUnformatted("This program make use of:");
        ImGui::BulletText("SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
        ImGui::BulletText("Dear ImGui %s", ImGui::GetVersion());
        ImGui::BulletText("libdumb %s", DUMB_VERSION_STR);
        ImGui::BulletText("libgme 0.6.3");
        ImGui::BulletText("%s", sc68_versionstr());
        ImGui::BulletText("libsidplayfp %d.%d.%d", LIBSIDPLAYFP_VERSION_MAJ, LIBSIDPLAYFP_VERSION_MIN, LIBSIDPLAYFP_VERSION_LEV);
        ImGui::NewLine();
        ImGui::TextUnformatted("Embeded font:");
        ImGui::BulletText("Atari ST 8x16 System");
        ImGui::BulletText("ProggyClean");
        ImGui::NewLine();
        ImGui::TextUnformatted("Version: " GIT_VERSION " (" GIT_COMMIT ")");
        ImGui::TextUnformatted("Build date: " BUILD_DATE);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(ImGui::GetContentRegionAvailWidth() - style.WindowPadding.x - ImGui::CalcTextSize(STR_CLOSE).x);
        if (ImGui::Button(STR_CLOSE "##closeAbout")) {
            mVisible = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

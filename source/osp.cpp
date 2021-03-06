#include "osp.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "platform.h" 
#include "strings.h"
#include "app_settings_strings.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_image.h>

Osp::Osp() :
    mShowWorkspace(true),
    mTextureSprites(0),
    mStatusMessage("Initializing..."),
    mLastFileSelected(""),
    mSettings(nullptr),
    mSpriteCatalog(nullptr),
    mFileManager(nullptr),
    mSoundEngine(nullptr) {
}

Osp::~Osp() {
}

bool Osp::setup(const std::string dataPath) {
    mSettings = std::shared_ptr<Settings>(new Settings());
    mSettings->load(CONFIG_FILENAME);

    // Load spritesheets
    mSpriteCatalog = std::shared_ptr<SpriteCatalog>(new SpriteCatalog());
    if (!mSpriteCatalog->setup(std::string(dataPath).append("/spritesheet/spritesheet.json"))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to load the spritesheet catalog: %s.\n", mSpriteCatalog->getError().c_str());
        return false;
    }

    // Load sprite texture
    glGenTextures(1, &mTextureSprites);
    if (mTextureSprites == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the spritesheet texture.\n");
        return false;
    }

    const auto spritesheet = IMG_Load(std::string(dataPath).append("/spritesheet/spritesheet.png").c_str());
    if (spritesheet == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to load the spritesheet texture.\n");
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, mTextureSprites);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, spritesheet->w, spritesheet->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, spritesheet->pixels);
    SDL_FreeSurface(spritesheet);

    // Setup sound engine
    mSoundEngine = std::unique_ptr<SoundEngine>(new SoundEngine());
    if (!mSoundEngine->setup(dataPath.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize SoundEngine.\n");
        return false;
    }

    // Apply user configuration
    auto& io = ImGui::GetIO();
    switch (mSettings->getInt(KEY_APP_STYLE, APP_STYLE_DEFAULT)) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
    }

    const auto font = mSettings->getInt(KEY_APP_FONT, APP_FONT_DEFAULT);
    if (font >= 0 && font < io.Fonts->Fonts.Size) {
        io.FontDefault = io.Fonts->Fonts[font];
    }
   
    // Setup file manager
    mFileManager = std::unique_ptr<FileManager>(new FileManager());
    if (!mFileManager->setup()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to initialize File systems.\n");
        return false;
    }

    const auto mouseEmulation = mSettings->getBool(KEY_APP_MOUSE_EMULATION, APP_MOUSE_EMULATION_DEFAULT);
    ImGui_ImplSDL2_SetMouseEmulationWithGamepad(mouseEmulation);
    if (mouseEmulation && !PLATFORM_HAS_MOUSE_CURSOR) {
        io.MouseDrawCursor = true;
    }

    const auto touchEnabled = mSettings->getBool(KEY_APP_TOUCH_ENABLED, APP_TOUCH_ENABLED_DEFAULT);
    if (touchEnabled) {
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    }

    // OK
    mStatusMessage = STR_READY;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP initialized.\n");
    return true;
}

void Osp::cleanup() {
    mSoundEngine->cleanup();
    mFileManager->cleanup();
    mSpriteCatalog->cleanup();
    
    glDeleteTextures(1, &mTextureSprites);
    mTextureSprites = 0;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "OSP cleanup.\n");
}

void Osp::render() {
    auto& io = ImGui::GetIO();
    const auto songMetaData = mSoundEngine->getMetaData();
    const auto fmState = mFileManager->getState();
    const auto sndState = mSoundEngine->getState();

    // Manage state of FileManager and SoundEngine
    if (fmState != FileManager::State::READY) {
        switch (fmState) {
            case FileManager::State::ERROR:
                mStatusMessage = mFileManager->getError();
                break;
            default:
                break;
        }
    }
    else {
        if (fmState == FileManager::State::ERROR) {
            mFileManager->clearError();
        } 

        // SoundEngine states
        switch (sndState) {
            case SoundEngine::State::FINISHED_NATURAL: {
                    const auto skipUnsupportedTunes = mSettings->getBool(KEY_APP_SKIP_UNSUPPORTED_TUNES, APP_SKIP_UNSUPPORTED_TUNES_DEFAULT);
                    selectNextTrack(skipUnsupportedTunes, true);
                }
                break;
            case SoundEngine::State::STARTED:
                mStatusMessage = STR_PLAYING;
                break;
            case SoundEngine::State::PAUSED:
                mStatusMessage = STR_PAUSED;
                break;
            case SoundEngine::State::FINISHED:        
                mStatusMessage = STR_READY;
                break;
            case SoundEngine::State::ERROR:
                mStatusMessage = mSoundEngine->getError();
                break;
            default:
                break;
        }
    }

    // Manage UI states / rendering
    auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    if (!mShowWorkspace) {
        windowFlags |= ImGuiWindowFlags_NoBackground;
    }

    // Main Window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("Workspace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    // Menu
    mMenuBar.render({
            .message = mStatusMessage,
            .fmState = fmState,
            .itemShowWorkspaceCheked = mShowWorkspace,
            .settings = mSettings,
        },
        [&](int style) {
            handleStyleChange(style);
        },
        [&](ImFont* font, int n) {
            handleFontChange(font, n);
        },
        [&](MenuBar::ItemId action) {
            handleMenuBarAction(action);
        });

    ImGui::PopStyleVar(1);

    // Workspace
    if (mShowWorkspace) {
        ImGui::Columns(2, "workspaceSeparator", false);

        //Explorer
        const auto selectedItem = !mLastFileSelected.empty()
            ? mLastFileSelected
            : mFileManager->getLastFolder();

        mExplorerFrame.render({
                .currentPath = mFileManager->getCurrentPath(),
                .listing = mFileManager->getCurrentPathEntries(),
                .selectedItemName = selectedItem,
                .isWorking = fmState == FileManager::State::LOADING
            },
            [&](FileSystem::Entry item) {
                handleExplorerItemClick(item, mFileManager->getCurrentPath());
            });

        ImGui::NextColumn();

        // Player
        mPlayerFrame.render({
                .texture = mTextureSprites,
                .state = sndState,
                .metaData = songMetaData,
                .catalog = mSpriteCatalog
            },
            [&](PlayerFrame::ButtonId button) { 
                handlePlayerButtonClick(button);
            });

        // Song meta data
        mMetaDataFrame.render({
                .state = sndState,
                .metaData = songMetaData
            });

        ImGui::Columns(1);
    }        
    ImGui::End();

    // Other windows & popups
    mSettingsWindow.render({
            .settings = mSettings
        },
        [&](SettingsWindow::AppSetting setting, bool value) {
            handleAppSettingsChange(setting, value);
        },
        [&](std::string key, int value) {
            mSettings->putInt(key, value);
            mSettings->save(CONFIG_FILENAME);
        },
        [&](std::string key, bool value) {
            mSettings->putBool(key, value);
            mSettings->save(CONFIG_FILENAME);
        });

    mMetricsWindow.render();
    mAboutWindow.render(mTextureSprites, mSpriteCatalog);
}

void Osp::selectNextTrack(bool skipInvalid, bool autoPlay) {
    const auto skipSubTunes = mSettings->getBool(KEY_APP_SKIP_SUBTUNES, APP_SKIP_SUBTUNES_DEFAULT);
    if (skipSubTunes || !mSoundEngine->nextTrack()) {
        if (const auto nextFileName = getNextFileName();
            nextFileName.empty() == false) {

            if (!engineLoad(mFileManager->getCurrentPath(), nextFileName)) {
                // Go until we found something to play
                if (skipInvalid) {
                    selectNextTrack(skipInvalid, autoPlay);
                }
            } else if (autoPlay) {
                mSoundEngine->play();
            }
        }
        else {
            mSoundEngine->stop();
        }
    }
}

void Osp::selectPrevTrack(bool skipInvalid, bool autoPlay) {
    const auto skipSubTunes = mSettings->getBool(KEY_APP_SKIP_SUBTUNES, APP_SKIP_SUBTUNES_DEFAULT);
    if (skipSubTunes || !mSoundEngine->prevTrack()) {
        if (const auto prevFileName = getPrevFileName();
            prevFileName.empty() == false) {
        
            if (!engineLoad(mFileManager->getCurrentPath(), prevFileName)) {
                // Go until we found something to play
                if (skipInvalid) {
                    selectPrevTrack(skipInvalid, autoPlay);
                }
            } else if (autoPlay) {
                mSoundEngine->play();
            }
        }
        else {
            mSoundEngine->stop();
        }
    }
}

std::string Osp::getPrevFileName() const {
    const auto items = mFileManager->getCurrentPathEntries();
    const auto itemCount = items.size();
    
    // No item selected, return first file from the end if exists
    if (mLastFileSelected.empty()) {
        for (size_t i=itemCount-1; i>0; i--) {
            const auto entry = items[i];
            if (!entry.folder) {
                return entry.name; 
            }
        }
        return mLastFileSelected;
    }

    // Try to find the previous item
    bool byPass = false;
    for (size_t i=itemCount-1; i>1; i--) {
        const auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            const auto previous = items[i-1];
            if (previous.folder) {
                byPass = true;
                continue;
            }

            return previous.name;
        }
    }

    return "";
}

std::string Osp::getNextFileName() const {
    const auto items = mFileManager->getCurrentPathEntries();
    const auto itemCount = items.size();
    
    // No item selected, return first file item if exists
    if (mLastFileSelected.empty()) {
        for (size_t i=0; i<itemCount; i++) {
            const auto entry = items[i];
            if (!entry.folder) {
                return entry.name;
            }
        }
        return mLastFileSelected;
    }


    // Try to find the next item
    bool byPass = false;
    for (size_t i=0; i<itemCount-1; i++) {
        const auto entry = items[i];
        if (entry.name == mLastFileSelected || byPass) {
            const auto next = items[i+1];
            if (next.folder) {
                byPass = true;
                continue;
            }

            return next.name;
        }
    }

    return "";
}

void Osp::handleExplorerItemClick(const FileSystem::Entry item, const std::filesystem::path currentExplorerPath) {
    const auto sndState = mSoundEngine->getState();

    if (item.folder) {
        mLastFileSelected.clear();
        if (! mFileManager->navigate(item.name.c_str())) {
            // If FileManager process don't started because of an error
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", mStatusMessage.c_str());
        } else {
            // If we were in error state it's time to clean
            if (sndState == SoundEngine::State::ERROR) {
                mSoundEngine->clearError();
            }
        }
    } else {
        if (mLastFileSelected != item.name || sndState == SoundEngine::State::FINISHED) {
            if (engineLoad(mFileManager->getCurrentPath(), item.name)) {
                mSoundEngine->play();
            }
        }
    }
}

void Osp::handlePlayerButtonClick(const PlayerFrame::ButtonId button) {
    const auto sndState = mSoundEngine->getState();

    switch (button) {
        case PlayerFrame::ButtonId::PLAY:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                    mSoundEngine->pause();
                    break;
                case SoundEngine::State::PAUSED:
                    mSoundEngine->play();
                    break;
                case SoundEngine::State::FINISHED:
                    if (! mLastFileSelected.empty()) {
                        if (engineLoad(mFileManager->getCurrentPath(), mLastFileSelected)) {
                            mSoundEngine->play();
                        }
                    }
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::STOP:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                    mSoundEngine->stop();
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::NEXT:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                case SoundEngine::State::FINISHED:
                case SoundEngine::State::ERROR: {
                    const auto skipUnsupportedTunes = mSettings->getBool(KEY_APP_SKIP_UNSUPPORTED_TUNES, APP_SKIP_UNSUPPORTED_TUNES_DEFAULT);
                    selectNextTrack(skipUnsupportedTunes,
                        sndState != SoundEngine::State::FINISHED && sndState != SoundEngine::State::ERROR);
                    }
                    break;
                default:
                    break;
            }
        break;
        case PlayerFrame::ButtonId::PREV:
            switch (sndState) {
                case SoundEngine::State::STARTED:
                case SoundEngine::State::PAUSED:
                case SoundEngine::State::FINISHED:
                case SoundEngine::State::ERROR: {
                    const auto skipUnsupportedTunes = mSettings->getBool(KEY_APP_SKIP_UNSUPPORTED_TUNES, APP_SKIP_UNSUPPORTED_TUNES_DEFAULT);
                    selectPrevTrack(skipUnsupportedTunes,
                        sndState != SoundEngine::State::FINISHED && sndState != SoundEngine::State::ERROR);
                    }
                    break;
                default:
                    break;
            }
        break;
    }
}

void Osp::handleAppSettingsChange(const SettingsWindow::AppSetting setting, bool value) {
    auto& io = ImGui::GetIO();

    switch (setting) {
        case SettingsWindow::AppSetting::MOUSE_EMULATION: {
            mSettings->putBool(KEY_APP_MOUSE_EMULATION, value);
            ImGui_ImplSDL2_SetMouseEmulationWithGamepad(value);
            io.MouseDrawCursor = !PLATFORM_HAS_MOUSE_CURSOR;
            break;
        }
        case SettingsWindow::AppSetting::TOUCH_ENABLED: {
            mSettings->putBool(KEY_APP_TOUCH_ENABLED, value);
            if (!value) {
                io.ConfigFlags &= ~ImGuiConfigFlags_IsTouchScreen;
            } else {
                io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
            }
            break;
        }
        case SettingsWindow::AppSetting::AUTOSKIP_UNSUPPORTED_FILES: {
            mSettings->putBool(KEY_APP_SKIP_UNSUPPORTED_TUNES, value);
            break;
        }
        case SettingsWindow::AppSetting::SKIP_SUBTUNES: {
            mSettings->putBool(KEY_APP_SKIP_SUBTUNES, value);
            break;
        }
        case SettingsWindow::AppSetting::ALWAYS_START_FIRST_TRACK: {
            mSettings->putBool(KEY_APP_ALWAYS_START_FIRST_TUNE, value);
            break;
        }
    }
    mSettings->save(CONFIG_FILENAME);
}

void Osp::handleStyleChange(int style) {
    switch (style) {
        case 0: ImGui::StyleColorsDark();
            break;
        case 1: ImGui::StyleColorsLight();
            break;
        case 2: ImGui::StyleColorsClassic();
            break;
    }
    mSettings->putInt(KEY_APP_STYLE, style);
    mSettings->save(CONFIG_FILENAME);
}

void Osp::handleFontChange(ImFont* font, int fontIndex) {
    auto& io = ImGui::GetIO();

    io.FontDefault = font;
    mSettings->putInt(KEY_APP_FONT, fontIndex);
    mSettings->save(CONFIG_FILENAME);
}

void Osp::handleMenuBarAction(const MenuBar::ItemId action) {
    switch (action) {
        case MenuBar::ItemId::TOGGLE_WORKSPACE_VISIBILITY:
            mShowWorkspace = !mShowWorkspace;
            break;
        case MenuBar::ItemId::SHOW_SETTINGS:
            mSettingsWindow.setVisible(true);
            break;
        case MenuBar::ItemId::SHOW_ABOUT:
            mAboutWindow.setVisible(true);
            break;
        case MenuBar::ItemId::SHOW_METRICS:
            mMetricsWindow.setVisible(true);
            break;
        case MenuBar::ItemId::QUIT:
            SDL_Event event;
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
            break;
    }
}

bool Osp::engineLoad(std::string path, std::string filename) {
    const auto file = mFileManager->getFile(path.append("/").append(filename));
    
    mLastFileSelected = filename;
    if (! mSoundEngine->load(file, mSettings)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", mSoundEngine->getError().c_str());
        return false;
    }

    return true;
}

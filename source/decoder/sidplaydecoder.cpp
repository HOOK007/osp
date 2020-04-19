#include "sidplaydecoder.h"

#include <fstream>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>
#include <SDL2/SDL_log.h>

SidPlayDecoder::SidPlayDecoder(const std::string dataPath) : Decoder(),
    mKernalPath(std::string(dataPath).append("/kernal")),
    mBasicPath(std::string(dataPath).append("/basic")),
    mChargenPath(std::string(dataPath).append("/chargen")),
    mSIDBuilder(nullptr),
    mTune(nullptr) {
}

SidPlayDecoder::~SidPlayDecoder() {
}

bool SidPlayDecoder::setup() {
    const auto kernal = std::unique_ptr<char[]>(loadRom(mKernalPath.c_str(), 8192));
    const auto basic = std::unique_ptr<char[]>(loadRom(mBasicPath.c_str(), 8192));
    const auto chargen = std::unique_ptr<char[]>(loadRom(mChargenPath.c_str(), 4096));
    const auto maxsids = (mPlayer.info()).maxsids();

    mPlayer.setRoms((const uint8_t*) kernal.get(), (const uint8_t*) basic.get(), (const uint8_t*) chargen.get());
    mSIDBuilder = std::unique_ptr<sidbuilder>(new ReSIDfpBuilder("OSP"));
    mSIDBuilder->create(maxsids);
    if (!mSIDBuilder->getStatus()) {
        mError = std::string("sid error: ").append(mSIDBuilder->error());
        return false;
    }

    SidConfig cfg;
    cfg.frequency = 48000;
    cfg.samplingMethod = SidConfig::INTERPOLATE;
    cfg.fastSampling = false;
    cfg.playback = SidConfig::STEREO;
    cfg.sidEmulation = mSIDBuilder.get();
    if (!mPlayer.config(cfg)) {
        mError = std::string("sid error: ").append(mPlayer.error());
        return false;
    }

    return true;
}

void SidPlayDecoder::cleanup() {
    if (mPlayer.isPlaying()) {
        mPlayer.stop();
    }
}

uint8_t SidPlayDecoder::getAudioChannels() const {
    return mPlayer.config().playback;
}

SDL_AudioFormat SidPlayDecoder::getAudioSampleFormat() const {
    return AUDIO_S16SYS;
}

int SidPlayDecoder::getAudioFrequency() const {
    return mPlayer.config().frequency;
}

const Decoder::MetaData SidPlayDecoder::getMetaData() {
    if (mTune == nullptr) {
        return mMetaData;
    }

    mMetaData.trackInformation.position = mPlayer.time();
    return mMetaData;
}

char* SidPlayDecoder::loadRom(const std::string path, const size_t romSize) {
    char* buffer = nullptr;
    std::ifstream is(path, std::ios::binary);
    if (is.good()) {
        buffer = new char[romSize];
        is.read(buffer, romSize);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s\n", path.c_str());
    }
    is.close();
    return buffer;
}

std::string SidPlayDecoder::getName() {
    return "sidplayfp";
}

bool SidPlayDecoder::canRead(const std::string extention) const {
    if(extention == ".sid") { return true; }
    if(extention == ".psid") { return true; }
    if(extention == ".rsid") { return true; }
    if(extention == ".mus") { return true; }
    return false;
}

bool SidPlayDecoder::play(const std::vector<char> buffer, bool defaultTune) {
    if (mTune = std::unique_ptr<SidTune>(new SidTune((const uint_least8_t*) buffer.data(), buffer.size()));
        mTune->getStatus() == false) {
        
        mError = std::string("Can't play file.");
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s", mTune->statusString());
        return false;
    }

    // Load tune into engine
    mTune->selectSong(defaultTune ? 0 : 1);
    if (!mPlayer.load(mTune.get())) {
        mError = std::string("Can't play file.");
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s", mPlayer.error());
        return false;
    }

    mMetaData = MetaData();
    parseDiskMetaData();
    parseTrackMetaData();
    
    return true;
}

void SidPlayDecoder::stop() {
    if (mTune != nullptr) {
        mPlayer.stop();
    }
}

bool SidPlayDecoder::nextTrack() {
    if (const auto musicInfo = mTune->getInfo();
        (int) musicInfo->currentSong() < mMetaData.diskInformation.trackCount) {
        
        mPlayer.load(nullptr);
        if (mTune->selectSong(musicInfo->currentSong()+1) == 0) {
            return false;
        }

        mPlayer.load(mTune.get());
        parseTrackMetaData();
        return true;
    }

    return false;
}

bool SidPlayDecoder::prevTrack() {
    if (const auto musicInfo = mTune->getInfo();
        musicInfo->currentSong() > 1) {

        mPlayer.load(nullptr);
        if (mTune->selectSong(musicInfo->currentSong()-1) == 0) {
            return false;
        }

        mPlayer.load(mTune.get());
        parseTrackMetaData();
        return true;
    }

    return false;
}

int SidPlayDecoder::process(Uint8* stream, const int len) {
    if (mTune == nullptr) return -1;

    unsigned int sz = len >> 1;
    unsigned int played = mPlayer.play((short*) stream, sz);

    if(played < sz && mPlayer.isPlaying()) {
        mError = mPlayer.error();
        return -1;
    }
    else if (played < sz) {
        return 1;
    }

    // doesn't update song number
    //const auto musicInfo = mTune->getInfo();
    //mMetaData.trackInformation.trackNumber = musicInfo->currentSong();

    return 0;
}

void SidPlayDecoder::parseDiskMetaData() {
    if (mTune == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: Cannot get track metadata.\n");
        return;
    }

    const auto musicInfo = mTune->getInfo();
    mMetaData.hasDiskInformation = musicInfo->songs() > 1;
    mMetaData.diskInformation.trackCount = musicInfo->songs();
}

void SidPlayDecoder::parseTrackMetaData() {
    if (mTune == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: Cannot get track metadata.\n");
        return;
    }

    const auto musicInfo = mTune->getInfo();
    mMetaData.trackInformation.title = musicInfo->infoString(0);
    mMetaData.trackInformation.author = musicInfo->infoString(1);
    mMetaData.trackInformation.copyright = musicInfo->infoString(2);
    mMetaData.trackInformation.duration = 0; // todo: how to get it ?
    mMetaData.trackInformation.trackNumber = musicInfo->currentSong();
}

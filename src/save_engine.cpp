#include "save_engine.hpp"
#include "spatial_manager.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr std::size_t kHeaderSize = 8;
constexpr std::size_t kStateSize = 32;
constexpr std::size_t kChecksumSize = 4;
constexpr std::size_t kSaveSize = kHeaderSize + kStateSize + kChecksumSize;
constexpr uint16_t kFormatVersion = SaveFile::VERSION;
constexpr std::array<uint8_t, 4> kFormatMagic{{'O', 'B', 'S', 'F'}};
constexpr char kSaveName[] = "save.bin";
std::atomic<uint64_t> tempCounter{0};
thread_local SaveEngine::SaveOutcome saveOutcome = SaveEngine::SaveOutcome::FAILED_BEFORE_RENAME;
thread_local SaveEngine::LoadOutcome loadOutcome = SaveEngine::LoadOutcome::MISSING;

struct LegacySaveFile {
    uint32_t magic;
    uint16_t version;
    GameState state;
    uint32_t checksum;
};
static_assert(sizeof(LegacySaveFile) == kSaveSize, "legacy v1 save layout changed");

std::string resolveSaveDir() {
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && *xdgData) return (fs::path(xdgData) / "obsfuria").string();
    const char* home = std::getenv("HOME");
    if (home && *home) return (fs::path(home) / ".local" / "share" / "obsfuria").string();
    return {};
}

void putU16(std::vector<uint8_t>& out, std::size_t at, uint16_t value) {
    out[at] = static_cast<uint8_t>(value & 0xff);
    out[at + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
}

void putU32(std::vector<uint8_t>& out, std::size_t at, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out[at + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xff);
}

uint16_t getU16(const std::vector<uint8_t>& in, std::size_t at) {
    return static_cast<uint16_t>(in[at]) | static_cast<uint16_t>(in[at + 1] << 8);
}

uint32_t getU32(const std::vector<uint8_t>& in, std::size_t at) {
    uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= static_cast<uint32_t>(in[at + i]) << (i * 8);
    return value;
}

bool readExactRegularFile(const std::string& path, std::vector<uint8_t>& bytes,
                          SaveEngine::LoadOutcome& outcome) {
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        outcome = errno == ENOENT ? SaveEngine::LoadOutcome::MISSING :
                  (errno == ELOOP ? SaveEngine::LoadOutcome::INVALID : SaveEngine::LoadOutcome::INACCESSIBLE);
        return false;
    }

    struct stat info {};
    bool ok = fstat(fd, &info) == 0;
    if (!ok) outcome = SaveEngine::LoadOutcome::INACCESSIBLE;
    else if (!S_ISREG(info.st_mode) || info.st_size != static_cast<off_t>(bytes.size())) {
        outcome = SaveEngine::LoadOutcome::INVALID;
        ok = false;
    }
    std::size_t offset = 0;
    while (ok && offset < bytes.size()) {
        const ssize_t count = read(fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) {
            ok = false;
            break;
        }
        offset += static_cast<std::size_t>(count);
    }

    // The fstat length can become stale if another process mutates the file.
    // Require EOF immediately after the expected payload.
    if (ok) {
        uint8_t extra = 0;
        ssize_t count;
        do {
            count = read(fd, &extra, 1);
        } while (count < 0 && errno == EINTR);
        ok = count == 0;
    }
    const int savedErrno = errno;
    if (close(fd) != 0) ok = false;
    errno = savedErrno;
    if (ok) outcome = SaveEngine::LoadOutcome::LOADED;
    else if (outcome != SaveEngine::LoadOutcome::INVALID && outcome != SaveEngine::LoadOutcome::INACCESSIBLE)
        outcome = SaveEngine::LoadOutcome::INACCESSIBLE;
    return ok;
}

bool hasRegularOrMissingTarget(int dirFd) {
    struct stat info {};
    if (fstatat(dirFd, kSaveName, &info, AT_SYMLINK_NOFOLLOW) == 0) return S_ISREG(info.st_mode);
    return errno == ENOENT;
}

bool decodeV2(std::vector<uint8_t> bytes, GameState& state) {
    if (bytes.size() != kSaveSize) return false;
    SpatialManager::applyCrypto(bytes, 0xAA);
    if (!std::equal(kFormatMagic.begin(), kFormatMagic.end(), bytes.begin()) ||
        getU16(bytes, 4) != kFormatVersion || getU16(bytes, 6) != kStateSize) return false;

    const uint32_t expectedChecksum = getU32(bytes, kHeaderSize + kStateSize);
    uint32_t crc = 0xFFFFFFFF;
    for (std::size_t i = kHeaderSize; i < kHeaderSize + kStateSize; ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    if (~crc != expectedChecksum) return false;

    GameState parsed;
    int32_t* fields[] = {&parsed.x, &parsed.y, &parsed.hp, &parsed.maxHp,
                         &parsed.atk, &parsed.def, &parsed.chanceToRun};
    for (std::size_t i = 0; i < 7; ++i) *fields[i] = static_cast<int32_t>(getU32(bytes, kHeaderSize + i * 4));
    const uint32_t difficulty = getU32(bytes, kHeaderSize + 28);
    if (difficulty > static_cast<uint32_t>(Difficulty::HARD)) return false;
    parsed.difficulty = static_cast<Difficulty>(difficulty);
    if (!SaveEngine::validateState(parsed)) return false;
    state = parsed;
    return true;
}

bool decodeV1(const std::vector<uint8_t>& encrypted, GameState& state) {
    if (encrypted.size() != sizeof(LegacySaveFile)) return false;
    std::vector<uint8_t> bytes = encrypted;
    SpatialManager::applyCrypto(bytes, 0xAA);
    LegacySaveFile file{};
    std::memcpy(&file, bytes.data(), sizeof(file));
    if (file.magic != SaveFile::MAGIC || file.version != 1 ||
        file.checksum != SaveEngine::computeChecksum(file.state) ||
        !SaveEngine::validateState(file.state)) return false;
    state = file.state;
    return true;
}

std::vector<uint8_t> encodeV2(const GameState& state) {
    std::vector<uint8_t> bytes(kSaveSize, 0);
    std::copy(kFormatMagic.begin(), kFormatMagic.end(), bytes.begin());
    putU16(bytes, 4, kFormatVersion);
    putU16(bytes, 6, static_cast<uint16_t>(kStateSize));
    const int32_t fields[] = {state.x, state.y, state.hp, state.maxHp,
                              state.atk, state.def, state.chanceToRun,
                              static_cast<int32_t>(state.difficulty)};
    for (std::size_t i = 0; i < 8; ++i) putU32(bytes, kHeaderSize + i * 4, static_cast<uint32_t>(fields[i]));

    uint32_t crc = 0xFFFFFFFF;
    for (std::size_t i = kHeaderSize; i < kHeaderSize + kStateSize; ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    putU32(bytes, kHeaderSize + kStateSize, ~crc);
    SpatialManager::applyCrypto(bytes, 0xAA);
    return bytes;
}

bool writeAll(int fd, const std::vector<uint8_t>& bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t count = write(fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

bool writeAtomically(const std::string& path, const std::vector<uint8_t>& bytes,
                     SaveEngine::SaveOutcome& outcome) {
    outcome = SaveEngine::SaveOutcome::FAILED_BEFORE_RENAME;
    const fs::path savePath(path);
    const std::string directory = savePath.parent_path().empty() ? "." : savePath.parent_path().string();
    const int dirFd = open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
    if (dirFd < 0) return false;
    if (!hasRegularOrMissingTarget(dirFd)) {
        close(dirFd);
        return false;
    }

    std::string tempName;
    int tempFd = -1;
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        tempName = ".save.bin.tmp." + std::to_string(static_cast<unsigned long long>(getpid())) + "." +
                   std::to_string(static_cast<unsigned long long>(tempCounter.fetch_add(1)));
        tempFd = openat(dirFd, tempName.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (tempFd >= 0) break;
        if (errno != EEXIST && errno != EINTR) {
            close(dirFd);
            return false;
        }
    }
    if (tempFd < 0) {
        close(dirFd);
        return false;
    }

    bool ok = writeAll(tempFd, bytes) && fsync(tempFd) == 0;
    if (close(tempFd) != 0) ok = false;
    bool renamed = false;
    if (ok) {
        if (renameat(dirFd, tempName.c_str(), dirFd, kSaveName) != 0) ok = false;
        else renamed = true;
    }
    if (renamed && fsync(dirFd) != 0) ok = false;
    if (renamed) outcome = ok ? SaveEngine::SaveOutcome::SUCCESS :
                                SaveEngine::SaveOutcome::INSTALLED_DURABILITY_UNCERTAIN;
    if (!ok) unlinkat(dirFd, tempName.c_str(), 0);
    close(dirFd);
    return ok;
}

bool loadNewSave(const std::string& path, GameState& state, bool migrateV1,
                 SaveEngine::LoadOutcome& outcome) {
    std::vector<uint8_t> bytes(kSaveSize);
    if (!readExactRegularFile(path, bytes, outcome)) return false;
    GameState parsed;
    if (decodeV2(bytes, parsed)) {
        state = parsed;
        outcome = SaveEngine::LoadOutcome::LOADED;
        return true;
    }
    if (!decodeV1(bytes, parsed)) {
        outcome = SaveEngine::LoadOutcome::INVALID;
        return false;
    }
    if (migrateV1) {
        SaveEngine::SaveOutcome writeOutcome;
        if (!writeAtomically(path, encodeV2(parsed), writeOutcome)) {
            outcome = SaveEngine::LoadOutcome::MIGRATION_FAILED;
            return false;
        }
    }
    state = parsed;
    outcome = SaveEngine::LoadOutcome::LOADED;
    return true;
}
} // namespace

std::string SaveEngine::getSavePath() {
    const std::string dir = resolveSaveDir();
    if (dir.empty()) return {};
    std::error_code ec;
    const bool existed = fs::exists(dir, ec);
    if (ec) return {};
    fs::create_directories(dir, ec);
    if (ec) return {};
    struct stat info {};
    if (lstat(dir.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) return {};
    if (!existed && chmod(dir.c_str(), 0700) != 0) return {};
    return (fs::path(dir) / kSaveName).string();
}

std::string SaveEngine::getLegacySavePath() {
    return "./save.bin";
}

uint32_t SaveEngine::computeChecksum(const GameState& state) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&state);
    uint32_t crc = 0xFFFFFFFF;
    for (std::size_t i = 0; i < sizeof(GameState); ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

bool SaveEngine::validateState(const GameState& state) {
    if (state.hp <= 0 || state.hp > 9999) return false;
    if (state.maxHp <= 0 || state.maxHp > 9999 || state.hp > state.maxHp) return false;
    if (state.atk < 0 || state.atk > 9999 || state.def < 0 || state.def > 9999) return false;
    if (state.chanceToRun < 0 || state.chanceToRun > 100) return false;
    if (state.x < 0 || state.x >= 20 || state.y < 0 || state.y >= 20) return false;
    return state.difficulty == Difficulty::EASY || state.difficulty == Difficulty::HARD;
}

bool SaveEngine::loadLegacy(const std::string& path, GameState& state) {
    std::vector<uint8_t> bytes(sizeof(GameState));
    if (!readExactRegularFile(path, bytes, loadOutcome)) return false;
    SpatialManager::applyCrypto(bytes, 0xAA);
    GameState parsed{};
    std::memcpy(&parsed, bytes.data(), sizeof(parsed));
    // Original raw saves did not have a stable difficulty value.
    parsed.difficulty = Difficulty::EASY;
    if (!validateState(parsed)) {
        loadOutcome = LoadOutcome::INVALID;
        return false;
    }
    state = parsed;
    return true;
}

bool SaveEngine::saveState(const GameState& state) {
    saveOutcome = SaveOutcome::FAILED_BEFORE_RENAME;
    if (!validateState(state)) return false;
    const std::string path = getSavePath();
    if (path.empty()) return false;
    return writeAtomically(path, encodeV2(state), saveOutcome);
}

bool SaveEngine::loadState(GameState& state) {
    loadOutcome = LoadOutcome::MISSING;
    const std::string path = getSavePath();
    if (path.empty()) {
        loadOutcome = LoadOutcome::INACCESSIBLE;
        return false;
    }
    struct stat info {};
    if (lstat(path.c_str(), &info) == 0) return loadNewSave(path, state, true, loadOutcome);
    if (errno != ENOENT) {
        loadOutcome = LoadOutcome::INACCESSIBLE;
        return false;
    }

    GameState legacyState;
    const std::string legacyPath = getLegacySavePath();
    if (!loadLegacy(legacyPath, legacyState)) return false;
    if (!saveState(legacyState)) {
        loadOutcome = LoadOutcome::MIGRATION_FAILED;
        return false;
    }
    state = legacyState;
    // Only remove the legacy file after the migrated save is durable and only
    // when it is still a regular file (never unlink through a symlink).
    struct stat legacyInfo {};
    if (lstat(legacyPath.c_str(), &legacyInfo) == 0 && S_ISREG(legacyInfo.st_mode)) unlink(legacyPath.c_str());
    loadOutcome = LoadOutcome::LOADED;
    return true;
}

SaveEngine::SaveOutcome SaveEngine::lastSaveOutcome() { return saveOutcome; }
SaveEngine::LoadOutcome SaveEngine::lastLoadOutcome() { return loadOutcome; }

bool SaveEngine::hasValidSave() {
    loadOutcome = LoadOutcome::MISSING;
    const std::string path = getSavePath();
    if (path.empty()) {
        loadOutcome = LoadOutcome::INACCESSIBLE;
        return false;
    }
    struct stat info {};
    if (lstat(path.c_str(), &info) == 0) {
        GameState state;
        return loadNewSave(path, state, false, loadOutcome);
    }
    if (errno != ENOENT) {
        loadOutcome = LoadOutcome::INACCESSIBLE;
        return false;
    }
    GameState legacyState;
    return loadLegacy(getLegacySavePath(), legacyState);
}

void SaveEngine::deleteSave() {
    const std::string path = getSavePath();
    if (!path.empty()) {
        struct stat info {};
        if (lstat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode)) unlink(path.c_str());
    }
    const std::string legacy = getLegacySavePath();
    struct stat legacyInfo {};
    if (lstat(legacy.c_str(), &legacyInfo) == 0 && S_ISREG(legacyInfo.st_mode)) unlink(legacy.c_str());
}

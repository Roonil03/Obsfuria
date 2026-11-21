#include "command_config.hpp"
#include "terminal_text.hpp"
#include <vector>
#include <map>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
constexpr std::size_t maxCommandFileBytes = 1024 * 1024;
constexpr std::size_t maxSelectorBytes = 64;

fs::path resolveAsset(const char* filename) {
    const fs::path relative = fs::path("assets") / filename;
    std::error_code ec;

    // Resolve assets beside the executable for relocatable installs and builds
    // launched from another working directory.
    std::array<char, 4096> executable{};
    const ssize_t length = readlink("/proc/self/exe", executable.data(), executable.size() - 1);
    if (length > 0) {
        const fs::path binDir = fs::path(std::string(executable.data(), static_cast<size_t>(length))).parent_path();
        const std::array<fs::path, 3> candidates = {
            binDir / relative,
            binDir / ".." / relative,
            binDir / ".." / "share" / "obsfuria" / relative
        };
        for (const auto& candidate : candidates) {
            ec.clear();
            if (fs::is_regular_file(candidate, ec)) return candidate;
        }
    }
    return {};
}

const std::vector<CommandEntry>& safeCommands() {
    // Fixed commands prevent a launch directory or packaging asset from
    // silently supplying shell code for the Safe List.
    static const std::vector<CommandEntry> entries = [] {
        static const char* names[] = {
            "ls", "ls -l", "ls -a", "ls -la", "pwd", "tree", "stat", "file",
            "du", "df", "uname", "hostname", "whoami", "id", "groups",
            "who", "w", "uptime", "free", "ps", "date", "cal", "locale",
            "env", "echo USER", "echo SHELL", "echo TERM", "ping", "ip a",
            "ss", "cat os-release", "cat version", "head passwd",
            "md5sum passwd", "sha256sum passwd", "basename", "dirname",
            "type echo", "tty", "echo test", "seq 1 5", "seq 10 -1 1",
            "factor 1024", "base64", "cowsay", "fortune"
        };
        std::vector<CommandEntry> result;
        result.reserve(sizeof(names) / sizeof(names[0]));
        for (const char* name : names) result.push_back({name, "printf 'Obsfuria node active\\n'"});
        return result;
    }();
    return entries;
}

bool validEntry(const CommandEntry& e) {
    return !e.name.empty() && e.name.front() != '#' && !e.executeCmd.empty() &&
        e.name.find_first_of(std::string("|\r\n\0", 4)) == std::string::npos &&
        e.executeCmd.find_first_of(std::string("\r\n\0", 3)) == std::string::npos;
}

CommandConfig::LoadOutcome readRegularFile(const fs::path& path, std::string& contents, std::size_t maxBytes) {
    contents.clear();
    const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOENT) {
            // A dangling symlink is an existing inaccessible configuration,
            // rather than permission to create a fresh list over it.
            std::error_code error;
            const auto status = fs::symlink_status(path, error);
            if (status.type() == fs::file_type::not_found) return CommandConfig::LoadOutcome::MISSING;
        }
        return CommandConfig::LoadOutcome::INACCESSIBLE;
    }
    struct stat status{};
    bool success = fstat(fd, &status) == 0 && S_ISREG(status.st_mode);
    bool tooLarge = success && status.st_size > static_cast<off_t>(maxBytes);
    char buffer[4096];
    while (success && !tooLarge) {
        const ssize_t count = read(fd, buffer, sizeof(buffer));
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) { success = false; break; }
        if (count == 0) break;
        if (static_cast<std::size_t>(count) > maxBytes - contents.size()) { tooLarge = true; break; }
        contents.append(buffer, static_cast<std::size_t>(count));
    }
    if (close(fd) != 0) success = false;
    if (!success || tooLarge) contents.clear();
    if (!success) return CommandConfig::LoadOutcome::INACCESSIBLE;
    return tooLarge ? CommandConfig::LoadOutcome::INVALID : CommandConfig::LoadOutcome::OK;
}

bool samePath(const fs::path& first, const fs::path& second) {
    std::error_code firstError;
    std::error_code secondError;
    const fs::path absoluteFirst = fs::absolute(first, firstError).lexically_normal();
    const fs::path absoluteSecond = fs::absolute(second, secondError).lexically_normal();
    return !firstError && !secondError && absoluteFirst == absoluteSecond;
}

// Replace the destination only after the complete new file has reached disk.
// A failed write leaves an existing list/selector untouched; mkstemp also
// prevents following a pre-existing temporary-file symlink.
bool replaceConfig(const fs::path& path, const std::string& contents) {
    if (path.empty()) return false;
    std::error_code ec;
    const fs::path directory = path.parent_path().empty() ? fs::path(".") : path.parent_path();
    fs::create_directories(directory, ec);
    if (ec) return false;
    std::string pattern = (directory / ".obsfuria-config-XXXXXX").string();
    std::vector<char> temporary(pattern.begin(), pattern.end());
    temporary.push_back('\0');
    const int fd = mkstemp(temporary.data());
    if (fd < 0) return false;
    bool success = true;
    size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t count = write(fd, contents.data() + offset, contents.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { success = false; break; }
        offset += static_cast<size_t>(count);
    }
    if (success && fsync(fd) != 0) success = false;
    if (close(fd) != 0) success = false;
    if (success && rename(temporary.data(), path.c_str()) != 0) success = false;
    if (!success) unlink(temporary.data());
    return success;
}

struct CaptureResult {
    std::string output;
    enum class Status { OK, FAILED, TIMED_OUT, LIMITED } status = Status::FAILED;
    bool previewTruncated = false;
};

CaptureResult captureCommand(const std::string& command,
                           std::chrono::steady_clock::time_point deadline,
                           bool encounterPreview) {
    constexpr size_t maxBytes = 16 * 1024;
    constexpr size_t maxLines = 25;
    // The encounter renderer shows six lines and at most 100 escaped bytes
    // per line. Every retained raw byte occupies at least one escaped byte.
    constexpr size_t maxPreviewLines = 6;
    constexpr size_t maxPreviewLineBytes = 100;
    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC) != 0) return {};
    // Keep pipe ends away from stdio even if a caller closed stdin/stdout.
    for (int& fd : pipeFds) {
        if (fd > STDERR_FILENO) continue;
        const int replacement = fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
        if (replacement < 0) {
            close(pipeFds[0]);
            close(pipeFds[1]);
            return {};
        }
        close(fd);
        fd = replacement;
    }

    const pid_t child = fork();
    if (child < 0) {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return {};
    }
    if (child == 0) {
        close(pipeFds[0]);
        setpgid(0, 0);
        const int nullFd = open("/dev/null", O_RDONLY);
        if (nullFd < 0 || dup2(nullFd, STDIN_FILENO) < 0) _exit(127);
        if (nullFd != STDIN_FILENO) close(nullFd);
        if (dup2(pipeFds[1], STDOUT_FILENO) < 0 ||
            dup2(pipeFds[1], STDERR_FILENO) < 0) _exit(127);
        close(pipeFds[1]);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipeFds[1]);
    setpgid(child, child);
    const int oldFlags = fcntl(pipeFds[0], F_GETFL, 0);
    const bool pipeReady = oldFlags >= 0 &&
        fcntl(pipeFds[0], F_SETFL, oldFlags | O_NONBLOCK) >= 0;

    std::string output;
    size_t bytes = 0;
    size_t lines = 0;
    size_t previewLineLength = 0;
    bool previewTruncated = false;
    bool pipeClosed = false;
    bool childExited = false;
    bool hitLimit = false;
    bool captureFailed = !pipeReady;
    char buffer[1024];
    while (!captureFailed && !(pipeClosed && childExited) && !hitLimit &&
           std::chrono::steady_clock::now() < deadline) {
        pollfd descriptor{pipeFds[0], POLLIN | POLLHUP, 0};
        const int ready = pipeClosed ? poll(nullptr, 0, 50) : poll(&descriptor, 1, 50);
        if (ready < 0 && errno != EINTR) { captureFailed = true; break; }
        if (ready > 0 && (descriptor.revents & (POLLERR | POLLNVAL))) {
            captureFailed = true;
            break;
        }
        if (!pipeClosed && ready > 0 && (descriptor.revents & (POLLIN | POLLHUP))) {
            while (true) {
                const ssize_t count = read(pipeFds[0], buffer, sizeof(buffer));
                if (count <= 0) {
                    if (count == 0) pipeClosed = true;
                    else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) captureFailed = true;
                    break;
                }
                for (ssize_t i = 0; i < count && bytes < maxBytes; ++i) {
                    const char c = buffer[i];
                    ++bytes;
                    // Keep draining after the visible preview fills up. Its
                    // size must not change when a command is stopped or hide
                    // its eventual exit status/timeout.
                    if (!encounterPreview) {
                        output += c;
                    } else if (!previewTruncated) {
                        if (lines >= maxPreviewLines ||
                            (c != '\n' && previewLineLength >= maxPreviewLineBytes)) {
                            previewTruncated = true;
                        } else {
                            output += c;
                            if (c == '\n') previewLineLength = 0;
                            else ++previewLineLength;
                        }
                    }
                    if (c == '\n' && ++lines >= maxLines) {
                        hitLimit = true;
                        break;
                    }
                }
                if (bytes >= maxBytes || hitLimit) {
                    hitLimit = true;
                    break;
                }
            }
        }
        siginfo_t childInfo{};
        const int observed = waitid(P_PID, static_cast<id_t>(child), &childInfo,
                                    WEXITED | WNOHANG | WNOWAIT);
        if (observed == 0 && childInfo.si_pid == child) childExited = true;
        else if (observed < 0 && errno != EINTR) captureFailed = true;
    }

    close(pipeFds[0]);
    // A command can leave background children behind after its stdout closes.
    // Keep the leader unreaped until its process group has been stopped so its
    // process-group ID cannot be reused in between.
    kill(-child, SIGKILL);
    if (!childExited) kill(child, SIGKILL);
    int status = 0;
    pid_t reaped;
    do { reaped = waitpid(child, &status, 0); } while (reaped < 0 && errno == EINTR);
    CaptureResult result;
    result.output = std::move(output);
    result.previewTruncated = previewTruncated;
    if (captureFailed || reaped != child) {
        result.status = CaptureResult::Status::FAILED;
    } else if (std::chrono::steady_clock::now() >= deadline && !hitLimit) {
        result.status = CaptureResult::Status::TIMED_OUT;
    } else if (hitLimit) {
        result.status = CaptureResult::Status::LIMITED;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        result.status = CaptureResult::Status::OK;
    }
    return result;
}
}

fs::path CommandConfig::getConfigPath() {
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && fs::path(xdgConfig).is_absolute()) {
        return fs::path(xdgConfig) / "obsfuria" / "commands.conf";
    }
    const char* home = std::getenv("HOME");
    if (home && fs::path(home).is_absolute()) {
        return fs::path(home) / ".config" / "obsfuria" / "commands.conf";
    }
    // Never discover executable command lists from the launch directory.
    const passwd* account = getpwuid(getuid());
    if (account && account->pw_dir && fs::path(account->pw_dir).is_absolute()) {
        return fs::path(account->pw_dir) / ".config" / "obsfuria" / "commands.conf";
    }
    return {};
}

fs::path CommandConfig::getUserListPath() {
    return getConfigPath();
}

fs::path CommandConfig::getActiveListPath() {
    fs::path path = getConfigPath();
    if (!path.empty()) path += ".active";
    return path;
}

CommandConfig::List CommandConfig::getActiveList() {
    const fs::path activePath = getActiveListPath();
    if (activePath.empty()) return List::SAFE;
    std::string contents;
    const auto outcome = readRegularFile(activePath, contents, maxSelectorBytes);
    if (outcome == LoadOutcome::OK) {
        std::istringstream in(contents);
        std::string value, extra;
        if ((in >> value) && !(in >> extra)) {
            if (value == "USER") return List::USER;
            if (value == "SAFE") return List::SAFE;
        }
        return List::SAFE;
    }
    if (outcome != LoadOutcome::MISSING) return List::SAFE;

    // Before the selector existed, commands.conf itself represented the
    // active list. Persist the one-time migration before later edits can
    // create a new commands.conf and accidentally make it look legacy.
    std::vector<CommandEntry> legacy;
    const List selected = loadCommands(getUserListPath(), legacy) == LoadOutcome::OK
        ? List::USER : List::SAFE;
    return setActiveList(selected) ? selected : List::SAFE;
}

bool CommandConfig::setActiveList(List list) {
    return replaceConfig(getActiveListPath(), list == List::USER ? "USER\n" : "SAFE\n");
}

std::vector<CommandEntry> CommandConfig::loadActiveCommands() {
    if (getActiveList() == List::USER) return loadCommands(getUserListPath());
    return getDefaultCommands();
}

fs::path CommandConfig::getDefaultAssetPath() {
    return resolveAsset("default_commands.txt");
}

fs::path CommandConfig::getCrazyAssetPath() {
    return resolveAsset("crazy_commands.txt");
}

std::vector<CommandEntry> CommandConfig::loadCommands(const fs::path& path) {
    std::vector<CommandEntry> commands;
    (void)loadCommands(path, commands);
    return commands;
}

CommandConfig::LoadOutcome CommandConfig::loadCommands(const fs::path& path, std::vector<CommandEntry>& commands) {
    commands.clear();
    std::string contents;
    const auto outcome = readRegularFile(path, contents, maxCommandFileBytes);
    if (outcome != LoadOutcome::OK) return outcome;
    std::istringstream in(contents);
    std::vector<CommandEntry> parsed;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('|');
        if (pos == std::string::npos) return LoadOutcome::INVALID;
        CommandEntry entry{line.substr(0, pos), line.substr(pos + 1)};
        if (!validEntry(entry)) return LoadOutcome::INVALID;
        parsed.push_back(std::move(entry));
    }
    commands = std::move(parsed);
    return LoadOutcome::OK;
}

bool CommandConfig::saveCommands(const fs::path& path, const std::vector<CommandEntry>& commands) {
    std::string contents;
    for (const auto& entry : commands) {
        if (!validEntry(entry)) return false;
        const std::size_t remaining = maxCommandFileBytes - contents.size();
        if (remaining < 2 || entry.name.size() > remaining - 2 ||
            entry.executeCmd.size() > remaining - 2 - entry.name.size()) return false;
        contents += entry.name + "|" + entry.executeCmd + "\n";
    }
    if (samePath(path, getUserListPath())) {
        // Initialize/migrate the selector before creating the user file.
        // Otherwise a new list can be mistaken for a pre-selector legacy list.
        (void)getActiveList();
        std::error_code ec;
        if (!fs::is_regular_file(getActiveListPath(), ec)) return false;
    }
    return replaceConfig(path, contents);
}

std::vector<CommandEntry> CommandConfig::getDefaultCommands() {
    return safeCommands();
}

std::string CommandConfig::escapeForDisplay(const std::string& value) {
    return TerminalText::escape(value);
}

std::vector<CommandEntry> CommandConfig::getCrazyCommands() {
    return loadCommands(getCrazyAssetPath());
}

namespace {
template <typename Entries>
std::map<std::string, std::string> buildCapturedArsenal(Entries& commands, bool encounterPreview) {
    std::map<std::string, std::string> arsenal;
    const auto totalDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(40);
    for (auto& e : commands) {
        if (!validEntry(e)) continue;
        const auto commandDeadline = std::min(totalDeadline,
            std::chrono::steady_clock::now() + std::chrono::seconds(1));
        CaptureResult captured = std::chrono::steady_clock::now() < totalDeadline
            ? captureCommand(e.executeCmd, commandDeadline, encounterPreview)
            : CaptureResult{{}, CaptureResult::Status::TIMED_OUT};
        std::string output = std::move(captured.output);
        std::string notice;
        if (captured.status == CaptureResult::Status::TIMED_OUT) notice = "[command timed out]";
        else if (captured.status == CaptureResult::Status::LIMITED) notice = "[output limit reached]";
        else if (captured.status == CaptureResult::Status::FAILED) notice = "[command failed]";
        else if (output.empty()) output = "[command produced no output]";
        if (encounterPreview) {
            if (captured.previewTruncated) {
                if (!output.empty() && output.back() != '\n') output += '\n';
                output += "[output truncated]";
            }
            // Escaping a long first line can fill the renderer's preview;
            // report execution status before that preview so it stays visible.
            if (!notice.empty()) output.insert(0, notice + '\n');
        } else if (!notice.empty()) {
            // Preserve the full/raw API's original status placement.
            size_t lines = 0;
            for (size_t index = 0; index < output.size(); ++index) {
                if (output[index] == '\n' && ++lines == 24) {
                    output.resize(index + 1);
                    break;
                }
            }
            if (!output.empty() && output.back() != '\n') output += '\n';
            output += notice;
        }
        arsenal[std::move(e.name)] = std::move(output);
    }
    arsenal["obsfuria_version"] = "1.0.0";
    return arsenal;
}
}

std::map<std::string, std::string> CommandConfig::buildArsenalMap(const std::vector<CommandEntry>& commands) {
    return buildCapturedArsenal(commands, false);
}

std::map<std::string, std::string> CommandConfig::buildEncounterArsenalMap(std::vector<CommandEntry> commands) {
    return buildCapturedArsenal(commands, true);
}

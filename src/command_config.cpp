#include "command_config.hpp"
#include "spatial_manager.hpp"
#include <fstream>
#include <sstream>
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
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
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

bool samePath(const fs::path& first, const fs::path& second) {
    std::error_code firstError;
    std::error_code secondError;
    const fs::path absoluteFirst = fs::absolute(first, firstError).lexically_normal();
    const fs::path absoluteSecond = fs::absolute(second, secondError).lexically_normal();
    return !firstError && !secondError && absoluteFirst == absoluteSecond;
}

std::string rawStringLiteral(const std::string& value) {
    std::string delimiter = "obsfuria";
    for (unsigned int suffix = 0;
        value.find(")" + delimiter + "\"") != std::string::npos;
        ++suffix) {
        delimiter = "obsfuria" + std::to_string(suffix);
    }
    return "R\"" + delimiter + "(" + value + ")" + delimiter + "\"";
}

struct CaptureResult {
    std::string output;
    enum class Status { OK, FAILED, TIMED_OUT, LIMITED } status = Status::FAILED;
};

CaptureResult captureCommand(const std::string& command,
                           std::chrono::steady_clock::time_point deadline) {
    constexpr size_t maxBytes = 16 * 1024;
    constexpr size_t maxLines = 25;
    int pipeFds[2];
    if (pipe(pipeFds) != 0) return {};

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
        close(nullFd);
        dup2(pipeFds[1], STDOUT_FILENO);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[1]);
        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipeFds[1]);
    setpgid(child, child);
    const int oldFlags = fcntl(pipeFds[0], F_GETFL, 0);
    if (oldFlags >= 0) fcntl(pipeFds[0], F_SETFL, oldFlags | O_NONBLOCK);

    std::string output;
    size_t lines = 0;
    bool pipeClosed = false;
    bool childExited = false;
    bool hitLimit = false;
    char buffer[1024];
    while (!(pipeClosed && childExited) && !hitLimit &&
           std::chrono::steady_clock::now() < deadline) {
        pollfd descriptor{pipeFds[0], POLLIN | POLLHUP, 0};
        const int ready = pipeClosed ? poll(nullptr, 0, 50) : poll(&descriptor, 1, 50);
        if (ready < 0 && errno != EINTR) break;
        if (!pipeClosed && ready > 0 && (descriptor.revents & (POLLIN | POLLHUP))) {
            while (true) {
                const ssize_t count = read(pipeFds[0], buffer, sizeof(buffer));
                if (count <= 0) {
                    if (count == 0) pipeClosed = true;
                    else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) pipeClosed = true;
                    break;
                }
                for (ssize_t i = 0; i < count && output.size() < maxBytes; ++i) {
                    const char c = buffer[i];
                    output += c;
                    if (c == '\n' && ++lines >= maxLines) {
                        hitLimit = true;
                        break;
                    }
                }
                if (output.size() >= maxBytes || hitLimit) {
                    hitLimit = true;
                    break;
                }
            }
        }
        siginfo_t childInfo{};
        if (waitid(P_PID, static_cast<id_t>(child), &childInfo,
                   WEXITED | WNOHANG | WNOWAIT) == 0 && childInfo.si_pid == child) {
            childExited = true;
        }
    }

    close(pipeFds[0]);
    // A command can leave background children behind after its stdout closes.
    // Keep the leader unreaped until its process group has been stopped so its
    // process-group ID cannot be reused in between.
    kill(-child, SIGKILL);
    if (!childExited) kill(child, SIGKILL);
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    CaptureResult result;
    result.output = std::move(output);
    if (std::chrono::steady_clock::now() >= deadline && !hitLimit) {
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
    if (xdgConfig && *xdgConfig) {
        return fs::path(xdgConfig) / "obsfuria" / "commands.conf";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return fs::path(home) / ".config" / "obsfuria" / "commands.conf";
    }
    return "./obsfuria_commands.conf";
}

fs::path CommandConfig::getUserListPath() {
    return getConfigPath();
}

fs::path CommandConfig::getActiveListPath() {
    fs::path path = getConfigPath();
    path += ".active";
    return path;
}

CommandConfig::List CommandConfig::getActiveList() {
    const fs::path activePath = getActiveListPath();
    std::ifstream in(activePath);
    std::string value;
    if (in >> value) {
        if (value == "USER") return List::USER;
        if (value == "SAFE") return List::SAFE;
        setActiveList(List::SAFE);
        return List::SAFE;
    }

    // Before the selector existed, commands.conf itself represented the
    // active list. Persist the one-time migration before later edits can
    // create a new commands.conf and accidentally make it look legacy.
    std::error_code ec;
    const bool selectorExists = fs::exists(activePath, ec);
    const List inferred = !ec && fs::is_regular_file(getUserListPath(), ec)
        ? List::USER : List::SAFE;
    const List selected = selectorExists || ec ? List::SAFE : inferred;
    setActiveList(selected);
    return selected;
}

bool CommandConfig::setActiveList(List list) {
    const fs::path path = getActiveListPath();
    if (!path.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec) return false;
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << (list == List::USER ? "USER\n" : "SAFE\n");
    out.close();
    return !out.fail();
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
    std::ifstream in(path);
    if (!in) return commands;
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('|');
        if (pos != std::string::npos) {
            CommandEntry e;
            e.name = line.substr(0, pos);
            e.executeCmd = line.substr(pos + 1);
            if (validEntry(e)) commands.push_back(std::move(e));
        }
    }
    return commands;
}

bool CommandConfig::saveCommands(const fs::path& path, const std::vector<CommandEntry>& commands) {
    for (const auto& entry : commands) if (!validEntry(entry)) return false;
    if (samePath(path, getUserListPath())) {
        // Initialize/migrate the selector before creating the user file.
        // Otherwise a new list can be mistaken for a pre-selector legacy list.
        (void)getActiveList();
        std::error_code ec;
        if (!fs::is_regular_file(getActiveListPath(), ec)) return false;
    }
    if (!path.parent_path().empty()) {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec) return false;
    }
    std::ofstream out(path);
    if (!out) return false;
    for (const auto& e : commands) {
        out << e.name << "|" << e.executeCmd << "\n";
    }
    return out.good();
}

std::vector<CommandEntry> CommandConfig::getDefaultCommands() {
    return safeCommands();
}

std::string CommandConfig::escapeForDisplay(const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string escaped;
    for (unsigned char ch : value) {
        if (ch < 0x20 || ch == 0x7f || (ch >= 0x80 && ch <= 0x9f)) {
            escaped += "\\x";
            escaped += hex[ch >> 4];
            escaped += hex[ch & 15];
        } else {
            escaped += static_cast<char>(ch);
        }
    }
    return escaped;
}

std::vector<CommandEntry> CommandConfig::getCrazyCommands() {
    return loadCommands(getCrazyAssetPath());
}

std::map<std::string, std::string> CommandConfig::buildArsenalMap(const std::vector<CommandEntry>& commands) {
    std::map<std::string, std::string> arsenal;
    const auto totalDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(40);
    for (const auto& e : commands) {
        const auto commandDeadline = std::min(totalDeadline,
            std::chrono::steady_clock::now() + std::chrono::seconds(1));
        CaptureResult captured = std::chrono::steady_clock::now() < totalDeadline
            ? captureCommand(e.executeCmd, commandDeadline)
            : CaptureResult{{}, CaptureResult::Status::TIMED_OUT};
        std::string output = std::move(captured.output);
        if (captured.status == CaptureResult::Status::TIMED_OUT) output += "\n[command timed out]";
        else if (captured.status == CaptureResult::Status::LIMITED) output += "\n[output limit reached]";
        else if (captured.status == CaptureResult::Status::FAILED) output += "\n[command failed]";
        else if (output.empty()) output = "[command produced no output]";
        // Limit to 25 lines
        size_t lines = 0;
        std::string limited;
        for (char c : output) {
            limited += c;
            if (c == '\n' && ++lines >= 25) break;
        }
        arsenal[e.name] = limited;
    }
    arsenal["obsfuria_version"] = "1.0.0";
    return arsenal;
}

bool CommandConfig::regenerateArsenal(const std::vector<CommandEntry>& commands) {
    auto arsenal = buildArsenalMap(commands);
    
    std::string outFile = "include/arsenal.hpp";
    std::ofstream out(outFile);
    if (!out) return false;
    
    out << "#ifndef ARSENAL_HPP\n";
    out << "#define ARSENAL_HPP\n\n";
    out << "#include <string>\n";
    out << "#include <map>\n\n";
    out << "class Arsenal {\n";
    out << "public:\n";
    out << "    static const std::map<std::string, std::string>& getMockCommands() {\n";
    out << "        static const std::map<std::string, std::string> commands = {\n";
    
    for (const auto& [name, output] : arsenal) {
        out << "            {" << rawStringLiteral(name) << ", ";
        out << rawStringLiteral(output) << "},\n";
    }
    
    out << "        };\n";
    out << "        return commands;\n";
    out << "    };\n";
    out << "};\n\n";
    out << "#endif\n";
    
    return out.good();
}

// fake_daemon: the fake daemon (fake_daemon_posix.h) as a program of its own,
// for tests that are not C++. The Python binding's tests (python/tests) run
// it, point Core at its socket with CONTACTCI_SERVICE_SOCKET, and drive it
// over its standard input and output.
//
// It prints "socket <path>" once it is listening, then answers each line it
// reads with one line: "ok", "ok <value>" or "error <why>". It exits at end of
// input, so it never outlives the test that started it.
//
//   version <text>         the version later session replies carry
//   grant <0|1>            whether later session replies grant haptic access
//   protocol <n>           the contract version later session replies carry
//   devices [<product>,<serial>,<left|right>,<0|1 connected>;...]
//                          what device lists return from now on, and signal the change
//   clients [<pid>,<name>;...]   likewise for client lists
//   session-state          a write Session's region: 144 bytes as hex, left hand first
//   session-signals        how often a write Session has signalled a change
//   global-state <hex>     set the global region (144 bytes as hex)
//   global-signal          signal that the global state changed
//   requests               the session requests so far, as <haptic><write><protocol>;...
//   connections            how many connections are open
//   disconnect             hang up on every connection
//   errors                 how Core broke the contract on the wire, if it did, joined by "; "

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fake_daemon_posix.h"

using testsupport::FakeDaemon;

namespace {
    std::vector<std::string> split(const std::string &text, char separator) {
        std::vector<std::string> parts;
        if (text.empty())
            return parts;
        std::stringstream stream(text);
        std::string part;
        while (std::getline(stream, part, separator))
            parts.push_back(part);
        return parts;
    }

    std::string to_hex(const void *data, std::size_t size) {
        static const char digits[] = "0123456789abcdef";
        const auto *bytes = static_cast<const unsigned char *>(data);
        std::string hex;
        for (std::size_t i = 0; i < size; i++) {
            hex += digits[bytes[i] >> 4];
            hex += digits[bytes[i] & 0xf];
        }
        return hex;
    }

    std::string from_hex(const std::string &hex) {
        if (hex.size() % 2 != 0)
            throw std::invalid_argument("an odd number of hex digits");
        std::string bytes;
        for (std::size_t i = 0; i < hex.size(); i += 2)
            bytes += static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16));
        return bytes;
    }

    // Returns what follows "ok" on the reply line; throws to answer "error".
    std::string run(FakeDaemon &daemon, const std::string &command, const std::string &argument) {
        if (command == "version") {
            daemon.set_version(argument);
        } else if (command == "grant") {
            daemon.set_grant_access(argument == "1");
        } else if (command == "protocol") {
            daemon.set_protocol_version(static_cast<std::uint32_t>(std::stoul(argument)));
        } else if (command == "devices") {
            std::vector<testsupport::FakeDevice> devices;
            for (const auto &entry : split(argument, ';')) {
                auto fields = split(entry, ',');
                if (fields.size() != 4)
                    throw std::invalid_argument("a device is <product>,<serial>,<left|right>,<0|1>");
                devices.push_back({fields[0], fields[1], fields[2] == "right", fields[3] == "1"});
            }
            daemon.set_devices(devices);
            daemon.devicesChanged.set();
        } else if (command == "clients") {
            std::vector<testsupport::FakeClient> clients;
            for (const auto &entry : split(argument, ';')) {
                auto fields = split(entry, ',');
                if (fields.size() != 2)
                    throw std::invalid_argument("a client is <pid>,<name>");
                clients.push_back({static_cast<std::uint32_t>(std::stoul(fields[0])), fields[1]});
            }
            daemon.set_clients(clients);
            daemon.clientsChanged.set();
        } else if (command == "session-state") {
            return to_hex(daemon.sessionState.data(), daemon.sessionState.size());
        } else if (command == "session-signals") {
            return std::to_string(daemon.sessionStateChanged.count());
        } else if (command == "global-state") {
            std::string bytes = from_hex(argument);
            if (bytes.size() != daemon.globalState.size())
                throw std::invalid_argument("the global region is " + std::to_string(daemon.globalState.size()) + " bytes");
            std::memcpy(daemon.globalState.data(), bytes.data(), bytes.size());
        } else if (command == "global-signal") {
            daemon.globalStateChanged.set();
        } else if (command == "requests") {
            std::string requests;
            for (const auto &request : daemon.session_requests())
                requests += std::string(requests.empty() ? "" : ";") + (request.isHapticSession ? "1" : "0") +
                            (request.wantsHapticWriteAccess ? "1" : "0") + std::to_string(request.protocolVersion);
            return requests;
        } else if (command == "connections") {
            return std::to_string(daemon.connections_open());
        } else if (command == "disconnect") {
            daemon.disconnect_all();
        } else if (command == "errors") {
            std::string errors;
            for (const auto &error : daemon.protocol_errors())
                errors += (errors.empty() ? "" : "; ") + error;
            return errors;
        } else {
            throw std::invalid_argument("unknown command '" + command + "'");
        }
        return "";
    }
}

int main() {
    try {
        FakeDaemon daemon;
        std::cout << "socket " << daemon.socket_path() << std::endl;

        std::string line;
        while (std::getline(std::cin, line)) {
            auto space = line.find(' ');
            std::string command = line.substr(0, space);
            std::string argument = space == std::string::npos ? "" : line.substr(space + 1);
            try {
                std::string value = run(daemon, command, argument);
                std::cout << (value.empty() ? "ok" : "ok " + value) << std::endl;
            } catch (const std::exception &error) {
                std::cout << "error " << error.what() << std::endl;
            }
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "fake_daemon: " << error.what() << std::endl;
        return 1;
    }
}

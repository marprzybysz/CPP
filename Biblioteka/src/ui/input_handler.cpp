#include "ui/input_handler.hpp"

#include <cctype>
#include <iostream>
#include <optional>
#include <string>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace {

ui::Key map_escape_sequence_to_key(const std::string& sequence) {
    if (sequence == "\x1b[A") {
        return ui::Key::Up;
    }
    if (sequence == "\x1b[B") {
        return ui::Key::Down;
    }
    if (sequence == "\x1b[D") {
        return ui::Key::Left;
    }
    if (sequence == "\x1b[C") {
        return ui::Key::Right;
    }
    if (sequence == "\x1b") {
        return ui::Key::Escape;
    }

    // Keypad Enter (common VT sequence)
    if (sequence == "\x1bOM") {
        return ui::Key::Enter;
    }

    // F2 (common terminal variants)
    if (sequence == "\x1bOQ" || sequence == "\x1b[12~") {
        return ui::Key::Submit;
    }

    return ui::Key::Unknown;
}

ui::Key map_single_char_to_key(char ch, bool enable_navigation_shortcuts) {
    if (ch == '\r' || ch == '\n') {
        return ui::Key::Enter;
    }

    if (ch == '\b' || ch == '\x7f') {
        return ui::Key::Backspace;
    }

    if (enable_navigation_shortcuts) {
        const char lowered = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        switch (lowered) {
        case 'w':
            return ui::Key::Up;
        case 's':
            return ui::Key::Down;
        case 'b':
            return ui::Key::Back;
        case 'q':
            return ui::Key::Quit;
        default:
            break;
        }
    }

    return ui::Key::Unknown;
}

class ScopedRawMode {
public:
    explicit ScopedRawMode(int fd) : fd_(fd) {
        if (isatty(fd_) == 0) {
            return;
        }

        if (tcgetattr(fd_, &original_) != 0) {
            return;
        }

        termios raw = original_;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_iflag &= static_cast<tcflag_t>(~(IXON | IXOFF));
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(fd_, TCSANOW, &raw) == 0) {
            active_ = true;
        }
    }

    ~ScopedRawMode() {
        if (active_) {
            (void)tcsetattr(fd_, TCSANOW, &original_);
        }
    }

    [[nodiscard]] bool active() const { return active_; }

private:
    int fd_;
    bool active_{false};
    termios original_{};
};

bool read_byte(int fd, char& out) {
    return ::read(fd, &out, 1) == 1;
}

std::optional<char> read_byte_with_timeout(int fd, int timeout_ms) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000) * 1000);

    const int rc = select(fd + 1, &set, nullptr, nullptr, &tv);
    if (rc <= 0) {
        return std::nullopt;
    }

    char out = 0;
    if (!read_byte(fd, out)) {
        return std::nullopt;
    }

    return out;
}

std::string read_escape_sequence(int fd) {
    std::string sequence(1, '\x1b');
    const auto second = read_byte_with_timeout(fd, 10);
    if (!second.has_value()) {
        return sequence;
    }

    sequence.push_back(*second);

    if (*second != '[' && *second != 'O') {
        return sequence;
    }

    for (int i = 0; i < 12; ++i) {
        const auto next = read_byte_with_timeout(fd, 10);
        if (!next.has_value()) {
            break;
        }

        sequence.push_back(*next);
        if (std::isalpha(static_cast<unsigned char>(*next)) != 0 || *next == '~') {
            break;
        }
    }

    return sequence;
}

ui::InputEvent read_line_event() {
    std::cout << "\n> " << std::flush;

    std::string raw;
    if (!std::getline(std::cin, raw)) {
        return {.key = ui::Key::Quit, .raw = "q"};
    }

    if (raw.empty()) {
        return {.key = ui::Key::Enter, .raw = ""};
    }

    if (raw.size() == 1) {
        return {.key = map_single_char_to_key(raw.front(), true), .raw = raw};
    }

    return {.key = ui::Key::Unknown, .raw = raw};
}

ui::InputEvent read_raw_key_event(bool enable_navigation_shortcuts) {
    constexpr int kStdinFd = STDIN_FILENO;
    if (isatty(kStdinFd) == 0) {
        return read_line_event();
    }

    ScopedRawMode raw_mode(kStdinFd);
    if (!raw_mode.active()) {
        return read_line_event();
    }

    char first = 0;
    if (!read_byte(kStdinFd, first)) {
        return {.key = ui::Key::Quit, .raw = "q"};
    }

    if (first == '\x1b') {
        const std::string sequence = read_escape_sequence(kStdinFd);
        return {.key = map_escape_sequence_to_key(sequence), .raw = sequence};
    }

    const ui::Key mapped = map_single_char_to_key(first, enable_navigation_shortcuts);
    if (mapped == ui::Key::Enter || mapped == ui::Key::Backspace || mapped == ui::Key::Submit) {
        return {.key = mapped, .raw = ""};
    }

    const std::string raw(1, first);
    if (mapped != ui::Key::Unknown) {
        return {.key = mapped, .raw = raw};
    }

    if (std::isprint(static_cast<unsigned char>(first)) != 0) {
        return {.key = ui::Key::Unknown, .raw = raw};
    }

    return {.key = ui::Key::Unknown, .raw = ""};
}

} // namespace

namespace ui {

InputEvent InputHandler::read_event(bool prefer_line_input, bool prefer_text_input) const {
    if (prefer_line_input) {
        return read_line_event();
    }

    return read_raw_key_event(!prefer_text_input);
}

} // namespace ui

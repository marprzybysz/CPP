#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace ui {

class Screen;

class ScreenManager {
public:
    void register_screen(std::unique_ptr<Screen> screen);
    bool set_active(const std::string& id);
    void clear_active();
    [[nodiscard]] bool has_active() const;
    [[nodiscard]] Screen* active_screen() const;

private:
    std::unordered_map<std::string, std::unique_ptr<Screen>> screens_;
    std::string active_screen_id_;
};

} // namespace ui

#include "ui/screen_manager.hpp"

#include "ui/screens/screen.hpp"

namespace ui {

ScreenManager::~ScreenManager() = default;

void ScreenManager::register_screen(std::unique_ptr<Screen> screen) {
    if (!screen) {
        return;
    }

    const std::string screen_id = screen->id();
    screens_.insert_or_assign(screen_id, std::move(screen));
}

bool ScreenManager::set_active(const std::string& id) {
    const auto it = screens_.find(id);
    if (it == screens_.end()) {
        return false;
    }

    active_screen_id_ = id;
    it->second->on_show();
    return true;
}

void ScreenManager::clear_active() {
    active_screen_id_.clear();
}

bool ScreenManager::has_active() const {
    return !active_screen_id_.empty();
}

Screen* ScreenManager::active_screen() const {
    if (active_screen_id_.empty()) {
        return nullptr;
    }

    const auto it = screens_.find(active_screen_id_);
    if (it == screens_.end()) {
        return nullptr;
    }

    return it->second.get();
}

} // namespace ui

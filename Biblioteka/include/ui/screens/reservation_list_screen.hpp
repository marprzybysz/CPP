#pragma once

#include <optional>
#include <string>
#include <vector>

#include "reservations/reservation.hpp"
#include "ui/components/footer.hpp"
#include "ui/components/header.hpp"
#include "ui/components/list_view.hpp"
#include "ui/components/status_bar.hpp"
#include "ui/controllers/reservations_controller.hpp"
#include "ui/screens/screen.hpp"

namespace ui::screens {

class ReservationListScreen : public Screen {
public:
    explicit ReservationListScreen(controllers::ReservationsController& controller);

    [[nodiscard]] std::string id() const override;
    [[nodiscard]] std::string title() const override;
    void on_show() override;
    void render(Renderer& renderer) const override;
    void handle_input(const InputEvent& event, ScreenManager& manager) override;

private:
    void reload_results();
    void refresh_rows();
    void sync_selected_reservation();
    void cycle_status_filter();
    [[nodiscard]] std::optional<std::size_t> selected_result_index() const;
    [[nodiscard]] std::string status_filter_label() const;

    controllers::ReservationsController& controller_;
    components::Header header_;
    components::ListView results_view_;
    components::StatusBar status_bar_;
    components::Footer footer_;

    controllers::ReservationListState list_state_;
    std::size_t status_filter_index_{0};
    std::vector<controllers::ReservationListEntry> rows_;
};

} // namespace ui::screens

#pragma once

#include "ui/controllers/books_controller.hpp"
#include "ui/controllers/copies_controller.hpp"
#include "ui/controllers/dashboard_controller.hpp"
#include "ui/controllers/inventory_controller.hpp"
#include "ui/controllers/locations_controller.hpp"
#include "ui/controllers/loans_controller.hpp"
#include "ui/controllers/readers_controller.hpp"
#include "ui/controllers/reservations_controller.hpp"

class Library;

namespace ui {

class ScreenManager;

namespace controllers {

class ApplicationController {
public:
    ApplicationController(Library& library, ScreenManager& screen_manager);

    void bootstrap();

private:
    Library& library_;
    ScreenManager& screen_manager_;
    DashboardController dashboard_controller_;
    BooksController books_controller_;
    CopiesController copies_controller_;
    LoansController loans_controller_;
    ReadersController readers_controller_;
    ReservationsController reservations_controller_;
    LocationsController locations_controller_;
    InventoryController inventory_controller_;
};

} // namespace controllers

} // namespace ui

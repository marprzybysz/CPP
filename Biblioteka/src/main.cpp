#include <iostream>

#include "db.hpp"
#include "errors/error_logger.hpp"
#include "errors/error_mapper.hpp"
#include "library.hpp"
#include "ui/application.hpp"

int main() {
    try {
        Db db("library.db");
        db.init_schema();

        Library library(db);
        ui::Application app(library);
        return app.run();
    } catch (const std::exception& e) {
        errors::log_error(e, std::cerr);
        std::cerr << "Blad: " << errors::to_user_message(e) << "\n";
        return 1;
    }
}

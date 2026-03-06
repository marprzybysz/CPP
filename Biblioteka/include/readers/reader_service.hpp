#pragma once

#include "common/system_id.hpp"
#include "readers/reader_repository.hpp"

#include <functional>
#include <string>

namespace readers {

class ReaderService {
public:
    using OperationLogger = std::function<void(const std::string&)>;

    ReaderService(ReaderRepository& repository, common::SystemIdGenerator& id_generator, OperationLogger logger = {});

    Reader add_reader(const CreateReaderInput& input);
    Reader edit_reader(const std::string& public_id, const UpdateReaderInput& input);
    std::vector<Reader> search_readers(const ReaderQuery& query) const;
    Reader get_reader_details(const std::string& public_id) const;
    Reader block_account(const std::string& public_id, const std::string& reason);
    Reader unblock_account(const std::string& public_id);

private:
    static bool is_valid_email(const std::string& email);
    static std::string normalize_text(std::string value);

    void ensure_card_sequence_initialized();

    ReaderRepository& repository_;
    common::SystemIdGenerator& id_generator_;
    OperationLogger logger_;
    bool is_card_sequence_initialized_{false};
};

} // namespace readers

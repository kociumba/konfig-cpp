#include "konfig.h"

namespace konfig {

/// returns the last error to occur, if called again will iterate through the error queue
std::optional<string> manager::get_err() {
    if (error_queue.empty()) {
        return std::nullopt;
    }
    auto err = error_queue.back();
    error_queue.pop_back();
    return err;
}

std::vector<string> manager::get_all_errors() const { return errors; }

bool manager::load() {
    mode = mode_enum::load;
    if (!std::filesystem::exists(file)) {
        if (!save()) {
            push_error("failed to create config file {}", file);
            return false;
        }
        return load(); // recurse load
    }
    const std::ifstream ifs(file);
    if (!ifs.is_open()) {
        push_error("failed to open file {}", file);
        return false;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    unmarshal(buffer.str());
    return errors.empty();
}

bool manager::save() {
    mode = mode_enum::save;
    config.clear();
    marshal(nullptr);
    std::ofstream ofs(file);
    if (!ofs.is_open()) {
        push_error("failed to open file {}", file);
        return false;
    }
    ofs << config;
    return errors.empty();
}

bool manager::marshal(string *out) {
    mode = mode_enum::save;
    in_operation = true;

    scope_stack.clear();
    level_children.clear();
    level_children.emplace_back();
    for (auto &section : sections) {
        if (begin_scope(section.first)) {
            std::visit(
                [this, section](auto &func) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(func)>, section_pure>) {
                        func();
                    } else if constexpr (std::is_same_v<std::decay_t<decltype(func)>,
                                                        section_err>) {
                        if (!func()) {
                            push_error("error marshaling section {}", section.first);
                        }
                    }
                },
                section.second);
            end_scope();
        }
    }
    if (out != nullptr) {
        const auto ss = std::stringstream{} << config;
        *out = ss.str();
    }

    in_operation = false;
    return true;
}
bool manager::unmarshal(const string &in) {
    mode = mode_enum::load;
    in_operation = true;

    scope_stack.clear();
    level_children.clear();
    level_children.emplace_back();
    config = toml::parse(in).table();
    for (auto &section : sections) {
        if (begin_scope(section.first)) {
            std::visit(
                [this, section](auto &func) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(func)>, section_pure>) {
                        func();
                    } else if constexpr (std::is_same_v<std::decay_t<decltype(func)>,
                                                        section_err>) {
                        if (!func()) {
                            push_error("error unmarshaling section {}", section.first);
                        }
                    }
                },
                section.second);
            end_scope();
        }
    }

    in_operation = false;
    return true;
}

bool manager::register_section(const string &name, const section &section) {
    if (sections.contains(name)) {
        push_error("section {} already exists", name);
        return false;
    }
    sections[name] = section;
    return true;
}

bool manager::remove_section(const string &name) {
    if (sections.contains(name)) {
        sections.erase(name);
        return true;
    }
    push_error("section {} does not exist", name);
    return false;
}

std::unique_ptr<manager> create(const string &file) {
    auto m = std::make_unique<manager>();
    m->file = file;
    m->level_children.emplace_back();
    m->mode = load;
    return m;
}

bool manager::begin_scope(const string &name) {
    if (name.empty()) {
        emplace_error("scope name cannot be empty");
        return false;
    }

    std::map<std::string, bool> &current_children = level_children.back();
    if (current_children.contains(name)) {
        push_error("scope {} already exists", name);
        return false;
    }

    current_children[name] = true;
    scope_stack.push_back(name);
    level_children.emplace_back();

    return true;
}

bool manager::end_scope() {
    if (!scope_stack.empty()) {
        std::string ended_scope_name = scope_stack.back();
        scope_stack.pop_back();
        level_children.pop_back();
        return true;
    }
    emplace_error("no active scope to end");
    return false;
}

toml::table *manager::resolve_current_table() {
    toml::table *tbl = &config;

    for (const auto &scope : scope_stack) {
        toml::node *node = tbl->get(scope);

        if (!node || !node->is_table()) {
            if (mode == mode_enum::save) {
                auto [it, _] = tbl->insert_or_assign(scope, toml::table{});
                node = &it->second;
            } else {
                return nullptr;
            }
        }

        tbl = node->as_table();
    }

    return tbl;
}

} // namespace konfig
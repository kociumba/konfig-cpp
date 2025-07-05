#ifndef KONFIG_CPP_LIBRARY_H
#define KONFIG_CPP_LIBRARY_H

#define TOML_ENABLE_UNRELEASED_FEATURES 1
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <print>
#include <source_location>
#include <string>
#include <variant>

#define TODO(format, ...)                                                                          \
    do {                                                                                           \
        std::source_location loc = std::source_location::current();                                \
        std::print(stderr, "TODO: {}:{}:{} in {}: " format "\n", loc.file_name(), loc.line(),      \
                   loc.column(), loc.function_name(), ##__VA_ARGS__);                              \
        assert(false && "TODO item encountered.");                                                 \
    } while (0)

#define KONFIG_SECTION(mgr, name, ...)                                                             \
    do {                                                                                           \
        if (mgr->register_section(name, [&]() { __VA_ARGS__; })) {                                 \
        } else {                                                                                   \
            std::print(stderr, "Failed to register section {}\n", name);                           \
        }                                                                                          \
    } while (0)

#define push_error(msg, ...)                                                                       \
    do {                                                                                           \
        std::string err = std::format(msg, ##__VA_ARGS__);                                         \
        errors.push_back(err);                                                                     \
        error_queue.push_back(err);                                                                \
    } while (0)

#define emplace_error(msg)                                                                         \
    do {                                                                                           \
        errors.emplace_back(msg);                                                                  \
        error_queue.emplace_back(msg);                                                             \
    } while (0)

namespace konfig {

using namespace std::chrono;
using std::function;
using std::string;

enum mode_enum { load, save };

using section_pure = function<void()>;
using section_err = function<bool()>;

using section = std::variant<section_pure, section_err>;

/// use @c konfig::create to initialize a manager
struct manager {
    string file;
    std::map<string, section> sections;
    std::vector<string> errors;
    std::vector<string> error_queue;
    std::vector<std::string> scope_stack;
    std::vector<std::map<std::string, bool>> level_children;
    toml::table config;
    mode_enum mode;
    bool in_operation = false;

    std::optional<string> get_err();
    std::vector<string> get_all_errors() const;
    bool load();
    bool save();
    bool marshal(string *out);
    bool unmarshal(const string &in);
    bool register_section(const string &name, const section &section);
    bool remove_section(const string &name);

    template <typename T>
    bool field(const string &name, T *ptr, const std::optional<T> def = std::nullopt);
    bool begin_scope(const string &name);
    bool end_scope();
    toml::table *resolve_current_table();
};

template <typename T> struct konfig_serializer {
    static bool load(toml::table *tbl, const string &name, T *ptr, const std::optional<T> &def) {
        std::print(stderr,
                   "default serializer used to load field {}, you should implement a "
                   "specialization of "
                   "konfig::konfig_serializer<T> for its type to resolve this error",
                   name);
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const T &value) {
        std::print(stderr,
                   "default serializer used to save field {}, you should implement a "
                   "specialization of "
                   "konfig::konfig_serializer<T> for its type to resolve this error",
                   name);
        return false;
    }
};

template <> struct konfig_serializer<int> {
    static bool load(toml::table *tbl, const string &name, int *ptr,
                     const std::optional<int> &def) {
        if (const auto val = tbl->get(name); val && val->is_integer()) {
            *ptr = val->as_integer()->get();
            return true;
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const int &value) {
        tbl->insert_or_assign(name, value);
        return true;
    }
};

template <> struct konfig_serializer<string> {
    static bool load(toml::table *tbl, const string &name, string *ptr,
                     const std::optional<string> &def) {
        if (const auto val = tbl->get(name); val && val->is_string()) {
            *ptr = val->as_string()->get();
            return true;
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const string &value) {
        tbl->insert_or_assign(name, value);
        return true;
    }
};

template <> struct konfig_serializer<bool> {
    static bool load(toml::table *tbl, const string &name, bool *ptr,
                     const std::optional<bool> &def) {
        if (const auto val = tbl->get(name); val && val->is_boolean()) {
            *ptr = val->as_boolean()->get();
            return true;
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const string &value) {
        tbl->insert_or_assign(name, value);
        return true;
    }
};

template <> struct konfig_serializer<double> {
    static bool load(toml::table *tbl, const string &name, double *ptr,
                     const std::optional<double> &def) {
        if (const auto val = tbl->get(name); val && val->is_floating_point()) {
            *ptr = val->as_floating_point()->get();
            return true;
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const double &value) {
        tbl->insert_or_assign(name, value);
        return true;
    }
};

template <> struct konfig_serializer<system_clock::time_point> {
    static bool load(toml::table *tbl, const string &name, system_clock::time_point *ptr,
                     const std::optional<system_clock::time_point> &def) {
        if (const auto val = tbl->get(name); val && val->is_string()) {
            std::stringstream ss(val->as_string()->get());
            ss >> parse(string{"%Y-%m-%dT%H:%M:%SZ"}, *ptr);
            if (!ss.fail()) {
                return true;
            }
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const string &name, const system_clock::time_point &value) {
        string t = std::format("{:%FT%TZ}", value);
        tbl->insert_or_assign(name, t);
        return true;
    }
};

template <> struct konfig_serializer<std::filesystem::path> {
    static bool load(toml::table *tbl, const std::string &name, std::filesystem::path *ptr,
                     const std::optional<std::filesystem::path> &def) {
        if (const auto val = tbl->get(name); val && val->is_string()) {
            *ptr = val->as_string()->get();
            return true;
        }
        if (def) {
            *ptr = *def;
            return true;
        }
        return false;
    }

    static bool save(toml::table *tbl, const std::string &name,
                     const std::filesystem::path &value) {
        tbl->insert_or_assign(name, value.string());
        return true;
    }
};

template <typename T> bool manager::field(const string &name, T *ptr, const std::optional<T> def) {
    if (!in_operation) {
        emplace_error("calls to manager::field outside save/load are ignored");
        return false;
    }

    toml::table *tbl = resolve_current_table();
    if (!tbl) {
        push_error("no valid table for field {}", name);
        return false;
    }

    switch (mode) {
    case mode_enum::load:
        if (!konfig_serializer<T>::load(tbl, name, ptr, def)) {
            push_error("field {} missing or type mismatch", name);
            return false;
        }
        break;
    case mode_enum::save:
        if (!konfig_serializer<T>::save(tbl, name, *ptr)) {
            push_error("failed to save field {}", name);
            return false;
        }
        break;
    default:
        emplace_error("invalid mode");
        assert(false && "invalid mode");
        return false;
    }
    return true;
}

std::unique_ptr<manager> create(const string &file);

} // namespace konfig

#endif // KONFIG_CPP_LIBRARY_H
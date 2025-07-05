#include "../konfig.h"
#include "test_utils.h"

int main() {
    int port = 8080;
    std::string host = "localhost";
    double age = 0.0;
    const auto mngr = konfig::create("konfig_test.toml");

    KONFIG_SECTION(mngr, "server", {
        mngr->field("port", &port);
        mngr->field("host", &host);
    });
    KONFIG_SECTION(mngr, "client", { mngr->field("age", &age); });

    if (!mngr->save()) {
        error(mngr->get_err().value());
    }
    std::print("port: {}\n", port);
    std::print("host: {}\n", host);

    port = 42069;
    if (!mngr->load()) {
        error(mngr->get_err().value());
    }
    std::print("port load(overwrite): {}\n", port);

    port = 42069;
    if (!mngr->save()) {
        error(mngr->get_err().value());
    }
    if (!mngr->load()) {
        error(mngr->get_err().value());
    }
    std::print("port saved/loaded: {}\n", port);

    std::print("{}\n", mngr->get_all_errors());
}

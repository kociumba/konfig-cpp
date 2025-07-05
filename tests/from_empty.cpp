#include "../konfig.h"
#include "test_utils.h"

int main() {
    int port = 8080;
    std::string host = "localhost";
    double age = 0.0;
    const auto mngr = konfig::create("from_empty.toml");

    KONFIG_SECTION(mngr, "server", {
        mngr->field("port", &port);
        mngr->field("host", &host);
    });
    KONFIG_SECTION(mngr, "client", { mngr->field("age", &age); });

    if (!mngr->load()) {
        error(mngr->get_err().value());
    }
}
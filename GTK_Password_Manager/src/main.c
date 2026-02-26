#include "core/app.h"
#include "utils/logger.h"

int main(int argc, char *argv[])
{
    logger_init(NULL, LOG_INFO);

    VaultApp *app = vault_app_new();
    vault_app_run(app, argc, argv);

    logger_close();
    return 0;
}

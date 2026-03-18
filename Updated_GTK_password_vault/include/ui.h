#ifndef UI_H
#define UI_H

#include "app_state.h"

// Create the main application window and widgets
AppState* ui_create_app(void);

// Refresh service buttons in the main window
void ui_refresh_services(AppState *app_state);

#endif // UI_H

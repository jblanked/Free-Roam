#include "app.hpp"

uint32_t FreeRoamApp::callback_to_submenu(void *context)
{
    FreeRoamApp *app = (FreeRoamApp *)context;
    if (app)
    {
        // quick reset game when returning to submenu
        if (app->game)
        {
            app->game.reset();
        }

        // quick reset settings when returning to submenu
        if (app->settings)
        {
            app->settings.reset();
        }

        // quick reset about when returning to submenu
        if (app->about)
        {
            app->about.reset();
        }
    }
    return FreeRoamViewSubmenu;
}

uint32_t FreeRoamApp::callback_exit_app(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

void FreeRoamApp::settings_item_selected_callback(void *context, uint32_t index)
{
    FreeRoamApp *app = (FreeRoamApp *)context;
    app->settingsItemSelected(index);
}

void FreeRoamApp::submenu_choices_callback(void *context, uint32_t index)
{
    FreeRoamApp *app = (FreeRoamApp *)context;
    app->callbackSubmenuChoices(index);
}

void FreeRoamApp::settingsItemSelected(uint32_t index)
{
    if (settings)
    {
        settings->settingsItemSelected(index);
    }
}

void FreeRoamApp::createAppDataPath()
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam");
    storage_common_mkdir(storage, directory_path);
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam/data");
    storage_common_mkdir(storage, directory_path);
    furi_record_close(RECORD_STORAGE);
}

void FreeRoamApp::callbackSubmenuChoices(uint32_t index)
{
    switch (index)
    {
    case FreeRoamSubmenuRun:
        game = std::make_unique<FreeRoamGame>();
        if (!game->init(&viewDispatcher, this))
        {
            FURI_LOG_E(TAG, "Failed to initialize game");
            game.reset();
            return;
        }
        view_dispatcher_switch_to_view(viewDispatcher, FreeRoamViewMain);
        break;
    case FreeRoamSubmenuAbout:
        about = std::make_unique<FreeRoamAbout>();
        if (!about->init(&viewDispatcher, this))
        {
            FURI_LOG_E(TAG, "Failed to initialize about");
            about.reset();
            return;
        }
        view_dispatcher_switch_to_view(viewDispatcher, FreeRoamViewAbout);
        break;
    case FreeRoamSubmenuSettings:
        settings = std::make_unique<FreeRoamSettings>();
        if (!settings->init(&viewDispatcher, this))
        {
            FURI_LOG_E(TAG, "Failed to initialize settings");
            settings.reset();
            return;
        }
        view_dispatcher_switch_to_view(viewDispatcher, FreeRoamViewSettings);
        break;
    default:
        break;
    }
}

bool FreeRoamApp::isBoardConnected()
{
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "FlipperHTTP is not initialized");
        return false;
    }

    if (!flipper_http_send_command(flipperHttp, HTTP_CMD_PING))
    {
        FURI_LOG_E(TAG, "Failed to ping the device");
        return false;
    }

    furi_delay_ms(100);

    // Try to wait for pong response.
    uint32_t counter = 100;
    while (flipperHttp->state == INACTIVE && --counter > 0)
    {
        furi_delay_ms(100);
    }

    // last response should be PONG
    return flipperHttp->last_response && strcmp(flipperHttp->last_response, "[PONG]") == 0;
}

bool FreeRoamApp::save_char(const char *path_name, const char *value)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam/data/%s.txt", path_name);
    storage_file_open(file, file_path, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    size_t data_size = strlen(value) + 1; // Include null terminator
    storage_file_write(file, value, data_size);
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return true;
}

bool FreeRoamApp::load_char(const char *path_name, char *value, size_t value_size)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam/data/%s.txt", path_name);
    if (!storage_file_open(file, file_path, FSAM_READ, FSOM_OPEN_EXISTING))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t read_count = storage_file_read(file, value, value_size);
    value[read_count - 1] = '\0';
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return strlen(value) > 0;
}

bool FreeRoamApp::isVibrationEnabled()
{
    char value[16] = {0};
    if (load_char("vibration", value, sizeof(value)))
    {
        return strcmp(value, "On") == 0;
    }
    return true;
}

bool FreeRoamApp::isSoundEnabled()
{
    char value[16] = {0};
    if (load_char("sound", value, sizeof(value)))
    {
        return strcmp(value, "On") == 0;
    }
    return true;
}

void FreeRoamApp::setVibrationEnabled(bool enabled)
{
    save_char("vibration", enabled ? "On" : "Off");
}

void FreeRoamApp::setSoundEnabled(bool enabled)
{
    save_char("sound", enabled ? "On" : "Off");
}

FuriString *FreeRoamApp::httpRequest(
    const char *url,
    HTTPMethod method,
    const char *headers,
    const char *payload)
{
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "FreeRoamApp::httpRequest: FlipperHTTP is NULL");
        return NULL;
    }
    snprintf(flipperHttp->file_path, sizeof(flipperHttp->file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam/data/temp.json");
    flipperHttp->save_received_data = true;
    flipperHttp->state = IDLE;
    if (!flipper_http_request(flipperHttp, method, url, headers, payload))
    {
        FURI_LOG_E(TAG, "FreeRoamApp::httpRequest: Failed to send HTTP request");
        return NULL;
    }
    flipperHttp->state = RECEIVING;
    while (flipperHttp->state != IDLE)
    {
        furi_delay_ms(100);
    }
    return flipper_http_load_from_file(flipperHttp->file_path);
}

bool FreeRoamApp::httpRequestAsync(
    const char *saveLocation,
    const char *url,
    HTTPMethod method,
    const char *headers,
    const char *payload)
{
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "FreeRoamApp::httpRequestAsync: FlipperHTTP is NULL");
        return false;
    }
    snprintf(flipperHttp->file_path, sizeof(flipperHttp->file_path), "%s/%s", STORAGE_EXT_PATH_PREFIX "/apps_data/free_roam/data", saveLocation);
    flipperHttp->save_received_data = true;
    flipperHttp->state = IDLE;
    if (!flipper_http_request(flipperHttp, method, url, headers, payload))
    {
        FURI_LOG_E(TAG, "FreeRoamApp::httpRequestAsync: Failed to send HTTP request");
        return false;
    }
    flipperHttp->state = RECEIVING;
    return true;
}

FreeRoamApp::FreeRoamApp()
{
    gui = static_cast<Gui *>(furi_record_open(RECORD_GUI));

    // Allocate ViewDispatcher
    if (!easy_flipper_set_view_dispatcher(&viewDispatcher, gui, this))
    {
        FURI_LOG_E(TAG, "Failed to allocate view dispatcher");
        return;
    }

    // Submenu
    if (!easy_flipper_set_submenu(&submenu, FreeRoamViewSubmenu,
                                  "Free Roam", callback_exit_app, &viewDispatcher))
    {
        FURI_LOG_E(TAG, "Failed to allocate submenu");
        return;
    }

    submenu_add_item(submenu, "Run", FreeRoamSubmenuRun, submenu_choices_callback, this);
    submenu_add_item(submenu, "About", FreeRoamSubmenuAbout, submenu_choices_callback, this);
    submenu_add_item(submenu, "Settings", FreeRoamSubmenuSettings, submenu_choices_callback, this);

    flipperHttp = flipper_http_alloc();
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "Failed to allocate FlipperHTTP");
        return;
    }

    createAppDataPath();

    // Switch to the submenu view
    view_dispatcher_switch_to_view(viewDispatcher, FreeRoamViewSubmenu);
}

FreeRoamApp::~FreeRoamApp()
{
    // Clean up game
    if (game)
    {
        game.reset();
    }

    // Clean up settings
    if (settings)
    {
        settings.reset();
    }

    // Clean up about
    if (about)
    {
        about.reset();
    }

    // Free submenu
    if (submenu)
    {
        view_dispatcher_remove_view(viewDispatcher, FreeRoamViewSubmenu);
        submenu_free(submenu);
    }

    // Free view dispatcher
    if (viewDispatcher)
    {
        view_dispatcher_free(viewDispatcher);
    }

    // Close GUI
    if (gui)
    {
        furi_record_close(RECORD_GUI);
    }

    // Free FlipperHTTP
    if (flipperHttp)
    {
        flipper_http_free(flipperHttp);
    }
}

void FreeRoamApp::run()
{
    view_dispatcher_run(viewDispatcher);
}

extern "C"
{
    int32_t free_roam_main(void *p)
    {
        // Suppress unused parameter warning
        UNUSED(p);

        // Create the app
        FreeRoamApp app;

        // Run the app
        app.run();

        // return success
        return 0;
    }
}

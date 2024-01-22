/**
 * @file menu.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../../launcher.h"
#include "mc_conf_internal.h"
#include "spdlog/spdlog.h"
#include "menu_render_callback.hpp"
#include <string>
#include <vector>
#include "../../../utils/common_define.h"


using namespace MOONCAKE::APPS;


void Launcher::_start_menu()
{
    spdlog::info("start menu");

    /* Create a menu to handle selector */
    _data.menu = new SMOOTH_MENU::Simple_Menu;
    _data.menu_render_cb = new LauncherRender_CB_t;
    ((LauncherRender_CB_t*)_data.menu_render_cb)->setHal(_data.hal);

    _data.menu->init(_data.hal->canvas()->width(), _data.hal->canvas()->height());
    _data.menu->setRenderCallback(_data.menu_render_cb);

    /* Set selector anim */
    auto cfg_selector = _data.menu->getSelector()->config();
    cfg_selector.animPath_x = LVGL::ease_out;
    cfg_selector.animPath_y = LVGL::ease_out;
    cfg_selector.animTime_x = 400;
    cfg_selector.animTime_y = 400;
    _data.menu->getSelector()->config(cfg_selector);

    /* Set menu looply */
    _data.menu->setMenuLoopMode(true);


    // // Fake apps
    // std::vector<std::string> app_list = {
    //     "TIMER",
    //     "RECORD",
    //     "REMOTE",
    //     "CHAT",
    //     "REPL",
    //     "SCAN",
    // };
    // for (int i = 0; i < app_list.size(); i++)
    // {
    //     _data.menu->getMenu()->addItem(
    //         app_list[i],
    //         ICON_GAP + i * (ICON_WIDTH + ICON_GAP),
    //         ICON_MARGIN_TOP,
    //         ICON_WIDTH,
    //         ICON_WIDTH
    //     );
    // }

    // Get install app list
    spdlog::info("installed apps num: {}", mcAppGetFramework()->getAppRegister().getInstalledAppNum());
    int i = 0;
    for (const auto& app : mcAppGetFramework()->getAppRegister().getInstalledAppList())
    {
        spdlog::info("app: {} icon: {}", app->getAppName(), app->getAppIcon());

        // Pass launcher
        if (app->getAddr() == getAppPacker())
            continue;

        // Push items
        _data.menu->getMenu()->addItem(
            app->getAppName(),
            ICON_GAP + i * (ICON_WIDTH + ICON_GAP),
            ICON_MARGIN_TOP,
            ICON_WIDTH,
            ICON_WIDTH,
            app->getAppIcon()
        );
        i++;
    }
}

void Launcher::_update_menu()
{
    if ((millis() - _data.menu_update_count) > _data.menu_update_preiod)
    {
        // Navigate
        if (_port_check_last_pressed())
            _data.menu->goLast();
        else if (_port_check_next_pressed())
            _data.menu->goNext();

        // If pressed enter
        else if (_port_check_key_pressed(42))
        {
            auto selected_item = _data.menu->getSelector()->getTargetItem();
            selected_item++;

            spdlog::info("select: {} try create", selected_item);

            // multi_heap_info_t info;
            // heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
            // spdlog::info("before createApp()");
            // spdlog::info("multi_heap_info.allocated_blocks      {}", info.allocated_blocks);
            // spdlog::info("multi_heap_info.free_blocks           {}", info.free_blocks);
            // spdlog::info("multi_heap_info.largest_free_block    {}", info.largest_free_block);
            // spdlog::info("multi_heap_info.minimum_free_bytes    {}", info.minimum_free_bytes);
            // spdlog::info("multi_heap_info.total_allocated_bytes {}", info.total_allocated_bytes);
            // spdlog::info("multi_heap_info.total_blocks          {}", info.total_blocks);
            // spdlog::info("multi_heap_info.total_free_bytes      {}", info.total_free_bytes);

            // Create app
            _data._opened_app = mcAppGetFramework()->createApp(mcAppGetFramework()->getInstalledAppList()[selected_item]);
            spdlog::info("addr: {}", (void*)_data._opened_app);

            // heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
            // spdlog::info("after createApp()");
            // spdlog::info("multi_heap_info.allocated_blocks      {}", info.allocated_blocks);
            // spdlog::info("multi_heap_info.free_blocks           {}", info.free_blocks);
            // spdlog::info("multi_heap_info.largest_free_block    {}", info.largest_free_block);
            // spdlog::info("multi_heap_info.minimum_free_bytes    {}", info.minimum_free_bytes);
            // spdlog::info("multi_heap_info.total_allocated_bytes {}", info.total_allocated_bytes);
            // spdlog::info("multi_heap_info.total_blocks          {}", info.total_blocks);
            // spdlog::info("multi_heap_info.total_free_bytes      {}", info.total_free_bytes);

            // Start app
            mcAppGetFramework()->startApp(_data._opened_app);

            // Stack launcher into background
            closeApp();
        }
        else if (_port_check_key_pressed(50))   // B for brightness
        {
            _data._brightness += 64;
            _data.hal->display()->setBrightness(_data._brightness);
        }

        // Update menu
        _data.menu->update(millis());
        _data.hal->canvas_update();

        // Reset flag
        _data.menu_update_count = millis();
    }
}

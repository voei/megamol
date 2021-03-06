/*
 * WindowManager.cpp
 *
 * Copyright (C) 2019 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "WindowManager.h"


using namespace megamol;
using namespace megamol::gui;


void WindowManager::SoftResetWindowSizePos(const std::string& window_name, WindowConfiguration& window_config) {
    assert(ImGui::GetCurrentContext() != nullptr);

    ImGuiIO& io = ImGui::GetIO();

    float width = window_config.win_reset_size.x;
    float height = window_config.win_reset_size.y;

    if (width > io.DisplaySize.x) {
        width = io.DisplaySize.x; // -(2.0f * style.DisplayWindowPadding.x);
    }
    if (height > io.DisplaySize.y) {
        height = io.DisplaySize.y; // - (2.0f * style.DisplayWindowPadding.y);
    }

    auto win_pos = ImGui::GetWindowPos();
    if (win_pos.x < 0) {
        win_pos.x = 0.0f; // style.DisplayWindowPadding.x;
    }
    if (win_pos.y < 0) {
        win_pos.y = 0.0f; // style.DisplayWindowPadding.y;
    }

    ImVec2 win_size;
    if (window_config.win_flags & ImGuiWindowFlags_AlwaysAutoResize) {
        win_size = ImGui::GetWindowSize();
    } else {
        win_size = ImVec2(width, height);
    }

    float win_width = io.DisplaySize.x - (win_pos.x); // +style.DisplayWindowPadding.x);
    if (win_width < win_size.x) {
        win_pos.x = io.DisplaySize.x - (win_size.x); // + style.DisplayWindowPadding.x);
    }
    float win_height = io.DisplaySize.y - (win_pos.y); // + style.DisplayWindowPadding.y);
    if (win_height < win_size.y) {
        win_pos.y = io.DisplaySize.y - (win_size.y); //+ style.DisplayWindowPadding.y);
    }

    ImGui::SetWindowSize(window_name.c_str(), ImVec2(width, height), ImGuiCond_Always);
    ImGui::SetWindowPos(window_name.c_str(), win_pos, ImGuiCond_Always);
}


void WindowManager::ResetWindowOnStateLoad(const std::string& window_name, WindowConfiguration& window_config) {
    assert(ImGui::GetCurrentContext() != nullptr);

    ImVec2 pos = window_config.win_position;
    ImVec2 size = window_config.win_size;

    ImGui::SetWindowSize(window_name.c_str(), size, ImGuiCond_Always);
    ImGui::SetWindowPos(window_name.c_str(), pos, ImGuiCond_Always);
}


bool WindowManager::AddWindowConfiguration(const std::string& window_name, WindowConfiguration& window_config) {
    if (window_name.empty()) {
        vislib::sys::Log::DefaultLog.WriteWarn("[WindowManager] No valid window name given.");
        return false;
    }
    if (this->windowConfigurationExists(window_name)) {
        vislib::sys::Log::DefaultLog.WriteWarn(
            "[WindowManager] Found already existing window with name '%s'. Window names must be unique.",
            window_name.c_str());
        return false;
    }
    this->windows.emplace(window_name, window_config);
    return true;
}


bool WindowManager::DeleteWindowConfiguration(const std::string& window_name) {
    if (!this->windowConfigurationExists(window_name)) {
        vislib::sys::Log::DefaultLog.WriteWarn(
            "[WindowManager] Found no existing window with name '%s'.", window_name.c_str());
        return false;
    }
    this->windows.erase(window_name);
    return true;
}


bool WindowManager::StateFromJSON(const std::string& json_string) {

    try {
        bool valid = true;
        std::map<std::string, WindowConfiguration> tmp_windows;

        nlohmann::json json;
        json = nlohmann::json::parse(json_string);

        if (!json.is_object()) {
            vislib::sys::Log::DefaultLog.WriteError("[WindowManager] State has to be a valid JSON object.");
            return false;
        }

        for (auto& w : json.items()) {
            std::string window_name = w.key();
            WindowConfiguration tmp_config;

            // Getting all configuration values for current window.
            auto config_values = w.value();

            // WindowConfiguration ------------------------------------
            // show
            if (config_values.at("win_show").is_boolean()) {
                config_values.at("win_show").get_to(tmp_config.win_show);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_show' as boolean.");
                valid = false;
            }
            // flags
            if (config_values.at("win_flags").is_number_integer()) {
                tmp_config.win_flags = static_cast<ImGuiWindowFlags>(config_values.at("win_flags").get<int>());
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_flags' as integer.");
                valid = false;
            }
            // callback
            if (config_values.at("win_callback").is_number_integer()) {
                tmp_config.win_callback = static_cast<DrawCallbacks>(config_values.at("win_callback").get<int>());
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_callback' as integer.");
                valid = false;
            }
            // hotkey
            if (config_values.at("win_hotkey").is_array() && (config_values.at("win_hotkey").size() == 2)) {
                if (config_values.at("win_hotkey")[0].is_number_integer() &&
                    config_values.at("win_hotkey")[1].is_number_integer()) {
                    int key = config_values.at("win_hotkey")[0].get<int>();
                    int mods = config_values.at("win_hotkey")[1].get<int>();
                    tmp_config.win_hotkey = core::view::KeyCode(
                        static_cast<core::view::Key>(key), static_cast<core::view::Modifiers>(mods));
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read 'win_hotkey' values as integers.");
                    valid = false;
                }
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_hotkey' as array of size two.");
                valid = false;
            }
            // position
            if (config_values.at("win_position").is_array() && (config_values.at("win_position").size() == 2)) {
                if (config_values.at("win_position")[0].is_number_float()) {
                    config_values.at("win_position")[0].get_to(tmp_config.win_position.x);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read first value of 'win_position' as float.");
                    valid = false;
                }
                if (config_values.at("win_position")[1].is_number_float()) {
                    config_values.at("win_position")[1].get_to(tmp_config.win_position.y);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read second value of 'win_position' as float.");
                    valid = false;
                }
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_position' as array of size two.");
                valid = false;
            }
            // size
            if (config_values.at("win_size").is_array() && (config_values.at("win_size").size() == 2)) {
                if (config_values.at("win_size")[0].is_number_float()) {
                    config_values.at("win_size")[0].get_to(tmp_config.win_size.x);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read first value of 'win_size' as float.");
                    valid = false;
                }
                if (config_values.at("win_size")[1].is_number_float()) {
                    config_values.at("win_size")[1].get_to(tmp_config.win_size.y);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read second value of 'win_size' as float.");
                    valid = false;
                }
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_size' as array of size two.");
                valid = false;
            }
            // soft_reset
            if (config_values.at("win_soft_reset").is_boolean()) {
                config_values.at("win_soft_reset").get_to(tmp_config.win_soft_reset);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_soft_reset' as boolean.");
                valid = false;
            }
            // reset_size
            if (config_values.at("win_reset_size").is_array() && (config_values.at("win_reset_size").size() == 2)) {
                if (config_values.at("win_reset_size")[0].is_number_float()) {
                    config_values.at("win_reset_size")[0].get_to(tmp_config.win_reset_size.x);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read first value of 'win_reset_size' as float.");
                    valid = false;
                }
                if (config_values.at("win_reset_size")[1].is_number_float()) {
                    config_values.at("win_reset_size")[1].get_to(tmp_config.win_reset_size.y);
                } else {
                    vislib::sys::Log::DefaultLog.WriteError(
                        "[WindowManager] JSON state: Failed to read second value  of 'win_reset_size' as float.");
                    valid = false;
                }
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'win_reset_size' as array of size two.");
                valid = false;
            }
            // MainConfig --------------------------------------------
            // main_project_file (supports UTF-8)
            if (config_values.at("main_project_file").is_string()) {
                config_values.at("main_project_file").get_to(tmp_config.main_project_file);
                this->utils.Utf8Decode(tmp_config.main_project_file);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'main_project_file' as string.");
                valid = false;
            }
            // ParamConfig --------------------------------------------
            // show_hotkeys
            if (config_values.at("param_show_hotkeys").is_boolean()) {
                config_values.at("param_show_hotkeys").get_to(tmp_config.param_show_hotkeys);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'param_show_hotkeys' as boolean.");
                valid = false;
            }
            // modules_list (no UTF-8 support needed)
            tmp_config.param_modules_list.clear();
            if (config_values.at("param_modules_list").is_array()) {
                size_t buf_size = config_values.at("param_modules_list").size();
                for (size_t i = 0; i < buf_size; ++i) {
                    if (config_values.at("param_modules_list")[i].is_string()) {
                        tmp_config.param_modules_list.emplace_back(
                            config_values.at("param_modules_list")[i].get<std::string>());
                    } else {
                        vislib::sys::Log::DefaultLog.WriteError(
                            "[WindowManager] JSON state: Failed to read element of 'param_modules_list' as string.");
                        valid = false;
                    }
                }
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'param_modules_list' as array.");
                valid = false;
            }
            // module_filter
            if (config_values.at("param_module_filter").is_number_integer()) {
                tmp_config.param_module_filter =
                    static_cast<FilterModes>(config_values.at("param_module_filter").get<int>());
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'param_module_filter' as integer.");
                valid = false;
            }
            // FpsMsConfig --------------------------------------------
            // show_options
            if (config_values.at("ms_show_options").is_boolean()) {
                config_values.at("ms_show_options").get_to(tmp_config.ms_show_options);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'ms_show_options' as boolean.");
                valid = false;
            }
            // max_value_count
            if (config_values.at("ms_max_history_count").is_number_integer()) {
                config_values.at("ms_max_history_count").get_to(tmp_config.ms_max_history_count);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'ms_max_history_count' as integer.");
                valid = false;
            }
            // max_delay
            if (config_values.at("ms_refresh_rate").is_number_float()) {
                config_values.at("ms_refresh_rate").get_to(tmp_config.ms_refresh_rate);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'ms_refresh_rate' as float.");
                valid = false;
            }
            // mode
            if (config_values.at("ms_mode").is_number_integer()) {
                tmp_config.ms_mode = static_cast<TimingModes>(config_values.at("ms_mode").get<int>());
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'ms_mode' as integer.");
                valid = false;
            }
            // FontConfig ---------------------------------------------
            // font_name (supports UTF-8)
            if (config_values.at("font_name").is_string()) {
                config_values.at("font_name").get_to(tmp_config.font_name);
                this->utils.Utf8Decode(tmp_config.font_name);
            } else {
                vislib::sys::Log::DefaultLog.WriteError(
                    "[WindowManager] JSON state: Failed to read 'font_name' as string.");
                valid = false;
            }

            // set reset flags
            tmp_config.win_reset = true;

            tmp_config.buf_font_reset = false;
            if (!tmp_config.font_name.empty()) {
                tmp_config.buf_font_reset = true;
            }
            tmp_windows.emplace(window_name, tmp_config);
        }

        if (!valid) {
            vislib::sys::Log::DefaultLog.WriteWarn("[WindowManager] Could not load state.");
            return false;
        }

        // Replace existing window configurations and add new windows.
        for (auto& new_win : tmp_windows) {
            bool found_existing = false;
            for (auto& win : this->windows) {
                // Check for same name
                if (win.first == new_win.first) {
                    win.second = new_win.second;
                    found_existing = true;
                }
            }
            if (!found_existing) {
                this->windows.emplace(new_win);
            }
        }

    } catch (nlohmann::json::type_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
        //} catch (nlohmann::json::exception& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
        //} catch (nlohmann::json::parse_error& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
    } catch (nlohmann::json::invalid_iterator& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::out_of_range& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::other_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (...) {
        vislib::sys::Log::DefaultLog.WriteError("[StateFromJSON] Unknown Error - Unable to parse JSON string.");
        return false;
    }

    return true;
}


bool WindowManager::StateToJSON(std::string& json_string) {
    json_string = "";

    try {
        nlohmann::json json;

        for (auto& w : this->windows) {
            if (w.second.win_store_config) {
                std::string window_name = w.first;
                WindowConfiguration window_config = w.second;
                json[window_name]["win_show"] = window_config.win_show;
                json[window_name]["win_flags"] = static_cast<int>(window_config.win_flags);
                json[window_name]["win_callback"] = static_cast<int>(window_config.win_callback);
                json[window_name]["win_hotkey"] = {
                    static_cast<int>(window_config.win_hotkey.key), window_config.win_hotkey.mods.toInt()};
                json[window_name]["win_position"] = {window_config.win_position.x, window_config.win_position.y};
                json[window_name]["win_size"] = {window_config.win_size.x, window_config.win_size.y};
                json[window_name]["win_soft_reset"] = window_config.win_soft_reset;
                json[window_name]["win_reset_size"] = {window_config.win_reset_size.x, window_config.win_reset_size.y};

                this->utils.Utf8Encode(window_config.main_project_file);
                json[window_name]["main_project_file"] = window_config.main_project_file;

                json[window_name]["param_show_hotkeys"] = window_config.param_show_hotkeys;
                json[window_name]["param_modules_list"] = window_config.param_modules_list;
                json[window_name]["param_module_filter"] = static_cast<int>(window_config.param_module_filter);

                json[window_name]["ms_show_options"] = window_config.ms_show_options;
                json[window_name]["ms_max_history_count"] = window_config.ms_max_history_count;
                json[window_name]["ms_refresh_rate"] = window_config.ms_refresh_rate;
                json[window_name]["ms_mode"] = static_cast<int>(window_config.ms_mode);

                this->utils.Utf8Encode(window_config.font_name);
                json[window_name]["font_name"] = window_config.font_name;
            }
        }

        json_string = json.dump(2); // Dump with indent of 2 spaces and new lines.

    } catch (nlohmann::json::type_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
        //} catch (nlohmann::json::exception& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
        //} catch (nlohmann::json::parse_error& e) {
        //    vislib::sys::Log::DefaultLog.WriteError(
        //        "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        //    return false;
    } catch (nlohmann::json::invalid_iterator& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::out_of_range& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (nlohmann::json::other_error& e) {
        vislib::sys::Log::DefaultLog.WriteError(
            "JSON ERROR - %s: %s (%s:%d)", __FUNCTION__, e.what(), __FILE__, __LINE__);
        return false;
    } catch (...) {
        vislib::sys::Log::DefaultLog.WriteError("[StateToJSON] Unknown Error - Unable to write JSON of state.");
        return false;
    }


    return true;
}

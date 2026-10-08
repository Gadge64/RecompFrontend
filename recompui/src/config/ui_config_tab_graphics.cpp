#include "librecomp/config.hpp"
#include "recompui/config.h"
#include "recompui/renderer.h"
#include "util/steam_deck.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>

static bool created_graphics_config = false;

namespace recompui {
    namespace config {
        recomp::config::Config &get_graphics_config() {
            if (!created_graphics_config) {
                throw std::runtime_error("Graphics config has not been created yet. Call create_graphics_tab() first.");
            }
            return config::get_config(config::graphics::id);
        }

        static ultramodern::renderer::WindowMode wm_default() {
            return is_steam_deck() ? ultramodern::renderer::WindowMode::Fullscreen : ultramodern::renderer::WindowMode::Windowed;
        }

        using EnumOptionVector = const std::vector<recomp::config::ConfigOptionEnumOption>;
        static EnumOptionVector resolution_options = {
            {ultramodern::renderer::Resolution::Original, "Original", "Original"},
            {ultramodern::renderer::Resolution::Original2x, "Original2x", "Original 2x"},
            {ultramodern::renderer::Resolution::Auto, "Auto", "Auto"},
        };

        // [wcw] Output resolution picker: the window's client size when windowed, the monitor's
        // display mode when fullscreen (restored on leaving fullscreen). Desktop = stock behaviour.
        struct OutputResolution { uint32_t width; uint32_t height; };
        static const OutputResolution output_resolutions[] = {
            { 0, 0 }, { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 1920, 1200 },
            { 2560, 1440 }, { 2560, 1600 }, { 3840, 2160 }, { 3840, 2400 },
        };
        static EnumOptionVector output_resolution_options = {
            {0u, "Desktop", "Desktop"},
            {1u, "1280x720", "720p"},
            {2u, "1600x900", "900p"},
            {3u, "1920x1080", "1080p"},
            {4u, "1920x1200", "1920x1200"},
            {5u, "2560x1440", "1440p"},
            {6u, "2560x1600", "2560x1600"},
            {7u, "3840x2160", "4K"},
            {8u, "3840x2400", "3840x2400"},
        };

        // [wcw] Framerate: 30 fps (the game's own rate, no generated frames) or 60/90/120 fps (generated frames, and in
        // fullscreen the display switches to that refresh rate). Value = fps. 90 and 120 are only offered when the
        // display has that rate (59/89/119 Hz count as 60/90/120).
        static uint32_t display_rate_for(uint32_t fps) {
            for (uint32_t rate : recompui::renderer::get_display_refresh_rates()) {
                if ((rate + 1 >= fps) && (rate <= fps + 1)) {
                    return rate;
                }
            }

            return 0;
        }

        static std::vector<recomp::config::ConfigOptionEnumOption> make_framerate_options() {
            std::vector<recomp::config::ConfigOptionEnumOption> options = {
                {30u, "30", "30 fps (Original)"},
                {60u, "60", "60 fps"},
            };

            const bool ratesKnown = !recompui::renderer::get_display_refresh_rates().empty();
            for (uint32_t fps : { 90u, 120u }) {
                if (!ratesKnown || (display_rate_for(fps) != 0)) {
                    options.emplace_back(fps, std::to_string(fps), std::to_string(fps) + " fps");
                }
            }

            return options;
        }

        enum class DownsamplingOption {
            Off = 0,
            X2 = 2,
            X4 = 4,
        };
        static EnumOptionVector downsampling_options = {
            {DownsamplingOption::Off, "Off"},
            {DownsamplingOption::X2, "2x"},
            {DownsamplingOption::X4, "4x"},
        };

        static EnumOptionVector window_mode_options = {
            {ultramodern::renderer::WindowMode::Windowed, "Windowed"},
            {ultramodern::renderer::WindowMode::Fullscreen, "Fullscreen"}
        };

        #if defined(_WIN32)
        #define ALLOW_D3D12
        #endif
        #if defined(_WIN32) || defined(__linux__)
        #define ALLOW_VULKAN
        #endif
        #if defined(__APPLE__)
        #define ALLOW_METAL
        #endif

        static EnumOptionVector graphics_api_options = {
            {ultramodern::renderer::GraphicsApi::Auto, "Auto"},
        #ifdef ALLOW_D3D12
            {ultramodern::renderer::GraphicsApi::D3D12, "D3D12"},
        #endif
        #ifdef ALLOW_VULKAN
            {ultramodern::renderer::GraphicsApi::Vulkan, "Vulkan"},
        #endif
        #ifdef ALLOW_METAL
            {ultramodern::renderer::GraphicsApi::Metal, "Metal"},
        #endif
        };

        static EnumOptionVector aspect_ratio_options = {
            {ultramodern::renderer::AspectRatio::Original, "Original"},
            {ultramodern::renderer::AspectRatio::Expand, "Expand"},
            // {ultramodern::renderer::AspectRatio::Manual, "Manual"},
        };

        static EnumOptionVector antialiasing_options = {
            {ultramodern::renderer::Antialiasing::None, "None"},
            {ultramodern::renderer::Antialiasing::MSAA2X, "MSAA2X", "2X"},
            {ultramodern::renderer::Antialiasing::MSAA4X, "MSAA4X", "4X"},
            // {ultramodern::renderer::Antialiasing::MSAA8X, "MSAA8X"},
        };

        static EnumOptionVector hpfb_options = {
            {ultramodern::renderer::HighPrecisionFramebuffer::Auto, "Auto"},
            {ultramodern::renderer::HighPrecisionFramebuffer::On, "On"},
            {ultramodern::renderer::HighPrecisionFramebuffer::Off, "Off"},
        };

        static EnumOptionVector hud_ratio_mode_options = {
            {ultramodern::renderer::HUDRatioMode::Original, "Original"},
            {ultramodern::renderer::HUDRatioMode::Clamp16x9, "Clamp16x9", "16:9"},
            {ultramodern::renderer::HUDRatioMode::Full, "Expand"},
        };

        static const std::string get_downsampling_details(ultramodern::renderer::Resolution res_option, DownsamplingOption ds_option) {
            switch (res_option) {
                default:
                case ultramodern::renderer::Resolution::Auto:
                    return "Downsampling is not available at auto resolution";
                case ultramodern::renderer::Resolution::Original:
                    if (ds_option == DownsamplingOption::X2) {
                        return "Rendered in 480p and scaled to 240p";
                    } else if (ds_option == DownsamplingOption::X4) {
                        return "Rendered in 960p and scaled to 240p";
                    }
                    return "";
                case ultramodern::renderer::Resolution::Original2x:
                    if (ds_option == DownsamplingOption::X2) {
                        return "Rendered in 960p and scaled to 480p";
                    } else if (ds_option == DownsamplingOption::X4) {
                        return "Rendered in 4K and scaled to 480p";
                    }
                    return "";
            }
        }

        using OptionChangeContext = recomp::config::OptionChangeContext;

        static recomp::config::ConfigValueVariant get_graphics_value_variant(const std::string& key, OptionChangeContext change_context) {
            if (change_context == OptionChangeContext::Temporary) {
                return get_graphics_config().get_temp_option_value(key);
            } else {
                return get_graphics_config().get_option_value(key);
            }
        }

        template <typename T, typename ValType>
        T get_graphics_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return static_cast<T>(std::get<ValType>(get_graphics_value_variant(key, change_context)));
        }

        template <typename T = uint32_t>
        T get_graphics_enum_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return get_graphics_value<T, uint32_t>(key, change_context);
        }
    
        template <typename T = uint32_t>
        T get_graphics_number_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return get_graphics_value<T, double>(key, change_context);
        }

        static bool get_graphics_bool_value(const std::string& key, OptionChangeContext change_context = OptionChangeContext::Permanent) {
            return std::get<bool>(get_graphics_value_variant(key, change_context));
        }

        static void determine_downsampling_display(ultramodern::renderer::Resolution res_option, OptionChangeContext change_context) {
            DownsamplingOption ds_opt = get_graphics_enum_value<DownsamplingOption>(graphics::options::ds_option, change_context);
            get_graphics_config().update_option_enum_details(graphics::options::ds_option, get_downsampling_details(res_option, ds_opt));
        }

        static void apply_graphics_config() {
            ultramodern::renderer::GraphicsConfig new_config;
            new_config.developer_mode = get_graphics_bool_value(graphics::options::developer_mode);
            new_config.res_option = get_graphics_enum_value<ultramodern::renderer::Resolution>(graphics::options::res_option);
            new_config.wm_option = get_graphics_enum_value<ultramodern::renderer::WindowMode>(graphics::options::wm_option);
            new_config.hr_option = get_graphics_enum_value<ultramodern::renderer::HUDRatioMode>(graphics::options::hr_option);
            new_config.api_option = get_graphics_enum_value<ultramodern::renderer::GraphicsApi>(graphics::options::api_option);
            new_config.ar_option = get_graphics_enum_value<ultramodern::renderer::AspectRatio>(graphics::options::ar_option);
            new_config.msaa_option = get_graphics_enum_value<ultramodern::renderer::Antialiasing>(graphics::options::msaa_option);

            new_config.hpfb_option = get_graphics_enum_value<ultramodern::renderer::HighPrecisionFramebuffer>(graphics::options::hpfb_option);

            new_config.ds_option = get_graphics_enum_value<int>(graphics::options::ds_option);

            uint32_t output_index = get_graphics_enum_value<uint32_t>(graphics::options::output_res_option);
            if (output_index >= std::size(output_resolutions)) {
                output_index = 0;
            }
            new_config.output_width = int(output_resolutions[output_index].width);
            new_config.output_height = int(output_resolutions[output_index].height);
            const uint32_t fps = get_graphics_enum_value<uint32_t>(graphics::options::framerate_option);
            if (fps <= 30) {
                // The game's own frames only; the display stays at the 60 fps rate (no 30 Hz mode, and switching
                // between 30 and 60 fps never changes the display mode).
                new_config.rr_option = ultramodern::renderer::RefreshRate::Original;
                new_config.rr_manual_value = 30;
                new_config.output_refresh = int(display_rate_for(60));
            }
            else {
                new_config.rr_option = ultramodern::renderer::RefreshRate::Manual;
                new_config.rr_manual_value = int(fps);
                const uint32_t displayRate = display_rate_for(fps);
                new_config.output_refresh = int((displayRate != 0) ? displayRate : fps);
            }

            ultramodern::renderer::set_graphics_config(new_config);
        }

        void graphics::update_msaa_supported(bool supported) {
            recomp::config::Config &config = get_graphics_config();
            if (!supported) {
                config.update_option_enum_details(
                    graphics::options::msaa_option,
                    supported ? "Available" : "Not available (missing sample positions support)"
                );
                config.update_option_disabled(
                    graphics::options::msaa_option,
                    true
                );
            } else {
                auto max_msaa = recompui::renderer::RT64MaxMSAA();
                if (max_msaa < RT64::UserConfiguration::Antialiasing::MSAA2X) {
                    config.update_enum_option_disabled(graphics::options::msaa_option, static_cast<uint32_t>(ultramodern::renderer::Antialiasing::MSAA2X), true);
                }
                if (max_msaa < RT64::UserConfiguration::Antialiasing::MSAA4X) {
                    config.update_enum_option_disabled(graphics::options::msaa_option, static_cast<uint32_t>(ultramodern::renderer::Antialiasing::MSAA4X), true);
                }
            }
        }

        void graphics::update_refresh_rate(uint32_t refresh_rate) {
            (void)refresh_rate; // The Framerate option lists fixed, tested rates instead.
        }

        void graphics::toggle_fullscreen() {
            auto current_wm = get_graphics_enum_value<ultramodern::renderer::WindowMode>(graphics::options::wm_option, OptionChangeContext::Temporary);
            auto new_wm = current_wm == ultramodern::renderer::WindowMode::Windowed ? ultramodern::renderer::WindowMode::Fullscreen : ultramodern::renderer::WindowMode::Windowed;
            get_graphics_config().set_option_value(graphics::options::wm_option, static_cast<uint32_t>(new_wm));
            get_graphics_config().apply_option_value(graphics::options::wm_option);
            apply_graphics_config();
        }

        recomp::config::Config &create_graphics_tab(const std::string &name) {
            created_graphics_config = true;

            recomp::config::Config &config = recompui::config::create_config_tab(name, graphics::id, true);
            config.set_save_callback(apply_graphics_config);
            config.set_load_callback(apply_graphics_config);

            config.add_bool_option(
                graphics::options::developer_mode,
                "Dev Mode",
                "Enables developer features.",
                false,
                true
            );

            config.add_enum_option(
                graphics::options::output_res_option,
                "Resolution",
                "Sets the game's screen resolution. In <recomp-color primary>Fullscreen</recomp-color> the display switches to this resolution while the game is fullscreen and goes back to your desktop resolution afterwards. In <recomp-color primary>Windowed</recomp-color> mode the window is sized to it (if it fits on screen). <recomp-color primary>Desktop</recomp-color> uses your desktop resolution."
                "<br />"
                "<br />"
                "Resolutions your display doesn't support fall back to the desktop resolution.",
                output_resolution_options,
                0u
            );

            config.add_enum_option(
                graphics::options::res_option,
                "Internal Resolution",
                "Sets the resolution the game renders at internally. <recomp-color primary>Auto</recomp-color> renders at the full screen resolution above. <recomp-color primary>Original</recomp-color> matches the game's original 240p resolution. <recomp-color primary>Original 2x</recomp-color> will render at 480p.",
                resolution_options,
                ultramodern::renderer::Resolution::Auto
            );
            {
                config.add_option_change_callback(
                    graphics::options::res_option,
                    [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant prev_value, OptionChangeContext change_context) {
                        auto new_opt = static_cast<ultramodern::renderer::Resolution>(std::get<uint32_t>(cur_value));
                        determine_downsampling_display(new_opt, change_context);
                    }
                );
            }

            config.add_enum_option(
                graphics::options::ds_option,
                "Downsampling Quality",
                "Renders at a higher resolution and scales it down to the output resolution for increased quality. Only available in <recomp-color primary>Original</recomp-color> and <recomp-color primary>Original 2x</recomp-color> resolution."
                "<br />"
                "<br />"
                "Note: <recomp-color primary>4x</recomp-color> downsampling quality at <recomp-color primary>Original 2x</recomp-color> resolution may cause performance issues on low end devices, as it will cause the game to render <recomp-color warning>at almost 4k internal resolution</recomp-color>.",
                downsampling_options,
                DownsamplingOption::Off
            );
            {
                config.add_option_change_callback(
                    graphics::options::ds_option,
                    [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant prev_value, OptionChangeContext change_context) {
                        auto resolution_opt = get_graphics_enum_value<ultramodern::renderer::Resolution>(graphics::options::res_option, change_context);
                        determine_downsampling_display(resolution_opt, change_context);
                    }
                );

                config.add_option_disable_dependency(
                    graphics::options::ds_option,
                    graphics::options::res_option,
                    ultramodern::renderer::Resolution::Auto
                );
    
                config.on_json_parse_option(graphics::options::ds_option, [](const nlohmann::json& j) {
                    return j.get<uint32_t>();
                });
    
                config.on_json_serialize_option(graphics::options::ds_option, [](const recomp::config::ConfigValueVariant& value) {
                    return nlohmann::json(std::get<uint32_t>(value));
                });
            }

            config.add_enum_option(
                graphics::options::ar_option,
                "Aspect Ratio",
                "Sets the horizontal aspect ratio. <recomp-color primary>Original</recomp-color> uses the game's original 4:3 aspect ratio. <recomp-color primary>Expand</recomp-color> will adjust to match the game window's aspect ratio.",
                aspect_ratio_options,
                ultramodern::renderer::AspectRatio::Expand
            );

            config.add_enum_option(
                graphics::options::wm_option,
                "Window Mode",
                "Sets whether the game should display <recomp-color primary>Windowed</recomp-color> or <recomp-color primary>Fullscreen</recomp-color>. You can also use <recomp-color primary>F11</recomp-color> or <recomp-color primary>Alt + Enter</recomp-color> to toggle this option.",
                window_mode_options,
                wm_default()
            );

            static const std::vector<recomp::config::ConfigOptionEnumOption> framerate_options = make_framerate_options();
            config.add_enum_option(
                graphics::options::framerate_option,
                "Framerate",
                "Sets how many frames per second are shown. The game itself runs at 30 frames per second: <recomp-color primary>30 fps (Original)</recomp-color> shows exactly those; higher settings add smooth in-between frames. This does not affect gameplay."
                "<br />"
                "<br />"
                "In <recomp-color primary>Fullscreen</recomp-color> the display switches to the matching refresh rate (your desktop's rate comes back afterwards); <recomp-color primary>90</recomp-color> and <recomp-color primary>120 fps</recomp-color> are only listed when your display supports them. In <recomp-color primary>Windowed</recomp-color> mode the framerate is limited to your desktop's refresh rate.",
                framerate_options,
                60u
            );

            config.add_enum_option(
                graphics::options::msaa_option,
                "MS Anti-Aliasing",
                "Sets the multisample anti-aliasing (MSAA) quality level. This reduces jagged edges in the final image at the expense of rendering performance."
                "<br />"
                "<br />"
                "<recomp-color primary>Note: This option won't be available if your GPU does not support programmable MSAA sample positions, as it is currently required to avoid rendering glitches.</recomp-color>",
                antialiasing_options,
                ultramodern::renderer::Antialiasing::MSAA2X
            );

            config.add_enum_option(
                graphics::options::hr_option,
                "HUD Placement",
                "Adjusts the placement of HUD elements to fit the selected aspect ratio. <recomp-color primary>Expand</recomp-color> will use the aspect ratio of the game's output window.",
                hud_ratio_mode_options,
                ultramodern::renderer::HUDRatioMode::Clamp16x9
            );

            config.add_enum_option(
                graphics::options::api_option,
                "Graphics API",
                "Selects the graphics API to use.",
                graphics_api_options,
                ultramodern::renderer::GraphicsApi::Auto,
                true
            );

            config.add_enum_option(
                graphics::options::hpfb_option,
                "High Precision Framebuffer",
                "Sets whether to use a high precision framebuffer.",
                hpfb_options,
                ultramodern::renderer::HighPrecisionFramebuffer::Off,
                true
            );

            return config;
        }
    }
} // namespace recompui

#ifndef CONFIG_H
#define CONFIG_H

#ifdef _WIN32
#pragma once
#endif

#include <vector>
#include <string>

#include <platform.h>
#include <convar.h>

#include <keydefs.h>

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#else
#error Implement Linux equivalent for toggle button
#endif

//-----------------------------------------------------------------------------
// CConfig
//-----------------------------------------------------------------------------

class CConfig
{
	friend void sc_load_config(const CCommand &args);
	friend void sc_save_config(const CCommand &args);

public:
	void Init();
	void UpdateConfigs();

	bool Load();
	void Save();

	void New();
	void Remove();

	struct config_vars
	{
		//-----------------------------------------------------------------------------
		// General
		//-----------------------------------------------------------------------------

		DWORD toggle_button = VK_INSERT;
		bool menu_auto_resize = true;
		int menu_theme = 0;
		float menu_opacity = 1.0f;
		bool tooltips = false;
		float logo_color[3] = { 1.0f, 1.0f, 1.0f };
		bool rainbow[2] = { false, false };
		int rainbow_speed = 3;

		bool menu_blur = true;
		float menu_blur_fadein_duration = 0.5f;
		float menu_blur_fadeout_duration = 0.4f;
		float menu_blur_radius = 20.f;
		float menu_blur_bokeh = 0.7f;
		int menu_blur_samples = 20;

		//-----------------------------------------------------------------------------
		// Shaders
		//-----------------------------------------------------------------------------
		
		bool shaders = true;

		bool shaders_show_depth_buffer = false;
		float shaders_depth_buffer_znear = 4.f;
		float shaders_depth_buffer_zfar = 4096.f;
		float shaders_depth_buffer_brightness = 1.f;

		bool shaders_ssao = false;
		bool shaders_ssao_onlyAO = false;
		float shaders_ssao_znear = 4.f;
		float shaders_ssao_zfar = 4096.f;
		float shaders_ssao_strength = 1.f;
		int shaders_ssao_samples = 32;
		float shaders_ssao_radius = 2.5f;
		float shaders_ssao_aoclamp = 0.14f;
		bool shaders_ssao_noise = true;
		float shaders_ssao_noiseamount = 0.0002f;
		float shaders_ssao_diffarea = 0.3f;
		float shaders_ssao_gdisplace = 0.4f;
		bool shaders_ssao_mist = false;
		float shaders_ssao_miststart = 0.f;
		float shaders_ssao_mistend = 4096.f;
		float shaders_ssao_lumInfluence = 0.7f;

		bool shaders_color_correction = false;
		float shaders_cc_target_gamma = 2.2f;
		float shaders_cc_monitor_gamma = 2.2f;
		float shaders_cc_hue_offset = 0.f;
		float shaders_cc_saturation = 1.f;
		float shaders_cc_contrast = 1.f;
		float shaders_cc_luminance = 1.f;
		float shaders_cc_black_level = 0.f;
		float shaders_cc_bright_boost = 0.f;
		float shaders_cc_R = 1.f;
		float shaders_cc_G = 1.f;
		float shaders_cc_B = 1.f;
		float shaders_cc_grain = 0.f;
		//float shaders_cc_sharpness = 0.f;

		bool shaders_chromatic_aberration = false;
		int shaders_chromatic_aberration_type = 1;
		float shaders_chromatic_aberration_dir_x = 1.f;
		float shaders_chromatic_aberration_dir_y = 1.f;
		float shaders_chromatic_aberration_shift = 0.025f;
		float shaders_chromatic_aberration_strength = 1.f;

		bool shaders_dof_blur = false;
		float shaders_dof_blur_min_range = 1024.f;
		float shaders_dof_blur_max_range = 4096.f;
		int shaders_dof_blur_interp_type = 1;
		float shaders_dof_blur_bluriness_range = 20.f;
		int shaders_dof_blur_quality = 20;
		float shaders_dof_blur_bokeh = 0.7f;

		bool shaders_motion_blur = true;
		float shaders_motion_blur_strength = 2.f;
		float shaders_motion_blur_min_speed = 270.f;
		float shaders_motion_blur_max_speed = 700.f;
		
		bool shaders_radial_blur = false;
		float shaders_radial_blur_distance = 1.f;
		float shaders_radial_blur_strength = 2.f;

		bool shaders_bokeh_blur = false;
		float shaders_bokeh_blur_radius = 20.f;
		float shaders_bokeh_blur_coeff = 0.7f;
		int shaders_bokeh_blur_samples = 20;

		bool shaders_gaussian_blur = false;
		float shaders_gaussian_blur_radius = 10.f;

		bool shaders_gaussian_blur_fast = false;
		float shaders_gaussian_blur_fast_radius = 1.f;
		
		bool shaders_vignette = false;
		float shaders_vignette_falloff = 0.5f;
		float shaders_vignette_amount = 0.4f;

		//-----------------------------------------------------------------------------
		// Aim
		//-----------------------------------------------------------------------------
		
		bool aimbot = false;
		bool ragebot = false;
		bool silent_aimbot = false;
		bool aimbot_aim_hitboxes = true;
		bool aimbot_aim_head = true;
		bool aimbot_aim_neck = true;
		bool aimbot_aim_chest = true;
		bool aimbot_aim_unknown_ents = false;
		bool aimbot_ignore_glass = false;
		bool aimbot_ignore_blockers = false;
		bool aimbot_consider_fov = false;
		float aimbot_fov = 90.f;
		float aimbot_distance = 2048.f;
		bool no_recoil = false;
		bool no_recoil_visual = false;

		//-----------------------------------------------------------------------------
		// ESP
		//-----------------------------------------------------------------------------

		bool esp = true;
		bool esp_debug = false;
		int esp_box = 1;
		bool esp_optimize = false;
		bool esp_snaplines = false;
		bool esp_box_outline = true;
		int esp_box_fill = 0;
		bool esp_box_index = false;
		bool esp_box_player_health = true;
		bool esp_box_player_armor = true;
		bool esp_box_distance = true;
		bool esp_box_entity_name = true;
		bool esp_box_player_name = true;
		bool esp_show_items = true;
		bool esp_show_visible_players = false;
		bool esp_ignore_unknown_ents = false;
		bool esp_skeleton = false;
		bool esp_bones_name = false;
		int esp_player_style = 0;
		int esp_entity_style = 0;
		int esp_targets = 0;
		int esp_box_targets = 0;
		int esp_distance_mode = 0;
		int esp_skeleton_type = 1;

		float esp_distance = 8192.0f;

		float esp_friend_player_color[3] = { 0.0f, 1.0f, 0.0f };
		float esp_enemy_player_color[3] = { 1.0f, 0.0f, 0.0f };

		float esp_friend_color[3] = { 0.0f, 1.0f, 0.0f };
		float esp_enemy_color[3] = { 1.0f, 0.0f, 0.0f };
		float esp_neutral_color[3] = { 1.0f, 1.0f, 0.0f };
		float esp_item_color[3] = { 0.0f, 0.53f, 1.0f };

		//-----------------------------------------------------------------------------
		// Visual
		//-----------------------------------------------------------------------------

		bool lightmap_override = false;
		float lightmap_brightness = 1.0f;
		float lightmap_color[3] = { 1.0f, 1.0f, 1.0f };

		bool no_shake = false;
		bool no_fade = false;

		bool skip_frames = false;
		int skip_frames_count = 1;

		bool show_sound_origin = false;

		bool show_players_push_direction = false;
		float push_direction_length = 16.0f;
		float push_direction_width = 1.0f;
		float push_direction_color[3] = { 1.0f, 0.f, 0.f };
		
		bool show_players_sight_direction = false;
		float players_sight_direction_length = 48.f;
		float players_sight_direction_width = 1.0f;
		float players_sight_direction_color[3] = { 1.0f, 1.f, 1.f };

		bool remap_hud_color = false;
		float hud_color[3] = { 1.0f, 0.78f, 0.f };

		int draw_entities = 0; // enum

		bool show_speed = false;
		bool show_vertical_speed = false;
		bool show_jumpspeed = false;
		float jumpspeed_fade_duration = 0.7f;
		float speed_width_fraction = 0.5f;
		float speed_height_fraction = 0.9f;
		float speed_color[3] = { 100.f / 255.f, 130.f / 255.f, 200.f / 255.f };
		
		bool show_speed_legacy = false;
		bool show_vertical_speed_legacy = false;
		float speed_width_fraction_legacy = 0.5f;
		float speed_height_fraction_legacy = 0.88f;
		float speed_color_legacy[4] = { 1.0f, 0.75f, 0.0f, 0.882f };

		bool wallhack = false;
		bool wallhack_negative = false;
		bool wallhack_white_walls = false;
		bool wallhack_wireframe = false;
		bool wallhack_wireframe_models = false;

		float wh_wireframe_width = 1.5f;
		float wh_wireframe_color[3] = { 0.0f, 1.0f, 0.0f };

		//-----------------------------------------------------------------------------
		// Models Manager
		//-----------------------------------------------------------------------------
		
		const char *replace_model = NULL;

		bool replace_players_models = false;
		bool replace_model_on_self = false;
		bool replace_players_models_with_randoms = false;
		bool replace_specified_players_models = false;
		bool dont_replace_specified_players_models = false;
		
		//-----------------------------------------------------------------------------
		// BSP
		//-----------------------------------------------------------------------------
		
		bool show_spawns = false;

		bool show_triggers = false;
		bool show_triggers_info = false;
		bool show_trigger_once = true;
		bool show_trigger_multiple = true;
		bool show_trigger_hurt = true;
		bool show_trigger_hurt_heal = true;
		bool show_trigger_push = true;
		bool show_trigger_teleport = true;
		bool show_trigger_changelevel = true;
		bool show_trigger_antirush = true;

		float trigger_once_color[4] = { 1.0f, 1.0f, 0.0f, 0.25f };
		float trigger_multiple_color[4] = { 1.0f, 1.0f, 0.0f, 0.25f };
		float trigger_hurt_color[4] = { 1.0f, 0.0f, 0.0f, 0.25f };
		float trigger_hurt_heal_color[4] = { 0.0f, 1.0f, 0.0f, 0.25f };
		float trigger_push_color[4] = { 1.0f, 0.5f, 0.0f, 0.25f };
		float trigger_teleport_color[4] = { 0.9084f, 0.0f, 0.9084f, 0.25f };
		float trigger_changelevel_color[4] = { 1.0f, 1.0f, 1.0f, 0.25f };
		float trigger_antirush_color[4] = { 0.0f, 0.25f, 1.0f, 0.25f };

		//-----------------------------------------------------------------------------
		// Flashlight
		//-----------------------------------------------------------------------------
		
		bool custom_flashlight = false;

		bool flashlight_localplayer = true;
		bool flashlight_players = true;
		bool flashlight_lighting_localplayer = true;
		bool flashlight_lighting_players = true;

		float flashlight_localplayer_flashlight_distance = 2000.f;
		float flashlight_localplayer_falloff_distance = 500.f;
		float flashlight_localplayer_radius = 80.f;
		float flashlight_localplayer_color[3] = { 1.0f, 1.0f, 1.0f };

		float flashlight_players_flashlight_distance = 2000.f;
		float flashlight_players_falloff_distance = 500.f;
		float flashlight_players_radius = 80.f;
		float flashlight_players_color[3] = { 1.0f, 1.0f, 1.0f };

		float flashlight_lighting_localplayer_distance = 18.f;
		float flashlight_lighting_localplayer_radius = 128.f;
		float flashlight_lighting_localplayer_color[3] = { 32.f / 255.0f, 32.f / 255.0f, 32.f / 255.0f };

		float flashlight_lighting_players_distance = 18.f;
		float flashlight_lighting_players_radius = 128.f;
		float flashlight_lighting_players_color[3] = { 32.f / 255.0f, 32.f / 255.0f, 32.f / 255.0f };

		//-----------------------------------------------------------------------------
		// Crosshair
		//-----------------------------------------------------------------------------

		bool draw_crosshair = true;
		bool draw_crosshair_dot = false;
		bool draw_crosshair_outline = true;
		int crosshair_size = 10;
		int crosshair_gap = 4;
		int crosshair_thickness = 2;
		int crosshair_outline_thickness = 1;
		float crosshair_outline_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float crosshair_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		//-----------------------------------------------------------------------------
		// Grenade's timer
		//-----------------------------------------------------------------------------
		
		bool grenade_timer = true;
		float grenade_timer_width_frac = 0.01f;
		float grenade_timer_height_frac = 0.5f;
		float grenade_timer_color[3] = { 100.f / 255.f, 130.f / 255.f, 200.f / 255.f };
		float grenade_explosive_time_color[3] = { 1.f, 65.f / 255.f, 65.f / 255.f };

		//-----------------------------------------------------------------------------
		// Chams
		//-----------------------------------------------------------------------------

		bool chams = false;

		int chams_players = 0;
		bool chams_players_wall = true;
		float chams_players_color[3] = { 0.0f, 1.0f, 0.0f };
		float chams_players_wall_color[3] = { 1.0f, 0.0f, 0.0f };

		int chams_entities = 0;
		bool chams_entities_wall = true;
		float chams_entities_color[3] = { 0.0f, 1.0f, 0.0f };
		float chams_entities_wall_color[3] = { 1.0f, 0.0f, 0.0f };

		int chams_items = 0;
		bool chams_items_wall = true;
		float chams_items_color[3] = { 0.0f, 0.0f, 1.0f };
		float chams_items_wall_color[3] = { 1.0f, 0.0f, 0.0f };

		//-----------------------------------------------------------------------------
		// Glow
		//-----------------------------------------------------------------------------
		
		bool glow = false;
		bool glow_optimize = false;

		int glow_players = 0;
		bool glow_players_wall = true;
		int glow_players_width = 10;
		float glow_players_color[3] = { 0.0f, 1.0f, 0.0f };

		int glow_entities = 0;
		bool glow_entities_wall = true;
		int glow_entities_width = 10;
		float glow_entities_color[3] = { 0.0f, 0.0f, 1.0f };

		int glow_items = 0;
		bool glow_items_wall = true;
		int glow_items_width = 10;
		float glow_items_color[3] = { 0.0f, 0.0f, 1.0f };

		//-----------------------------------------------------------------------------
		// Dynamic glow
		//-----------------------------------------------------------------------------

		bool dyn_glow_attach = false;

		bool dyn_glow_self = false;
		float dyn_glow_self_radius = 125.0f;
		float dyn_glow_self_decay = 0;
		float dyn_glow_self_color[3] = { 1.0f, 1.0f, 1.0f };

		bool dyn_glow_players = false;
		float dyn_glow_players_radius = 75.0f;
		float dyn_glow_players_decay = 0;
		float dyn_glow_players_color[3] = { 0.0f, 1.0f, 0.0f };

		bool dyn_glow_entities = false;
		float dyn_glow_entities_radius = 75.0f;
		float dyn_glow_entities_decay = 0;
		float dyn_glow_entities_color[3] = { 1.0f, 0.0f, 0.0f };

		bool dyn_glow_items = false;
		float dyn_glow_items_radius = 50.0f;
		float dyn_glow_items_decay = 0;
		float dyn_glow_items_color[3] = { 0.0f, 0.0f, 1.0f };

		//-----------------------------------------------------------------------------
		// Strafer
		//-----------------------------------------------------------------------------

		bool strafe = true;
		bool strafe_ignore_ground = true;
		int strafe_dir = 3; // enum
		int strafe_type = 0; // enum

		//-----------------------------------------------------------------------------
		// Fake lag
		//-----------------------------------------------------------------------------

		bool fakelag = false;
		bool fakelag_adaptive_ex_interp = false;
		int fakelag_limit = 64; // enum
		int fakelag_move = 0; // enum
		int fakelag_type = 0; // enum
		float fakelag_variance = 3.5f;

		//-----------------------------------------------------------------------------
		// Anti-AFK
		//-----------------------------------------------------------------------------

		int antiafk = 0; // enum
		bool antiafk_rotate_camera = true;
		bool antiafk_stay_within_range = true;
		bool antiafk_reset_stay_pos = true;
		float antiafk_stay_radius = 200.0f;
		float antiafk_stay_radius_offset_angle = 30.0f;
		float antiafk_rotation_angle = -0.7f;

		//-----------------------------------------------------------------------------
		// Misc.
		//-----------------------------------------------------------------------------

		bool autojump = true;
		bool jumpbug = false;
		bool edgejump = false;
		bool ducktap = false;
		bool fastrun = false;
		bool quake_guns = false;
		bool autoreload = false;
		bool tertiary_attack_glitch = false;
		bool save_soundcache = false;
		bool rotate_dead_body = false;
		bool remove_fov_cap = false;
		bool auto_ceil_clipping = false;
		int no_weapon_anim = 0; // enum

		bool auto_wallstrafing = false;
		float wallstrafing_angle = 6.5f;
		float wallstrafing_dist = 32.0f;

		bool color_pulsator = false;
		bool color_pulsator_top = true;
		bool color_pulsator_bottom = true;
		float color_pulsator_delay = 0.5f;

		bool revert_pitch = false;
		bool revert_yaw = false;

		bool lock_pitch = false;
		bool lock_yaw = false;
		float lock_pitch_angle = 0.0f;
		float lock_yaw_angle = 0.0f;

		bool spin_yaw_angle = false;
		bool spin_pitch_angle = false;
		float spin_yaw_rotation_angle = 0.5f;
		float spin_pitch_rotation_angle = 0.5f;

		bool ignore_different_map_versions = false;
		bool use_only_helmet_models = false;
		bool use_helmet_model_on_self = false;

		bool fast_crowbar = false;
		bool fast_crowbar2 = false;
		bool fast_medkit = false;
		bool one_tick_exploit = false;
		int one_tick_exploit_lag_interval = 45;
		float one_tick_exploit_speedhack = 20000.0f;

		//-----------------------------------------------------------------------------
		// Speedrun Tools
		//-----------------------------------------------------------------------------

		bool st_timer = false;

		bool st_player_hulls = false;
		bool st_server_player_hulls = false;
		float st_player_hulls_color[4] = { 232.f / 255.f, 0.f / 255.f, 232.f / 255.f, 96.f / 255.f };
		float st_player_hulls_dead_color[4] = { 189.f / 255.f, 134.f / 255.f, 240.f / 255.f, 96.f / 255.f };

		float st_timer_color[3] = { 100.f / 255.f, 130.f / 255.f, 200.f / 255.f };
		float st_timer_width_frac = 0.01f;
		float st_timer_height_frac = 0.35f;

		float st_hud_color[3] = { 180.f / 255.f, 220.f / 255.f, 1.f };

		bool st_show_view_angles = false;
		float st_show_view_angles_width_frac = 0.895f;
		float st_show_view_angles_height_frac = 0.25f;

		bool st_show_pos = false;
		bool st_show_pos_view_origin = true;
		float st_show_pos_width_frac = 0.009f;
		float st_show_pos_height_frac = 0.15f;

		bool st_show_velocity = false;
		float st_show_velocity_width_frac = 0.895f;
		float st_show_velocity_height_frac = 0.09f;

		bool st_show_gauss_boost_info = false;
		float st_show_gauss_boost_info_width_frac = 0.3f;
		float st_show_gauss_boost_info_height_frac = 0.03f;

		bool st_show_selfgauss_info = false;
		float st_show_selfgauss_width_frac = 0.575f;
		float st_show_selfgauss_height_frac = 0.03f;

		bool st_show_entity_info = false;
		bool st_show_entity_info_check_players = true;
		float st_show_entity_info_width_frac = 0.009f;
		float st_show_entity_info_height_frac = 0.4f;
		
		//-----------------------------------------------------------------------------
		// Spam inputs
		//-----------------------------------------------------------------------------

		bool keyspam_hold_mode = true;
		bool keyspam_e = false;
		bool keyspam_w = false;
		bool keyspam_s = false;
		bool keyspam_q = false;
		bool keyspam_ctrl = false;

		//-----------------------------------------------------------------------------
		// Fog
		//-----------------------------------------------------------------------------

		bool fog = false;
		bool remove_water_fog = false;
		bool fog_skybox = true;
		float fog_start = 0.0f;
		float fog_end = 1000.0f;
		float fog_density = 0.25f;
		float fog_color[3] = { 1.0f, 1.0f, 1.0f };

		//-----------------------------------------------------------------------------
		// Skybox
		//-----------------------------------------------------------------------------

		int skybox = 0;

		//-----------------------------------------------------------------------------
		// Radar
		//-----------------------------------------------------------------------------
		
		bool radar = false;
		bool radar_show_player_name = true;
		bool radar_show_entity_name = true;
		int radar_type = 0;
		int radar_size = 250;
		float radar_distance = 2048.f;
		float radar_width_frac = 0.01f;
		float radar_height_frac = 0.01f;

		//-----------------------------------------------------------------------------
		// Chat colors
		//-----------------------------------------------------------------------------

		bool enable_chat_colors = true;
		float player_name_color[3] = { 0.6f, 0.75f, 1.0f };
		float chat_rainbow_update_delay = 0.05f;
		float chat_rainbow_hue_delta = 0.015f;
		float chat_rainbow_saturation = 0.8f;
		float chat_rainbow_lightness = 0.5f;
		float chat_color_one[3] = { 0.25f, 0.25f, 1.0f };
		float chat_color_two[3] = { 1.0f, 0.25f, 0.25f };
		float chat_color_three[3] = { 1.0f, 1.0f, 0.25f };
		float chat_color_four[3] = { 0.25f, 1.0f, 0.25f };
		float chat_color_five[3] = { 1.0f, 0.57f, 0.0f };
		float chat_color_six[3] = { 1.0f, 1.0f, 1.0f };

		//-----------------------------------------------------------------------------
		// Cam hack
		//-----------------------------------------------------------------------------

		bool camhack_show_model = true;
		bool camhack_hide_hud = true;
		float camhack_speed_factor = 1.0f;

		//-----------------------------------------------------------------------------
		// First-person roaming
		//-----------------------------------------------------------------------------

		bool fp_roaming = true;
		bool fp_roaming_draw_crosshair = true;
		bool fp_roaming_lerp = true;
		float fp_roaming_lerp_value = 0.15f;
		
		//-----------------------------------------------------------------------------
		// Enhanced thirdperson
		//-----------------------------------------------------------------------------

		bool thirdperson = false;
		bool thirdperson_ignore_pitch = false;
		bool thirdperson_ignore_yaw = false;
		bool thirdperson_edit_mode = false;
		bool thirdperson_hidehud = true;
		bool thirdperson_clip_to_wall = true;
		int thirdperson_trace_type = 1;
		float thirdperson_angles[3] = { 0.f, 0.f, 0.f };
		float thirdperson_origin[3] = { -64.f, 0.f, 12.f };

		//-----------------------------------------------------------------------------
		// Auto vote (outdated)
		//-----------------------------------------------------------------------------

		//int autovote_mode = 0; // enum
		//bool autovote_custom = true;
		//bool autovote_ignore_filter = false;

		//-----------------------------------------------------------------------------
		// Custom vote popup
		//-----------------------------------------------------------------------------

		bool vote_popup = true;
		int vote_popup_yes_key = K_F1;
		int vote_popup_no_key = K_F2;
		int vote_popup_width_size = 250;
		int vote_popup_height_size = 125;
		int vote_popup_w_border_pix = 12;
		int vote_popup_h_border_pix = 7;
		float vote_popup_width_frac = 0.015f;
		float vote_popup_height_frac = 0.37f;
	};

	config_vars cvars;
	config_vars default_cvars;

	std::string current_config;
	std::vector<std::string> configs;
};

//-----------------------------------------------------------------------------
// Export
//-----------------------------------------------------------------------------

extern CConfig g_Config;

#endif // CONFIG_H
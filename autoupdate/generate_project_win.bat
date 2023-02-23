perl generate_app_info.pl sven_internal sven_internal.dll 2 0 27

perl bin2c.pl ../resources/sven_internal/images/logo.png src/files/au_logo_raw au_logo
perl bin2c.pl ../resources/sven_internal/images/menu_image.png src/files/au_menu_image_raw au_menu_image
perl bin2c.pl ../resources/sven_internal/message_spammer/bunny.txt src/files/au_bunny_raw au_bunny
perl bin2c.pl ../resources/sven_internal/scripts/main.lua src/files/au_mainlua_raw au_mainlua
perl bin2c.pl ../resources/sven_internal/scripts/test.lua src/files/au_testlua_raw au_testlua
perl bin2c.pl ../resources/svencoop/sound/sven_internal/beep_synthtone01.wav src/files/au_beep_synthtone01wav_raw au_beep_synthtone01wav
perl bin2c.pl ../resources/svencoop/sound/sven_internal/hitmarker.wav src/files/au_hitmarkerwav_raw au_hitmarkerwav
perl bin2c.pl ../resources/svencoop/sound/sven_internal/menu_click01.wav src/files/au_menu_click01wav_raw au_menu_click01wav
perl bin2c.pl ../resources/svencoop/sound/sven_internal/talk.wav src/files/au_talkwav_raw au_talkwav
perl bin2c.pl ../resources/svencoop/sven_internal/tex/hitmarker.tga src/files/au_hitmarkertga_raw au_hitmarkertga
perl bin2c.pl ../resources/svencoop/sven_internal/tex/radar_round.tga src/files/au_radar_roundtga_raw au_radar_roundtga
perl bin2c.pl ../resources/svenmod/plugins/sven_internal.dll src/files/au_sven_internal_raw au_sven_internal
perl bin2c.pl ../resources/glew32.dll src/files/au_glew32_raw au_glew32

IF NOT EXIST "project" (
	mkdir project
)

cd project

cmake .. -A Win32

pause

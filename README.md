# Sven Internal
Sven Internal is a C++ plugin for [SvenMod](https://github.com/sw1ft747/SvenMod) that provides to you various cheats and gameplay enhances

Currently supported version of the game: 5.25

Imagine cheating in a co-op game =)

# How to install
First, if you don't have installed SvenMod then download it and install (see [readme](https://github.com/sw1ft747/svenmod)). Download the archive `svenint.rar` from ([releases](https://github.com/sw1ft747/sven_internal/releases)) and place all files from the archive in the root folder of the game. Next, add the plugin (`sven_internal.dll`) to the file `plugins.txt` (see the header `Adding plugins` in SvenMod's [readme](https://github.com/sw1ft747/svenmod)). 

# Features
- Menu (key **INSERT** as default)
- Customizable Configs
- ESP
- Aimbot
- No Recoil
- Wallhack
- Glow & Chams
- Vectorial Strafer
- Auto Bunnyhop
- Auto Jumpbug
- Fast Run
- Fake Lag
- Color Pulsator
- Speed Hack
- Anti-AFK
- Cam Hack
- Key Spammer
- First-Person Roaming
- Various Visual Hacks (Velometer, Crosshair, etc.)
- Message Spammer
- Models Manager
- Skybox Replacement
- Custom Vote Popup
- Custom Chat Colors

# Files of the plugin
The plugin uses subfolder `sven_internal` in the root directory of the game.

How it looks: `../Sven Co-op/sven_internal/`.

This folder is used to save the config, load list of players (their Steam64 ID) for **Chat Colors** and load spam tasks for **Message Spammer**.

Folder `config` contains all config files. You can save your config via menu or console command `sc_save_config`, also you can load a config via concommand `sc_load_config <optional: filename>`. Automatically the plugin load the config named `default.ini`.

File `chat_colors_players.txt` allows to change the color of nickname for a specific player, will be automatically loaded at plugin load.

Folder `message_spammer` is used by **Message Spammer** to load spam tasks.

Also, when cheat loaded it will execute `sven_internal.cfg` file from folder `../Sven Co-op/svencoop/`.

# Console Variables/Commands
Type in the console the following command: `sm printcvars all ? sc_`.

The command above will print information about each CVar/ConCommand that belongs to the plugin.

# Chat Colors
That feature lets you change the color of nickname of players when they write something in the chat.

File `chat_colors_players.txt` used for adding the players in such format: `STEAM64ID : COLOR_NUMBER`.

To convert traditional SteamID to Steam64 ID faster, you can use the following console command: `sc_steamid_to_steam64id [SteamID]`.

There're currently 5 color numbers (slots) plus rainbow color, thus you can use 6 unique and customizable colors (can be changed in Menu).

Rainbow color has slot 0, others custom colors have slotos from 1 to 5.

Example for the file:
```
76561198819023292 : 0 # it's a comment
76561197962091295 : 5 ; it's a comment too!
```

File `chat_colors_players.txt` automatically loads when cheat loaded. Also, you can use the console command `sc_chat_colors_load_players` to reload the list of players.

# Models Manager
Lets you change the models of players the working directory is `models_manager`

File `random_models.txt` contains the list of models that will be randomly used

File `target_players.txt` contains the list of pairs of type `STEAM64ID = MODELNAME` to replace models for specified players

File `ignored_players.txt` contains the list of Steam64 ID's that will be ignored when you replace models for specified players

The files above will be automatically loaded when the plugin is loaded

For more information, check the files above, they contain comments

# Message Spammer
Roughly, it's some kind of AHK.

It uses `*.txt` files from folder `message_spammer` to run spam tasks. Use the following console command to run a spam task: `sc_ms_add <filename>`.

It supports 3 keywords: `loop`, `send` and `sleep`.

Important: Message Spammer reads the `*.txt` files sequentially.

For example:
```
sleep 1.5
send test string
```
It will be executed like: wait 1.5 seconds, then send to game chat `test string`

### loop
Must be placed at the beginning of the file.

Loops the spam task, otherwise if you don't place that keyword in your .txt file, then the spam task will be executed once.

### send [message]
Sends a message to game chat.

One argument: [message].

### sleep [delay]
Sleeps a spam task.

One argument: [delay].

**Example:**
```
loop
send stuck
sleep 0.35
send play
sleep 0.35
send diff
sleep 0.35
send alone
sleep 0.35
send light
sleep 0.35
send my ears
sleep 0.35
send vote
sleep 0.35
send buy
sleep 0.35
send thief
sleep 125.0
```

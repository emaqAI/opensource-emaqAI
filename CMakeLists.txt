# Chrome City - PS1 (PSX) homebrew game
# Built with PSn00bSDK (https://github.com/Lameguy64/PSn00bSDK)

cmake_minimum_required(VERSION 3.21)

project(
	ChromeCity
	LANGUAGES    C ASM
	VERSION      1.0.0
	DESCRIPTION  "Chrome City - an open-world driving/crime game for the original PlayStation"
	HOMEPAGE_URL "https://github.com/emaqai/opensource-emaqai"
)

psn00bsdk_add_executable(chromecity GPREL
	src/main.c
	src/render.c
	src/clip.c
	src/world.c
	src/vehicle.c
	src/camera.c
	src/traffic.c
	src/police.c
	src/mission.c
	src/hud.c
	src/input.c
)

target_include_directories(chromecity PRIVATE src)

psn00bsdk_add_cd_image(
	iso        # Target name
	chromecity # Output file name (= chromecity.bin + chromecity.cue)
	iso.xml    # Path to config file
	DEPENDS chromecity system.cnf
)

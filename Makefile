# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core

# User config

NAME		= Nitro-Player
GAME_TITLE = Nitro Composer playback demo
GAME_AUTHOR = henke37
GAME_SUBTITLE = prealpha
#GAME_ICON = icon.bmp
NITROFSDIR = FileSystem

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9arm7/Makefile

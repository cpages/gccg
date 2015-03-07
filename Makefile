#####################################################################
# 
# Configuration
#
#####################################################################

# System identification string
SYSTEM="`uname -m` `uname -s` build by `echo $$USER` `date +'%Y-%m-%d'`"
# C++ compiler
CXX=g++
# C compiler
CC=gcc
# Linker
LD=g++
# Explicit location of sdl-config if not in $PATH.
SDLCONFIG=sdl-config
# Tracker address
TRACKER=http://tracker.thepiratebay.org/announce
# Set these if compiling with squirrel.
#CFLAGS_SQUIRREL=-I../SQUIRREL2/include -D USE_SQUIRREL
#LIBS_SQUIRREL=-lsquirrel -lsqstdlib -L../SQUIRREL2/lib
#COMMON_SQUIRREL=tmp/parser_libsquirrel.o

#####################################################################
#
# Definitions
#
#####################################################################

DEFINES=-DPACKAGE=\"GCCG\" -DSYSTEM=\"$(SYSTEM)\" -DCCG_DATADIR=\".\" -DCCG_SAVEDIR=\"./save\" -DSTACK_TRACE

#CFLAGS=-I./src/include -g -Wall `$(SDLCONFIG) --cflags` -O3 $(CFLAGS_SQUIRREL)
CFLAGS=-I./src/include -g -Wall `$(SDLCONFIG) --cflags` $(CFLAGS_SQUIRREL)

CXXFLAGS=-ftemplate-depth-30 $(CFLAGS)

LIBS=`$(SDLCONFIG) --libs` -lSDL_net -lSDL_image -ljpeg -lSDL_ttf -lSDL_mixer $(LIBS_SQUIRREL)

LIBS_TEXT=`$(SDLCONFIG) --libs` -lSDL_net -lSDL_mixer $(LIBS_SQUIRREL)

COMMON=tmp/parser_libcards.o tmp/parser_libnet.o tmp/parser.o tmp/data_filedb.o tmp/parser_lib.o tmp/tools.o tmp/carddata.o tmp/xml_parser.o tmp/security.o tmp/data.o tmp/localization.o $(COMMON_SQUIRREL)

CLIENT=tmp/client.o $(COMMON) tmp/driver.o tmp/game.o tmp/interpreter.o tmp/SDL_rotozoom.o

SERVER=tmp/server.o $(COMMON) 

GCCG=tmp/gccg.o $(COMMON)

CXXCMD=$(CXX) $(ARGS) $(DEFINES) $(CXXFLAGS) -c

CCCMD=$(CC) $(ARGS) $(DEFINES) $(CFLAGS) -c

#####################################################################
#
# Help
#
#####################################################################

help:
	@echo
	@echo Usage:
	@echo
	@echo "  make all            - rebuild client and server binaries"
	@echo "  make client         - rebuild client binaries"
	@echo "  make server         - rebuild server binaries"
	@echo "  make clean          - clear backup and temporary files"
	@echo "  make gccg           - rebuild simple command line script interpreter"
	@echo

#####################################################################
#
# Targets
#
#####################################################################

all: client server

client: ccg_text_client ccg_client

server: ccg_server

.PHONY: clean pkgclean official

clean:
	rm -f tmp/*.o *~ */*~ */*/*~ */*/*/*~ sheet*.*

ccg_text_client: $(CLIENT) tmp/text-driver.o tmp/game-text-version.o
	$(LD) -o ccg_text_client $(CLIENT) tmp/text-driver.o tmp/game-text-version.o $(LIBS_TEXT)
	strip ccg_text_client

ccg_client: $(CLIENT) tmp/sdl-driver.o tmp/game-sdl-version.o
	$(LD) -o ccg_client $(CLIENT) tmp/sdl-driver.o tmp/game-sdl-version.o $(LIBS)
	strip ccg_client

ccg_server: $(SERVER)
	$(LD) -o ccg_server $(SERVER) $(LIBS_TEXT)
	strip ccg_server

gccg: $(GCCG)
	$(LD) -o gccg $(GCCG) $(LIBS_TEXT)

parse_stats: tmp/parse_stats.o $(COMMON)
	$(LD) -o parse_stats tmp/parse_stats.o $(COMMON) $(LIBS_TEXT)

tmp/%.o: src/%.cpp src/include/*.h
	mkdir -p tmp
	$(CXXCMD) -o $@ $<

tmp/game-text-version.o: src/game-draw.cpp
	$(CXXCMD) -D TEXT_VERSION -o tmp/game-text-version.o src/game-draw.cpp

tmp/game-sdl-version.o: src/game-draw.cpp
	$(CXXCMD) -D SDL_VERSION -o tmp/game-sdl-version.o src/game-draw.cpp

tmp/SDL_rotozoom.o: src/SDL_rotozoom.c
	$(CCCMD) -o tmp/SDL_rotozoom.o src/SDL_rotozoom.c

# web:
# 	rm -rf ./build/sites/gccg.sourceforge.net/
# 	mkdir -p ./build/sites/gccg.sourceforge.net/
# 	rm -f doc/gccg.sourceforge.net/*~
# 	rsync --delete -ax doc/gccg.sourceforge.net/* ./build/sites/gccg.sourceforge.net/
# 	mkdir -p ./build/sites/gccg.sourceforge.net/graphics/avatar
# 	cp graphics/avatar/*.png ./build/sites/gccg.sourceforge.net/graphics/avatar
# 	perl/svn_cleanup.pl ./build/sites/gccg.sourceforge.net/ > /dev/null
# 	rm -rf ./build/sites/gccg.sourceforge.net/modules
# 	mkdir -p ./build/sites/gccg.sourceforge.net/modules
# 	cp ./build/mirrors/gccg.sourceforge.net/available.xml ./build/sites/gccg.sourceforge.net/modules/
# 	cp ./build/mirrors/gccg.sourceforge.net/gccg-*.tgz ./build/sites/gccg.sourceforge.net/modules/
# 	cp installer/gccg_install.zip ./build/sites/gccg.sourceforge.net/downloads

# web-update:
# 	rsync --delete -avx ./build/sites/gccg.sourceforge.net/ `./tools/user uid`,gccg@web.sf.net:htdocs

# revision:
# 	strip $(TARGETS)
# 	./gccg_package revision

# mirrors:
# 	./gccg_package create_mirrors gccg.sourceforge.net www.derangedmonkey.com lotrtcgdb.com

# rebuild:
# 	strip $(TARGETS)
# 	cp $(TARGETS) `tools/sys_module`
# 	./gccg_package b

# nr-zip:
# 	tools/make_zip nr

# nr-torrent:
# 	rm -rf build
# 	mkdir build
# 	rm -rf ../build/torrents/Gccg-Nr-`tools/make_zip --version nr`
# 	unzip -d build ../build/torrents/Gccg-Nr-`tools/make_zip --version nr`.zip
# 	mkdir -p ../build/torrents/
# 	mv build/Gccg-Nr-* ../build/torrents/
# 	btmakemetafile $(TRACKER) ../build/torrents/Gccg-Nr-`tools/make_zip --version nr`
# 	rm -rf build
# 	cp ../build/torrents/Gccg-Nr-`tools/make_zip --version nr`.torrent save

# full-build:
# 	$(MAKE) clean
# 	rm -rf ../build/modules
# 	$(MAKE) -j2 all
# 	$(MAKE) rebuild
# 	rm -rf ../build/torrents
# 	$(MAKE) nr-zip
# 	$(MAKE) nr-torrent
# 	$(MAKE) mirrors
# 	$(MAKE) web

# update:
# 	$(MAKE) rebuild
# 	$(MAKE) mirrors
# 	$(MAKE) web
# 	$(MAKE) web-update

cmake_minimum_required(VERSION 3.5)
project(fly8 LANGUAGES C CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(CheckFunctionExists)
check_function_exists(stricmp HAS_STRICMP)
check_function_exists(strnicmp HAS_STRNICMP)
check_function_exists(strdup HAS_STRDUP)
check_function_exists(strcasecmp HAS_STRCASECMP)
check_function_exists(strncasecmp HAS_STRNCASECMP)
check_function_exists(perror HAS_PERROR)

option(GENERATE_FP_TABLE "Precompute the Fixed-Point Function Tables" OFF)

# configuration defaults.
set(INIFILE "fly.ini")
set(LOGFILE "fly.log")

set(MAC_EXT "mac")
set(MAX_EXT "max")
set(VMD_EXT "vmd")
set(PRM_EXT "prm")
set(SHP_EXT "vxx")
set(NAV_EXT "nav")
set(LND_EXT "lnd")

set(RTMODE "rt")
set(RBMODE "rb")
set(WTMODE "w")
set(WBMODE "wb")
set(ATMODE "at")
set(ABMODE "ab")

set(C_MAIN SDL_main)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(FLY8_SYSTEM_TYPE MSWIN)
    set(SYS_MSWIN_32 ON)
    set(SYS_MSWIN_NT ON)
    set(HAVE_JOYSTICK ON)
    set(HAVE_MIDI ON)
    set(HAVE_WAVE ON)
    set(PATHSEP ";")
    set(NOSTDERR ON)
    set(SYS_WINSOCK ON)
    set(HAVE_UDP ON)
    set(PARMS_BUILD_SCRIPT misc/build.bat)
    if (MSVC)
        set(SHORT_TYPE int)
        set(INLINED __inline)
        set(FASTCALL __fastcall)
        set(AFASTCALL __fastcall)

        if(NOT DEFINED CONAN_LINK_RUNTIME AND DEFINED CONAN_SETTINGS_COMPILER_RUNTIME)
            message(WARNING "Forcing MSVC runtime based on conan profile")
            # this means Conan is not running the build, so we need to fix our linker flags ourself.... although we can probably cheat...
            set(CONAN_LINK_RUNTIME "/${CONAN_SETTINGS_COMPILER_RUNTIME}")
            conan_set_vs_runtime()

            # finally, warn us about the flags.
            foreach(FLAG CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO)
                message(STATUS "Compiler Flags ${FLAG} is set to ${${FLAG}}")
            endforeach()
        endif()

    endif ()
endif ()

configure_file(config.h.in config.h)

# everything references these two include dirs.
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# surpress MSVC complaining about ANSI C functions.
if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
endif ()

# objects
set(OBJ_SRC
        object.c
        obasic.c
        obox.c
        obroken.c
        ochute.c
        oclassic.c
        ocrater.c
        ofplane.c
        oground.c
        ogtarget.c
        ohouse.c
        olow.c
        om61.c
        omk82.c
        opaddoc.c
        oplane.c
        orunway.c
        osmoke.c
        otarget.c
        otower.c
        oviewer.c
        oxplane.c
        oyplane.c
        ogen.c
        ocar.c)

# portable programs
set(MAIN_SRC
        airdata.c
        alarms.c
        altitude.c
        autop.c
        body.c
        btrail.c
        cockpit.c
        colors.c
        colors.h
        config.h
        command.c
        compass.c
        debug.c
        grmgr.c
        editstr.c
        engine.c
        field.c
        fly.h
        fly8.c
        fly8str.c
        gear.c
        heading.c
        hud.c
        hudmenu.c
        ifuncs.c
        info.c
        init.c
        kbdmgr.c
        keypad.c
        lamps.c
        land.c
        log.c
        loop.c
        macros.c
        mat.c
        max.c
        memory.c
        menu.c
        menus.c
        message.c
        nav.c
        need.c
        netmgr.c
        nogr.c
        nokbd.c
        nosound.c
        nosystem.c
        notimer.c
        objects.c
        panel.c
        pid.c
        piper.c
        pitch.c
        player.c
        ptrmgr.c
        prm.c
        radar.c
        random.c
        remote.c
        show.c
        sixdof.c
        sky.c
        sndmgr.c
        speed.c
        stack.c
        stfont1.c
        stfont2.c
        stores.c
        stroke.c
        symbols.c
        system.c
        term.c
        tunes.c
        util.c
        views.c
        vmodes.c
        vv.c
        waypoint.c
        window.c
        buffers.c
        lnd.c
        version.c)
add_executable(fly8
        ${MAIN_SRC}
        ${OBJ_SRC})
target_link_libraries(fly8
        PRIVATE
        CONAN_PKG::sdl2
        CONAN_PKG::lua
        )
target_include_directories(fly8
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/COMMON
        )
source_group(Objects FILES ${OBJ_SRC})
source_group(Core FILES ${MAIN_SRC})
install(TARGETS
        fly8
        RUNTIME DESTINATION .)

### Generate the FP function tables.
if(GENERATE_FP_TABLE)
    add_executable(ifuncsm
            ifuncs.c debug.c fly.h colors.h config.h)
    target_compile_definitions(ifuncsm PRIVATE GEN_TAB)
    target_include_directories(ifuncsm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(
            OUTPUT itabs.h
            COMMAND ifuncsm ARGS > itabs.h)
    target_sources(fly8 PRIVATE itabs.h)
    target_compile_definitions(fly8 PRIVATE EXT_TABS)
endif()

### fly8 utilities.
add_executable(mac2max utils/mac2max.c keydef.h keymap.h keyname.h)
add_executable(max2mac utils/max2mac.c keydef.h keymap.h keyname.h)
install(TARGETS
        mac2max max2mac
        RUNTIME DESTINATION .)

### Build the UDP Server
if(HAVE_UDP)
    add_executable(fly8udp
            COMMON/fly8udp.c
            COMMON/fly8udp.h)
    add_executable(udpmgr
            COMMON/udpmgr.c
            COMMON/fly8udp.h)
    install(TARGETS fly8udp udpmgr RUNTIME DESTINATION .)
endif()

### Fly8 Data
file(GLOB FILES_VMD LIST_DIRECTORIES false ${FLY8_SYSTEM_TYPE}/*.vmd)
file(GLOB FILES_SYSTEM_READMES LIST_DIRECTORIES false ${FLY8_SYSTEM_TYPE}/readme.*)
file(GLOB FILES_PARMS LIST_DIRECTORIES false parms/*.prm parms/*.nav parms/*.lnd)
file(GLOB FILES_SHAPES LIST_DIRECTORIES false shapes/*.vxx)
file(GLOB FILES_DOCS LIST_DIRECTORIES false docs/*.doc docs/*.pdf)

install(FILES
        ${FLY8_SYSTEM_TYPE}/fly.ini
        docs/bugs.txt
        ${FILES_VMD}
        ${FILES_SYSTEM_READMES}
        ${FILES_PARMS}
        ${FILES_SHAPES}
        ${FILES_DOCS}
        DESTINATION .)
# default plane shape
install(FILES shapes/wf16.vxx
        DESTINATION .
        RENAME plane.vxx)

### Fly8 uncooked data
file(GLOB FILES_RAWDATA parms/*.prc parms/*.lnc parms/*.nac)
install(FILES
        parms.h
        shape.h
        colors.h
        parms/expr.awk
        ${PARMS_BUILD_SCRIPT}
        ${REDISTRIBUTABLE_AWK}
        ${REDISTRIBUTABLE_CPP}
        ${FILES_RAWDATA}
        DESTINATION "data")

# system conditional sources
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_target_properties(fly8 PROPERTIES WIN32_EXECUTABLE ON)
    target_include_directories(fly8
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/MSWIN)
    target_sources(fly8
            PRIVATE
            SDL/console.c
            SDL/timer.c
            SDL/grsdl.c
            SDL/sdlplatform.c
            COMMON/bgr.c
            COMMON/bgr.h
            COMMON/bgrasm.c
            COMMON/vgr.c
            COMMON/vgr.h
            COMMON/grstat.c
            COMMON/misc.c
            COMMON/mouse.c
            COMMON/mouse.h
            COMMON/plsound.c
            COMMON/plsound.h
            COMMON/stick.c
            COMMON/stick.h
            COMMON/udp.c
            MSWIN/drivers.c
            MSWIN/w32stick.c
            MSWIN/mmsound.c
            MSWIN/wave.c
            MSWIN/wave.h
            MSWIN/plmidi.c
            MSWIN/common.c
            MSWIN/common.h
            )
    target_link_libraries(fly8
            PRIVATE
            wsock32 user32 ddraw winmm gdi32)
    target_link_libraries(fly8udp
            PRIVATE
            wsock32)
    target_link_libraries(udpmgr
            PRIVATE
            wsock32)
endif ()

workspace "tdcaster"
    architecture "x86_64"
    configurations { "debug", "release" }
    platforms { "linux" }
    startproject "tdcaster"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
srcdir = "%{prj.name}/src"
project "tdcaster"
    location "tdcaster"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("obj/" .. outputdir .. "/%{prj.name}")
    files
    {
        srcdir .. "/**.h",
        srcdir .. "/**.cpp",
        srcdir .. "/**.hpp"
    }
    links {
        "GL",
        "pthread",
        "png",
        "stdc++fs",
        "X11"
    }
    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"
    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "Speed"
workspace "tdcaster"
    architecture "x86_64"
    configurations { "debug", "release" }
    platforms { "linux", "win64" }
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
    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"
    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "Speed"
    filter "platforms:linux"
        links {
            "GL",
            "pthread",
            "png",
            "stdc++fs",
            "X11"
        }
    filter "platforms:win64"
        links {
            "user32",
            "gdi32",
            "opengl32",
            "gdiplus",
            "Shlwapi",
            "stdc++fs"
        }
workspace "Hulk engine"
	 architecture "x64"


	 configurations
	 {
	 	"Debug",
	 	"Release",
	 	"Dist"
	 }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "Hulk"
	location "Hulk"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")


	_CRT_SECURE_NO_WARNINGS;
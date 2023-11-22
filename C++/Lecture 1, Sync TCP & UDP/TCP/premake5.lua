workspace "SyncTCP"
   configurations { "Debug", "Release" }

project "SyncTCPServer"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   files { "SyncTcpServer.cpp" }

project "SyncTCPClient"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   files { "SyncTcpClient.cpp" }

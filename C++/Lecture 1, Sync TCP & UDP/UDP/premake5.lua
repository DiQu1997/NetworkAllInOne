workspace "SyncUDP"
   configurations { "Debug", "Release" }

project "SyncUDPServer"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   files { "SyncUdpServer.cpp" }

project "SyncUDPClient"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   files { "SyncUdpClient.cpp" }

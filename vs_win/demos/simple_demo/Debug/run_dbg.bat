Set Driver=%1
set LOGROOT=%2
Set InternalLog=%LOGROOT%_intern.txt
Set ExternalLog=%LOGROOT%_extern.txt
D:\blender-git\build_windows_x64_vc15_Release\bin\Debug\blender.exe -P D:\unrealProjects\hIK\tester_unreal\Externals\bvh_posture_reset\vs_win\demos\simple_demo\Debug\imp_bvh.py > %ExternalLog%
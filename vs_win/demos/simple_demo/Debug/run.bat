Set Driver=%1
set LOGROOT=%2
Set ExternalLog=%LOGROOT%_extern.txt
"C:\Blender Foundation\Blender\blender.exe" -P D:\unrealProjects\hIK\tester_unreal\Externals\bvh_posture_reset\vs_win\demos\simple_demo\Debug\imp_bvh.py > %ExternalLog%
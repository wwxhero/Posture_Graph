@echo off
rem posture_graph_gen_parallel <INTERESTS_XML> <BVH_DIR_SRC> <PGS_DIR_DST> <Epsilon> <N_threads>
rem posture_graph_merge <INTERESTS_XML> <PG_DIR_SRC> <PG_DIR_DST> <PG_NAME> <Epsilon> <N_threads>
Set Interests_XML=%1
Set BVH_dir_src_trimmed=%2
Set PG_dir_dst=%3
Set Eps=%4
Set /A Eps_merge = Eps+1
Set N_threads=%5
Set PGS_dir_dst=%PG_dir_dst%\Intermediate
Set PG_dir_dst_sub=%PG_dir_dst%\Sub

rem mkdir %PGS_dir_dst%

rem to generate posture graphs for each HTR file stored in BVH_DIR_SRC
rem echo posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% ">" posture_graph_gen_parallel.log
rem posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% > posture_graph_gen_parallel.log

mkdir %PG_dir_dst_sub%
rem to merge all the PGs into one PG

rem Set List_sub_folders=cmuconvert-mb2-01-09 cmuconvert-mb2-10-14 cmuconvert-mb2-102-111 cmuconvert-mb2-113-128 cmuconvert-mb2-131-135 cmuconvert-mb2-136-140 cmuconvert-mb2-141-144 cmuconvert-mb2-15-19 cmuconvert-mb2-20-29 cmuconvert-mb2-30-34 cmuconvert-mb2-35-39 cmuconvert-mb2-40-45 cmuconvert-mb2-46-56 cmuconvert-mb2-60-75 cmuconvert-mb2-76-80 cmuconvert-mb2-81-85 cmuconvert-mb2-86-94 cmuconvert01-09
Set List_sub_folders[0]=cmuconvert-mb2-01-09
Set List_sub_folders[1]=cmuconvert-mb2-10-14
Set List_sub_folders[2]=cmuconvert-mb2-102-111
Set List_sub_folders[3]=cmuconvert-mb2-113-128
Set List_sub_folders[4]=cmuconvert-mb2-131-135
Set List_sub_folders[5]=cmuconvert-mb2-136-140
Set List_sub_folders[6]=cmuconvert-mb2-141-144
Set List_sub_folders[7]=cmuconvert-mb2-15-19
Set List_sub_folders[8]=cmuconvert-mb2-20-29
Set List_sub_folders[9]=cmuconvert-mb2-30-34
Set List_sub_folders[10]=cmuconvert-mb2-35-39
Set List_sub_folders[11]=cmuconvert-mb2-40-45
Set List_sub_folders[12]=cmuconvert-mb2-46-56
Set List_sub_folders[13]=cmuconvert-mb2-60-75
Set List_sub_folders[14]=cmuconvert-mb2-76-80
Set List_sub_folders[15]=cmuconvert-mb2-81-85
Set List_sub_folders[16]=cmuconvert-mb2-86-94
Set List_sub_folders[17]=cmuconvert01-09

Set N_sub_folders=17

rem del .\posture_graph_merge_parallel_LowerBack.log

rem for %%a in (%List_sub_folders%) do (
rem    mkdir %PG_dir_dst_sub%\%%a
rem    posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%a %PG_dir_dst_sub%\%%a LowerBack %Eps_merge% 4 >> posture_graph_merge_parallel_LowerBack.log
rem    rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%a %PG_dir_dst_sub%\%%a LeftUpLeg %Eps_merge% 2 >> posture_graph_merge_parallel_LeftUpLeg.log
rem    rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%a %PG_dir_dst_sub%\%%a RightUpLeg %Eps_merge% 2 >> posture_graph_merge_parallel_RightUpLeg.log
rem    rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%a %PG_dir_dst_sub%\%%a LeftArm %Eps_merge% 2 >> posture_graph_merge_parallel_LeftArm.log
rem    rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%a %PG_dir_dst_sub%\%%a RightArm %Eps_merge% 2 >> posture_graph_merge_parallel_RightArm.log
rem )

copy %PG_dir_dst_sub%\%List_sub_folders[0]%\*  %PG_dir_dst%

for /L %%i in (1, 1, %N_sub_folders%) do (
	rem call echo %%List_sub_folders[%%i]%%
	call echo posture_graph_merge_file %%Interests_XML%% %%PG_dir_dst%% %%PG_dir_dst_sub%%\%%List_sub_folders[%%i]%% LowerBack %%Eps_merge%% %%PG_dir_dst%%
	call posture_graph_merge_file %%Interests_XML%% %%PG_dir_dst%% %%PG_dir_dst_sub%%\%%List_sub_folders[%%i]%% LowerBack %%Eps_merge%% %%PG_dir_dst%%
	rem call echo !List_sub_folders[%%i]!
	rem echo %List_sub_folders[0]%
	rem echo %%List_sub_folders[%%i]%%
)



rem echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% LeftArm %Eps_merge% 2 ">" posture_graph_merge_parallel_LeftArm.log
rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% LeftArm %Eps_merge% 2 > posture_graph_merge_parallel_LeftArm.log
rem echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% RightArm %Eps_merge% 2 ">" posture_graph_merge_parallel_RightArm.log
rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% RightArm %Eps_merge% 2 > posture_graph_merge_parallel_RightArm.log
rem echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% LeftUpLeg %Eps_merge% 2 ">" posture_graph_merge_parallel_LeftUpLeg.log
rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% LeftUpLeg %Eps_merge% 2 > posture_graph_merge_parallel_LeftUpLeg.log
rem echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% RightUpLeg %Eps_merge% 2 ">" posture_graph_merge_parallel_RightUpLeg.log
rem posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% RightUpLeg %Eps_merge% 2 > posture_graph_merge_parallel_RightUpLeg.log



@echo off
rem usage: pg_merge <INTERESTS_XML> <BVH_DIR_SRC> <PGS_DIR_DST> <Epsilon> <N_threads>
rem 			posture_graph_gen_parallel <INTERESTS_XML> <BVH_DIR_SRC> <PGS_DIR_DST> <Epsilon> <N_threads>
rem 			posture_graph_merge <INTERESTS_XML> <PG_DIR_SRC> <PG_DIR_DST> <PG_NAME> <Epsilon> <N_threads>
Set Interests_XML=%1
Set BVH_dir_src_trimmed=%2
Set PG_dir_dst=%3
Set Eps=%4
Set /A Eps_merge = Eps+1
Set N_threads=%5
Set PGS_dir_dst=%PG_dir_dst%\Intermediate
Set PG_dir_dst_sub=%PG_dir_dst%\SubMerge

mkdir %PGS_dir_dst%

rem to generate posture graphs for each HTR file stored in BVH_DIR_SRC
echo posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% ">" posture_graph_gen_parallel.log
posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% > posture_graph_gen_parallel.log

mkdir %PG_dir_dst_sub%
rem to merge all the PGs into one PG

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

del .\posture_graph_merge_parallel_LowerBack.log
del .\posture_graph_merge_parallel_LeftUpLeg.log
del .\posture_graph_merge_parallel_RightUpLeg.log
del .\posture_graph_merge_parallel_LeftArm.log
del .\posture_graph_merge_parallel_RightArm.log

for /L %%i in (0, 1, %N_sub_folders%) do (
	call mkdir %PG_dir_dst_sub%\%%List_sub_folders[%%i]%%
	call echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LowerBack %Eps_merge% 2
	call posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LowerBack %Eps_merge% 2 >> posture_graph_merge_parallel_LowerBack.log
	call echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LeftUpLeg %Eps_merge% 2
	call posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LeftUpLeg %Eps_merge% 2 >> posture_graph_merge_parallel_LeftUpLeg.log
	call echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% RightUpLeg %Eps_merge% 2
	call posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% RightUpLeg %Eps_merge% 2 >> posture_graph_merge_parallel_RightUpLeg.log
	call echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LeftArm %Eps_merge% 2
	call posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% LeftArm %Eps_merge% 2 >> posture_graph_merge_parallel_LeftArm.log
	call echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% RightArm %Eps_merge% 2
	call posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst%\%%List_sub_folders[%%i]%% %PG_dir_dst_sub%\%%List_sub_folders[%%i]%% RightArm %Eps_merge% 2 >> posture_graph_merge_parallel_RightArm.log
)

echo posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% RightArm %Eps_merge% 5
posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% RightArm %Eps_merge% 5 >> posture_graph_merge_parallel_RightArm

echo posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LeftArm %Eps_merge% 5
posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LeftArm %Eps_merge% 5 >> posture_graph_merge_parallel_LeftArm

echo posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% RightUpLeg %Eps_merge% 5
posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% RightUpLeg %Eps_merge% 5 >> posture_graph_merge_parallel_RightUpLeg

echo posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LeftUpLeg %Eps_merge% 5
posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LeftUpLeg %Eps_merge% 5 >> posture_graph_merge_parallel_LeftUpLeg

echo posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LowerBack %Eps_merge% 5
posture_graph_merge_parallel %Interests_XML% %PG_dir_dst_sub% %PG_dir_dst% LowerBack %Eps_merge% 5 >> posture_graph_merge_parallel_LowerBack
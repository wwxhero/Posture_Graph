@echo off

Set BVH_dir_src=%1
Set PG_dir_dst=%2
Set Eps=%3
Set N_threads=%4
Set T_reset_dir=%PG_dir_dst%\Intermediate_T_reset_0
Set HTR_raw_dir=%PG_dir_dst%\Intermediate_HTR_raw_1
Set HTR_disect_dir=%PG_dir_dst%\Intermediate_HTR_disect_2
Set HTR_trimmed_dir=%PG_dir_dst%\Intermediate_HTR_trimmed_3
Set HTR_dissect_XML=%BVH_dir_src%\disection.xml
Set PG_Interests_XML=%BVH_dir_src%\pg_interests.xml
Set PGS_dir_dst=%PG_dir_dst%\Intermediate_PGS_4

setlocal enabledelayedexpansion

set argCount=0
for %%x in (%*) do (
   set /A argCount+=1
   set "argVec[!argCount!]=%%~x"
)

IF NOT %argCount% == 4 (
	echo "Usage: build_pg <BVH_SRC_DIR> <PG_DST_DIR> <Epsilon> <N_thread>"
	exit 1
) ELSE (
	mkdir %PG_dir_dst%

	mkdir %T_reset_dir%
	echo bvh_posture_reset_parallel %BVH_dir_src% %T_reset_dir% %N_threads% ">" bvh_posture_reset.log
	bvh_posture_reset_parallel %BVH_dir_src% %T_reset_dir% %N_threads% > bvh_posture_reset.log

	mkdir %HTR_raw_dir%
	echo bvh_htr_conv_parallel %T_reset_dir% %HTR_raw_dir% %N_threads% ">" bvh_tr_conv.log
	bvh_htr_conv_parallel %T_reset_dir% %HTR_raw_dir% %N_threads% > bvh_tr_conv.log

	mkdir %HTR_disect_dir%
	echo htr_dissect_parallel %HTR_dissect_XML% %HTR_raw_dir% %HTR_disect_dir% %N_threads% ">" htr_dissect.log
	htr_dissect_parallel %HTR_dissect_XML% %HTR_raw_dir% %HTR_disect_dir% %N_threads% > htr_dissect.log
	copy %HTR_dissect_XML% %HTR_disect_dir%

	mkdir %HTR_trimmed_dir%
	echo htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LeftUpLeg.htr %N_threads% LeftFoot ">" htr_trimmed.log
	htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LeftUpLeg.htr %N_threads% LeftFoot > htr_trimmed.log

	echo htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% RightUpLeg.htr %N_threads% RightFoot ">>" htr_trimmed.log
	htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% RightUpLeg.htr %N_threads% RightFoot >> htr_trimmed.log

	echo htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LeftArm.htr %N_threads% LeftHand ">>" htr_trimmed.log
	htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LeftArm.htr %N_threads% LeftHand >> htr_trimmed.log

	echo htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% RightArm.htr %N_threads% RightHand ">>" htr_trimmed.log
	htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% RightArm.htr %N_threads% RightHand >> htr_trimmed.log

	echo htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LowerBack.htr %N_threads% Head LeftShoulder RightShoulder ">>" htr_trimmed.log
	htr_bvh_trim_parallel %HTR_disect_dir% %HTR_trimmed_dir% LowerBack.htr %N_threads% Head LeftShoulder RightShoulder >> htr_trimmed.log

	mkdir %PGS_dir_dst%
	rem to generate posture graphs for each HTR file stored in BVH_DIR_SRC
	echo posture_graph_gen_parallel %PG_Interests_XML% %HTR_trimmed_dir% %PGS_dir_dst% %Eps% %N_threads% ">" posture_graph_gen_parallel.log
	posture_graph_gen_parallel %PG_Interests_XML% %HTR_trimmed_dir% %PGS_dir_dst% %Eps% %N_threads% > posture_graph_gen_parallel.log
	copy %PG_Interests_XML% %PGS_dir_dst%

	rem to merge all the PGs into one PG
	rem it is a risk to memory system running 5 parts in parallel
	echo posture_graph_merge_parallel %PG_Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LowerBack LeftUpLeg RightUpLeg LeftArm RightArm ">" posture_graph_merge_parallel.log
	posture_graph_merge_parallel %PG_Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LowerBack LeftUpLeg RightUpLeg LeftArm RightArm > posture_graph_merge_parallel.log
	copy %PG_Interests_XML% %PG_dir_dst%

	exit 0
)

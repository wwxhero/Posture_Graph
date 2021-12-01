@echo off
rem usage: pg <INTERESTS_XML> <BVH_DIR_SRC> <PGS_DIR_DST> <Epsilon> <N_threads>
rem posture_graph_gen_parallel <INTERESTS_XML> <BVH_DIR_SRC> <PGS_DIR_DST> <Epsilon> <N_threads>
rem posture_graph_merge <INTERESTS_XML> <PG_DIR_SRC> <PG_DIR_DST> <PG_NAME> <Epsilon> <N_threads>
Set Interests_XML=%1
Set BVH_dir_src_trimmed=%2
Set PG_dir_dst=%3
Set Eps=%4
Set /A Eps_merge = Eps+1
Set N_threads=%5
Set PGS_dir_dst=%PG_dir_dst%\Intermediate
mkdir %PGS_dir_dst%
rem to generate posture graphs for each HTR file stored in BVH_DIR_SRC
echo posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% ">" posture_graph_gen_parallel.log
posture_graph_gen_parallel %Interests_XML% %BVH_dir_src_trimmed% %PGS_dir_dst% %Eps% %N_threads% > posture_graph_gen_parallel.log

rem to merge all the PGs into one PG

echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LowerBack LeftUpLeg RightUpLeg ">" posture_graph_merge_parallel_LowerBack_LeftUpLeg_RightUpLeg.log
posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LowerBack LeftUpLeg RightUpLeg > posture_graph_merge_parallel_LowerBack_LeftUpLeg_RightUpLeg.log

echo posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LeftArm RightArm ">" posture_graph_merge_parallel_LeftArm_RightArm.log
posture_graph_merge_parallel %Interests_XML% %PGS_dir_dst% %PG_dir_dst% %Eps% LeftArm RightArm > posture_graph_merge_parallel_LeftArm_RightArm.log



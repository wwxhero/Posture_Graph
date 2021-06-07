import bpy
import queue
import os

def stop_playback(scene):
    if(scene.frame_current == scene.frame_end):
        bpy.ops.screen.animation_cancel(restore_frame=False)

driver_file = os.environ['Driver']
bpy.ops.import_anim.bvh(filepath=driver_file, update_scene_duration=True)


bpy.app.handlers.frame_change_pre.append(stop_playback)


# bpy.ops.wm.quit_blender()




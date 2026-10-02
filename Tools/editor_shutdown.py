"""Let Slate finish deferred task notifications before automated editor exit.

For -ExecutePythonScript automation on UE 5.8.3. Calling quit_editor directly
bypasses the Python runner notification cleanup and crashes in ICU on this Mac.
Release keep-alive instead so the runner destroys its notification and queues
QUIT_EDITOR itself. Keep the editor ticking; time.sleep prevents that cleanup.
"""
import time
import unreal

_handle = None
_deadline = 0.0


def quit_after_notifications(seconds=6.0):
    global _handle, _deadline
    if _handle is not None:
        return
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    _deadline = time.monotonic() + max(2.0, seconds)

    def finish(delta_seconds):
        global _handle
        if time.monotonic() < _deadline:
            return
        unreal.unregister_slate_post_tick_callback(_handle)
        _handle = None
        unreal.log('AUTOMATION_NOTIFICATIONS_DRAINED: returning shutdown to Python runner')
        # Its RequestExit destroys the runner notification BEFORE queuing
        # QUIT_EDITOR. SystemLibrary.quit_editor bypasses that cleanup path.
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)

    _handle = unreal.register_slate_post_tick_callback(finish)
    unreal.log('AUTOMATION_SHUTDOWN_PENDING: keeping UI ticks active for notification cleanup')

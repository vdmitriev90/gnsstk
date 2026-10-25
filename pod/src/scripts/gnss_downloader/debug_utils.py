def setup_debug():
  import debugpy
  debugpy.listen(("localhost", 5678))
  print("[DEBUG] waiting for debugger on port 5678...")
  debugpy.wait_for_client()
  print("[DEBUG] debugger attached")

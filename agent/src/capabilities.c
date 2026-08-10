  // agent/src/capabilities.c — capability snapshot + JoinReq builder.
// TODO: implement vom_caps_init() (spawn a 30 s timer thread that
//       re-runs vom_sys_info_collect in the background so a hot-plug
//       of a USB dock or keyboard updates the capnp snapshot).
// TODO: implement vom_caps_snapshot() (returns the latest cached copy).
// TODO: implement vom_caps_build_join_req() that fills the generated
//       capnp JoinReq struct from the agent id + cached caps.


   
#define VOM_CAPS_INIT 30 
#define VOM_CAPS_SNAPSHOT
#define VOM_CAPS_BUILD_JOIN_REQ()


int main(int argc, char const *argv[])
{
  

  return 0;
}

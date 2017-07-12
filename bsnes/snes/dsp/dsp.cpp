#include <snes.hpp>

#define DSP_CPP
namespace SNES {

#if defined(DEBUGGER)
  #include "debugger/debugger.cpp"
  DSPDebugger dsp;
#else
  DSP dsp;
#endif

#include "serialization.cpp"

#define REG(n) state.regs[r_##n]
#define VREG(n) state.regs[v.vidx + v_##n]

#include "gaussian.cpp"
#include "counter.cpp"
#include "envelope.cpp"
#include "brr.cpp"
#include "misc.cpp"
#include "voice.cpp"
#include "echo.cpp"

/* timing */

void DSP::step(unsigned clocks) {
  clock += clocks;
}

void DSP::synchronize_smp() {
  if(SMP::Threaded == true) {
    if(clock >= 0 && scheduler.sync != Scheduler::SynchronizeMode::All) co_switch(smp.thread);
  } else {
    while(clock >= 0) smp.enter();
  }
}

void DSP::Enter() { dsp.enter(); }

void DSP::enter() {
#if DSP_THREADED
  #define PHASE(n)
  #define TICK tick()
  while(true) {
    if(scheduler.sync == Scheduler::SynchronizeMode::All) {
      scheduler.exit(Scheduler::ExitReason::SynchronizeEvent);
    }
#else
  #define PHASE(n) case n:
  #define TICK return tick()
  switch(phase & 31) {
#endif
    PHASE(0)
    voice_5(voice[0]);
    voice_2(voice[1]);
    TICK;

    PHASE(1)
    voice_6(voice[0]);
    voice_3(voice[1]);
    TICK;

    PHASE(2)
    voice_7(voice[0]);
    voice_4(voice[1]);
    voice_1(voice[3]);
    TICK;

    PHASE(3)
    voice_8(voice[0]);
    voice_5(voice[1]);
    voice_2(voice[2]);
    TICK;

    PHASE(4)
    voice_9(voice[0]);
    voice_6(voice[1]);
    voice_3(voice[2]);
    TICK;

    PHASE(5)
    voice_7(voice[1]);
    voice_4(voice[2]);
    voice_1(voice[4]);
    TICK;

    PHASE(6)
    voice_8(voice[1]);
    voice_5(voice[2]);
    voice_2(voice[3]);
    TICK;

    PHASE(7)
    voice_9(voice[1]);
    voice_6(voice[2]);
    voice_3(voice[3]);
    TICK;

    PHASE(8)
    voice_7(voice[2]);
    voice_4(voice[3]);
    voice_1(voice[5]);
    TICK;

    PHASE(9)
    voice_8(voice[2]);
    voice_5(voice[3]);
    voice_2(voice[4]);
    TICK;

    PHASE(10)
    voice_9(voice[2]);
    voice_6(voice[3]);
    voice_3(voice[4]);
    TICK;

    PHASE(11)
    voice_7(voice[3]);
    voice_4(voice[4]);
    voice_1(voice[6]);
    TICK;

    PHASE(12)
    voice_8(voice[3]);
    voice_5(voice[4]);
    voice_2(voice[5]);
    TICK;

    PHASE(13)
    voice_9(voice[3]);
    voice_6(voice[4]);
    voice_3(voice[5]);
    TICK;

    PHASE(14)
    voice_7(voice[4]);
    voice_4(voice[5]);
    voice_1(voice[7]);
    TICK;

    PHASE(15)
    voice_8(voice[4]);
    voice_5(voice[5]);
    voice_2(voice[6]);
    TICK;

    PHASE(16)
    voice_9(voice[4]);
    voice_6(voice[5]);
    voice_3(voice[6]);
    TICK;

    PHASE(17)
    voice_1(voice[0]);
    voice_7(voice[5]);
    voice_4(voice[6]);
    TICK;

    PHASE(18)
    voice_8(voice[5]);
    voice_5(voice[6]);
    voice_2(voice[7]);
    TICK;

    PHASE(19)
    voice_9(voice[5]);
    voice_6(voice[6]);
    voice_3(voice[7]);
    TICK;

    PHASE(20)
    voice_1(voice[1]);
    voice_7(voice[6]);
    voice_4(voice[7]);
    TICK;

    PHASE(21)
    voice_8(voice[6]);
    voice_5(voice[7]);
    voice_2(voice[0]);
    TICK;

    PHASE(22)
    voice_3a(voice[0]);
    voice_9(voice[6]);
    voice_6(voice[7]);
    echo_22();
    TICK;

    PHASE(23)
    voice_7(voice[7]);
    echo_23();
    TICK;

    PHASE(24)
    voice_8(voice[7]);
    echo_24();
    TICK;

    PHASE(25)
    voice_3b(voice[0]);
    voice_9(voice[7]);
    echo_25();
    TICK;

    PHASE(26)
    echo_26();
    TICK;

    PHASE(27)
    misc_27();
    echo_27();
    TICK;

    PHASE(28)
    misc_28();
    echo_28();
    TICK;

    PHASE(29)
    misc_29();
    echo_29();
    TICK;

    PHASE(30)
    misc_30();
    voice_3c(voice[0]);
    echo_30();
    TICK;

    PHASE(31)
    voice_4(voice[0]);
    voice_1(voice[2]);
    TICK;
  }
#undef PHASE
#undef TICK
}

void DSP::tick() {
  step(3 * 8);
#if DSP_THREADED
  synchronize_smp();
#else
  phase++;
#endif
}

/* register interface for S-SMP $00f2,$00f3 */

bool DSP::mute() {
  return state.regs[r_flg] & 0x40;
}

uint8 DSP::read(uint8 addr) {
  return state.regs[addr];
}

void DSP::write(uint8 addr, uint8 data) {
  state.regs[addr] = data;

  if((addr & 0x0f) == v_envx) {
    state.envx_buf = data;
  } else if((addr & 0x0f) == v_outx) {
    state.outx_buf = data;
  } else if(addr == r_kon) {
    state.new_kon = data;
  } else if(addr == r_endx) {
    //always cleared, regardless of data written
    state.endx_buf = 0;
    state.regs[r_endx] = 0;
  }
}

/* initialization */

void DSP::power() {
  memset(&state.regs, 0, sizeof state.regs);
  state.echo_hist_pos = 0;
  state.every_other_sample = false;
  state.kon = 0;
  state.noise = 0;
  state.counter = 0;
  state.echo_offset = 0;
  state.echo_length = 0;
  state.new_kon = 0;
  state.endx_buf = 0;
  state.envx_buf = 0;
  state.outx_buf = 0;
  state.t_pmon = 0;
  state.t_non = 0;
  state.t_eon = 0;
  state.t_dir = 0;
  state.t_koff = 0;
  state.t_brr_next_addr = 0;
  state.t_adsr0 = 0;
  state.t_brr_header = 0;
  state.t_brr_byte = 0;
  state.t_srcn = 0;
  state.t_esa = 0;
  state.t_echo_disabled = 0;
  state.t_dir_addr = 0;
  state.t_pitch = 0;
  state.t_output = 0;
  state.t_looped = 0;
  state.t_echo_ptr = 0;
  state.t_main_out[0] = state.t_main_out[1] = 0;
  state.t_echo_out[0] = state.t_echo_out[1] = 0;
  state.t_echo_in[0] = state.t_echo_in[1] = 0;

  for(unsigned i = 0; i < 8; i++) {
    voice[i].buf_pos = 0;
    voice[i].interp_pos = 0;
    voice[i].brr_addr = 0;
    voice[i].brr_offset = 1;
    voice[i].vbit = 1 << i;
    voice[i].vidx = i * 0x10;
    voice[i].kon_delay = 0;
    voice[i].env_mode = env_release;
    voice[i].env = 0;
    voice[i].t_envx_out = 0;
    voice[i].hidden_env = 0;
  }

  reset();
}

void DSP::reset() {
#if DSP_THREADED
  create(Enter, system.apu_frequency());
#else
  frequency = system.apu_frequency();
  clock = 0;
  phase = 0;
#endif

  REG(flg) = 0xe0;

  state.noise              = 0x4000;
  state.echo_hist_pos      = 0;
  state.every_other_sample = 1;
  state.echo_offset        = 0;
  state.counter            = 0;
}

DSP::DSP() {
  static_assert(sizeof(int) >= 32 / 8,    "int >= 32-bits");
  static_assert((int8)0x80 == -0x80,      "8-bit sign extension");
  static_assert((int16)0x8000 == -0x8000, "16-bit sign extension");
  static_assert((uint16)0xffff0000 == 0,  "16-bit unsigned clip");
  static_assert((-1 >> 1) == -1,          "arithmetic shift right");

  //-0x8000 <= n <= +0x7fff
  assert(sclamp<16>(+0x8000) == +0x7fff);
  assert(sclamp<16>(-0x8001) == -0x8000);
}

DSP::~DSP() {
}

}

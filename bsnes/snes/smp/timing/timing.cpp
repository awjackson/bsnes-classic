#ifdef SMP_CPP

void SMP::add_clocks(unsigned clocks) {
  step(clocks);
  synchronize_dsp();

  //forcefully sync S-SMP to S-CPU in case chips are not communicating
  //sync if S-SMP is more than 24 samples ahead of S-CPU
  if(clock > +(768 * 24 * (int64)24000000)) synchronize_cpu();
}

void SMP::cycle_edge(unsigned speed) {
  static const uint8 wait_states[] = {0, 24*1, 24*4, 24*9};
  static const uint8 timer_ticks[] = {3*1, 3*2, 3*4, 3*8};

  unsigned ticks = timer_ticks[speed];
  t0.tick(ticks);
  t1.tick(ticks);
  t2.tick(ticks);

  //24 clocks have already been added for this cycle at this point
  if(speed) add_clocks(wait_states[speed]);
}

template<unsigned timer_frequency>
void SMP::sSMPTimer<timer_frequency>::tick(unsigned step) {
  //stage 0 increment
  stage0_ticks += step;
  if(stage0_ticks < timer_frequency) return;
  stage0_ticks -= timer_frequency;

  //stage 1 increment
  stage1_ticks ^= 1;
  sync_stage1();
}

template<unsigned frequency>
void SMP::sSMPTimer<frequency>::sync_stage1() {
  bool new_line = stage1_ticks;
  if(smp.status.timers_enabled == false) new_line = false;
  if(smp.status.timers_disabled == true) new_line = false;

  bool old_line = current_line;
  current_line = new_line;
  if(old_line != 1 || new_line != 0) return;  //only pulse on 1->0 transition

  //stage 2 increment
  if(enabled == false) return;
  stage2_ticks++;
  if(stage2_ticks != target) return;

  //stage 3 increment
  stage2_ticks = 0;
  stage3_ticks++;
  stage3_ticks &= 15;
}

#endif

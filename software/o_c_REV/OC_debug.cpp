#include <Arduino.h>
#include "OC_ADC.h"
#include "OC_config.h"
#include "OC_debug.h"
#include "OC_menus.h"
#include "OC_ui.h"
#include "util/util_misc.h"
#include "extern/dspinst.h"

extern void POLYLFO_debug();
extern void LORENZ_debug();

namespace OC {

namespace DEBUG {
  SmoothedValue<uint32_t, 16> ISR_cycles;
  SmoothedValue<uint32_t, 16> UI_cycles;
  uint32_t UI_event_count;
  uint32_t UI_max_queue_depth;
  uint32_t UI_queue_overflow;

  void Init() {
    debug::CycleMeasurement::Init();
    DebugPins::Init();
  }
}; // namespace DEBUG

static void debug_menu_core() {

  uint32_t cycles = DEBUG::ISR_cycles.value();
  uint32_t isr_us = multiply_u32xu32_rshift32(cycles, (1ULL << 32) / (F_CPU / 1000000));

  graphics.setPrintPos(2, 12);
  graphics.printf("F_CPU: %uMHz", F_CPU / 1000 / 1000, OC_CORE_TIMER_RATE);
  graphics.setPrintPos(2, 22);
  graphics.printf("CORE :%3u/%uus %2u%%", isr_us, OC_CORE_TIMER_RATE, (isr_us * 100) /  OC_CORE_TIMER_RATE);

  cycles = DEBUG::UI_cycles.value();
  isr_us = multiply_u32xu32_rshift32(cycles, (1ULL << 32) / (F_CPU / 1000000));
  graphics.setPrintPos(2, 32);
  graphics.printf("POLL :%3uus (%uus)", (isr_us * 100) /  OC_CORE_TIMER_RATE, OC_UI_TIMER_RATE);
#ifdef OC_UI_DEBUG
  graphics.setPrintPos(2, 42);
  graphics.printf("UI # : %u", DEBUG::UI_event_count);
  graphics.setPrintPos(2, 52);
  graphics.printf("UI ! : %u", DEBUG::UI_queue_overflow);
#endif
}

static void debug_menu_gfx() {
  graphics.drawFrame(0, 0, 128, 64);

  graphics.setPrintPos(0, 12);
  graphics.print("W");
}

static void debug_menu_adc() {
    graphics.setPrintPos(2, 12);
    graphics.print("CV1: "); graphics.pretty_print(ADC::value<ADC_CHANNEL_1>(), 6);

    graphics.setPrintPos(2, 22);
    graphics.print("CV2: "); graphics.pretty_print(ADC::value<ADC_CHANNEL_2>(), 6);

    graphics.setPrintPos(2, 32);
    graphics.print("CV3: "); graphics.pretty_print(ADC::value<ADC_CHANNEL_3>(), 6);

    graphics.setPrintPos(2, 42);
    graphics.print("CV4: "); graphics.pretty_print(ADC::value<ADC_CHANNEL_4>(), 6);

//      graphics.setPrintPos(2, 42);
//      graphics.print((long)ADC::busy_waits());
//      graphics.setPrintPos(2, 42); graphics.print(ADC::fail_flag0());
//      graphics.setPrintPos(2, 52); graphics.print(ADC::fail_flag1());
}

struct DebugMenu {
  const char *title;
  void (*display_fn)();
};

static const DebugMenu debug_menus[] = {
  { " CORE", debug_menu_core },
  { " GFX", debug_menu_gfx },
  { " ADC", debug_menu_adc },
  { " POLYLFO", POLYLFO_debug },
  { " LORENZ", LORENZ_debug },
  { nullptr, nullptr }
};

void Ui::DebugStats() {
  SERIAL_PRINTLN("DEBUG/STATS MENU");

  const DebugMenu *current_menu = &debug_menus[0];
  bool exit_loop = false;
  while (!exit_loop) {

    GRAPHICS_BEGIN_FRAME(false);
      graphics.setPrintPos(2, 2);
      graphics.printf("%d/%u", (int)(current_menu - &debug_menus[0]) + 1, ARRAY_SIZE(debug_menus) - 1);
      graphics.print(current_menu->title);
      current_menu->display_fn();
    GRAPHICS_END_FRAME();

    while (event_queue_.available()) {
      UI::Event event = event_queue_.PullEvent();
      if (CONTROL_BUTTON_R == event.control) {
        exit_loop = true;
      } else if (CONTROL_BUTTON_L == event.control) {
        ++current_menu;
        if (!current_menu->title || !current_menu->display_fn)
          current_menu = &debug_menus[0];
      }
    }
  }

  event_queue_.Flush();
  event_queue_.Poke();
}

}; // namespace OC

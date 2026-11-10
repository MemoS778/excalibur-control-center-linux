#ifndef UI_HH
#define UI_HH

#include "../cpu/cpu.hh"
#include "../gpu/optimus.hh"
#include "../keyboard/keyboard.hh"
#include "locale.hh"
#include <cmath>
#include <fstream>
#include <gtk/gtk.h>
#include <pwd.h>
#include <string>
#include <unistd.h>

class ExcaliburGUI {
public:
  ExcaliburGUI(GtkApplication *app, KeyboardController &kbd,
               const Locale *locale, OptimusSwitch &gpu_switch,
               CpuController &cpu_ctrl);
  void build();

private:
  GtkApplication *app_;
  KeyboardController &kbd_;
  OptimusSwitch &switch_;
  CpuController &cpu_ctrl_;
  GtkWidget *window_;
  GtkWidget *drawing_area_;
  GtkWidget *core_dropdown_ = nullptr;
  GtkWidget *cpu_current_label_ = nullptr;
  GtkWidget *min_scale_ = nullptr;
  GtkWidget *max_scale_ = nullptr;
  GtkWidget *turbo_switch_ = nullptr;
  guint cpu_timer_id_ = 0;

  const Locale *lang;

  bool preview_ = false, led_on_ = true;
  double brightness_ = .5, hue_ = .07, sat_ = .85;
  GtkWidget *status_ = nullptr, *zone_all_ = nullptr;
  void apply_lighting();
  float curr_r = 1.0, curr_g = .5, curr_b = .15;

  ZoneID selected_zone = ZoneID::ALL;
  KBPattern selected_pattern = KBPattern::SOLID;

  static void on_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                      gpointer data);
};

std::string get_lang_path();
void set_lang_en();
void set_lang_tr();

#endif

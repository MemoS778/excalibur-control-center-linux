#ifndef UI_HH
#define UI_HH

#include <gtk/gtk.h>
#include <string>
#include <cmath>
#include <fstream>
#include <pwd.h>
#include <unistd.h>
#include "../keyboard/keyboard.hh"
#include "../gpu/optimus.hh"
#include "../cpu/cpu.hh"
#include "locale.hh"

class ExcaliburGUI {
public:
  ExcaliburGUI(GtkApplication* app, KeyboardController& kbd, const Locale* locale, OptimusSwitch& gpu_switch, CpuController& cpu_ctrl);
  void build(); 
  
private:
  GtkApplication *app_;
  KeyboardController& kbd_;
  OptimusSwitch& switch_;
  CpuController& cpu_ctrl_;
  GtkWidget *window_;
  GtkWidget *drawing_area_;
  GtkWidget* core_dropdown_ = nullptr;
  GtkWidget* cpu_current_label_ = nullptr;
  GtkWidget* min_scale_ = nullptr;
  GtkWidget* max_scale_ = nullptr;
  GtkWidget* turbo_switch_ = nullptr;
  guint cpu_timer_id_ = 0;

  const Locale* lang;

  float curr_r = 0.0, curr_g = 1.0, curr_b = 0.0;

  ZoneID selected_zone = ZoneID::ALL;
  KBPattern selected_pattern = KBPattern::SOLID;

  static void on_draw(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data);
};

std::string get_lang_path();  
void set_lang_en();  
void set_lang_tr();

#endif

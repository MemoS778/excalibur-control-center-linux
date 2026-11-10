#ifndef UI_HH
#define UI_HH

#include "../keyboard/keyboard.hh"
#include <cmath>
#include <fstream>
#include <gtk/gtk.h>
#include <pwd.h>
#include <string>
#include <unistd.h>

class ExcaliburGUI {
public:
  ExcaliburGUI(GtkApplication *app, KeyboardController &kbd);
  void build();

private:
  GtkApplication *app_;
  KeyboardController &kbd_;
  GtkWidget *window_;
  GtkWidget *drawing_area_;
  GtkWidget *profile_slot_ = nullptr, *power_ = nullptr;
  GtkWidget *wheel_ = nullptr, *brightness_scale_ = nullptr;
  GtkWidget *zone_buttons_[4] = {}, *pattern_buttons_[4] = {};
  bool loading_profile_ = false;
  void profile_action(bool save);
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

#endif

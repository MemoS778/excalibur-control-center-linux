#include "ui.hh"
#include <algorithm>

ExcaliburGUI::ExcaliburGUI(GtkApplication *app, KeyboardController &kbd,
                           const Locale *locale, OptimusSwitch &gpu_switch,
                           CpuController &cpu_ctrl)
    : app_(app), kbd_(kbd), switch_(gpu_switch), cpu_ctrl_(cpu_ctrl),
      lang(locale) {
  preview_ = std::getenv("ECC_PREVIEW") != nullptr;
}

static GtkWidget *create_ui_button(const char *label,
                                   const char *css_class = nullptr) {
  auto *b = gtk_button_new_with_label(label);
  if (css_class)
    gtk_widget_add_css_class(b, css_class);
  return b;
}
static void hsv(double h, double s, double &r, double &g, double &b) {
  h = h - floor(h);
  double x = h * 6;
  int i = int(x);
  double f = x - i, p = 1 - s, q = 1 - s * f, t = 1 - s * (1 - f);
  switch (i % 6) {
  case 0:
    r = 1;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = 1;
    b = p;
    break;
  case 2:
    r = p;
    g = 1;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = 1;
    break;
  case 4:
    r = t;
    g = p;
    b = 1;
    break;
  default:
    r = 1;
    g = p;
    b = q;
  }
}
void ExcaliburGUI::apply_lighting() {
  if (!preview_) {
    kbd_.set_color(curr_r * 255, curr_g * 255, curr_b * 255,
                   led_on_ ? brightness_ * 255 : 0, selected_zone);
    if (led_on_ && selected_pattern != KBPattern::SOLID)
      kbd_.set_mode(selected_pattern);
  }
  gtk_label_set_text(GTK_LABEL(status_),
                     preview_ ? "Önizleme · donanım değiştirilmiyor"
                              : "Aydınlatma ayarları uygulandı");
  gtk_widget_queue_draw(drawing_area_);
}
void ExcaliburGUI::build() {
  window_ = gtk_application_window_new(app_);
  gtk_window_set_title(GTK_WINDOW(window_), "Excalibur Control Center");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1040, 600);
  auto *css = gtk_css_provider_new();
  gtk_css_provider_load_from_string(css, R"CSS(
 window {background:#101113;color:#d6d6d8;font-family:sans-serif;font-size:12px;}
 .brand {background:linear-gradient(110deg,#292c31 20%,#472a20 75%,#a54316);padding:16px 22px;letter-spacing:2px;font-weight:800;font-size:16px;color:#fff;}
 .sidebar {background:#17191c;padding:20px 10px;border-right:1px solid #343639;}
 button {background:#25272b;color:#bcbec2;border:1px solid #35373b;border-radius:4px;box-shadow:none;padding:10px;}
 button:hover {background:#34302b;color:#fff;border-color:#b76028;}
 button:checked,.accent {background:#3a291f;color:#ff9b49;border-color:#c96a2b;}
 .nav {padding:28px 8px;}
 .panel {background:#222426;padding:18px;border-left:1px solid #383a3c;}
 .zones {padding:16px;}
 .footer {background:#202224;padding:14px;border-top:1px solid #37393b;}
 .muted {color:#93979e;font-size:11px;}
 .large {font-size:25px;font-weight:700;color:#f4f4f5;}
 scale highlight {background:#ec873c;} scale slider {background:#ffb775;min-width:12px;min-height:12px;}
 switch:checked {background:#d77932;}
 popover contents {background:#25272b;color:#ddd;}
 )CSS");
  gtk_style_context_add_provider_for_display(
      gtk_widget_get_display(window_), GTK_STYLE_PROVIDER(css),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
  auto *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(window_), root);
  auto *brand = gtk_label_new("◈  CONTROL CENTER                               "
                              "       EXCALIBUR / LINUX");
  gtk_widget_add_css_class(brand, "brand");
  gtk_label_set_xalign(GTK_LABEL(brand), 0);
  gtk_box_append(GTK_BOX(root), brand);
  auto *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_vexpand(body, TRUE);
  gtk_box_append(GTK_BOX(root), body);
  auto *nav = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_add_css_class(nav, "sidebar");
  gtk_widget_set_size_request(nav, 125, -1);
  gtk_box_append(GTK_BOX(body), nav);
  auto *stack = gtk_stack_new();
  gtk_widget_set_hexpand(stack, TRUE);
  gtk_widget_set_vexpand(stack, TRUE);
  auto *system_nav = gtk_toggle_button_new_with_label("◉\n\nSistem Modu");
  auto *led_nav = gtk_toggle_button_new_with_label("☼\n\nLED Kontrol");
  gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(system_nav),
                              GTK_TOGGLE_BUTTON(led_nav));
  for (auto *b : {system_nav, led_nav}) {
    gtk_widget_add_css_class(b, "nav");
    gtk_box_append(GTK_BOX(nav), b);
    g_object_set_data(G_OBJECT(b), "stack", stack);
  }
  g_object_set_data(G_OBJECT(system_nav), "page", (gpointer) "system");
  g_object_set_data(G_OBJECT(led_nav), "page", (gpointer) "led");
  for (auto *b : {system_nav, led_nav})
    g_signal_connect(
        b, "toggled",
        reinterpret_cast<GCallback>(+[](GtkToggleButton *b, gpointer) {
          if (gtk_toggle_button_get_active(b))
            gtk_stack_set_visible_child_name(
                GTK_STACK(g_object_get_data(G_OBJECT(b), "stack")),
                static_cast<const char *>(
                    g_object_get_data(G_OBJECT(b), "page")));
        }),
        nullptr);
  gtk_box_append(GTK_BOX(body), stack);
  auto *led = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_stack_add_named(GTK_STACK(stack), led, "led");
  auto *center = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(center, TRUE);
  gtk_box_append(GTK_BOX(led), center);
  auto *zones = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_add_css_class(zones, "zones");
  gtk_box_append(GTK_BOX(center), zones);
  const char *zn[] = {"Bölge A", "Bölge B", "Bölge C", "Hepsi"};
  ZoneID zi[] = {ZoneID::LEFT, ZoneID::MID, ZoneID::RIGHT, ZoneID::ALL};
  GtkToggleButton *first = nullptr;
  for (int i = 0; i < 4; i++) {
    auto *b = gtk_toggle_button_new_with_label(zn[i]);
    if (first)
      gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(b), first);
    else
      first = GTK_TOGGLE_BUTTON(b);
    g_object_set_data(G_OBJECT(b), "zone", GINT_TO_POINTER(int(zi[i])));
    g_signal_connect(
        b, "toggled",
        reinterpret_cast<GCallback>(+[](GtkToggleButton *b, gpointer d) {
          if (gtk_toggle_button_get_active(b)) {
            auto *s = static_cast<ExcaliburGUI *>(d);
            s->selected_zone =
                ZoneID(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "zone")));
            gtk_widget_queue_draw(s->drawing_area_);
          }
        }),
        this);
    gtk_box_append(GTK_BOX(zones), b);
    if (i == 3)
      zone_all_ = b;
  }
  drawing_area_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area_, 500, 310);
  gtk_widget_set_vexpand(drawing_area_, TRUE);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area_), on_draw, this,
                                 nullptr);
  gtk_box_append(GTK_BOX(center), drawing_area_);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(zone_all_), TRUE);
  auto *foot = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_add_css_class(foot, "footer");
  gtk_box_append(GTK_BOX(center), foot);
  gtk_box_append(GTK_BOX(foot),
                 gtk_label_new("EXCALIBUR G870  /  RGB AYDINLATMA"));
  status_ =
      gtk_label_new(preview_ ? "Önizleme · donanım değiştirilmiyor"
                             : "Rengi ve bölgeyi seçip Uygula düğmesine basın");
  gtk_widget_add_css_class(status_, "muted");
  gtk_box_append(GTK_BOX(foot), status_);
  auto *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_size_request(right, 260, -1);
  gtk_widget_add_css_class(right, "panel");
  gtk_box_append(GTK_BOX(led), right);
  auto *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append(GTK_BOX(right), row);
  gtk_box_append(GTK_BOX(row), gtk_label_new("LED açık / kapalı"));
  auto *power = gtk_switch_new();
  gtk_switch_set_active(GTK_SWITCH(power), TRUE);
  gtk_box_append(GTK_BOX(row), power);
  g_signal_connect(
      power, "notify::active",
      reinterpret_cast<GCallback>(+[](GObject *b, GParamSpec *, gpointer d) {
        auto *s = static_cast<ExcaliburGUI *>(d);
        s->led_on_ = gtk_switch_get_active(GTK_SWITCH(b));
        s->apply_lighting();
      }),
      this);
  auto *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_box_append(GTK_BOX(right), grid);
  const char *pn[] = {"☼  Statik", "≈  Nefes", "✳  Renk döngüsü",
                      "▤  Ambilight"};
  KBPattern pi[] = {KBPattern::SOLID, KBPattern::BREATHE, KBPattern::CYCLE,
                    KBPattern::AMBILIGHT};
  first = nullptr;
  for (int i = 0; i < 4; i++) {
    auto *b = gtk_toggle_button_new_with_label(pn[i]);
    gtk_widget_set_hexpand(b, TRUE);
    if (first)
      gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(b), first);
    else {
      first = GTK_TOGGLE_BUTTON(b);
      gtk_toggle_button_set_active(first, TRUE);
    }
    g_object_set_data(G_OBJECT(b), "pattern", GINT_TO_POINTER(int(pi[i])));
    g_signal_connect(
        b, "toggled",
        reinterpret_cast<GCallback>(+[](GtkToggleButton *b, gpointer d) {
          if (gtk_toggle_button_get_active(b)) {
            auto *s = static_cast<ExcaliburGUI *>(d);
            s->selected_pattern = KBPattern(
                GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "pattern")));
          }
        }),
        this);
    gtk_grid_attach(GTK_GRID(grid), b, i % 2, i / 2, 1, 1);
  }
  auto *wheel = gtk_drawing_area_new();
  gtk_widget_set_size_request(wheel, 230, 230);
  gtk_widget_set_halign(wheel, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(right), wheel);
  gtk_drawing_area_set_draw_func(
      GTK_DRAWING_AREA(wheel),
      +[](GtkDrawingArea *, cairo_t *cr, int w, int h, gpointer d) {
        auto *s = static_cast<ExcaliburGUI *>(d);
        double radius = std::min(w, h) / 2. - 7;
        for (int y = 0; y < h; y += 2)
          for (int x = 0; x < w; x += 2) {
            double dx = x - w / 2., dy = y - h / 2.,
                   sat = hypot(dx, dy) / radius;
            if (sat <= 1) {
              double r, g, b;
              hsv(atan2(dy, dx) / (2 * M_PI), sat, r, g, b);
              cairo_set_source_rgb(cr, r, g, b);
              cairo_rectangle(cr, x, y, 2, 2);
              cairo_fill(cr);
            }
          }
        cairo_set_source_rgb(cr, .8, .8, .8);
        cairo_set_line_width(cr, 2);
        cairo_arc(cr, w / 2., h / 2., radius + 3, 0, 2 * M_PI);
        cairo_stroke(cr);
        cairo_arc(cr, w / 2. + cos(s->hue_ * 2 * M_PI) * s->sat_ * radius,
                  h / 2. + sin(s->hue_ * 2 * M_PI) * s->sat_ * radius, 5, 0,
                  2 * M_PI);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_stroke_preserve(cr);
        cairo_set_line_width(cr, 1);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_stroke(cr);
      },
      this, nullptr);
  auto *click = gtk_gesture_click_new();
  g_signal_connect(
      click, "pressed",
      reinterpret_cast<GCallback>(+[](GtkGestureClick *g, int, double x,
                                      double y, gpointer d) {
        auto *s = static_cast<ExcaliburGUI *>(d);
        auto *w = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(g));
        double dx = x - gtk_widget_get_width(w) / 2.,
               dy = y - gtk_widget_get_height(w) / 2.;
        double radius =
            std::min(gtk_widget_get_width(w), gtk_widget_get_height(w)) / 2. -
            7;
        if (hypot(dx, dy) > radius)
          return;
        s->hue_ = atan2(dy, dx) / (2 * M_PI);
        s->sat_ = hypot(dx, dy) / radius;
        double r, gg, b;
        hsv(s->hue_, s->sat_, r, gg, b);
        s->curr_r = r;
        s->curr_g = gg;
        s->curr_b = b;
        gtk_widget_queue_draw(w);
        gtk_widget_queue_draw(s->drawing_area_);
      }),
      this);
  gtk_widget_add_controller(wheel, GTK_EVENT_CONTROLLER(click));
  gtk_box_append(GTK_BOX(right), gtk_label_new("Parlaklık"));
  auto *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value(GTK_RANGE(scale), 50);
  gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
  gtk_box_append(GTK_BOX(right), scale);
  g_signal_connect(scale, "value-changed",
                   reinterpret_cast<GCallback>(+[](GtkRange *r, gpointer d) {
                     auto *s = static_cast<ExcaliburGUI *>(d);
                     s->brightness_ = gtk_range_get_value(r) / 100.;
                     gtk_widget_queue_draw(s->drawing_area_);
                   }),
                   this);
  auto *apply = create_ui_button("Uygula", "accent");
  gtk_box_append(GTK_BOX(right), apply);
  g_signal_connect(apply, "clicked",
                   reinterpret_cast<GCallback>(+[](GtkButton *, gpointer d) {
                     static_cast<ExcaliburGUI *>(d)->apply_lighting();
                   }),
                   this);
  auto *system_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
  gtk_widget_add_css_class(system_box, "panel");
  gtk_stack_add_named(GTK_STACK(stack), system_box, "system");
  gtk_box_append(GTK_BOX(system_box), gtk_label_new("SİSTEM MODU"));
  GtkWidget *cpu_btn = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(cpu_btn), "CPU");
  GtkWidget *popover = gtk_popover_new();
  GtkWidget *cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top(cpu_box, 10);
  gtk_widget_set_margin_bottom(cpu_box, 10);
  gtk_widget_set_margin_start(cpu_box, 20);
  gtk_widget_set_margin_end(cpu_box, 10);
  gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_LEFT);
  gtk_widget_set_size_request(cpu_box, 200, -1);

  uint8_t core_count = cpu_ctrl_.get_core_count();

  GtkStringList *core_list = gtk_string_list_new(nullptr);
  gtk_string_list_append(core_list, "All");

  for (int i = 0; i < core_count; ++i) {
    gtk_string_list_append(core_list, ("CPU" + std::to_string(i)).c_str());
  }

  core_dropdown_ = gtk_drop_down_new(G_LIST_MODEL(core_list), nullptr);
  gtk_box_append(GTK_BOX(cpu_box), core_dropdown_);

  CpuLimits limits = cpu_ctrl_.get_cpu_limits(0);
  gtk_box_append(GTK_BOX(cpu_box), gtk_label_new("Min Freq (MHz)"));
  min_scale_ =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, limits.hw_min / 1000,
                               limits.hw_max / 1000, 100);
  gtk_range_set_value(GTK_RANGE(min_scale_), limits.hw_min / 1000);
  gtk_scale_set_draw_value(GTK_SCALE(min_scale_), TRUE);
  gtk_box_append(GTK_BOX(cpu_box), min_scale_);

  gtk_box_append(GTK_BOX(cpu_box), gtk_label_new("Max Freq (MHz)"));
  max_scale_ =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, limits.hw_min / 1000,
                               limits.hw_max / 1000, 100);
  gtk_range_set_value(GTK_RANGE(max_scale_), limits.hw_max / 1000);
  gtk_scale_set_draw_value(GTK_SCALE(max_scale_), TRUE);
  gtk_box_append(GTK_BOX(cpu_box), max_scale_);

  GtkWidget *turbo_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append(GTK_BOX(turbo_row), gtk_label_new("Turbo Boost"));
  turbo_switch_ = gtk_switch_new();
  gtk_widget_set_halign(turbo_switch_, GTK_ALIGN_END);
  gtk_widget_set_hexpand(turbo_switch_, TRUE);
  gtk_switch_set_active(GTK_SWITCH(turbo_switch_), cpu_ctrl_.get_intel_turbo());
  gtk_box_append(GTK_BOX(turbo_row), turbo_switch_);
  gtk_box_append(GTK_BOX(cpu_box), turbo_row);

  GtkWidget *cpu_apply_btn = create_ui_button("Apply", "suggested-action");
  g_signal_connect(
      cpu_apply_btn, "clicked",
      reinterpret_cast<GCallback>(+[](GtkButton *, gpointer data) {
        auto *self = static_cast<ExcaliburGUI *>(data);
        if (self->preview_)
          return;
        guint sel =
            gtk_drop_down_get_selected(GTK_DROP_DOWN(self->core_dropdown_));
        int core = (sel == 0) ? -1 : (int)(sel - 1);

        uint32_t min_mhz =
            (uint32_t)gtk_range_get_value(GTK_RANGE(self->min_scale_));
        uint32_t max_mhz =
            (uint32_t)gtk_range_get_value(GTK_RANGE(self->max_scale_));

        self->cpu_ctrl_.set_cpu_freq(max_mhz * 1000, core, false);
        self->cpu_ctrl_.set_cpu_freq(min_mhz * 1000, core, true);
        self->cpu_ctrl_.set_intel_turbo(
            gtk_switch_get_active(GTK_SWITCH(self->turbo_switch_)));
      }),
      this);
  gtk_box_append(GTK_BOX(cpu_box), cpu_apply_btn);

  gtk_popover_set_child(GTK_POPOVER(popover), cpu_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(cpu_btn), popover);
  gtk_box_append(GTK_BOX(system_box), cpu_btn);

  gtk_box_append(
      GTK_BOX(system_box),
      gtk_label_new(
          "Ekran kartı seçimi sistem yapılandırmasını "
          "değiştirir.\nDeğişiklikten sonra yeniden başlatma gerekebilir."));
  for (int i = 0; i < 2; i++) {
    auto *b = create_ui_button(i ? "NVIDIA ekran kartına geç"
                                 : "Intel ekran kartına geç");
    g_object_set_data(G_OBJECT(b), "gpu", GINT_TO_POINTER(i));
    gtk_box_append(GTK_BOX(system_box), b);
    g_signal_connect(
        b, "clicked",
        reinterpret_cast<GCallback>(+[](GtkButton *b, gpointer d) {
          auto *s = static_cast<ExcaliburGUI *>(d);
          if (s->preview_)
            return;
          int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "gpu"));
          g_object_set_data(G_OBJECT(s->window_), "gpu", GINT_TO_POINTER(id));
          auto *dialog = gtk_alert_dialog_new("Ekran kartı değiştirilsin mi?");
          const char *buttons[] = {"Vazgeç", "Değiştir", nullptr};
          gtk_alert_dialog_set_buttons(dialog, buttons);
          gtk_alert_dialog_set_cancel_button(dialog, 0);
          gtk_alert_dialog_set_default_button(dialog, 0);
          gtk_alert_dialog_set_detail(
              dialog, "Sistem ekran kartı ayarları değiştirilecek. Yeniden "
                      "başlatma gerekebilir.");
          gtk_alert_dialog_choose(
              dialog, GTK_WINDOW(s->window_), nullptr,
              +[](GObject *o, GAsyncResult *r, gpointer d) {
                auto *s = static_cast<ExcaliburGUI *>(d);
                GError *e = nullptr;
                int choice =
                    gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(o), r, &e);
                if (e) {
                  g_error_free(e);
                  return;
                }
                if (choice == 1)
                  s->switch_.switch_gpus(GPOINTER_TO_INT(
                      g_object_get_data(G_OBJECT(s->window_), "gpu")));
              },
              s);
          g_object_unref(dialog);
        }),
        this);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(led_nav), TRUE);
  gtk_window_present(GTK_WINDOW(window_));
}

void ExcaliburGUI::on_draw(GtkDrawingArea *, cairo_t *cr, int width, int height,
                           gpointer data) {
  auto *s = static_cast<ExcaliburGUI *>(data);
  cairo_set_source_rgb(cr, .045, .05, .055);
  cairo_paint(cr);
  double k = std::min(width / 550., height / 340.);
  cairo_translate(cr, (width - 520 * k) / 2, (height - 310 * k) / 2);
  cairo_scale(cr, k, k);
  auto *grad = cairo_pattern_create_linear(0, 0, 0, 310);
  cairo_pattern_add_color_stop_rgb(grad, 0, .18, .19, .20);
  cairo_pattern_add_color_stop_rgb(grad, .5, .08, .09, .10);
  cairo_pattern_add_color_stop_rgb(grad, 1, .16, .17, .18);
  cairo_rectangle(cr, 0, 0, 520, 300);
  cairo_set_source(cr, grad);
  cairo_fill(cr);
  cairo_pattern_destroy(grad);
  cairo_set_source_rgb(cr, .7, .7, .7);
  cairo_move_to(cr, 20, 25);
  cairo_set_font_size(cr, 11);
  cairo_show_text(cr, "E X C A L I B U R");
  const char *rows[] = {"1234567890-=", "QWERTYUIOP[]", "ASDFGHJKL;'",
                        "ZXCVBNM,./"};
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 15; x++) {
      double r, g, b;
      hsv(x / 20., .85, r, g, b);
      bool selected = s->selected_zone == ZoneID::ALL ||
                      (x < 5 && s->selected_zone == ZoneID::LEFT) ||
                      (x >= 5 && x < 10 && s->selected_zone == ZoneID::MID) ||
                      (x >= 10 && s->selected_zone == ZoneID::RIGHT);
      if (selected) {
        r = s->curr_r;
        g = s->curr_g;
        b = s->curr_b;
      }
      double v = s->led_on_ ? (.25 + .75 * s->brightness_) : .08;
      cairo_set_source_rgb(cr, r * v, g * v, b * v);
      cairo_set_line_width(cr, 1.2);
      cairo_rectangle(cr, 16 + x * 33, 48 + y * 27, 28, 21);
      cairo_stroke(cr);
      cairo_set_font_size(cr, 9);
      cairo_move_to(cr, 24 + x * 33, 62 + y * 27);
      char text[2] = {
          y < 4 && x < int(std::string(rows[y]).size()) ? rows[y][x] : ' ', 0};
      cairo_show_text(cr, text);
    }
  cairo_set_source_rgb(cr, .35, .37, .39);
  cairo_rectangle(cr, 170, 210, 175, 65);
  cairo_stroke(cr);
  cairo_set_source_rgb(cr, .45, .47, .49);
  cairo_set_font_size(cr, 10);
  cairo_move_to(cr, 135, 325);
  cairo_show_text(cr, "AYDINLATMA ONIZLEMESI / 3 BOLGE");
}
std::string get_lang_path() {
  const char *sudo_user = std::getenv("SUDO_USER");
  std::string path;

  if (sudo_user) {
    struct passwd *pw = getpwnam(sudo_user);
    if (pw)
      path = pw->pw_dir;
  } else {
    const char *home = std::getenv("HOME");
    if (home)
      path = home;
  }

  if (path.empty())
    return "/tmp/.ecc_lang";
  return path + "/.cache/.ecc_lang";
}

void set_lang_en() { std::remove(get_lang_path().c_str()); }

void set_lang_tr() {
  std::ofstream file(get_lang_path());
  file.close();
}

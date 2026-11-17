#include "ui.hh"
#include "profile.hh"
#include <algorithm>
#include <cstdlib>
#include <sstream>

ExcaliburGUI::ExcaliburGUI(GtkApplication *app, KeyboardController &kbd)
    : app_(app), kbd_(kbd), worker_([&kbd](LightingRequest r) {
        const auto start = std::chrono::steady_clock::now();
        bool ok = kbd.apply(r.r, r.g, r.b, r.brightness, r.zone, r.pattern);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        g_print("[RGB] backend=ec pattern=0x%02x result=%s elapsed_ms=%ld; "
                "physical LED response unverified\n",
                unsigned(r.pattern), ok ? "written" : "failed", long(elapsed));
        return ok ? std::string{} : kbd.last_error();
      }) {
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
ExcaliburGUI::~ExcaliburGUI() {
  if (pending_apply_)
    g_source_remove(pending_apply_);
}

void ExcaliburGUI::apply_lighting() {
  g_print("[RGB] preview=%d zone=0x%02x pattern=0x%02x rgb=%u,%u,%u "
          "brightness=%u enabled=%d\n",
          preview_, unsigned(selected_zone), unsigned(selected_pattern),
          unsigned(curr_r * 255), unsigned(curr_g * 255),
          unsigned(curr_b * 255), unsigned(brightness_ * 255), led_on_);
  if (preview_) {
    gtk_label_set_text(GTK_LABEL(status_), "Önizleme · donanım değiştirilmiyor");
    gtk_widget_queue_draw(drawing_area_);
    return;
  }
  worker_.submit({uint8_t(curr_r * 255), uint8_t(curr_g * 255),
                  uint8_t(curr_b * 255), uint8_t(led_on_ ? brightness_ * 255 : 0),
                  selected_zone, selected_pattern});
  gtk_label_set_text(GTK_LABEL(status_), "Ayarlar gönderiliyor…");
  if (!pending_apply_)
    pending_apply_ = g_timeout_add(20, +[](gpointer data) -> gboolean {
      auto *self = static_cast<ExcaliburGUI *>(data);
      if (self->window_ && self->poll_lighting()) return G_SOURCE_CONTINUE;
      self->pending_apply_ = 0;
      return G_SOURCE_REMOVE;
    }, this);
}

bool ExcaliburGUI::poll_lighting() {
  auto result = worker_.poll();
  if (result && !worker_.busy()) {
    const std::string message = result->empty()
        ? "Komutlar gönderildi; fiziksel klavye rengini kontrol edin."
        : "Klavyeye ayar gönderilemedi: " + *result;
    gtk_label_set_text(GTK_LABEL(status_), message.c_str());
    gtk_widget_queue_draw(drawing_area_);
  }
  return worker_.busy();
}
void ExcaliburGUI::build() {
  if (window_) {
    gtk_window_present(GTK_WINDOW(window_));
    return;
  }
  window_ = gtk_application_window_new(app_);
  g_object_add_weak_pointer(G_OBJECT(window_),
                            reinterpret_cast<gpointer *>(&window_));
  gtk_window_set_title(GTK_WINDOW(window_), "Excalibur RGB Control");
  gtk_window_set_default_size(GTK_WINDOW(window_), 940, 640);
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
 scale highlight {background:#ec873c;} scale slider {background:#ffb775;min-width:20px;min-height:20px;padding:0;border:0;box-shadow:none;}
 switch:checked {background:#d77932;}
 popover contents {background:#25272b;color:#ddd;}
 )CSS");
  gtk_style_context_add_provider_for_display(
      gtk_widget_get_display(window_), GTK_STYLE_PROVIDER(css),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
  auto *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(window_), root);
  auto *brand = gtk_label_new("◈  RGB CONTROL                               "
                              "       EXCALIBUR / LINUX");
  gtk_widget_add_css_class(brand, "brand");
  gtk_label_set_xalign(GTK_LABEL(brand), 0);
  gtk_box_append(GTK_BOX(root), brand);
  auto *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_vexpand(body, TRUE);
  gtk_box_append(GTK_BOX(root), body);
  auto *led = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(led, TRUE);
  gtk_box_append(GTK_BOX(body), led);
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
    zone_buttons_[i] = b;
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
                 gtk_label_new("EXCALIBUR  /  KLAVYE AYDINLATMASI"));
  status_ =
      gtk_label_new(preview_ ? "Önizleme · donanım değiştirilmiyor"
                             : "Rengi ve bölgeyi seçip Uygula düğmesine basın");
  gtk_widget_add_css_class(status_, "muted");
  gtk_label_set_wrap(GTK_LABEL(status_), TRUE);
  gtk_label_set_max_width_chars(GTK_LABEL(status_), 60);
  gtk_box_append(GTK_BOX(foot), status_);
  auto *profiles = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(foot), profiles);
  const char *slots[] = {"Profil 1", "Profil 2", "Profil 3", nullptr};
  profile_slot_ = gtk_drop_down_new_from_strings(slots);
  gtk_box_append(GTK_BOX(profiles), profile_slot_);
  auto *save = create_ui_button("Seçili ayarı kaydet");
  auto *load = create_ui_button("Profili yükle");
  gtk_box_append(GTK_BOX(profiles), save);
  gtk_box_append(GTK_BOX(profiles), load);
  g_signal_connect(save, "clicked",
                   reinterpret_cast<GCallback>(+[](GtkButton *, gpointer d) {
                     static_cast<ExcaliburGUI *>(d)->profile_action(true);
                   }),
                   this);
  g_signal_connect(load, "clicked",
                   reinterpret_cast<GCallback>(+[](GtkButton *, gpointer d) {
                     static_cast<ExcaliburGUI *>(d)->profile_action(false);
                   }),
                   this);
  auto *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_size_request(right, 260, -1);
  gtk_widget_add_css_class(right, "panel");
  gtk_box_append(GTK_BOX(led), right);
  auto *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append(GTK_BOX(right), row);
  gtk_box_append(GTK_BOX(row), gtk_label_new("LED açık / kapalı"));
  auto *power = gtk_switch_new();
  power_ = power;
  gtk_switch_set_active(GTK_SWITCH(power), TRUE);
  gtk_box_append(GTK_BOX(row), power);
  g_signal_connect(
      power, "notify::active",
      reinterpret_cast<GCallback>(+[](GObject *b, GParamSpec *, gpointer d) {
        auto *s = static_cast<ExcaliburGUI *>(d);
        s->led_on_ = gtk_switch_get_active(GTK_SWITCH(b));
        if (!s->loading_profile_)
          s->apply_lighting();
      }),
      this);
  auto *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_box_append(GTK_BOX(right), grid);
  const char *pn[] = {"☼  Statik", "≈  Nefes", "✳  Renk döngüsü",
                      "▤  Efekt 0x07 (deneysel)"};
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
    if (i == 3)
      gtk_widget_set_tooltip_text(
          b, "Bu EC komutunun G870/CQ171 üzerinde animasyon başlattığı "
             "doğrulanmadı; yalnızca seçili renk görünebilir. Ekranı takip etmez.");
    if (i == 2)
      gtk_widget_set_tooltip_text(b, "Klavyenin otomatik renk döngüsü. "
                                     "Seçtiğiniz sabit rengi kullanmayabilir.");
    pattern_buttons_[i] = b;
    gtk_grid_attach(GTK_GRID(grid), b, i % 2, i / 2, 1, 1);
  }
  auto *wheel = gtk_drawing_area_new();
  wheel_ = wheel;
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
  brightness_scale_ = scale;
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

// File access runs as the invoking user, including when hardware access needs
// sudo. This also keeps profiles user-owned and prevents root writes via user
// paths.
namespace {
class ProfileUser {
  uid_t old_uid = geteuid();
  gid_t old_gid = getegid();

public:
  bool ready = false;
  std::string directory;
  ProfileUser() {
    uid_t uid = getuid();
    if (old_uid == 0) {
      const char *value = std::getenv("SUDO_UID");
      if (value) {
        char *end = nullptr;
        unsigned long n = std::strtoul(value, &end, 10);
        if (!*value || *end || n > UINT32_MAX)
          return;
        uid = static_cast<uid_t>(n);
      }
    }
    passwd *pw = getpwuid(uid);
    if (!pw)
      return;
    directory = std::string(pw->pw_dir) + "/.config/excalibur-rgb-control";
    if (old_uid == 0) {
      if (setegid(pw->pw_gid) != 0)
        return;
      if (seteuid(uid) != 0) {
        setegid(old_gid);
        return;
      }
    }
    ready = true;
  }
  ~ProfileUser() {
    if (geteuid() != old_uid && seteuid(old_uid) != 0)
      std::abort();
    if (getegid() != old_gid && setegid(old_gid) != 0)
      std::abort();
  }
};
} // namespace

void ExcaliburGUI::profile_action(bool save) {
  // ProfileUser changes process credentials; never overlap this with EC I/O.
  if (worker_.busy()) {
    gtk_label_set_text(GTK_LABEL(status_), "Profil işlemi için gönderimin bitmesini bekleyin.");
    return;
  }
  LightingProfile p;
  std::string message;
  bool loaded = false;
  {
    ProfileUser user;
    if (!user.ready)
      message = "Profil dizinine erişilemedi.";
    else {
      auto path =
          user.directory + "/profile-" +
          std::to_string(
              gtk_drop_down_get_selected(GTK_DROP_DOWN(profile_slot_)) + 1) +
          ".conf";
      if (save) {
        p = {hue_,
             sat_,
             brightness_,
             int(selected_zone),
             int(selected_pattern),
             led_on_ ? 1 : 0};
        std::ostringstream out;
        write_profile(out, p);
        auto content = out.str();
        GError *error = nullptr;
        if (g_mkdir_with_parents(user.directory.c_str(), 0700) == 0 &&
            g_file_set_contents_full(
                path.c_str(), content.c_str(), content.size(),
                G_FILE_SET_CONTENTS_CONSISTENT, 0600, &error))
          message =
              "Seçili bölgenin renk, efekt ve parlaklık ayarı kaydedildi.";
        else
          message = "Profil kaydedilemedi. Dizin izinlerini kontrol edin.";
        if (error)
          g_error_free(error);
      } else {
        std::ifstream in(path);
        loaded = read_profile(in, p);
        message =
            loaded ? "Profil yüklendi. Klavyeye göndermek için Uygula'ya basın."
                   : "Bu profil boş veya dosya geçersiz.";
      }
    }
  }
  if (loaded) {
    loading_profile_ = true;
    hue_ = p.hue;
    sat_ = p.saturation;
    brightness_ = p.brightness;
    led_on_ = p.enabled;
    selected_zone = ZoneID(p.zone);
    selected_pattern = KBPattern(p.pattern);
    double r, g, b;
    hsv(hue_, sat_, r, g, b);
    curr_r = r;
    curr_g = g;
    curr_b = b;
    const int zones[] = {0x50, 0x40, 0x30, 0};
    const int patterns[] = {1, 3, 6, 7};
    for (int i = 0; i < 4; i++) {
      if (p.zone == zones[i])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(zone_buttons_[i]), TRUE);
      if (p.pattern == patterns[i])
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pattern_buttons_[i]),
                                     TRUE);
    }
    gtk_switch_set_active(GTK_SWITCH(power_), led_on_);
    gtk_range_set_value(GTK_RANGE(brightness_scale_), brightness_ * 100);
    gtk_widget_queue_draw(wheel_);
    gtk_widget_queue_draw(drawing_area_);
    loading_profile_ = false;
  }
  gtk_label_set_text(GTK_LABEL(status_), message.c_str());
}

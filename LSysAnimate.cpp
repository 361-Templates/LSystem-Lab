#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "emp/math/Random.hpp"
#include "emp/web/Animate.hpp"
#include "emp/web/Button.hpp"
#include "emp/web/Div.hpp"
#include "emp/web/web.hpp"
 #include "emp/config/ArgManager.hpp"
 #include "emp/prefab/ConfigPanel.hpp"
 #include "emp/web/UrlParams.hpp"

EMP_BUILD_CONFIG(MyConfigType,
    VALUE(ITERATIONS, int, 5, "How many iterations should run?")
)

emp::web::Document doc("animation");
emp::web::Document settings("settings");
emp::web::Document buttons("buttons");
MyConfigType config;

// Structure holding the state of the rendering turtle
struct TurtleState {
  double x;
  double y;
  double angle;
  double line_width;
  int branch_depth;
};

class LSystemAnimator : public emp::web::Animate {
 private:
  double canvas_width = 600.0;
  double canvas_height = 600.0;

  // Web UI Components
  emp::web::Canvas canvas{canvas_width, canvas_height, "canvas"};
  emp::web::Div info_panel{"info_panel"};

  // L-System Grammar & Parameters
  std::string axiom;
  std::string current_string;
  std::unordered_map<char, std::string> rules;

  int current_depth = 0;
  int max_depth = 1;
  double initial_step_length = 80.0;
  double step_length = 80.0;
  double step_decay = 0.55;
  double angle_rad = 0.0;

  // Animation timing
  int frame_counter = 0;
  int frames_per_expansion = 90;

 public:
  LSystemAnimator() {
    // Mount web UI elements directly
    doc << canvas;
    doc << GetToggleButton("Toggle");
    doc << GetStepButton("Step");
    doc << emp::web::Button([this]() { Reset(); }, "Reset", "reset_btn");
    doc << info_panel;

    // apply configuration query params and config files to config
    auto specs = emp::ArgManager::make_builtin_specs(&config);
    emp::ArgManager am(emp::web::GetUrlParams(), specs);
    
    am.UseCallbacks();
    if (am.HasUnused()) std::exit(EXIT_FAILURE);

    SetupGrammar();

    // setup configuration panel
    emp::prefab::ConfigPanel config_panel(config);
    //settings << config_panel; // Uncomment this when it's time to mess with the config

    Reset();
  }

  void SetupGrammar() {
    axiom = "A";
    rules['A'] = "A";

    angle_rad = 30.0 * (3.14159265358979323846 / 180.0);  // Turn angle in radians
    initial_step_length = 70.0;                       // Initial segment length in pixels
    step_decay = 0.5;                               // Multiplied to step_length each update to shrink steps
    max_depth = 1;                                  // Max number of iterations of the rules
  }

  void Reset() {
    current_string = axiom;
    current_depth = 0;
    step_length = initial_step_length;
    frame_counter = 0;

    DrawLSystem();
    UpdateUI();
  }

  // Handles string expansion loop
  void Expand() {
    if (current_depth >= max_depth) return;

    std::string next_string = "";
    for (char c : current_string) {
      if (rules.count(c)) {
        next_string += rules.at(c);
      } else {
        next_string += c;
      }
    }

    current_string = next_string;
    step_length *= step_decay;
    current_depth++;

    UpdateUI();
  }

  // Handles the turtle drawing and stack management
  void DrawLSystem() {
    canvas.Clear("#0b1d28");

    double cur_x = canvas_width / 2.0;
    double cur_y = canvas_height - 30.0;
    double cur_angle = -3.14159265358979323846 / 2.0;
    double cur_width = 3.5;
    int branch_depth = 0;

    std::vector<TurtleState> state_stack;

    for (char c : current_string) {
      if (c == 'F') {
        double next_x = cur_x + step_length * std::cos(cur_angle);
        double next_y = cur_y + step_length * std::sin(cur_angle);

        double line_width = 0.8;

        canvas.Line(cur_x, cur_y, next_x, next_y, "green", line_width);

        cur_x = next_x;
        cur_y = next_y;
      } else if (c == '+') {
        cur_angle += angle_rad;
      } else if (c == '-') {
        cur_angle -= angle_rad;
      } else if (c == '[') {
        state_stack.push_back({cur_x, cur_y, cur_angle, cur_width, branch_depth});
        branch_depth++;
      } else if (c == ']') {
        if (!state_stack.empty()) {
          TurtleState top = state_stack.back();
          state_stack.pop_back();
          cur_x = top.x;
          cur_y = top.y;
          cur_angle = top.angle;
          cur_width = top.line_width;
          branch_depth = top.branch_depth;
        }
      } 
    }
  }

  void UpdateUI() {
    info_panel.Clear();
    info_panel << "<p><b>Growth Depth:</b> " << current_depth << " / " << max_depth << "</p>";
    info_panel << "<p><b>Genotype Length (Characters):</b> " << current_string.length() << "</p>";
  }

  void DoFrame() override {
    if (current_depth >= max_depth) return;

    frame_counter++;
    if (frame_counter >= frames_per_expansion) {
      Expand();
      DrawLSystem();
      frame_counter = 0;
    }
  }
};

LSystemAnimator animator;

int main() {
  animator.Step();
}
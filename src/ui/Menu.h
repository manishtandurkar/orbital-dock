#pragma once

#include <string>

enum class MenuAction {
    None,
    Start,
    Resume,
    Restart,
    MainMenu,
    Quit
};

struct MissionSummary {
    bool won = false;
    std::string message;
    float missionSeconds = 0.0f;
    float hullFraction = 0.0f;
    int impacts = 0;
    float lifeSupportLeftSeconds = 0.0f;
};

class Menu {
public:
    MenuAction drawMainMenu(bool audioAvailable);
    MenuAction drawPauseMenu();
    MenuAction drawEndScreen(const MissionSummary& summary);

private:
    void drawControls() const;

    bool showControls_ = false;
};

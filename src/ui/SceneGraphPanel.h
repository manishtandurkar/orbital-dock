#pragma once

class SceneNode;

class SceneGraphPanel {
public:
    void draw(const SceneNode& root) const;

private:
    void drawNode(const SceneNode& node) const;
};

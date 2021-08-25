#ifndef RESIZETOOL_H
#define RESIZETOOL_H

#include "selecttool.h"

class ResizeTool : public SelectTool {
public:
    explicit ResizeTool(ObjectCtrl *controller, EditorTool::SelectList &selection);

    void update(bool pivot, bool local, float snap) override;

    void beginControl() override;

    QString icon() const override;
    QString name() const override;

protected:
     AABBox m_SavedBox;

     AABBox m_Box;

};

#endif // RESIZETOOL_H
